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

#include "comacint.h"

#include "comac-box-inline.h"
#include "comac-boxes-private.h"
#include "comac-clip-inline.h"
#include "comac-clip-private.h"
#include "comac-composite-rectangles-private.h"
#include "comac-compositor-private.h"
#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-pattern-inline.h"
#include "comac-paginated-private.h"
#include "comac-recording-surface-inline.h"
#include "comac-surface-subsurface-private.h"
#include "comac-surface-snapshot-inline.h"
#include "comac-surface-observer-private.h"
#include "comac-region-private.h"
#include "comac-spans-private.h"
#include "comac-traps-private.h"
#include "comac-tristrip-private.h"

typedef comac_int_status_t (*draw_func_t) (
    const comac_traps_compositor_t *compositor,
    comac_surface_t *dst,
    void *closure,
    comac_operator_t op,
    comac_surface_t *src,
    int src_x,
    int src_y,
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
    const comac_traps_compositor_t *compositor;
    comac_surface_t *dst;
    comac_boxes_t boxes;
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

    if (COMAC_ALPHA_SHORT_IS_OPAQUE (coverage))
	return;

    _comac_box_from_integers (&info->boxes.chunks.base[0], x, y, w, h);

    _comac_color_init_rgba (&color, 0, 0, 0, coverage / (double) 0xffff);
    info->compositor->fill_boxes (info->dst,
				  COMAC_OPERATOR_IN,
				  &color,
				  &info->boxes);
}

static void
add_rect_with_offset (
    comac_boxes_t *boxes, int x1, int y1, int x2, int y2, int dx, int dy)
{
    comac_box_t box;
    comac_int_status_t status;

    box.p1.x = _comac_fixed_from_int (x1 - dx);
    box.p1.y = _comac_fixed_from_int (y1 - dy);
    box.p2.x = _comac_fixed_from_int (x2 - dx);
    box.p2.y = _comac_fixed_from_int (y2 - dy);

    status = _comac_boxes_add (boxes, COMAC_ANTIALIAS_DEFAULT, &box);
    assert (status == COMAC_INT_STATUS_SUCCESS);
}

static comac_int_status_t
combine_clip_as_traps (const comac_traps_compositor_t *compositor,
		       comac_surface_t *mask,
		       const comac_clip_t *clip,
		       const comac_rectangle_int_t *extents)
{
    comac_polygon_t polygon;
    comac_fill_rule_t fill_rule;
    comac_antialias_t antialias;
    comac_traps_t traps;
    comac_surface_t *src;
    comac_box_t box;
    comac_rectangle_int_t fixup;
    int src_x, src_y;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = _comac_clip_get_polygon (clip, &polygon, &fill_rule, &antialias);
    if (status)
	return status;

    _comac_traps_init (&traps);
    status =
	_comac_bentley_ottmann_tessellate_polygon (&traps, &polygon, fill_rule);
    _comac_polygon_fini (&polygon);
    if (unlikely (status))
	return status;

    src = compositor->pattern_to_surface (mask,
					  NULL,
					  FALSE,
					  extents,
					  NULL,
					  &src_x,
					  &src_y);
    if (unlikely (src->status)) {
	_comac_traps_fini (&traps);
	return src->status;
    }

    status = compositor->composite_traps (mask,
					  COMAC_OPERATOR_IN,
					  src,
					  src_x,
					  src_y,
					  extents->x,
					  extents->y,
					  extents,
					  antialias,
					  &traps);

    _comac_traps_extents (&traps, &box);
    _comac_box_round_to_rectangle (&box, &fixup);
    _comac_traps_fini (&traps);
    comac_surface_destroy (src);

    if (unlikely (status))
	return status;

    if (! _comac_rectangle_intersect (&fixup, extents))
	return COMAC_STATUS_SUCCESS;

    if (fixup.width < extents->width || fixup.height < extents->height) {
	comac_boxes_t clear;

	_comac_boxes_init (&clear);

	/* top */
	if (fixup.y != extents->y) {
	    add_rect_with_offset (&clear,
				  extents->x,
				  extents->y,
				  extents->x + extents->width,
				  fixup.y,
				  extents->x,
				  extents->y);
	}
	/* left */
	if (fixup.x != extents->x) {
	    add_rect_with_offset (&clear,
				  extents->x,
				  fixup.y,
				  fixup.x,
				  fixup.y + fixup.height,
				  extents->x,
				  extents->y);
	}
	/* right */
	if (fixup.x + fixup.width != extents->x + extents->width) {
	    add_rect_with_offset (&clear,
				  fixup.x + fixup.width,
				  fixup.y,
				  extents->x + extents->width,
				  fixup.y + fixup.height,
				  extents->x,
				  extents->y);
	}
	/* bottom */
	if (fixup.y + fixup.height != extents->y + extents->height) {
	    add_rect_with_offset (&clear,
				  extents->x,
				  fixup.y + fixup.height,
				  extents->x + extents->width,
				  extents->y + extents->height,
				  extents->x,
				  extents->y);
	}

	status = compositor->fill_boxes (mask,
					 COMAC_OPERATOR_CLEAR,
					 COMAC_COLOR_TRANSPARENT,
					 &clear);

	_comac_boxes_fini (&clear);
    }

    return status;
}

static comac_status_t
__clip_to_surface (const comac_traps_compositor_t *compositor,
		   const comac_composite_rectangles_t *composite,
		   const comac_rectangle_int_t *extents,
		   comac_surface_t **surface)
{
    comac_surface_t *mask;
    comac_polygon_t polygon;
    comac_fill_rule_t fill_rule;
    comac_antialias_t antialias;
    comac_traps_t traps;
    comac_boxes_t clear;
    comac_surface_t *src;
    int src_x, src_y;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = _comac_clip_get_polygon (composite->clip,
				      &polygon,
				      &fill_rule,
				      &antialias);
    if (status)
	return status;

    _comac_traps_init (&traps);
    status =
	_comac_bentley_ottmann_tessellate_polygon (&traps, &polygon, fill_rule);
    _comac_polygon_fini (&polygon);
    if (unlikely (status))
	return status;

    mask = _comac_surface_create_scratch (composite->surface,
					  COMAC_CONTENT_ALPHA,
					  extents->width,
					  extents->height,
					  NULL);
    if (unlikely (mask->status)) {
	_comac_traps_fini (&traps);
	return status;
    }

    src = compositor->pattern_to_surface (mask,
					  NULL,
					  FALSE,
					  extents,
					  NULL,
					  &src_x,
					  &src_y);
    if (unlikely (status = src->status))
	goto error;

    status = compositor->acquire (mask);
    if (unlikely (status))
	goto error;

    _comac_boxes_init_from_rectangle (&clear,
				      0,
				      0,
				      extents->width,
				      extents->height);
    status = compositor->fill_boxes (mask,
				     COMAC_OPERATOR_CLEAR,
				     COMAC_COLOR_TRANSPARENT,
				     &clear);
    if (unlikely (status))
	goto error_release;

    status = compositor->composite_traps (mask,
					  COMAC_OPERATOR_ADD,
					  src,
					  src_x,
					  src_y,
					  extents->x,
					  extents->y,
					  extents,
					  antialias,
					  &traps);
    if (unlikely (status))
	goto error_release;

    compositor->release (mask);
    *surface = mask;
out:
    comac_surface_destroy (src);
    _comac_traps_fini (&traps);
    return status;

error_release:
    compositor->release (mask);
error:
    comac_surface_destroy (mask);
    goto out;
}

