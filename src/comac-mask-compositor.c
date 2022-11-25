/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2011 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the comac graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *      Joonas Pihlaja <jpihlaja@cc.helsinki.fi>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

/* This compositor renders the shape to a mask using an image surface
 * then calls composite.
 */

#include "comacint.h"

#include "comac-clip-inline.h"
#include "comac-compositor-private.h"
#include "comac-image-surface-private.h"
#include "comac-pattern-inline.h"
#include "comac-region-private.h"
#include "comac-surface-observer-private.h"
#include "comac-surface-offset-private.h"
#include "comac-surface-snapshot-private.h"
#include "comac-surface-subsurface-private.h"

typedef comac_int_status_t (*draw_func_t) (
    const comac_mask_compositor_t *compositor,
    comac_surface_t *dst,
    void *closure,
    comac_operator_t op,
    const comac_pattern_t *src,
    const comac_rectangle_int_t *src_sample,
    int dst_x,
    int dst_y,
    const comac_rectangle_int_t *extents,
    comac_clip_t *clip);

static void
do_unaligned_row (void (*blt) (void *closure,
			       int16_t x,
			       int16_t y,
			       int16_t w,
			       int16_t h,
			       uint16_t coverage),
		  void *closure,
		  const comac_box_t *b,
		  int tx,
		  int y,
		  int h,
		  uint16_t coverage)
{
    int x1 = _comac_fixed_integer_part (b->p1.x) - tx;
    int x2 = _comac_fixed_integer_part (b->p2.x) - tx;
    if (x2 > x1) {
	if (! _comac_fixed_is_integer (b->p1.x)) {
	    blt (closure,
		 x1,
		 y,
		 1,
		 h,
		 coverage * (256 - _comac_fixed_fractional_part (b->p1.x)));
	    x1++;
	}

	if (x2 > x1)
	    blt (closure, x1, y, x2 - x1, h, (coverage << 8) - (coverage >> 8));

	if (! _comac_fixed_is_integer (b->p2.x))
	    blt (closure,
		 x2,
		 y,
		 1,
		 h,
		 coverage * _comac_fixed_fractional_part (b->p2.x));
    } else
	blt (closure, x1, y, 1, h, coverage * (b->p2.x - b->p1.x));
}

static void
do_unaligned_box (void (*blt) (void *closure,
			       int16_t x,
			       int16_t y,
			       int16_t w,
			       int16_t h,
			       uint16_t coverage),
		  void *closure,
		  const comac_box_t *b,
		  int tx,
		  int ty)
{
    int y1 = _comac_fixed_integer_part (b->p1.y) - ty;
    int y2 = _comac_fixed_integer_part (b->p2.y) - ty;
    if (y2 > y1) {
	if (! _comac_fixed_is_integer (b->p1.y)) {
	    do_unaligned_row (blt,
			      closure,
			      b,
			      tx,
			      y1,
			      1,
			      256 - _comac_fixed_fractional_part (b->p1.y));
	    y1++;
	}

	if (y2 > y1)
	    do_unaligned_row (blt, closure, b, tx, y1, y2 - y1, 256);

	if (! _comac_fixed_is_integer (b->p2.y))
	    do_unaligned_row (blt,
			      closure,
			      b,
			      tx,
			      y2,
			      1,
			      _comac_fixed_fractional_part (b->p2.y));
    } else
	do_unaligned_row (blt, closure, b, tx, y1, 1, b->p2.y - b->p1.y);
}

struct blt_in {
    const comac_mask_compositor_t *compositor;
    comac_surface_t *dst;
};

static void
blt_in (void *closure,
	int16_t x,
	int16_t y,
	int16_t w,
	int16_t h,
	uint16_t coverage)
{
    struct blt_in *info = closure;
    comac_color_t color;
    comac_rectangle_int_t rect;

    if (coverage == 0xffff)
	return;

    rect.x = x;
    rect.y = y;
    rect.width = w;
    rect.height = h;

    _comac_color_init_rgba (&color, 0, 0, 0, coverage / (double) 0xffff);
    info->compositor->fill_rectangles (info->dst,
				       COMAC_OPERATOR_IN,
				       &color,
				       &rect,
				       1);
}

static comac_surface_t *
create_composite_mask (const comac_mask_compositor_t *compositor,
		       comac_surface_t *dst,
		       void *draw_closure,
		       draw_func_t draw_func,
		       draw_func_t mask_func,
		       const comac_composite_rectangles_t *extents)
{
    comac_surface_t *surface;
    comac_int_status_t status;
    struct blt_in info;
    int i;

    surface = _comac_surface_create_scratch (dst,
					     COMAC_CONTENT_ALPHA,
					     extents->bounded.width,
					     extents->bounded.height,
					     NULL);
    if (unlikely (surface->status))
	return surface;

    status = compositor->acquire (surface);
    if (unlikely (status)) {
	comac_surface_destroy (surface);
	return _comac_int_surface_create_in_error (status);
    }

    if (! surface->is_clear) {
	comac_rectangle_int_t rect;

	rect.x = rect.y = 0;
	rect.width = extents->bounded.width;
	rect.height = extents->bounded.height;

	status = compositor->fill_rectangles (surface,
					      COMAC_OPERATOR_CLEAR,
					      COMAC_COLOR_TRANSPARENT,
					      &rect,
					      1);
	if (unlikely (status))
	    goto error;
    }

    if (mask_func) {
	status = mask_func (compositor,
			    surface,
			    draw_closure,
			    COMAC_OPERATOR_SOURCE,
			    NULL,
			    NULL,
			    extents->bounded.x,
			    extents->bounded.y,
			    &extents->bounded,
			    extents->clip);
	if (likely (status != COMAC_INT_STATUS_UNSUPPORTED))
	    goto out;
    }

    /* Is it worth setting the clip region here? */
    status = draw_func (compositor,
			surface,
			draw_closure,
			COMAC_OPERATOR_ADD,
			NULL,
			NULL,
			extents->bounded.x,
			extents->bounded.y,
			&extents->bounded,
			NULL);
    if (unlikely (status))
	goto error;

    info.compositor = compositor;
    info.dst = surface;
    for (i = 0; i < extents->clip->num_boxes; i++) {
	comac_box_t *b = &extents->clip->boxes[i];

	if (! _comac_fixed_is_integer (b->p1.x) ||
	    ! _comac_fixed_is_integer (b->p1.y) ||
	    ! _comac_fixed_is_integer (b->p2.x) ||
	    ! _comac_fixed_is_integer (b->p2.y)) {
	    do_unaligned_box (blt_in,
			      &info,
			      b,
			      extents->bounded.x,
			      extents->bounded.y);
	}
    }

    if (extents->clip->path != NULL) {
	status = _comac_clip_combine_with_surface (extents->clip,
						   surface,
						   extents->bounded.x,
						   extents->bounded.y);
	if (unlikely (status))
	    goto error;
    }

out:
    compositor->release (surface);
    surface->is_clear = FALSE;
    return surface;

error:
    compositor->release (surface);
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO) {
	comac_surface_destroy (surface);
	surface = _comac_int_surface_create_in_error (status);
    }
    return surface;
}