static comac_surface_t *
traps_get_clip_surface (const comac_traps_compositor_t *compositor,
			const comac_composite_rectangles_t *composite,
			const comac_rectangle_int_t *extents)
{
    comac_surface_t *surface = NULL;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = __clip_to_surface (compositor, composite, extents, &surface);
    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	surface = _comac_surface_create_scratch (composite->surface,
						 COMAC_CONTENT_ALPHA,
						 extents->width,
						 extents->height,
						 COMAC_COLOR_WHITE);
	if (unlikely (surface->status))
	    return surface;

	status = _comac_clip_combine_with_surface (composite->clip,
						   surface,
						   extents->x,
						   extents->y);
    }
    if (unlikely (status)) {
	comac_surface_destroy (surface);
	surface = _comac_surface_create_in_error (status);
    }

    return surface;
}

static void
blt_unaligned_boxes (const comac_traps_compositor_t *compositor,
		     comac_surface_t *surface,
		     int dx,
		     int dy,
		     comac_box_t *boxes,
		     int num_boxes)
{
    struct blt_in info;
    int i;

    info.compositor = compositor;
    info.dst = surface;
    _comac_boxes_init (&info.boxes);
    info.boxes.num_boxes = 1;
    for (i = 0; i < num_boxes; i++) {
	comac_box_t *b = &boxes[i];

	if (! _comac_fixed_is_integer (b->p1.x) ||
	    ! _comac_fixed_is_integer (b->p1.y) ||
	    ! _comac_fixed_is_integer (b->p2.x) ||
	    ! _comac_fixed_is_integer (b->p2.y)) {
	    do_unaligned_box (blt_in, &info, b, dx, dy);
	}
    }
}

static comac_surface_t *
create_composite_mask (const comac_traps_compositor_t *compositor,
		       comac_surface_t *dst,
		       void *draw_closure,
		       draw_func_t draw_func,
		       draw_func_t mask_func,
		       const comac_composite_rectangles_t *extents)
{
    comac_surface_t *surface, *src;
    comac_int_status_t status;
    int src_x, src_y;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    surface = _comac_surface_create_scratch (dst,
					     COMAC_CONTENT_ALPHA,
					     extents->bounded.width,
					     extents->bounded.height,
					     NULL);
    if (unlikely (surface->status))
	return surface;

    src = compositor->pattern_to_surface (surface,
					  &_comac_pattern_white.base,
					  FALSE,
					  &extents->bounded,
					  &extents->bounded,
					  &src_x,
					  &src_y);
    if (unlikely (src->status)) {
	comac_surface_destroy (surface);
	return src;
    }

    status = compositor->acquire (surface);
    if (unlikely (status)) {
	comac_surface_destroy (src);
	comac_surface_destroy (surface);
	return _comac_surface_create_in_error (status);
    }

    if (! surface->is_clear) {
	comac_boxes_t clear;

	_comac_boxes_init_from_rectangle (&clear,
					  0,
					  0,
					  extents->bounded.width,
					  extents->bounded.height);
	status = compositor->fill_boxes (surface,
					 COMAC_OPERATOR_CLEAR,
					 COMAC_COLOR_TRANSPARENT,
					 &clear);
	if (unlikely (status))
	    goto error;

	surface->is_clear = TRUE;
    }

    if (mask_func) {
	status = mask_func (compositor,
			    surface,
			    draw_closure,
			    COMAC_OPERATOR_SOURCE,
			    src,
			    src_x,
			    src_y,
			    extents->bounded.x,
			    extents->bounded.y,
			    &extents->bounded,
			    extents->clip);
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    surface->is_clear = FALSE;
	    goto out;
	}
	if (unlikely (status != COMAC_INT_STATUS_UNSUPPORTED))
	    goto error;
    }

    /* Is it worth setting the clip region here? */
    status = draw_func (compositor,
			surface,
			draw_closure,
			COMAC_OPERATOR_ADD,
			src,
			src_x,
			src_y,
			extents->bounded.x,
			extents->bounded.y,
			&extents->bounded,
			NULL);
    if (unlikely (status))
	goto error;

    surface->is_clear = FALSE;
    if (extents->clip->path != NULL) {
	status = combine_clip_as_traps (compositor,
					surface,
					extents->clip,
					&extents->bounded);
	if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	    status = _comac_clip_combine_with_surface (extents->clip,
						       surface,
						       extents->bounded.x,
						       extents->bounded.y);
	}
	if (unlikely (status))
	    goto error;
    } else if (extents->clip->boxes) {
	blt_unaligned_boxes (compositor,
			     surface,
			     extents->bounded.x,
			     extents->bounded.y,
			     extents->clip->boxes,
			     extents->clip->num_boxes);
    }

out:
    compositor->release (surface);
    comac_surface_destroy (src);
    return surface;

error:
    compositor->release (surface);
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO) {
	comac_surface_destroy (surface);
	surface = _comac_surface_create_in_error (status);
    }
    comac_surface_destroy (src);
    return surface;
}

/* Handles compositing with a clip surface when the operator allows
 * us to combine the clip with the mask
 */
static comac_status_t
clip_and_composite_with_mask (const comac_traps_compositor_t *compositor,
			      const comac_composite_rectangles_t *extents,
			      draw_func_t draw_func,
			      draw_func_t mask_func,
			      void *draw_closure,
			      comac_operator_t op,
			      comac_surface_t *src,
			      int src_x,
			      int src_y)
{
    comac_surface_t *dst = extents->surface;
    comac_surface_t *mask;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    mask = create_composite_mask (compositor,
				  dst,
				  draw_closure,
				  draw_func,
				  mask_func,
				  extents);
    if (unlikely (mask->status))
	return mask->status;

    if (mask->is_clear)
	goto skip;

    if (src != NULL || dst->content != COMAC_CONTENT_ALPHA) {
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

skip:
    comac_surface_destroy (mask);
    return COMAC_STATUS_SUCCESS;
}

/* Handles compositing with a clip surface when we have to do the operation
 * in two pieces and combine them together.
 */
static comac_status_t
clip_and_composite_combine (const comac_traps_compositor_t *compositor,
			    const comac_composite_rectangles_t *extents,
			    draw_func_t draw_func,
			    void *draw_closure,
			    comac_operator_t op,
			    comac_surface_t *src,
			    int src_x,
			    int src_y)
{
    comac_surface_t *dst = extents->surface;
    comac_surface_t *tmp, *clip;
    comac_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    tmp = _comac_surface_create_scratch (dst,
					 dst->content,
					 extents->bounded.width,
					 extents->bounded.height,
					 NULL);
    if (unlikely (tmp->status))
	return tmp->status;

    status = compositor->acquire (tmp);
    if (unlikely (status)) {
	comac_surface_destroy (tmp);
	return status;
    }

    compositor->composite (tmp,
			   dst->is_clear ? COMAC_OPERATOR_CLEAR
					 : COMAC_OPERATOR_SOURCE,
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
			src,
			src_x,
			src_y,
			extents->bounded.x,
			extents->bounded.y,
			&extents->bounded,
			NULL);

    if (unlikely (status))
	goto cleanup;

    clip = traps_get_clip_surface (compositor, extents, &extents->bounded);
    if (unlikely ((status = clip->status)))
	goto cleanup;

    if (dst->is_clear) {
	compositor->composite (dst,
			       COMAC_OPERATOR_SOURCE,
			       tmp,
			       clip,
			       0,
			       0,
			       0,
			       0,
			       extents->bounded.x,
			       extents->bounded.y,
			       extents->bounded.width,
			       extents->bounded.height);
    } else {
	compositor->lerp (dst,
			  tmp,
			  clip,
			  0,
			  0,
			  0,
			  0,
			  extents->bounded.x,
			  extents->bounded.y,
			  extents->bounded.width,
			  extents->bounded.height);
    }
    comac_surface_destroy (clip);

cleanup:
    compositor->release (tmp);
    comac_surface_destroy (tmp);

    return status;
}

/* Handles compositing for %COMAC_OPERATOR_SOURCE, which is special; it's
 * defined as (src IN mask IN clip) ADD (dst OUT (mask IN clip))
 */
static comac_status_t
clip_and_composite_source (const comac_traps_compositor_t *compositor,
			   comac_surface_t *dst,
			   draw_func_t draw_func,
			   draw_func_t mask_func,
			   void *draw_closure,
			   comac_surface_t *src,
			   int src_x,
			   int src_y,
			   const comac_composite_rectangles_t *extents)
{
    comac_surface_t *mask;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    /* Create a surface that is mask IN clip */
    mask = create_composite_mask (compositor,
				  dst,
				  draw_closure,
				  draw_func,
				  mask_func,
				  extents);
    if (unlikely (mask->status))
	return mask->status;

    if (mask->is_clear)
	goto skip;

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
	compositor->lerp (dst,
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

skip:
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
reduce_alpha_op (comac_composite_rectangles_t *extents)
{
    comac_surface_t *dst = extents->surface;
    comac_operator_t op = extents->op;
    const comac_pattern_t *pattern = &extents->source_pattern.base;
    return dst->is_clear && dst->content == COMAC_CONTENT_ALPHA &&
	   _comac_pattern_is_opaque_solid (pattern) && can_reduce_alpha_op (op);
}

static comac_status_t
fixup_unbounded_with_mask (const comac_traps_compositor_t *compositor,
			   const comac_composite_rectangles_t *extents)
{
    comac_surface_t *dst = extents->surface;
    comac_surface_t *mask;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    /* XXX can we avoid querying the clip surface again? */
    mask = traps_get_clip_surface (compositor, extents, &extents->unbounded);
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
			       0,
			       0,
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
			       0,
			       y - extents->unbounded.y,
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
			       x - extents->unbounded.x,
			       y - extents->unbounded.y,
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
			       0,
			       y - extents->unbounded.y,
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

static void
add_rect (comac_boxes_t *boxes, int x1, int y1, int x2, int y2)
{
    comac_box_t box;
    comac_int_status_t status;

    box.p1.x = _comac_fixed_from_int (x1);
    box.p1.y = _comac_fixed_from_int (y1);
    box.p2.x = _comac_fixed_from_int (x2);
    box.p2.y = _comac_fixed_from_int (y2);

    status = _comac_boxes_add (boxes, COMAC_ANTIALIAS_DEFAULT, &box);
    assert (status == COMAC_INT_STATUS_SUCCESS);
}

static comac_status_t
fixup_unbounded (const comac_traps_compositor_t *compositor,
		 comac_composite_rectangles_t *extents,
		 comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    comac_boxes_t clear, tmp;
    comac_box_t box;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (extents->bounded.width == extents->unbounded.width &&
	extents->bounded.height == extents->unbounded.height) {
	return COMAC_STATUS_SUCCESS;
    }

    assert (extents->clip->path == NULL);

    /* subtract the drawn boxes from the unbounded area */
    _comac_boxes_init (&clear);

    box.p1.x =
	_comac_fixed_from_int (extents->unbounded.x + extents->unbounded.width);
    box.p1.y = _comac_fixed_from_int (extents->unbounded.y);
    box.p2.x = _comac_fixed_from_int (extents->unbounded.x);
    box.p2.y = _comac_fixed_from_int (extents->unbounded.y +
				      extents->unbounded.height);

    if (boxes == NULL) {
	if (extents->bounded.width == 0 || extents->bounded.height == 0) {
	    goto empty;
	} else {
	    /* top */
	    if (extents->bounded.y != extents->unbounded.y) {
		add_rect (&clear,
			  extents->unbounded.x,
			  extents->unbounded.y,
			  extents->unbounded.x + extents->unbounded.width,
			  extents->bounded.y);
	    }
	    /* left */
	    if (extents->bounded.x != extents->unbounded.x) {
		add_rect (&clear,
			  extents->unbounded.x,
			  extents->bounded.y,
			  extents->bounded.x,
			  extents->bounded.y + extents->bounded.height);
	    }
	    /* right */
	    if (extents->bounded.x + extents->bounded.width !=
		extents->unbounded.x + extents->unbounded.width) {
		add_rect (&clear,
			  extents->bounded.x + extents->bounded.width,
			  extents->bounded.y,
			  extents->unbounded.x + extents->unbounded.width,
			  extents->bounded.y + extents->bounded.height);
	    }
	    /* bottom */
	    if (extents->bounded.y + extents->bounded.height !=
		extents->unbounded.y + extents->unbounded.height) {
		add_rect (&clear,
			  extents->unbounded.x,
			  extents->bounded.y + extents->bounded.height,
			  extents->unbounded.x + extents->unbounded.width,
			  extents->unbounded.y + extents->unbounded.height);
	    }
	}
    } else if (boxes->num_boxes) {
	_comac_boxes_init (&tmp);

	assert (boxes->is_pixel_aligned);

	status = _comac_boxes_add (&tmp, COMAC_ANTIALIAS_DEFAULT, &box);
	assert (status == COMAC_INT_STATUS_SUCCESS);

	tmp.chunks.next = &boxes->chunks;
	tmp.num_boxes += boxes->num_boxes;

	status =
	    _comac_bentley_ottmann_tessellate_boxes (&tmp,
						     COMAC_FILL_RULE_WINDING,
						     &clear);
	tmp.chunks.next = NULL;
	if (unlikely (status))
	    goto error;
    } else {
    empty:
	box.p1.x = _comac_fixed_from_int (extents->unbounded.x);
	box.p2.x = _comac_fixed_from_int (extents->unbounded.x +
					  extents->unbounded.width);

	status = _comac_boxes_add (&clear, COMAC_ANTIALIAS_DEFAULT, &box);
	assert (status == COMAC_INT_STATUS_SUCCESS);
    }

    /* Now intersect with the clip boxes */
    if (extents->clip->num_boxes) {
	_comac_boxes_init_for_array (&tmp,
				     extents->clip->boxes,
				     extents->clip->num_boxes);
	status = _comac_boxes_intersect (&clear, &tmp, &clear);
	if (unlikely (status))
	    goto error;
    }

    status = compositor->fill_boxes (dst,
				     COMAC_OPERATOR_CLEAR,
				     COMAC_COLOR_TRANSPARENT,
				     &clear);

error:
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
    unsigned int flags = 0;

    if (extents->clip->num_boxes > 1 ||
	extents->mask.width > extents->unbounded.width ||
	extents->mask.height > extents->unbounded.height) {
	flags |= NEED_CLIP_REGION;
    }

    if (extents->clip->num_boxes > 1 ||
	extents->mask.width > extents->bounded.width ||
	extents->mask.height > extents->bounded.height) {
	flags |= FORCE_CLIP_REGION;
    }

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
clip_and_composite (const comac_traps_compositor_t *compositor,
		    comac_composite_rectangles_t *extents,
		    draw_func_t draw_func,
		    draw_func_t mask_func,
		    void *draw_closure,
		    unsigned int need_clip)
{
    comac_surface_t *dst = extents->surface;
    comac_operator_t op = extents->op;
    comac_pattern_t *source = &extents->source_pattern.base;
    comac_surface_t *src;
    int src_x, src_y;
    comac_region_t *clip_region = NULL;
    comac_status_t status = COMAC_STATUS_SUCCESS;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (reduce_alpha_op (extents)) {
	op = COMAC_OPERATOR_ADD;
	source = NULL;
    }

    if (op == COMAC_OPERATOR_CLEAR) {
	op = COMAC_OPERATOR_DEST_OUT;
	source = NULL;
    }

    compositor->acquire (dst);

    if (need_clip & NEED_CLIP_REGION) {
	const comac_rectangle_int_t *limit;

	if ((need_clip & FORCE_CLIP_REGION) == 0)
	    limit = &extents->unbounded;
	else
	    limit = &extents->destination;

	clip_region = _comac_clip_get_region (extents->clip);
	if (clip_region != NULL &&
	    comac_region_contains_rectangle (clip_region, limit) ==
		COMAC_REGION_OVERLAP_IN)
	    clip_region = NULL;

	if (clip_region != NULL) {
	    status = compositor->set_clip_region (dst, clip_region);
	    if (unlikely (status)) {
		compositor->release (dst);
		return status;
	    }
	}
    }

    if (extents->bounded.width == 0 || extents->bounded.height == 0)
	goto skip;

    src = compositor->pattern_to_surface (dst,
					  source,
					  FALSE,
					  &extents->bounded,
					  &extents->source_sample_area,
					  &src_x,
					  &src_y);
    if (unlikely (status = src->status))
	goto error;

    if (op == COMAC_OPERATOR_SOURCE) {
	status = clip_and_composite_source (compositor,
					    dst,
					    draw_func,
					    mask_func,
					    draw_closure,
					    src,
					    src_x,
					    src_y,
					    extents);
    } else {
	if (need_clip & NEED_CLIP_SURFACE) {
	    if (extents->is_bounded) {
		status = clip_and_composite_with_mask (compositor,
						       extents,
						       draw_func,
						       mask_func,
						       draw_closure,
						       op,
						       src,
						       src_x,
						       src_y);
	    } else {
		status = clip_and_composite_combine (compositor,
						     extents,
						     draw_func,
						     draw_closure,
						     op,
						     src,
						     src_x,
						     src_y);
	    }
	} else {
	    status = draw_func (compositor,
				dst,
				draw_closure,
				op,
				src,
				src_x,
				src_y,
				0,
				0,
				&extents->bounded,
				extents->clip);
	}
    }
    comac_surface_destroy (src);

skip:
    if (status == COMAC_STATUS_SUCCESS && ! extents->is_bounded) {
	if (need_clip & NEED_CLIP_SURFACE)
	    status = fixup_unbounded_with_mask (compositor, extents);
	else
	    status = fixup_unbounded (compositor, extents, NULL);
    }

error:
    if (clip_region)
	compositor->set_clip_region (dst, NULL);

    compositor->release (dst);

    return status;
}

/* meta-ops */

typedef struct {
    comac_traps_t traps;
    comac_antialias_t antialias;
} composite_traps_info_t;

static comac_int_status_t
composite_traps (const comac_traps_compositor_t *compositor,
		 comac_surface_t *dst,
		 void *closure,
		 comac_operator_t op,
		 comac_surface_t *src,
		 int src_x,
		 int src_y,
		 int dst_x,
		 int dst_y,
		 const comac_rectangle_int_t *extents,
		 comac_clip_t *clip)
{
    composite_traps_info_t *info = closure;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    return compositor->composite_traps (dst,
					op,
					src,
					src_x - dst_x,
					src_y - dst_y,
					dst_x,
					dst_y,
					extents,
					info->antialias,
					&info->traps);
}

typedef struct {
    comac_tristrip_t strip;
    comac_antialias_t antialias;
} composite_tristrip_info_t;

static comac_int_status_t
composite_tristrip (const comac_traps_compositor_t *compositor,
		    comac_surface_t *dst,
		    void *closure,
		    comac_operator_t op,
		    comac_surface_t *src,
		    int src_x,
		    int src_y,
		    int dst_x,
		    int dst_y,
		    const comac_rectangle_int_t *extents,
		    comac_clip_t *clip)
{
    composite_tristrip_info_t *info = closure;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    return compositor->composite_tristrip (dst,
					   op,
					   src,
					   src_x - dst_x,
					   src_y - dst_y,
					   dst_x,
					   dst_y,
					   extents,
					   info->antialias,
					   &info->strip);
}

static comac_bool_t
is_recording_pattern (const comac_pattern_t *pattern)
{
    comac_surface_t *surface;

    if (pattern->type != COMAC_PATTERN_TYPE_SURFACE)
	return FALSE;

    surface = ((const comac_surface_pattern_t *) pattern)->surface;
    surface = _comac_surface_get_source (surface, NULL);
    return _comac_surface_is_recording (surface);
}

static comac_surface_t *
recording_pattern_get_surface (const comac_pattern_t *pattern)
{
    comac_surface_t *surface;

    surface = ((const comac_surface_pattern_t *) pattern)->surface;
    return _comac_surface_get_source (surface, NULL);
}

static comac_bool_t
recording_pattern_contains_sample (const comac_pattern_t *pattern,
				   const comac_rectangle_int_t *sample)
{
    comac_recording_surface_t *surface;

    if (! is_recording_pattern (pattern))
	return FALSE;

    if (pattern->extend == COMAC_EXTEND_NONE)
	return TRUE;

    surface =
	(comac_recording_surface_t *) recording_pattern_get_surface (pattern);
    if (surface->unbounded)
	return TRUE;

    return _comac_rectangle_contains_rectangle (&surface->extents, sample);
}

static comac_bool_t
op_reduces_to_source (comac_composite_rectangles_t *extents)
{
    if (extents->op == COMAC_OPERATOR_SOURCE)
	return TRUE;

    if (extents->surface->is_clear)
	return extents->op == COMAC_OPERATOR_OVER ||
	       extents->op == COMAC_OPERATOR_ADD;

    return FALSE;
}

static comac_status_t
composite_aligned_boxes (const comac_traps_compositor_t *compositor,
			 comac_composite_rectangles_t *extents,
			 comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    comac_operator_t op = extents->op;
    comac_bool_t need_clip_mask = ! _comac_clip_is_region (extents->clip);
    comac_bool_t op_is_source;
    comac_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (need_clip_mask &&
	(! extents->is_bounded || extents->op == COMAC_OPERATOR_SOURCE)) {
	return COMAC_INT_STATUS_UNSUPPORTED;
    }

    op_is_source = op_reduces_to_source (extents);

    /* Are we just copying a recording surface? */
    if (! need_clip_mask && op_is_source &&
	recording_pattern_contains_sample (&extents->source_pattern.base,
					   &extents->source_sample_area)) {
	comac_clip_t *recording_clip;
	const comac_pattern_t *source = &extents->source_pattern.base;
	const comac_matrix_t *m;
	comac_matrix_t matrix;

	/* XXX could also do tiling repeat modes... */

	/* first clear the area about to be overwritten */
	if (! dst->is_clear) {
	    status = compositor->acquire (dst);
	    if (unlikely (status))
		return status;

	    status = compositor->fill_boxes (dst,
					     COMAC_OPERATOR_CLEAR,
					     COMAC_COLOR_TRANSPARENT,
					     boxes);
	    compositor->release (dst);
	    if (unlikely (status))
		return status;
	}

	m = &source->matrix;
	if (_comac_surface_has_device_transform (dst)) {
	    comac_matrix_multiply (&matrix,
				   &source->matrix,
				   &dst->device_transform);
	    m = &matrix;
	}

	recording_clip = _comac_clip_from_boxes (boxes);
	status = _comac_recording_surface_replay_with_clip (
	    recording_pattern_get_surface (source),
	    m,
	    dst,
	    recording_clip);
	_comac_clip_destroy (recording_clip);

	return status;
    }

    status = compositor->acquire (dst);
    if (unlikely (status))
	return status;

    if (! need_clip_mask &&
	(op == COMAC_OPERATOR_CLEAR ||
	 extents->source_pattern.base.type == COMAC_PATTERN_TYPE_SOLID)) {
	const comac_color_t *color;

	if (op == COMAC_OPERATOR_CLEAR) {
	    color = COMAC_COLOR_TRANSPARENT;
	} else {
	    color =
		&((comac_solid_pattern_t *) &extents->source_pattern)->color;
	    if (op_is_source)
		op = COMAC_OPERATOR_SOURCE;
	}

	status = compositor->fill_boxes (dst, op, color, boxes);
    } else {
	comac_surface_t *src, *mask = NULL;
	comac_pattern_t *source = &extents->source_pattern.base;
	int src_x, src_y;
	int mask_x = 0, mask_y = 0;

	if (need_clip_mask) {
	    mask =
		traps_get_clip_surface (compositor, extents, &extents->bounded);
	    if (unlikely (mask->status))
		return mask->status;

	    mask_x = -extents->bounded.x;
	    mask_y = -extents->bounded.y;

	    if (op == COMAC_OPERATOR_CLEAR) {
		source = NULL;
		op = COMAC_OPERATOR_DEST_OUT;
	    }
	} else if (op_is_source)
	    op = COMAC_OPERATOR_SOURCE;

	src = compositor->pattern_to_surface (dst,
					      source,
					      FALSE,
					      &extents->bounded,
					      &extents->source_sample_area,
					      &src_x,
					      &src_y);
	if (likely (src->status == COMAC_STATUS_SUCCESS)) {
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
	} else
	    status = src->status;

	comac_surface_destroy (mask);
    }

    if (status == COMAC_STATUS_SUCCESS && ! extents->is_bounded)
	status = fixup_unbounded (compositor, extents, boxes);

    compositor->release (dst);

    return status;
}

static comac_status_t
upload_boxes (const comac_traps_compositor_t *compositor,
	      comac_composite_rectangles_t *extents,
	      comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    const comac_pattern_t *source = &extents->source_pattern.base;
    comac_surface_t *src;
    comac_rectangle_int_t limit;
    comac_int_status_t status;
    int tx, ty;

    TRACE ((stderr, "%s\n", __FUNCTION__));

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

static comac_int_status_t
trim_extents_to_traps (comac_composite_rectangles_t *extents,
		       comac_traps_t *traps)
{
    comac_box_t box;

    _comac_traps_extents (traps, &box);
    return _comac_composite_rectangles_intersect_mask_extents (extents, &box);
}

static comac_int_status_t
trim_extents_to_tristrip (comac_composite_rectangles_t *extents,
			  comac_tristrip_t *strip)
{
    comac_box_t box;

    _comac_tristrip_extents (strip, &box);
    return _comac_composite_rectangles_intersect_mask_extents (extents, &box);
}

static comac_int_status_t
trim_extents_to_boxes (comac_composite_rectangles_t *extents,
		       comac_boxes_t *boxes)
{
    comac_box_t box;

    _comac_boxes_extents (boxes, &box);
    return _comac_composite_rectangles_intersect_mask_extents (extents, &box);
}

static comac_int_status_t
boxes_for_traps (comac_boxes_t *boxes,
		 comac_traps_t *traps,
		 comac_antialias_t antialias)
{
    int i, j;

    /* first check that the traps are rectilinear */
    if (antialias == COMAC_ANTIALIAS_NONE) {
	for (i = 0; i < traps->num_traps; i++) {
	    const comac_trapezoid_t *t = &traps->traps[i];
	    if (_comac_fixed_integer_round_down (t->left.p1.x) !=
		    _comac_fixed_integer_round_down (t->left.p2.x) ||
		_comac_fixed_integer_round_down (t->right.p1.x) !=
		    _comac_fixed_integer_round_down (t->right.p2.x)) {
		return COMAC_INT_STATUS_UNSUPPORTED;
	    }
	}
    } else {
	for (i = 0; i < traps->num_traps; i++) {
	    const comac_trapezoid_t *t = &traps->traps[i];
	    if (t->left.p1.x != t->left.p2.x || t->right.p1.x != t->right.p2.x)
		return COMAC_INT_STATUS_UNSUPPORTED;
	}
    }

    _comac_boxes_init (boxes);

    boxes->chunks.base = (comac_box_t *) traps->traps;
    boxes->chunks.size = traps->num_traps;

    if (antialias != COMAC_ANTIALIAS_NONE) {
	for (i = j = 0; i < traps->num_traps; i++) {
	    /* Note the traps and boxes alias so we need to take the local copies first. */
	    comac_fixed_t x1 = traps->traps[i].left.p1.x;
	    comac_fixed_t x2 = traps->traps[i].right.p1.x;
	    comac_fixed_t y1 = traps->traps[i].top;
	    comac_fixed_t y2 = traps->traps[i].bottom;

	    if (x1 == x2 || y1 == y2)
		continue;

	    boxes->chunks.base[j].p1.x = x1;
	    boxes->chunks.base[j].p1.y = y1;
	    boxes->chunks.base[j].p2.x = x2;
	    boxes->chunks.base[j].p2.y = y2;
	    j++;

	    if (boxes->is_pixel_aligned) {
		boxes->is_pixel_aligned = _comac_fixed_is_integer (x1) &&
					  _comac_fixed_is_integer (y1) &&
					  _comac_fixed_is_integer (x2) &&
					  _comac_fixed_is_integer (y2);
	    }
	}
    } else {
	boxes->is_pixel_aligned = TRUE;

	for (i = j = 0; i < traps->num_traps; i++) {
	    /* Note the traps and boxes alias so we need to take the local copies first. */
	    comac_fixed_t x1 = traps->traps[i].left.p1.x;
	    comac_fixed_t x2 = traps->traps[i].right.p1.x;
	    comac_fixed_t y1 = traps->traps[i].top;
	    comac_fixed_t y2 = traps->traps[i].bottom;

	    /* round down here to match Pixman's behavior when using traps. */
	    boxes->chunks.base[j].p1.x = _comac_fixed_round_down (x1);
	    boxes->chunks.base[j].p1.y = _comac_fixed_round_down (y1);
	    boxes->chunks.base[j].p2.x = _comac_fixed_round_down (x2);
	    boxes->chunks.base[j].p2.y = _comac_fixed_round_down (y2);
	    j += (boxes->chunks.base[j].p1.x != boxes->chunks.base[j].p2.x &&
		  boxes->chunks.base[j].p1.y != boxes->chunks.base[j].p2.y);
	}
    }
    boxes->chunks.count = j;
    boxes->num_boxes = j;

    return COMAC_INT_STATUS_SUCCESS;
}

static comac_status_t
clip_and_composite_boxes (const comac_traps_compositor_t *compositor,
			  comac_composite_rectangles_t *extents,
			  comac_boxes_t *boxes);

static comac_status_t
clip_and_composite_polygon (const comac_traps_compositor_t *compositor,
			    comac_composite_rectangles_t *extents,
			    comac_polygon_t *polygon,
			    comac_antialias_t antialias,
			    comac_fill_rule_t fill_rule,
			    comac_bool_t curvy)
{
    composite_traps_info_t traps;
    comac_surface_t *dst = extents->surface;
    comac_bool_t clip_surface = ! _comac_clip_is_region (extents->clip);
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (polygon->num_edges == 0) {
	status = COMAC_INT_STATUS_SUCCESS;

	if (! extents->is_bounded) {
	    comac_region_t *clip_region =
		_comac_clip_get_region (extents->clip);

	    if (clip_region &&
		comac_region_contains_rectangle (clip_region,
						 &extents->unbounded) ==
		    COMAC_REGION_OVERLAP_IN)
		clip_region = NULL;

	    if (clip_region != NULL) {
		status = compositor->set_clip_region (dst, clip_region);
		if (unlikely (status))
		    return status;
	    }

	    if (clip_surface)
		status = fixup_unbounded_with_mask (compositor, extents);
	    else
		status = fixup_unbounded (compositor, extents, NULL);

	    if (clip_region != NULL)
		compositor->set_clip_region (dst, NULL);
	}

	return status;
    }

    if (extents->clip->path != NULL && extents->is_bounded) {
	comac_polygon_t clipper;
	comac_fill_rule_t clipper_fill_rule;
	comac_antialias_t clipper_antialias;

	status = _comac_clip_get_polygon (extents->clip,
					  &clipper,
					  &clipper_fill_rule,
					  &clipper_antialias);
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    if (clipper_antialias == antialias) {
		status = _comac_polygon_intersect (polygon,
						   fill_rule,
						   &clipper,
						   clipper_fill_rule);
		if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
		    comac_clip_t *clip =
			_comac_clip_copy_region (extents->clip);
		    _comac_clip_destroy (extents->clip);
		    extents->clip = clip;

		    fill_rule = COMAC_FILL_RULE_WINDING;
		}
		_comac_polygon_fini (&clipper);
	    }
	}
    }

    if (antialias == COMAC_ANTIALIAS_NONE && curvy) {
	comac_boxes_t boxes;

	_comac_boxes_init (&boxes);
	status = _comac_rasterise_polygon_to_boxes (polygon, fill_rule, &boxes);
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    assert (boxes.is_pixel_aligned);
	    status = clip_and_composite_boxes (compositor, extents, &boxes);
	}
	_comac_boxes_fini (&boxes);
	if ((status != COMAC_INT_STATUS_UNSUPPORTED))
	    return status;
    }

    _comac_traps_init (&traps.traps);

    if (antialias == COMAC_ANTIALIAS_NONE && curvy) {
	status = _comac_rasterise_polygon_to_traps (polygon,
						    fill_rule,
						    antialias,
						    &traps.traps);
    } else {
	status = _comac_bentley_ottmann_tessellate_polygon (&traps.traps,
							    polygon,
							    fill_rule);
    }
    if (unlikely (status))
	goto CLEANUP_TRAPS;

    status = trim_extents_to_traps (extents, &traps.traps);
    if (unlikely (status))
	goto CLEANUP_TRAPS;

    /* Use a fast path if the trapezoids consist of a set of boxes.  */
    status = COMAC_INT_STATUS_UNSUPPORTED;
    if (1) {
	comac_boxes_t boxes;

	status = boxes_for_traps (&boxes, &traps.traps, antialias);
	if (status == COMAC_INT_STATUS_SUCCESS) {
	    status = clip_and_composite_boxes (compositor, extents, &boxes);
	    /* XXX need to reconstruct the traps! */
	    assert (status != COMAC_INT_STATUS_UNSUPPORTED);
	}
    }
    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	/* Otherwise render the trapezoids to a mask and composite in the usual
	 * fashion.
	 */
	unsigned int flags = 0;

	/* For unbounded operations, the X11 server will estimate the
	 * affected rectangle and apply the operation to that. However,
	 * there are cases where this is an overestimate (e.g. the
	 * clip-fill-{eo,nz}-unbounded test).
	 *
	 * The clip will trim that overestimate to our expectations.
	 */
	if (! extents->is_bounded)
	    flags |= FORCE_CLIP_REGION;

	traps.antialias = antialias;
	status = clip_and_composite (compositor,
				     extents,
				     composite_traps,
				     NULL,
				     &traps,
				     need_unbounded_clip (extents) | flags);
    }

CLEANUP_TRAPS:
    _comac_traps_fini (&traps.traps);

    return status;
}

struct composite_opacity_info {
    const comac_traps_compositor_t *compositor;
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
    const comac_traps_compositor_t *compositor = info->compositor;
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
composite_opacity_boxes (const comac_traps_compositor_t *compositor,
			 comac_surface_t *dst,
			 void *closure,
			 comac_operator_t op,
			 comac_surface_t *src,
			 int src_x,
			 int src_y,
			 int dst_x,
			 int dst_y,
			 const comac_rectangle_int_t *extents,
			 comac_clip_t *clip)
{
    const comac_solid_pattern_t *mask = closure;
    struct composite_opacity_info info;
    int i;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    info.compositor = compositor;
    info.op = op;
    info.dst = dst;

    info.src = src;
    info.src_x = src_x;
    info.src_y = src_y;

    assert (mask->color.colorspace == COMAC_COLORSPACE_RGB);
    info.opacity = mask->color.c.rgb.alpha / (double) 0xffff;

    /* XXX for lots of boxes create a clip region for the fully opaque areas */
    for (i = 0; i < clip->num_boxes; i++)
	do_unaligned_box (composite_opacity,
			  &info,
			  &clip->boxes[i],
			  dst_x,
			  dst_y);

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite_boxes (const comac_traps_compositor_t *compositor,
		 comac_surface_t *dst,
		 void *closure,
		 comac_operator_t op,
		 comac_surface_t *src,
		 int src_x,
		 int src_y,
		 int dst_x,
		 int dst_y,
		 const comac_rectangle_int_t *extents,
		 comac_clip_t *clip)
{
    comac_traps_t traps;
    comac_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = _comac_traps_init_boxes (&traps, closure);
    if (unlikely (status))
	return status;

    status = compositor->composite_traps (dst,
					  op,
					  src,
					  src_x - dst_x,
					  src_y - dst_y,
					  dst_x,
					  dst_y,
					  extents,
					  COMAC_ANTIALIAS_DEFAULT,
					  &traps);
    _comac_traps_fini (&traps);

    return status;
}

static comac_status_t
clip_and_composite_boxes (const comac_traps_compositor_t *compositor,
			  comac_composite_rectangles_t *extents,
			  comac_boxes_t *boxes)
{
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (boxes->num_boxes == 0 && extents->is_bounded)
	return COMAC_STATUS_SUCCESS;

    status = trim_extents_to_boxes (extents, boxes);
    if (unlikely (status))
	return status;

    if (boxes->is_pixel_aligned && extents->clip->path == NULL &&
	extents->source_pattern.base.type == COMAC_PATTERN_TYPE_SURFACE &&
	(op_reduces_to_source (extents) ||
	 (extents->op == COMAC_OPERATOR_OVER &&
	  (extents->source_pattern.surface.surface->content &
	   COMAC_CONTENT_ALPHA) == 0))) {
	status = upload_boxes (compositor, extents, boxes);
	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    /* Can we reduce drawing through a clip-mask to simply drawing the clip? */
    if (extents->clip->path != NULL && extents->is_bounded) {
	comac_polygon_t polygon;
	comac_fill_rule_t fill_rule;
	comac_antialias_t antialias;
	comac_clip_t *clip;

	clip = _comac_clip_copy (extents->clip);
	clip = _comac_clip_intersect_boxes (clip, boxes);
	if (_comac_clip_is_all_clipped (clip))
	    return COMAC_INT_STATUS_NOTHING_TO_DO;

	status =
	    _comac_clip_get_polygon (clip, &polygon, &fill_rule, &antialias);
	_comac_clip_path_destroy (clip->path);
	clip->path = NULL;
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    comac_clip_t *saved_clip = extents->clip;
	    extents->clip = clip;

	    status = clip_and_composite_polygon (compositor,
						 extents,
						 &polygon,
						 antialias,
						 fill_rule,
						 FALSE);

	    clip = extents->clip;
	    extents->clip = saved_clip;

	    _comac_polygon_fini (&polygon);
	}
	_comac_clip_destroy (clip);

	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    /* Use a fast path if the boxes are pixel aligned (or nearly aligned!) */
    if (boxes->is_pixel_aligned) {
	status = composite_aligned_boxes (compositor, extents, boxes);
	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return clip_and_composite (compositor,
			       extents,
			       composite_boxes,
			       NULL,
			       boxes,
			       need_unbounded_clip (extents));
}

static comac_int_status_t
composite_traps_as_boxes (const comac_traps_compositor_t *compositor,
			  comac_composite_rectangles_t *extents,
			  composite_traps_info_t *info)
{
    comac_boxes_t boxes;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (! _comac_traps_to_boxes (&info->traps, info->antialias, &boxes))
	return COMAC_INT_STATUS_UNSUPPORTED;

    return clip_and_composite_boxes (compositor, extents, &boxes);
}

static comac_int_status_t
clip_and_composite_traps (const comac_traps_compositor_t *compositor,
			  comac_composite_rectangles_t *extents,
			  composite_traps_info_t *info,
			  unsigned flags)
{
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = trim_extents_to_traps (extents, &info->traps);
    if (unlikely (status != COMAC_INT_STATUS_SUCCESS))
	return status;

    status = COMAC_INT_STATUS_UNSUPPORTED;
    if ((flags & FORCE_CLIP_REGION) == 0)
	status = composite_traps_as_boxes (compositor, extents, info);
    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	/* For unbounded operations, the X11 server will estimate the
	 * affected rectangle and apply the operation to that. However,
	 * there are cases where this is an overestimate (e.g. the
	 * clip-fill-{eo,nz}-unbounded test).
	 *
	 * The clip will trim that overestimate to our expectations.
	 */
	if (! extents->is_bounded)
	    flags |= FORCE_CLIP_REGION;

	status = clip_and_composite (compositor,
				     extents,
				     composite_traps,
				     NULL,
				     info,
				     need_unbounded_clip (extents) | flags);
    }

    return status;
}

static comac_int_status_t
clip_and_composite_tristrip (const comac_traps_compositor_t *compositor,
			     comac_composite_rectangles_t *extents,
			     composite_tristrip_info_t *info)
{
    comac_int_status_t status;
    unsigned int flags = 0;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = trim_extents_to_tristrip (extents, &info->strip);
    if (unlikely (status != COMAC_INT_STATUS_SUCCESS))
	return status;

    if (! extents->is_bounded)
	flags |= FORCE_CLIP_REGION;

    status = clip_and_composite (compositor,
				 extents,
				 composite_tristrip,
				 NULL,
				 info,
				 need_unbounded_clip (extents) | flags);

    return status;
}

struct composite_mask {
    comac_surface_t *mask;
    int mask_x, mask_y;
};

static comac_int_status_t
composite_mask (const comac_traps_compositor_t *compositor,
		comac_surface_t *dst,
		void *closure,
		comac_operator_t op,
		comac_surface_t *src,
		int src_x,
		int src_y,
		int dst_x,
		int dst_y,
		const comac_rectangle_int_t *extents,
		comac_clip_t *clip)
{
    struct composite_mask *data = closure;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (src != NULL) {
	compositor->composite (dst,
			       op,
			       src,
			       data->mask,
			       extents->x + src_x,
			       extents->y + src_y,
			       extents->x + data->mask_x,
			       extents->y + data->mask_y,
			       extents->x - dst_x,
			       extents->y - dst_y,
			       extents->width,
			       extents->height);
    } else {
	compositor->composite (dst,
			       op,
			       data->mask,
			       NULL,
			       extents->x + data->mask_x,
			       extents->y + data->mask_y,
			       0,
			       0,
			       extents->x - dst_x,
			       extents->y - dst_y,
			       extents->width,
			       extents->height);
    }

    return COMAC_STATUS_SUCCESS;
}

struct composite_box_info {
    const comac_traps_compositor_t *compositor;
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
    const comac_traps_compositor_t *compositor = info->compositor;

    TRACE ((stderr, "%s\n", __FUNCTION__));

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
composite_mask_clip_boxes (const comac_traps_compositor_t *compositor,
			   comac_surface_t *dst,
			   void *closure,
			   comac_operator_t op,
			   comac_surface_t *src,
			   int src_x,
			   int src_y,
			   int dst_x,
			   int dst_y,
			   const comac_rectangle_int_t *extents,
			   comac_clip_t *clip)
{
    struct composite_mask *data = closure;
    struct composite_box_info info;
    int i;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    info.compositor = compositor;
    info.op = COMAC_OPERATOR_SOURCE;
    info.dst = dst;
    info.src = data->mask;
    info.src_x = data->mask_x;
    info.src_y = data->mask_y;

    info.src_x += dst_x;
    info.src_y += dst_y;

    for (i = 0; i < clip->num_boxes; i++)
	do_unaligned_box (composite_box, &info, &clip->boxes[i], dst_x, dst_y);

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite_mask_clip (const comac_traps_compositor_t *compositor,
		     comac_surface_t *dst,
		     void *closure,
		     comac_operator_t op,
		     comac_surface_t *src,
		     int src_x,
		     int src_y,
		     int dst_x,
		     int dst_y,
		     const comac_rectangle_int_t *extents,
		     comac_clip_t *clip)
{
    struct composite_mask *data = closure;
    comac_polygon_t polygon;
    comac_fill_rule_t fill_rule;
    composite_traps_info_t info;
    comac_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status =
	_comac_clip_get_polygon (clip, &polygon, &fill_rule, &info.antialias);
    if (unlikely (status))
	return status;

    _comac_traps_init (&info.traps);
    status = _comac_bentley_ottmann_tessellate_polygon (&info.traps,
							&polygon,
							fill_rule);
    _comac_polygon_fini (&polygon);
    if (unlikely (status))
	return status;

    status = composite_traps (compositor,
			      dst,
			      &info,
			      COMAC_OPERATOR_SOURCE,
			      data->mask,
			      data->mask_x + dst_x,
			      data->mask_y + dst_y,
			      dst_x,
			      dst_y,
			      extents,
			      NULL);
    _comac_traps_fini (&info.traps);

    return status;
}

/* high-level compositor interface */

static comac_int_status_t
_comac_traps_compositor_paint (const comac_compositor_t *_compositor,
			       comac_composite_rectangles_t *extents)
{
    comac_traps_compositor_t *compositor =
	(comac_traps_compositor_t *) _compositor;
    comac_boxes_t boxes;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    _comac_clip_steal_boxes (extents->clip, &boxes);
    status = clip_and_composite_boxes (compositor, extents, &boxes);
    _comac_clip_unsteal_boxes (extents->clip, &boxes);

    return status;
}

static comac_int_status_t
_comac_traps_compositor_mask (const comac_compositor_t *_compositor,
			      comac_composite_rectangles_t *extents)
{
    const comac_traps_compositor_t *compositor =
	(comac_traps_compositor_t *) _compositor;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    if (extents->mask_pattern.base.type == COMAC_PATTERN_TYPE_SOLID &&
	extents->clip->path == NULL) {
	status = clip_and_composite (compositor,
				     extents,
				     composite_opacity_boxes,
				     composite_opacity_boxes,
				     &extents->mask_pattern,
				     need_unbounded_clip (extents));
    } else {
	struct composite_mask data;

	data.mask = compositor->pattern_to_surface (extents->surface,
						    &extents->mask_pattern.base,
						    TRUE,
						    &extents->bounded,
						    &extents->mask_sample_area,
						    &data.mask_x,
						    &data.mask_y);
	if (unlikely (data.mask->status))
	    return data.mask->status;

	status =
	    clip_and_composite (compositor,
				extents,
				composite_mask,
				extents->clip->path ? composite_mask_clip
						    : composite_mask_clip_boxes,
				&data,
				need_bounded_clip (extents));

	comac_surface_destroy (data.mask);
    }

    return status;
}

static comac_int_status_t
_comac_traps_compositor_stroke (const comac_compositor_t *_compositor,
				comac_composite_rectangles_t *extents,
				const comac_path_fixed_t *path,
				const comac_stroke_style_t *style,
				const comac_matrix_t *ctm,
				const comac_matrix_t *ctm_inverse,
				double tolerance,
				comac_antialias_t antialias)
{
    const comac_traps_compositor_t *compositor =
	(comac_traps_compositor_t *) _compositor;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    status = COMAC_INT_STATUS_UNSUPPORTED;
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

    if (status == COMAC_INT_STATUS_UNSUPPORTED && 0 &&
	_comac_clip_is_region (extents->clip)) /* XXX */
    {
	composite_tristrip_info_t info;

	info.antialias = antialias;
	_comac_tristrip_init_with_clip (&info.strip, extents->clip);
	status = _comac_path_fixed_stroke_to_tristrip (path,
						       style,
						       ctm,
						       ctm_inverse,
						       tolerance,
						       &info.strip);
	if (likely (status == COMAC_INT_STATUS_SUCCESS))
	    status = clip_and_composite_tristrip (compositor, extents, &info);
	_comac_tristrip_fini (&info.strip);
    }

    if (status == COMAC_INT_STATUS_UNSUPPORTED && path->has_curve_to &&
	antialias == COMAC_ANTIALIAS_NONE) {
	comac_polygon_t polygon;

	_comac_polygon_init_with_clip (&polygon, extents->clip);
	status = _comac_path_fixed_stroke_to_polygon (path,
						      style,
						      ctm,
						      ctm_inverse,
						      tolerance,
						      &polygon);
	if (likely (status == COMAC_INT_STATUS_SUCCESS))
	    status = clip_and_composite_polygon (compositor,
						 extents,
						 &polygon,
						 COMAC_ANTIALIAS_NONE,
						 COMAC_FILL_RULE_WINDING,
						 TRUE);
	_comac_polygon_fini (&polygon);
    }

    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	comac_int_status_t (*func) (const comac_path_fixed_t *path,
				    const comac_stroke_style_t *stroke_style,
				    const comac_matrix_t *ctm,
				    const comac_matrix_t *ctm_inverse,
				    double tolerance,
				    comac_traps_t *traps);
	composite_traps_info_t info;
	unsigned flags;

	if (antialias == COMAC_ANTIALIAS_BEST ||
	    antialias == COMAC_ANTIALIAS_GOOD) {
	    func = _comac_path_fixed_stroke_polygon_to_traps;
	    flags = 0;
	} else {
	    func = _comac_path_fixed_stroke_to_traps;
	    flags = need_bounded_clip (extents) & ~NEED_CLIP_SURFACE;
	}

	info.antialias = antialias;
	_comac_traps_init_with_clip (&info.traps, extents->clip);
	status = func (path, style, ctm, ctm_inverse, tolerance, &info.traps);
	if (likely (status == COMAC_INT_STATUS_SUCCESS))
	    status =
		clip_and_composite_traps (compositor, extents, &info, flags);
	_comac_traps_fini (&info.traps);
    }

    return status;
}

static comac_int_status_t
_comac_traps_compositor_fill (const comac_compositor_t *_compositor,
			      comac_composite_rectangles_t *extents,
			      const comac_path_fixed_t *path,
			      comac_fill_rule_t fill_rule,
			      double tolerance,
			      comac_antialias_t antialias)
{
    const comac_traps_compositor_t *compositor =
	(comac_traps_compositor_t *) _compositor;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    status = COMAC_INT_STATUS_UNSUPPORTED;
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
	comac_polygon_t polygon;

#if 0
	if (extents->mask.width  > extents->unbounded.width ||
	    extents->mask.height > extents->unbounded.height)
	{
	    comac_box_t limits;
	    _comac_box_from_rectangle (&limits, &extents->unbounded);
	    _comac_polygon_init (&polygon, &limits, 1);
	}
	else
	{
	    _comac_polygon_init (&polygon, NULL, 0);
	}

	status = _comac_path_fixed_fill_to_polygon (path, tolerance, &polygon);
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    status = _comac_polygon_intersect_with_boxes (&polygon, &fill_rule,
							  extents->clip->boxes,
							  extents->clip->num_boxes);
	}
#else
	_comac_polygon_init_with_clip (&polygon, extents->clip);
	status = _comac_path_fixed_fill_to_polygon (path, tolerance, &polygon);
#endif
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    status = clip_and_composite_polygon (compositor,
						 extents,
						 &polygon,
						 antialias,
						 fill_rule,
						 path->has_curve_to);
	}
	_comac_polygon_fini (&polygon);
    }

    return status;
}

static comac_int_status_t
composite_glyphs (const comac_traps_compositor_t *compositor,
		  comac_surface_t *dst,
		  void *closure,
		  comac_operator_t op,
		  comac_surface_t *src,
		  int src_x,
		  int src_y,
		  int dst_x,
		  int dst_y,
		  const comac_rectangle_int_t *extents,
		  comac_clip_t *clip)
{
    comac_composite_glyphs_info_t *info = closure;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (op == COMAC_OPERATOR_ADD && (dst->content & COMAC_CONTENT_COLOR) == 0)
	info->use_mask = 0;

    return compositor
	->composite_glyphs (dst, op, src, src_x, src_y, dst_x, dst_y, info);
}

static comac_int_status_t
_comac_traps_compositor_glyphs (const comac_compositor_t *_compositor,
				comac_composite_rectangles_t *extents,
				comac_scaled_font_t *scaled_font,
				comac_glyph_t *glyphs,
				int num_glyphs,
				comac_bool_t overlap)
{
    const comac_traps_compositor_t *compositor =
	(comac_traps_compositor_t *) _compositor;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    status = compositor->check_composite (extents);
    if (unlikely (status))
	return status;

    _comac_scaled_font_freeze_cache (scaled_font);
    status = compositor->check_composite_glyphs (extents,
						 scaled_font,
						 glyphs,
						 &num_glyphs);
    if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	comac_composite_glyphs_info_t info;

	info.font = scaled_font;
	info.glyphs = glyphs;
	info.num_glyphs = num_glyphs;
	info.use_mask = overlap || ! extents->is_bounded;
	info.extents = extents->bounded;

	status = clip_and_composite (compositor,
				     extents,
				     composite_glyphs,
				     NULL,
				     &info,
				     need_bounded_clip (extents) |
					 FORCE_CLIP_REGION);
    }
    _comac_scaled_font_thaw_cache (scaled_font);

    return status;
}

void
_comac_traps_compositor_init (comac_traps_compositor_t *compositor,
			      const comac_compositor_t *delegate)
{
    compositor->base.delegate = delegate;

    compositor->base.paint = _comac_traps_compositor_paint;
    compositor->base.mask = _comac_traps_compositor_mask;
    compositor->base.fill = _comac_traps_compositor_fill;
    compositor->base.stroke = _comac_traps_compositor_stroke;
    compositor->base.glyphs = _comac_traps_compositor_glyphs;
}