/* Handles compositing with a clip surface when the operator allows
 * us to combine the clip with the mask
 */
static comac_status_t
clip_and_composite_with_mask (const comac_mask_compositor_t *compositor,
			      void *draw_closure,
			      draw_func_t draw_func,
			      draw_func_t mask_func,
			      comac_operator_t op,
			      comac_pattern_t *pattern,
			      const comac_composite_rectangles_t *extents)
{
    comac_surface_t *dst = extents->surface;
    comac_surface_t *mask, *src;
    int src_x, src_y;

    mask = create_composite_mask (compositor,
				  dst,
				  draw_closure,
				  draw_func,
				  mask_func,
				  extents);
    if (unlikely (mask->status))
	return mask->status;

    if (pattern != NULL || dst->content != COMAC_CONTENT_ALPHA) {
	src = compositor->pattern_to_surface (dst,
					      &extents->source_pattern.base,
					      FALSE,
					      &extents->bounded,
					      &extents->source_sample_area,
					      &src_x,
					      &src_y);
	if (unlikely (src->status)) {
	    comac_surface_destroy (mask);
	    return src->status;
	}

	compositor->composite (dst,
			       op,
			       src,
			       mask,
			       extents->bounded.x + src_x,
			       extents->bounded.y + src_y,
			       0,
			       0,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);

	comac_surface_destroy (src);
    } else {
	compositor->composite (dst,
			       op,
			       mask,
			       NULL,
			       0,
			       0,
			       0,
			       0,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);
    }
    comac_surface_destroy (mask);

    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
get_clip_source (const comac_mask_compositor_t *compositor,
		 comac_clip_t *clip,
		 comac_surface_t *dst,
		 const comac_rectangle_int_t *bounds,
		 int *out_x,
		 int *out_y)
{
    comac_surface_pattern_t pattern;
    comac_rectangle_int_t r;
    comac_surface_t *surface;

    surface = _comac_clip_get_image (clip, dst, bounds);
    if (unlikely (surface->status))
	return surface;

    _comac_pattern_init_for_surface (&pattern, surface);
    pattern.base.filter = COMAC_FILTER_NEAREST;
    comac_surface_destroy (surface);

    r.x = r.y = 0;
    r.width = bounds->width;
    r.height = bounds->height;

    surface = compositor->pattern_to_surface (dst,
					      &pattern.base,
					      TRUE,
					      &r,
					      &r,
					      out_x,
					      out_y);
    _comac_pattern_fini (&pattern.base);

    *out_x += -bounds->x;
    *out_y += -bounds->y;
    return surface;
}

/* Handles compositing with a clip surface when we have to do the operation
 * in two pieces and combine them together.
 */
static comac_status_t
clip_and_composite_combine (const comac_mask_compositor_t *compositor,
			    void *draw_closure,
			    draw_func_t draw_func,
			    comac_operator_t op,
			    const comac_pattern_t *pattern,
			    const comac_composite_rectangles_t *extents)
{
    comac_surface_t *dst = extents->surface;
    comac_surface_t *tmp, *clip;
    comac_status_t status;
    int clip_x, clip_y;

    tmp = _comac_surface_create_scratch (dst,
					 dst->content,
					 extents->bounded.width,
					 extents->bounded.height,
					 NULL);
    if (unlikely (tmp->status))
	return tmp->status;

    compositor->composite (tmp,
			   COMAC_OPERATOR_SOURCE,
			   dst,
			   NULL,
			   extents->bounded.x,
			   extents->bounded.y,
			   0,
			   0,
			   0,
			   0,
			   extents->bounded.width,
			   extents->bounded.height);

    status = draw_func (compositor,
			tmp,
			draw_closure,
			op,
			pattern,
			&extents->source_sample_area,
			extents->bounded.x,
			extents->bounded.y,
			&extents->bounded,
			NULL);
    if (unlikely (status))
	goto cleanup;

    clip = get_clip_source (compositor,
			    extents->clip,
			    dst,
			    &extents->bounded,
			    &clip_x,
			    &clip_y);
    if (unlikely ((status = clip->status)))
	goto cleanup;

    if (dst->is_clear) {
	compositor->composite (dst,
			       COMAC_OPERATOR_SOURCE,
			       tmp,
			       clip,
			       0,
			       0,
			       clip_x,
			       clip_y,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);
    } else {
	/* Punch the clip out of the destination */
	compositor->composite (dst,
			       COMAC_OPERATOR_DEST_OUT,
			       clip,
			       NULL,
			       clip_x,
			       clip_y,
			       0,
			       0,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);

	/* Now add the two results together */
	compositor->composite (dst,
			       COMAC_OPERATOR_ADD,
			       tmp,
			       clip,
			       0,
			       0,
			       clip_x,
			       clip_y,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);
    }
    comac_surface_destroy (clip);

cleanup:
    comac_surface_destroy (tmp);
    return status;
}

/* Handles compositing for %COMAC_OPERATOR_SOURCE, which is special; it's
 * defined as (src IN mask IN clip) ADD (dst OUT (mask IN clip))
 */
static comac_status_t
clip_and_composite_source (const comac_mask_compositor_t *compositor,
			   void *draw_closure,
			   draw_func_t draw_func,
			   draw_func_t mask_func,
			   comac_pattern_t *pattern,
			   const comac_composite_rectangles_t *extents)
{
    comac_surface_t *dst = extents->surface;
    comac_surface_t *mask, *src;
    int src_x, src_y;

    /* Create a surface that is mask IN clip */
    mask = create_composite_mask (compositor,
				  dst,
				  draw_closure,
				  draw_func,
				  mask_func,
				  extents);
    if (unlikely (mask->status))
	return mask->status;

    src = compositor->pattern_to_surface (dst,
					  pattern,
					  FALSE,
					  &extents->bounded,
					  &extents->source_sample_area,
					  &src_x,
					  &src_y);
    if (unlikely (src->status)) {
	comac_surface_destroy (mask);
	return src->status;
    }

    if (dst->is_clear) {
	compositor->composite (dst,
			       COMAC_OPERATOR_SOURCE,
			       src,
			       mask,
			       extents->bounded.x + src_x,
			       extents->bounded.y + src_y,
			       0,
			       0,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);
    } else {
	/* Compute dest' = dest OUT (mask IN clip) */
	compositor->composite (dst,
			       COMAC_OPERATOR_DEST_OUT,
			       mask,
			       NULL,
			       0,
			       0,
			       0,
			       0,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);

	/* Now compute (src IN (mask IN clip)) ADD dest' */
	compositor->composite (dst,
			       COMAC_OPERATOR_ADD,
			       src,
			       mask,
			       extents->bounded.x + src_x,
			       extents->bounded.y + src_y,
			       0,
			       0,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);
    }

    comac_surface_destroy (src);
    comac_surface_destroy (mask);

    return COMAC_STATUS_SUCCESS;
}

static comac_bool_t
can_reduce_alpha_op (comac_operator_t op)
{
    int iop = op;
    switch (iop) {
    case COMAC_OPERATOR_OVER:
    case COMAC_OPERATOR_SOURCE:
    case COMAC_OPERATOR_ADD:
	return TRUE;
    default:
	return FALSE;
    }
}

static comac_bool_t
reduce_alpha_op (comac_surface_t *dst,
		 comac_operator_t op,
		 const comac_pattern_t *pattern)
{
    return dst->is_clear && dst->content == COMAC_CONTENT_ALPHA &&
	   _comac_pattern_is_opaque_solid (pattern) && can_reduce_alpha_op (op);
}

static comac_status_t
fixup_unbounded (const comac_mask_compositor_t *compositor,
		 comac_surface_t *dst,
		 const comac_composite_rectangles_t *extents)
{
    comac_rectangle_int_t rects[4];
    int n;

    if (extents->bounded.width == extents->unbounded.width &&
	extents->bounded.height == extents->unbounded.height) {
	return COMAC_STATUS_SUCCESS;
    }

    n = 0;
    if (extents->bounded.width == 0 || extents->bounded.height == 0) {
	rects[n].x = extents->unbounded.x;
	rects[n].width = extents->unbounded.width;
	rects[n].y = extents->unbounded.y;
	rects[n].height = extents->unbounded.height;
	n++;
    } else {
	/* top */
	if (extents->bounded.y != extents->unbounded.y) {
	    rects[n].x = extents->unbounded.x;
	    rects[n].width = extents->unbounded.width;
	    rects[n].y = extents->unbounded.y;
	    rects[n].height = extents->bounded.y - extents->unbounded.y;
	    n++;
	}
	/* left */
	if (extents->bounded.x != extents->unbounded.x) {
	    rects[n].x = extents->unbounded.x;
	    rects[n].width = extents->bounded.x - extents->unbounded.x;
	    rects[n].y = extents->bounded.y;
	    rects[n].height = extents->bounded.height;
	    n++;
	}
	/* right */
	if (extents->bounded.x + extents->bounded.width !=
	    extents->unbounded.x + extents->unbounded.width) {
	    rects[n].x = extents->bounded.x + extents->bounded.width;
	    rects[n].width =
		extents->unbounded.x + extents->unbounded.width - rects[n].x;
	    rects[n].y = extents->bounded.y;
	    rects[n].height = extents->bounded.height;
	    n++;
	}
	/* bottom */
	if (extents->bounded.y + extents->bounded.height !=
	    extents->unbounded.y + extents->unbounded.height) {
	    rects[n].x = extents->unbounded.x;
	    rects[n].width = extents->unbounded.width;
	    rects[n].y = extents->bounded.y + extents->bounded.height;
	    rects[n].height =
		extents->unbounded.y + extents->unbounded.height - rects[n].y;
	    n++;
	}
    }

    return compositor->fill_rectangles (dst,
					COMAC_OPERATOR_CLEAR,
					COMAC_COLOR_TRANSPARENT,
					rects,
					n);
}

static comac_status_t
fixup_unbounded_with_mask (const comac_mask_compositor_t *compositor,
			   comac_surface_t *dst,
			   const comac_composite_rectangles_t *extents)
{
    comac_surface_t *mask;
    int mask_x, mask_y;

    mask = get_clip_source (compositor,
			    extents->clip,
			    dst,
			    &extents->unbounded,
			    &mask_x,
			    &mask_y);
    if (unlikely (mask->status))
	return mask->status;

    /* top */
    if (extents->bounded.y != extents->unbounded.y) {
	int x = extents->unbounded.x;
	int y = extents->unbounded.y;
	int width = extents->unbounded.width;
	int height = extents->bounded.y - y;

	compositor->composite (dst,
			       COMAC_OPERATOR_DEST_OUT,
			       mask,
			       NULL,
			       x + mask_x,
			       y + mask_y,
			       0,
			       0,
			       x,
			       y,
			       width,
			       height);
    }

    /* left */
    if (extents->bounded.x != extents->unbounded.x) {
	int x = extents->unbounded.x;
	int y = extents->bounded.y;
	int width = extents->bounded.x - x;
	int height = extents->bounded.height;

	compositor->composite (dst,
			       COMAC_OPERATOR_DEST_OUT,
			       mask,
			       NULL,
			       x + mask_x,
			       y + mask_y,
			       0,
			       0,
			       x,
			       y,
			       width,
			       height);
    }

    /* right */
    if (extents->bounded.x + extents->bounded.width !=
	extents->unbounded.x + extents->unbounded.width) {
	int x = extents->bounded.x + extents->bounded.width;
	int y = extents->bounded.y;
	int width = extents->unbounded.x + extents->unbounded.width - x;
	int height = extents->bounded.height;

	compositor->composite (dst,
			       COMAC_OPERATOR_DEST_OUT,
			       mask,
			       NULL,
			       x + mask_x,
			       y + mask_y,
			       0,
			       0,
			       x,
			       y,
			       width,
			       height);
    }

    /* bottom */
    if (extents->bounded.y + extents->bounded.height !=
	extents->unbounded.y + extents->unbounded.height) {
	int x = extents->unbounded.x;
	int y = extents->bounded.y + extents->bounded.height;
	int width = extents->unbounded.width;
	int height = extents->unbounded.y + extents->unbounded.height - y;

	compositor->composite (dst,
			       COMAC_OPERATOR_DEST_OUT,
			       mask,
			       NULL,
			       x + mask_x,
			       y + mask_y,
			       0,
			       0,
			       x,
			       y,
			       width,
			       height);
    }

    comac_surface_destroy (mask);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
fixup_unbounded_boxes (const comac_mask_compositor_t *compositor,
		       const comac_composite_rectangles_t *extents,
		       comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    comac_boxes_t clear;
    comac_region_t *clip_region;
    comac_box_t box;
    comac_status_t status;
    struct _comac_boxes_chunk *chunk;
    int i;

    assert (boxes->is_pixel_aligned);

    clip_region = NULL;
    if (_comac_clip_is_region (extents->clip) &&
	(clip_region = _comac_clip_get_region (extents->clip)) &&
	comac_region_contains_rectangle (clip_region, &extents->bounded) ==
	    COMAC_REGION_OVERLAP_IN)
	clip_region = NULL;

    if (boxes->num_boxes <= 1 && clip_region == NULL)
	return fixup_unbounded (compositor, dst, extents);

    _comac_boxes_init (&clear);

    box.p1.x =
	_comac_fixed_from_int (extents->unbounded.x + extents->unbounded.width);
    box.p1.y = _comac_fixed_from_int (extents->unbounded.y);
    box.p2.x = _comac_fixed_from_int (extents->unbounded.x);
    box.p2.y = _comac_fixed_from_int (extents->unbounded.y +
				      extents->unbounded.height);

    if (clip_region == NULL) {
	comac_boxes_t tmp;

	_comac_boxes_init (&tmp);

	status = _comac_boxes_add (&tmp, COMAC_ANTIALIAS_DEFAULT, &box);
	assert (status == COMAC_STATUS_SUCCESS);

	tmp.chunks.next = &boxes->chunks;
	tmp.num_boxes += boxes->num_boxes;

	status =
	    _comac_bentley_ottmann_tessellate_boxes (&tmp,
						     COMAC_FILL_RULE_WINDING,
						     &clear);

	tmp.chunks.next = NULL;
    } else {
	pixman_box32_t *pbox;

	pbox = pixman_region32_rectangles (&clip_region->rgn, &i);
	_comac_boxes_limit (&clear, (comac_box_t *) pbox, i);

	status = _comac_boxes_add (&clear, COMAC_ANTIALIAS_DEFAULT, &box);
	assert (status == COMAC_STATUS_SUCCESS);

	for (chunk = &boxes->chunks; chunk != NULL; chunk = chunk->next) {
	    for (i = 0; i < chunk->count; i++) {
		status = _comac_boxes_add (&clear,
					   COMAC_ANTIALIAS_DEFAULT,
					   &chunk->base[i]);
		if (unlikely (status)) {
		    _comac_boxes_fini (&clear);
		    return status;
		}
	    }
	}

	status =
	    _comac_bentley_ottmann_tessellate_boxes (&clear,
						     COMAC_FILL_RULE_WINDING,
						     &clear);
    }

    if (likely (status == COMAC_STATUS_SUCCESS)) {
	status = compositor->fill_boxes (dst,
					 COMAC_OPERATOR_CLEAR,
					 COMAC_COLOR_TRANSPARENT,
					 &clear);
    }

    _comac_boxes_fini (&clear);

    return status;
}

enum {
    NEED_CLIP_REGION = 0x1,
    NEED_CLIP_SURFACE = 0x2,
    FORCE_CLIP_REGION = 0x4,
};

static comac_bool_t
need_bounded_clip (comac_composite_rectangles_t *extents)
{
    unsigned int flags = NEED_CLIP_REGION;
    if (! _comac_clip_is_region (extents->clip))
	flags |= NEED_CLIP_SURFACE;
    return flags;
}

static comac_bool_t
need_unbounded_clip (comac_composite_rectangles_t *extents)
{
    unsigned int flags = 0;
    if (! extents->is_bounded) {
	flags |= NEED_CLIP_REGION;
	if (! _comac_clip_is_region (extents->clip))
	    flags |= NEED_CLIP_SURFACE;
    }
    if (extents->clip->path != NULL)
	flags |= NEED_CLIP_SURFACE;
    return flags;
}

static comac_status_t
clip_and_composite (const comac_mask_compositor_t *compositor,
		    draw_func_t draw_func,
		    draw_func_t mask_func,
		    void *draw_closure,
		    comac_composite_rectangles_t *extents,
		    unsigned int need_clip)
{
    comac_surface_t *dst = extents->surface;
    comac_operator_t op = extents->op;
    comac_pattern_t *src = &extents->source_pattern.base;
    comac_region_t *clip_region = NULL;
    comac_status_t status;

    compositor->acquire (dst);

    if (need_clip & NEED_CLIP_REGION) {
	clip_region = _comac_clip_get_region (extents->clip);
	if ((need_clip & FORCE_CLIP_REGION) == 0 &&
	    _comac_composite_rectangles_can_reduce_clip (extents,
							 extents->clip))
	    clip_region = NULL;
	if (clip_region != NULL) {
	    status = compositor->set_clip_region (dst, clip_region);
	    if (unlikely (status)) {
		compositor->release (dst);
		return status;
	    }
	}
    }

    if (reduce_alpha_op (dst, op, &extents->source_pattern.base)) {
	op = COMAC_OPERATOR_ADD;
	src = NULL;
    }

    if (op == COMAC_OPERATOR_SOURCE) {
	status = clip_and_composite_source (compositor,
					    draw_closure,
					    draw_func,
					    mask_func,
					    src,
					    extents);
    } else {
	if (op == COMAC_OPERATOR_CLEAR) {
	    op = COMAC_OPERATOR_DEST_OUT;
	    src = NULL;
	}

	if (need_clip & NEED_CLIP_SURFACE) {
	    if (extents->is_bounded) {
		status = clip_and_composite_with_mask (compositor,
						       draw_closure,
						       draw_func,
						       mask_func,
						       op,
						       src,
						       extents);
	    } else {
		status = clip_and_composite_combine (compositor,
						     draw_closure,
						     draw_func,
						     op,
						     src,
						     extents);
	    }
	} else {
	    status = draw_func (compositor,
				dst,
				draw_closure,
				op,
				src,
				&extents->source_sample_area,
				0,
				0,
				&extents->bounded,
				extents->clip);
	}
    }

    if (status == COMAC_STATUS_SUCCESS && ! extents->is_bounded) {
	if (need_clip & NEED_CLIP_SURFACE)
	    status = fixup_unbounded_with_mask (compositor, dst, extents);
	else
	    status = fixup_unbounded (compositor, dst, extents);
    }

    if (clip_region)
	compositor->set_clip_region (dst, NULL);

    compositor->release (dst);

    return status;
}

static comac_int_status_t
trim_extents_to_boxes (comac_composite_rectangles_t *extents,
		       comac_boxes_t *boxes)
{
    comac_box_t box;

    _comac_boxes_extents (boxes, &box);
    return _comac_composite_rectangles_intersect_mask_extents (extents, &box);
}

static comac_status_t
upload_boxes (const comac_mask_compositor_t *compositor,
	      comac_composite_rectangles_t *extents,
	      comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    const comac_pattern_t *source = &extents->source_pattern.base;
    comac_surface_t *src;
    comac_rectangle_int_t limit;
    comac_int_status_t status;
    int tx, ty;

    src =
	_comac_pattern_get_source ((comac_surface_pattern_t *) source, &limit);
    if (! (src->type == COMAC_SURFACE_TYPE_IMAGE || src->type == dst->type))
	return COMAC_INT_STATUS_UNSUPPORTED;

    if (! _comac_matrix_is_integer_translation (&source->matrix, &tx, &ty))
	return COMAC_INT_STATUS_UNSUPPORTED;

    /* Check that the data is entirely within the image */
    if (extents->bounded.x + tx < limit.x || extents->bounded.y + ty < limit.y)
	return COMAC_INT_STATUS_UNSUPPORTED;

    if (extents->bounded.x + extents->bounded.width + tx >
	    limit.x + limit.width ||
	extents->bounded.y + extents->bounded.height + ty >
	    limit.y + limit.height)
	return COMAC_INT_STATUS_UNSUPPORTED;

    tx += limit.x;
    ty += limit.y;

    if (src->type == COMAC_SURFACE_TYPE_IMAGE)
	status = compositor->draw_image_boxes (dst,
					       (comac_image_surface_t *) src,
					       boxes,
					       tx,
					       ty);
    else
	status =
	    compositor->copy_boxes (dst, src, boxes, &extents->bounded, tx, ty);

    return status;
}

static comac_status_t
composite_boxes (const comac_mask_compositor_t *compositor,
		 const comac_composite_rectangles_t *extents,
		 comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    comac_operator_t op = extents->op;
    const comac_pattern_t *source = &extents->source_pattern.base;
    comac_bool_t need_clip_mask = extents->clip->path != NULL;
    comac_status_t status;

    if (need_clip_mask &&
	(! extents->is_bounded || op == COMAC_OPERATOR_SOURCE)) {
	return COMAC_INT_STATUS_UNSUPPORTED;
    }

    status = compositor->acquire (dst);
    if (unlikely (status))
	return status;

    if (! need_clip_mask && source->type == COMAC_PATTERN_TYPE_SOLID) {
	const comac_color_t *color;

	color = &((comac_solid_pattern_t *) source)->color;
	status = compositor->fill_boxes (dst, op, color, boxes);
    } else {
	comac_surface_t *src, *mask = NULL;
	int src_x, src_y;
	int mask_x = 0, mask_y = 0;

	if (need_clip_mask) {
	    mask = get_clip_source (compositor,
				    extents->clip,
				    dst,
				    &extents->bounded,
				    &mask_x,
				    &mask_y);
	    if (unlikely (mask->status))
		return mask->status;

	    if (op == COMAC_OPERATOR_CLEAR) {
		source = NULL;
		op = COMAC_OPERATOR_DEST_OUT;
	    }
	}

	if (source || mask == NULL) {
	    src = compositor->pattern_to_surface (dst,
						  source,
						  FALSE,
						  &extents->bounded,
						  &extents->source_sample_area,
						  &src_x,
						  &src_y);
	} else {
	    src = mask;
	    src_x = mask_x;
	    src_y = mask_y;
	    mask = NULL;
	}

	status = compositor->composite_boxes (dst,
					      op,
					      src,
					      mask,
					      src_x,
					      src_y,
					      mask_x,
					      mask_y,
					      0,
					      0,
					      boxes,
					      &extents->bounded);

	comac_surface_destroy (src);
	comac_surface_destroy (mask);
    }

    if (status == COMAC_STATUS_SUCCESS && ! extents->is_bounded)
	status = fixup_unbounded_boxes (compositor, extents, boxes);

    compositor->release (dst);

    return status;
}

static comac_status_t
clip_and_composite_boxes (const comac_mask_compositor_t *compositor,
			  comac_composite_rectangles_t *extents,
			  comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    comac_int_status_t status;

    if (boxes->num_boxes == 0) {
	if (extents->is_bounded)
	    return COMAC_STATUS_SUCCESS;

	return fixup_unbounded_boxes (compositor, extents, boxes);
    }

    if (! boxes->is_pixel_aligned)
	return COMAC_INT_STATUS_UNSUPPORTED;

    status = trim_extents_to_boxes (extents, boxes);
    if (unlikely (status))
	return status;

    if (extents->source_pattern.base.type == COMAC_PATTERN_TYPE_SURFACE &&
	extents->clip->path == NULL &&
	(extents->op == COMAC_OPERATOR_SOURCE ||
	 (dst->is_clear && (extents->op == COMAC_OPERATOR_OVER ||
			    extents->op == COMAC_OPERATOR_ADD)))) {
	status = upload_boxes (compositor, extents, boxes);
	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return composite_boxes (compositor, extents, boxes);
}

/* high-level compositor interface */

static comac_int_status_t
_comac_mask_compositor_paint (const comac_compositor_t *_compositor,
			      comac_composite_rectangles_t *extents)
{
    comac_mask_compositor_t *compositor =
	(comac_mask_compositor_t *) _compositor;
    comac_boxes_t boxes;
    comac_int_status_t status;

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    _comac_clip_steal_boxes (extents->clip, &boxes);
    status = clip_and_composite_boxes (compositor, extents, &boxes);
    _comac_clip_unsteal_boxes (extents->clip, &boxes);

    return status;
}

struct composite_opacity_info {
    const comac_mask_compositor_t *compositor;
    uint8_t op;
    comac_surface_t *dst;
    comac_surface_t *src;
    int src_x, src_y;
    double opacity;
};

static void
composite_opacity (void *closure,
		   int16_t x,
		   int16_t y,
		   int16_t w,
		   int16_t h,
		   uint16_t coverage)
{
    struct composite_opacity_info *info = closure;
    const comac_mask_compositor_t *compositor = info->compositor;
    comac_surface_t *mask;
    int mask_x, mask_y;
    comac_color_t color;
    comac_solid_pattern_t solid;

    _comac_color_init_rgba (&color, 0, 0, 0, info->opacity * coverage);
    _comac_pattern_init_solid (&solid, &color);
    mask = compositor->pattern_to_surface (info->dst,
					   &solid.base,
					   TRUE,
					   &_comac_unbounded_rectangle,
					   &_comac_unbounded_rectangle,
					   &mask_x,
					   &mask_y);
    if (likely (mask->status == COMAC_STATUS_SUCCESS)) {
	if (info->src) {
	    compositor->composite (info->dst,
				   info->op,
				   info->src,
				   mask,
				   x + info->src_x,
				   y + info->src_y,
				   mask_x,
				   mask_y,
				   x,
				   y,
				   w,
				   h);
	} else {
	    compositor->composite (info->dst,
				   info->op,
				   mask,
				   NULL,
				   mask_x,
				   mask_y,
				   0,
				   0,
				   x,
				   y,
				   w,
				   h);
	}
    }

    comac_surface_destroy (mask);
}

static comac_int_status_t
composite_opacity_boxes (const comac_mask_compositor_t *compositor,
			 comac_surface_t *dst,
			 void *closure,
			 comac_operator_t op,
			 const comac_pattern_t *src_pattern,
			 const comac_rectangle_int_t *src_sample,
			 int dst_x,
			 int dst_y,
			 const comac_rectangle_int_t *extents,
			 comac_clip_t *clip)
{
    const comac_solid_pattern_t *mask_pattern = closure;
    struct composite_opacity_info info;
    int i;

    assert (clip);

    info.compositor = compositor;
    info.op = op;
    info.dst = dst;

    if (src_pattern != NULL) {
	info.src = compositor->pattern_to_surface (dst,
						   src_pattern,
						   FALSE,
						   extents,
						   src_sample,
						   &info.src_x,
						   &info.src_y);
	if (unlikely (info.src->status))
	    return info.src->status;
    } else
	info.src = NULL;

    info.opacity = mask_pattern->color.alpha / (double) 0xffff;

    /* XXX for lots of boxes create a clip region for the fully opaque areas */
    for (i = 0; i < clip->num_boxes; i++)
	do_unaligned_box (composite_opacity,
			  &info,
			  &clip->boxes[i],
			  dst_x,
			  dst_y);
    comac_surface_destroy (info.src);

    return COMAC_STATUS_SUCCESS;
}

struct composite_box_info {
    const comac_mask_compositor_t *compositor;
    comac_surface_t *dst;
    comac_surface_t *src;
    int src_x, src_y;
    uint8_t op;
};

static void
composite_box (void *closure,
	       int16_t x,
	       int16_t y,
	       int16_t w,
	       int16_t h,
	       uint16_t coverage)
{
    struct composite_box_info *info = closure;
    const comac_mask_compositor_t *compositor = info->compositor;

    if (! COMAC_ALPHA_SHORT_IS_OPAQUE (coverage)) {
	comac_surface_t *mask;
	comac_color_t color;
	comac_solid_pattern_t solid;
	int mask_x, mask_y;

	_comac_color_init_rgba (&color, 0, 0, 0, coverage / (double) 0xffff);
	_comac_pattern_init_solid (&solid, &color);

	mask = compositor->pattern_to_surface (info->dst,
					       &solid.base,
					       FALSE,
					       &_comac_unbounded_rectangle,
					       &_comac_unbounded_rectangle,
					       &mask_x,
					       &mask_y);

	if (likely (mask->status == COMAC_STATUS_SUCCESS)) {
	    compositor->composite (info->dst,
				   info->op,
				   info->src,
				   mask,
				   x + info->src_x,
				   y + info->src_y,
				   mask_x,
				   mask_y,
				   x,
				   y,
				   w,
				   h);
	}

	comac_surface_destroy (mask);
    } else {
	compositor->composite (info->dst,
			       info->op,
			       info->src,
			       NULL,
			       x + info->src_x,
			       y + info->src_y,
			       0,
			       0,
			       x,
			       y,
			       w,
			       h);
    }
}

static comac_int_status_t
composite_mask_clip_boxes (const comac_mask_compositor_t *compositor,
			   comac_surface_t *dst,
			   void *closure,
			   comac_operator_t op,
			   const comac_pattern_t *src_pattern,
			   const comac_rectangle_int_t *src_sample,
			   int dst_x,
			   int dst_y,
			   const comac_rectangle_int_t *extents,
			   comac_clip_t *clip)
{
    comac_composite_rectangles_t *composite = closure;
    struct composite_box_info info;
    int i;

    assert (src_pattern == NULL);
    assert (op == COMAC_OPERATOR_SOURCE);

    info.compositor = compositor;
    info.op = COMAC_OPERATOR_SOURCE;
    info.dst = dst;
    info.src = compositor->pattern_to_surface (dst,
					       &composite->mask_pattern.base,
					       FALSE,
					       extents,
					       &composite->mask_sample_area,
					       &info.src_x,
					       &info.src_y);
    if (unlikely (info.src->status))
	return info.src->status;

    info.src_x += dst_x;
    info.src_y += dst_y;

    for (i = 0; i < clip->num_boxes; i++)
	do_unaligned_box (composite_box, &info, &clip->boxes[i], dst_x, dst_y);

    comac_surface_destroy (info.src);

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite_mask (const comac_mask_compositor_t *compositor,
		comac_surface_t *dst,
		void *closure,
		comac_operator_t op,
		const comac_pattern_t *src_pattern,
		const comac_rectangle_int_t *src_sample,
		int dst_x,
		int dst_y,
		const comac_rectangle_int_t *extents,
		comac_clip_t *clip)
{
    comac_composite_rectangles_t *composite = closure;
    comac_surface_t *src, *mask;
    int src_x, src_y;
    int mask_x, mask_y;

    if (src_pattern != NULL) {
	src = compositor->pattern_to_surface (dst,
					      src_pattern,
					      FALSE,
					      extents,
					      src_sample,
					      &src_x,
					      &src_y);
	if (unlikely (src->status))
	    return src->status;

	mask = compositor->pattern_to_surface (dst,
					       &composite->mask_pattern.base,
					       TRUE,
					       extents,
					       &composite->mask_sample_area,
					       &mask_x,
					       &mask_y);
	if (unlikely (mask->status)) {
	    comac_surface_destroy (src);
	    return mask->status;
	}

	compositor->composite (dst,
			       op,
			       src,
			       mask,
			       extents->x + src_x,
			       extents->y + src_y,
			       extents->x + mask_x,
			       extents->y + mask_y,
			       extents->x - dst_x,
			       extents->y - dst_y,
			       extents->width,
			       extents->height);

	comac_surface_destroy (mask);
	comac_surface_destroy (src);
    } else {
	src = compositor->pattern_to_surface (dst,
					      &composite->mask_pattern.base,
					      FALSE,
					      extents,
					      &composite->mask_sample_area,
					      &src_x,
					      &src_y);
	if (unlikely (src->status))
	    return src->status;

	compositor->composite (dst,
			       op,
			       src,
			       NULL,
			       extents->x + src_x,
			       extents->y + src_y,
			       0,
			       0,
			       extents->x - dst_x,
			       extents->y - dst_y,
			       extents->width,
			       extents->height);

	comac_surface_destroy (src);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_comac_mask_compositor_mask (const comac_compositor_t *_compositor,
			     comac_composite_rectangles_t *extents)
{
    const comac_mask_compositor_t *compositor =
	(comac_mask_compositor_t *) _compositor;
    comac_int_status_t status = COMAC_INT_STATUS_UNSUPPORTED;

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    if (extents->mask_pattern.base.type == COMAC_PATTERN_TYPE_SOLID &&
	extents->clip->path == NULL && _comac_clip_is_region (extents->clip)) {
	status = clip_and_composite (compositor,
				     composite_opacity_boxes,
				     composite_opacity_boxes,
				     &extents->mask_pattern.solid,
				     extents,
				     need_unbounded_clip (extents));
    } else {
	status = clip_and_composite (
	    compositor,
	    composite_mask,
	    extents->clip->path == NULL ? composite_mask_clip_boxes : NULL,
	    extents,
	    extents,
	    need_bounded_clip (extents));
    }

    return status;
}

static comac_int_status_t
_comac_mask_compositor_stroke (const comac_compositor_t *_compositor,
			       comac_composite_rectangles_t *extents,
			       const comac_path_fixed_t *path,
			       const comac_stroke_style_t *style,
			       const comac_matrix_t *ctm,
			       const comac_matrix_t *ctm_inverse,
			       double tolerance,
			       comac_antialias_t antialias)
{
    const comac_mask_compositor_t *compositor =
	(comac_mask_compositor_t *) _compositor;
    comac_surface_t *mask;
    comac_surface_pattern_t pattern;
    comac_int_status_t status = COMAC_INT_STATUS_UNSUPPORTED;

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    if (_comac_path_fixed_stroke_is_rectilinear (path)) {
	comac_boxes_t boxes;

	_comac_boxes_init_with_clip (&boxes, extents->clip);
	status = _comac_path_fixed_stroke_rectilinear_to_boxes (path,
								style,
								ctm,
								antialias,
								&boxes);
	if (likely (status == COMAC_INT_STATUS_SUCCESS))
	    status = clip_and_composite_boxes (compositor, extents, &boxes);
	_comac_boxes_fini (&boxes);
    }

    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	mask = comac_surface_create_similar_image (extents->surface,
						   COMAC_FORMAT_A8,
						   extents->bounded.width,
						   extents->bounded.height);
	if (unlikely (mask->status))
	    return mask->status;

	status = _comac_surface_offset_stroke (mask,
					       extents->bounded.x,
					       extents->bounded.y,
					       COMAC_OPERATOR_ADD,
					       &_comac_pattern_white.base,
					       path,
					       style,
					       ctm,
					       ctm_inverse,
					       tolerance,
					       antialias,
					       extents->clip);
	if (unlikely (status)) {
	    comac_surface_destroy (mask);
	    return status;
	}

	_comac_pattern_init_for_surface (&pattern, mask);
	comac_surface_destroy (mask);

	comac_matrix_init_translate (&pattern.base.matrix,
				     -extents->bounded.x,
				     -extents->bounded.y);
	pattern.base.filter = COMAC_FILTER_NEAREST;
	pattern.base.extend = COMAC_EXTEND_NONE;
	status = _comac_surface_mask (extents->surface,
				      extents->op,
				      &extents->source_pattern.base,
				      &pattern.base,
				      extents->clip);
	_comac_pattern_fini (&pattern.base);
    }

    return status;
}

static comac_int_status_t
_comac_mask_compositor_fill (const comac_compositor_t *_compositor,
			     comac_composite_rectangles_t *extents,
			     const comac_path_fixed_t *path,
			     comac_fill_rule_t fill_rule,
			     double tolerance,
			     comac_antialias_t antialias)
{
    const comac_mask_compositor_t *compositor =
	(comac_mask_compositor_t *) _compositor;
    comac_surface_t *mask;
    comac_surface_pattern_t pattern;
    comac_int_status_t status = COMAC_INT_STATUS_UNSUPPORTED;

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    if (_comac_path_fixed_fill_is_rectilinear (path)) {
	comac_boxes_t boxes;

	_comac_boxes_init_with_clip (&boxes, extents->clip);
	status = _comac_path_fixed_fill_rectilinear_to_boxes (path,
							      fill_rule,
							      antialias,
							      &boxes);
	if (likely (status == COMAC_INT_STATUS_SUCCESS))
	    status = clip_and_composite_boxes (compositor, extents, &boxes);
	_comac_boxes_fini (&boxes);
    }

    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	mask = comac_surface_create_similar_image (extents->surface,
						   COMAC_FORMAT_A8,
						   extents->bounded.width,
						   extents->bounded.height);
	if (unlikely (mask->status))
	    return mask->status;

	status = _comac_surface_offset_fill (mask,
					     extents->bounded.x,
					     extents->bounded.y,
					     COMAC_OPERATOR_ADD,
					     &_comac_pattern_white.base,
					     path,
					     fill_rule,
					     tolerance,
					     antialias,
					     extents->clip);
	if (unlikely (status)) {
	    comac_surface_destroy (mask);
	    return status;
	}

	_comac_pattern_init_for_surface (&pattern, mask);
	comac_surface_destroy (mask);

	comac_matrix_init_translate (&pattern.base.matrix,
				     -extents->bounded.x,
				     -extents->bounded.y);
	pattern.base.filter = COMAC_FILTER_NEAREST;
	pattern.base.extend = COMAC_EXTEND_NONE;
	status = _comac_surface_mask (extents->surface,
				      extents->op,
				      &extents->source_pattern.base,
				      &pattern.base,
				      extents->clip);
	_comac_pattern_fini (&pattern.base);
    }

    return status;
}

static comac_int_status_t
_comac_mask_compositor_glyphs (const comac_compositor_t *_compositor,
			       comac_composite_rectangles_t *extents,
			       comac_scaled_font_t *scaled_font,
			       comac_glyph_t *glyphs,
			       int num_glyphs,
			       comac_bool_t overlap)
{
    const comac_mask_compositor_t *compositor =
	(comac_mask_compositor_t *) _compositor;
    comac_surface_t *mask;
    comac_surface_pattern_t pattern;
    comac_int_status_t status;

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return COMAC_INT_STATUS_UNSUPPORTED;

    mask = comac_surface_create_similar_image (extents->surface,
					       COMAC_FORMAT_A8,
					       extents->bounded.width,
					       extents->bounded.height);
    if (unlikely (mask->status))
	return mask->status;

    status = _comac_surface_offset_glyphs (mask,
					   extents->bounded.x,
					   extents->bounded.y,
					   COMAC_OPERATOR_ADD,
					   &_comac_pattern_white.base,
					   scaled_font,
					   glyphs,
					   num_glyphs,
					   extents->clip);
    if (unlikely (status)) {
	comac_surface_destroy (mask);
	return status;
    }

    _comac_pattern_init_for_surface (&pattern, mask);
    comac_surface_destroy (mask);

    comac_matrix_init_translate (&pattern.base.matrix,
				 -extents->bounded.x,
				 -extents->bounded.y);
    pattern.base.filter = COMAC_FILTER_NEAREST;
    pattern.base.extend = COMAC_EXTEND_NONE;
    status = _comac_surface_mask (extents->surface,
				  extents->op,
				  &extents->source_pattern.base,
				  &pattern.base,
				  extents->clip);
    _comac_pattern_fini (&pattern.base);

    return status;
}

void
_comac_mask_compositor_init (comac_mask_compositor_t *compositor,
			     const comac_compositor_t *delegate)
{
    compositor->base.delegate = delegate;

    compositor->base.paint = _comac_mask_compositor_paint;
    compositor->base.mask = _comac_mask_compositor_mask;
    compositor->base.fill = _comac_mask_compositor_fill;
    compositor->base.stroke = _comac_mask_compositor_stroke;
    compositor->base.glyphs = _comac_mask_compositor_glyphs;
}
