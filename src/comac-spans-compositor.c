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

#include "comac-compositor-private.h"
#include "comac-clip-inline.h"
#include "comac-clip-private.h"
#include "comac-image-surface-private.h"
#include "comac-paginated-private.h"
#include "comac-pattern-inline.h"
#include "comac-region-private.h"
#include "comac-recording-surface-inline.h"
#include "comac-spans-compositor-private.h"
#include "comac-surface-subsurface-private.h"
#include "comac-surface-snapshot-private.h"
#include "comac-surface-observer-private.h"

typedef struct {
    comac_polygon_t *polygon;
    comac_fill_rule_t fill_rule;
    comac_antialias_t antialias;
} composite_spans_info_t;

static comac_int_status_t
composite_polygon (const comac_spans_compositor_t *compositor,
		   comac_composite_rectangles_t *extents,
		   comac_polygon_t *polygon,
		   comac_fill_rule_t fill_rule,
		   comac_antialias_t antialias);

static comac_int_status_t
composite_boxes (const comac_spans_compositor_t *compositor,
		 comac_composite_rectangles_t *extents,
		 comac_boxes_t *boxes);

static comac_int_status_t
clip_and_composite_polygon (const comac_spans_compositor_t *compositor,
			    comac_composite_rectangles_t *extents,
			    comac_polygon_t *polygon,
			    comac_fill_rule_t fill_rule,
			    comac_antialias_t antialias);
static comac_surface_t *
get_clip_surface (const comac_spans_compositor_t *compositor,
		  comac_surface_t *dst,
		  const comac_clip_t *clip,
		  const comac_rectangle_int_t *extents)
{
    comac_composite_rectangles_t composite;
    comac_surface_t *surface;
    comac_box_t box;
    comac_polygon_t polygon;
    const comac_clip_path_t *clip_path;
    comac_antialias_t antialias;
    comac_fill_rule_t fill_rule;
    comac_int_status_t status;

    assert (clip->path);

    surface = _comac_surface_create_scratch (dst,
					     COMAC_CONTENT_ALPHA,
					     extents->width,
					     extents->height,
					     COMAC_COLOR_TRANSPARENT);

    _comac_box_from_rectangle (&box, extents);
    _comac_polygon_init (&polygon, &box, 1);

    clip_path = clip->path;
    status = _comac_path_fixed_fill_to_polygon (&clip_path->path,
						clip_path->tolerance,
						&polygon);
    if (unlikely (status))
	goto cleanup_polygon;

    polygon.num_limits = 0;

    antialias = clip_path->antialias;
    fill_rule = clip_path->fill_rule;

    if (clip->boxes) {
	comac_polygon_t intersect;
	comac_boxes_t tmp;

	_comac_boxes_init_for_array (&tmp, clip->boxes, clip->num_boxes);
	status = _comac_polygon_init_boxes (&intersect, &tmp);
	if (unlikely (status))
	    goto cleanup_polygon;

	status = _comac_polygon_intersect (&polygon,
					   fill_rule,
					   &intersect,
					   COMAC_FILL_RULE_WINDING);
	_comac_polygon_fini (&intersect);

	if (unlikely (status))
	    goto cleanup_polygon;

	fill_rule = COMAC_FILL_RULE_WINDING;
    }

    polygon.limits = NULL;
    polygon.num_limits = 0;

    clip_path = clip_path->prev;
    while (clip_path) {
	if (clip_path->antialias == antialias) {
	    comac_polygon_t next;

	    _comac_polygon_init (&next, NULL, 0);
	    status = _comac_path_fixed_fill_to_polygon (&clip_path->path,
							clip_path->tolerance,
							&next);
	    if (likely (status == COMAC_INT_STATUS_SUCCESS))
		status = _comac_polygon_intersect (&polygon,
						   fill_rule,
						   &next,
						   clip_path->fill_rule);
	    _comac_polygon_fini (&next);
	    if (unlikely (status))
		goto cleanup_polygon;

	    fill_rule = COMAC_FILL_RULE_WINDING;
	}

	clip_path = clip_path->prev;
    }

    _comac_polygon_translate (&polygon, -extents->x, -extents->y);
    status = _comac_composite_rectangles_init_for_polygon (
	&composite,
	surface,
	COMAC_OPERATOR_ADD,
	&_comac_pattern_white.base,
	&polygon,
	NULL);
    if (unlikely (status))
	goto cleanup_polygon;

    status = composite_polygon (compositor,
				&composite,
				&polygon,
				fill_rule,
				antialias);
    _comac_composite_rectangles_fini (&composite);
    _comac_polygon_fini (&polygon);
    if (unlikely (status))
	goto error;

    _comac_polygon_init (&polygon, &box, 1);

    clip_path = clip->path;
    antialias = clip_path->antialias == COMAC_ANTIALIAS_DEFAULT
		    ? COMAC_ANTIALIAS_NONE
		    : COMAC_ANTIALIAS_DEFAULT;
    clip_path = clip_path->prev;
    while (clip_path) {
	if (clip_path->antialias == antialias) {
	    if (polygon.num_edges == 0) {
		status =
		    _comac_path_fixed_fill_to_polygon (&clip_path->path,
						       clip_path->tolerance,
						       &polygon);

		fill_rule = clip_path->fill_rule;
		polygon.limits = NULL;
		polygon.num_limits = 0;
	    } else {
		comac_polygon_t next;

		_comac_polygon_init (&next, NULL, 0);
		status =
		    _comac_path_fixed_fill_to_polygon (&clip_path->path,
						       clip_path->tolerance,
						       &next);
		if (likely (status == COMAC_INT_STATUS_SUCCESS))
		    status = _comac_polygon_intersect (&polygon,
						       fill_rule,
						       &next,
						       clip_path->fill_rule);
		_comac_polygon_fini (&next);
		fill_rule = COMAC_FILL_RULE_WINDING;
	    }
	    if (unlikely (status))
		goto error;
	}

	clip_path = clip_path->prev;
    }

    if (polygon.num_edges) {
	_comac_polygon_translate (&polygon, -extents->x, -extents->y);
	status = _comac_composite_rectangles_init_for_polygon (
	    &composite,
	    surface,
	    COMAC_OPERATOR_IN,
	    &_comac_pattern_white.base,
	    &polygon,
	    NULL);
	if (unlikely (status))
	    goto cleanup_polygon;

	status = composite_polygon (compositor,
				    &composite,
				    &polygon,
				    fill_rule,
				    antialias);
	_comac_composite_rectangles_fini (&composite);
	_comac_polygon_fini (&polygon);
	if (unlikely (status))
	    goto error;
    }

    return surface;

cleanup_polygon:
    _comac_polygon_fini (&polygon);
error:
    comac_surface_destroy (surface);
    return _comac_int_surface_create_in_error (status);
}

static comac_int_status_t
fixup_unbounded_mask (const comac_spans_compositor_t *compositor,
		      const comac_composite_rectangles_t *extents,
		      comac_boxes_t *boxes)
{
    comac_composite_rectangles_t composite;
    comac_surface_t *clip;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    clip = get_clip_surface (compositor,
			     extents->surface,
			     extents->clip,
			     &extents->unbounded);
    if (unlikely (clip->status)) {
	if ((comac_int_status_t) clip->status == COMAC_INT_STATUS_NOTHING_TO_DO)
	    return COMAC_STATUS_SUCCESS;

	return clip->status;
    }

    status =
	_comac_composite_rectangles_init_for_boxes (&composite,
						    extents->surface,
						    COMAC_OPERATOR_CLEAR,
						    &_comac_pattern_clear.base,
						    boxes,
						    NULL);
    if (unlikely (status))
	goto cleanup_clip;

    _comac_pattern_init_for_surface (&composite.mask_pattern.surface, clip);
    composite.mask_pattern.base.filter = COMAC_FILTER_NEAREST;
    composite.mask_pattern.base.extend = COMAC_EXTEND_NONE;

    status = composite_boxes (compositor, &composite, boxes);

    _comac_pattern_fini (&composite.mask_pattern.base);
    _comac_composite_rectangles_fini (&composite);

cleanup_clip:
    comac_surface_destroy (clip);
    return status;
}

static comac_int_status_t
fixup_unbounded_polygon (const comac_spans_compositor_t *compositor,
			 const comac_composite_rectangles_t *extents,
			 comac_boxes_t *boxes)
{
    comac_polygon_t polygon, intersect;
    comac_composite_rectangles_t composite;
    comac_fill_rule_t fill_rule;
    comac_antialias_t antialias;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    /* Can we treat the clip as a regular clear-polygon and use it to fill? */
    status = _comac_clip_get_polygon (extents->clip,
				      &polygon,
				      &fill_rule,
				      &antialias);
    if (status == COMAC_INT_STATUS_UNSUPPORTED)
	return status;

    status = _comac_polygon_init_boxes (&intersect, boxes);
    if (unlikely (status))
	goto cleanup_polygon;

    status = _comac_polygon_intersect (&polygon,
				       fill_rule,
				       &intersect,
				       COMAC_FILL_RULE_WINDING);
    _comac_polygon_fini (&intersect);

    if (unlikely (status))
	goto cleanup_polygon;

    status = _comac_composite_rectangles_init_for_polygon (
	&composite,
	extents->surface,
	COMAC_OPERATOR_CLEAR,
	&_comac_pattern_clear.base,
	&polygon,
	NULL);
    if (unlikely (status))
	goto cleanup_polygon;

    status = composite_polygon (compositor,
				&composite,
				&polygon,
				fill_rule,
				antialias);

    _comac_composite_rectangles_fini (&composite);
cleanup_polygon:
    _comac_polygon_fini (&polygon);

    return status;
}

static comac_int_status_t
fixup_unbounded_boxes (const comac_spans_compositor_t *compositor,
		       const comac_composite_rectangles_t *extents,
		       comac_boxes_t *boxes)
{
    comac_boxes_t tmp, clear;
    comac_box_t box;
    comac_int_status_t status;

    assert (boxes->is_pixel_aligned);

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (extents->bounded.width == extents->unbounded.width &&
	extents->bounded.height == extents->unbounded.height) {
	return COMAC_STATUS_SUCCESS;
    }

    /* subtract the drawn boxes from the unbounded area */
    _comac_boxes_init (&clear);

    box.p1.x =
	_comac_fixed_from_int (extents->unbounded.x + extents->unbounded.width);
    box.p1.y = _comac_fixed_from_int (extents->unbounded.y);
    box.p2.x = _comac_fixed_from_int (extents->unbounded.x);
    box.p2.y = _comac_fixed_from_int (extents->unbounded.y +
				      extents->unbounded.height);

    if (boxes->num_boxes) {
	_comac_boxes_init (&tmp);

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
	box.p1.x = _comac_fixed_from_int (extents->unbounded.x);
	box.p2.x = _comac_fixed_from_int (extents->unbounded.x +
					  extents->unbounded.width);

	status = _comac_boxes_add (&clear, COMAC_ANTIALIAS_DEFAULT, &box);
	assert (status == COMAC_INT_STATUS_SUCCESS);
    }

    /* If we have a clip polygon, we need to intersect with that as well */
    if (extents->clip->path) {
	status = fixup_unbounded_polygon (compositor, extents, &clear);
	if (status == COMAC_INT_STATUS_UNSUPPORTED)
	    status = fixup_unbounded_mask (compositor, extents, &clear);
    } else {
	/* Otherwise just intersect with the clip boxes */
	if (extents->clip->num_boxes) {
	    _comac_boxes_init_for_array (&tmp,
					 extents->clip->boxes,
					 extents->clip->num_boxes);
	    status = _comac_boxes_intersect (&clear, &tmp, &clear);
	    if (unlikely (status))
		goto error;
	}

	if (clear.is_pixel_aligned) {
	    status = compositor->fill_boxes (extents->surface,
					     COMAC_OPERATOR_CLEAR,
					     COMAC_COLOR_TRANSPARENT,
					     &clear);
	} else {
	    comac_composite_rectangles_t composite;

	    status = _comac_composite_rectangles_init_for_boxes (
		&composite,
		extents->surface,
		COMAC_OPERATOR_CLEAR,
		&_comac_pattern_clear.base,
		&clear,
		NULL);
	    if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
		status = composite_boxes (compositor, &composite, &clear);
		_comac_composite_rectangles_fini (&composite);
	    }
	}
    }

error:
    _comac_boxes_fini (&clear);
    return status;
}

static comac_surface_t *
unwrap_source (const comac_pattern_t *pattern)
{
    comac_rectangle_int_t limit;

    return _comac_pattern_get_source ((comac_surface_pattern_t *) pattern,
				      &limit);
}

static comac_bool_t
is_recording_pattern (const comac_pattern_t *pattern)
{
    comac_surface_t *surface;

    if (pattern->type != COMAC_PATTERN_TYPE_SURFACE)
	return FALSE;

    surface = ((const comac_surface_pattern_t *) pattern)->surface;
    return _comac_surface_is_recording (surface);
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

    surface = (comac_recording_surface_t *) unwrap_source (pattern);
    if (surface->unbounded)
	return TRUE;

    return _comac_rectangle_contains_rectangle (&surface->extents, sample);
}

static comac_bool_t
op_reduces_to_source (const comac_composite_rectangles_t *extents,
		      comac_bool_t no_mask)
{
    if (extents->op == COMAC_OPERATOR_SOURCE)
	return TRUE;

    if (extents->surface->is_clear)
	return extents->op == COMAC_OPERATOR_OVER ||
	       extents->op == COMAC_OPERATOR_ADD;

    if (no_mask && extents->op == COMAC_OPERATOR_OVER)
	return _comac_pattern_is_opaque (&extents->source_pattern.base,
					 &extents->source_sample_area);

    return FALSE;
}

static comac_status_t
upload_boxes (const comac_spans_compositor_t *compositor,
	      const comac_composite_rectangles_t *extents,
	      comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    const comac_surface_pattern_t *source = &extents->source_pattern.surface;
    comac_surface_t *src;
    comac_rectangle_int_t limit;
    comac_int_status_t status;
    int tx, ty;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    src = _comac_pattern_get_source (source, &limit);
    if (! (src->type == COMAC_SURFACE_TYPE_IMAGE || src->type == dst->type))
	return COMAC_INT_STATUS_UNSUPPORTED;

    if (! _comac_matrix_is_integer_translation (&source->base.matrix, &tx, &ty))
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

static comac_bool_t
_clip_is_region (const comac_clip_t *clip)
{
    int i;

    if (clip->is_region)
	return TRUE;

    if (clip->path)
	return FALSE;

    for (i = 0; i < clip->num_boxes; i++) {
	const comac_box_t *b = &clip->boxes[i];
	if (! _comac_fixed_is_integer (b->p1.x | b->p1.y | b->p2.x | b->p2.y))
	    return FALSE;
    }

    return TRUE;
}

static comac_int_status_t
composite_aligned_boxes (const comac_spans_compositor_t *compositor,
			 const comac_composite_rectangles_t *extents,
			 comac_boxes_t *boxes)
{
    comac_surface_t *dst = extents->surface;
    comac_operator_t op = extents->op;
    const comac_pattern_t *source = &extents->source_pattern.base;
    comac_int_status_t status;
    comac_bool_t need_clip_mask = ! _clip_is_region (extents->clip);
    comac_bool_t op_is_source;
    comac_bool_t no_mask;
    comac_bool_t inplace;

    TRACE ((stderr,
	    "%s: need_clip_mask=%d, is-bounded=%d\n",
	    __FUNCTION__,
	    need_clip_mask,
	    extents->is_bounded));
    if (need_clip_mask && ! extents->is_bounded) {
	TRACE ((stderr, "%s: unsupported clip\n", __FUNCTION__));
	return COMAC_INT_STATUS_UNSUPPORTED;
    }

    assert (extents->mask_pattern.solid.color.colorspace ==
	    COMAC_COLORSPACE_RGB);
    no_mask = extents->mask_pattern.base.type == COMAC_PATTERN_TYPE_SOLID &&
	      COMAC_COLOR_IS_OPAQUE (&extents->mask_pattern.solid.color.c.rgb);
    op_is_source = op_reduces_to_source (extents, no_mask);
    inplace = ! need_clip_mask && op_is_source && no_mask;

    TRACE ((stderr,
	    "%s: op-is-source=%d [op=%d], no-mask=%d, inplace=%d\n",
	    __FUNCTION__,
	    op_is_source,
	    op,
	    no_mask,
	    inplace));

    if (op == COMAC_OPERATOR_SOURCE && (need_clip_mask || ! no_mask)) {
	/* SOURCE with a mask is actually a LERP in comac semantics */
	if ((compositor->flags & COMAC_SPANS_COMPOSITOR_HAS_LERP) == 0) {
	    TRACE ((stderr, "%s: unsupported lerp\n", __FUNCTION__));
	    return COMAC_INT_STATUS_UNSUPPORTED;
	}
    }

    /* Are we just copying a recording surface? */
    if (inplace &&
	recording_pattern_contains_sample (&extents->source_pattern.base,
					   &extents->source_sample_area)) {
	comac_clip_t *recording_clip;
	const comac_pattern_t *source = &extents->source_pattern.base;
	const comac_matrix_t *m;
	comac_matrix_t matrix;

	/* XXX could also do tiling repeat modes... */

	/* first clear the area about to be overwritten */
	if (! dst->is_clear) {
	    status = compositor->fill_boxes (dst,
					     COMAC_OPERATOR_CLEAR,
					     COMAC_COLOR_TRANSPARENT,
					     boxes);
	    if (unlikely (status))
		return status;

	    dst->is_clear = TRUE;
	}

	m = &source->matrix;
	if (_comac_surface_has_device_transform (dst)) {
	    comac_matrix_multiply (&matrix,
				   &source->matrix,
				   &dst->device_transform);
	    m = &matrix;
	}

	recording_clip = _comac_clip_from_boxes (boxes);
	status =
	    _comac_recording_surface_replay_with_clip (unwrap_source (source),
						       m,
						       dst,
						       recording_clip);
	_comac_clip_destroy (recording_clip);

	return status;
    }

    status = COMAC_INT_STATUS_UNSUPPORTED;
    if (! need_clip_mask && no_mask &&
	source->type == COMAC_PATTERN_TYPE_SOLID) {
	const comac_color_t *color;

	color = &((comac_solid_pattern_t *) source)->color;
	if (op_is_source)
	    op = COMAC_OPERATOR_SOURCE;
	status = compositor->fill_boxes (dst, op, color, boxes);
    } else if (inplace && source->type == COMAC_PATTERN_TYPE_SURFACE) {
	status = upload_boxes (compositor, extents, boxes);
    }
    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	comac_surface_t *src;
	comac_surface_t *mask = NULL;
	int src_x, src_y;
	int mask_x = 0, mask_y = 0;

	/* All typical cases will have been resolved before now... */
	if (need_clip_mask) {
	    mask = get_clip_surface (compositor,
				     dst,
				     extents->clip,
				     &extents->bounded);
	    if (unlikely (mask->status))
		return mask->status;

	    mask_x = -extents->bounded.x;
	    mask_y = -extents->bounded.y;
	}

	/* XXX but this is still ugly */
	if (! no_mask) {
	    src = compositor->pattern_to_surface (dst,
						  &extents->mask_pattern.base,
						  TRUE,
						  &extents->bounded,
						  &extents->mask_sample_area,
						  &src_x,
						  &src_y);
	    if (unlikely (src->status)) {
		comac_surface_destroy (mask);
		return src->status;
	    }

	    if (mask != NULL) {
		status = compositor->composite_boxes (mask,
						      COMAC_OPERATOR_IN,
						      src,
						      NULL,
						      src_x,
						      src_y,
						      0,
						      0,
						      mask_x,
						      mask_y,
						      boxes,
						      &extents->bounded);

		comac_surface_destroy (src);
	    } else {
		mask = src;
		mask_x = src_x;
		mask_y = src_y;
	    }
	}

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

    if (status == COMAC_INT_STATUS_SUCCESS && ! extents->is_bounded)
	status = fixup_unbounded_boxes (compositor, extents, boxes);

    return status;
}

static comac_bool_t
composite_needs_clip (const comac_composite_rectangles_t *composite,
		      const comac_box_t *extents)
{
    return ! _comac_clip_contains_box (composite->clip, extents);
}

static comac_int_status_t
composite_boxes (const comac_spans_compositor_t *compositor,
		 comac_composite_rectangles_t *extents,
		 comac_boxes_t *boxes)
{
    comac_abstract_span_renderer_t renderer;
    comac_rectangular_scan_converter_t converter;
    const struct _comac_boxes_chunk *chunk;
    comac_int_status_t status;
    comac_box_t box;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    _comac_box_from_rectangle (&box, &extents->unbounded);
    if (composite_needs_clip (extents, &box)) {
	TRACE ((stderr, "%s: unsupported clip\n", __FUNCTION__));
	return COMAC_INT_STATUS_UNSUPPORTED;
    }

    _comac_rectangular_scan_converter_init (&converter, &extents->unbounded);
    for (chunk = &boxes->chunks; chunk != NULL; chunk = chunk->next) {
	const comac_box_t *box = chunk->base;
	int i;

	for (i = 0; i < chunk->count; i++) {
	    status = _comac_rectangular_scan_converter_add_box (&converter,
								&box[i],
								1);
	    if (unlikely (status))
		goto cleanup_converter;
	}
    }

    status = compositor->renderer_init (&renderer,
					extents,
					COMAC_ANTIALIAS_DEFAULT,
					FALSE);
    if (likely (status == COMAC_INT_STATUS_SUCCESS))
	status = converter.base.generate (&converter.base, &renderer.base);
    compositor->renderer_fini (&renderer, status);

cleanup_converter:
    converter.base.destroy (&converter.base);
    return status;
}

static comac_int_status_t
composite_polygon (const comac_spans_compositor_t *compositor,
		   comac_composite_rectangles_t *extents,
		   comac_polygon_t *polygon,
		   comac_fill_rule_t fill_rule,
		   comac_antialias_t antialias)
{
    comac_abstract_span_renderer_t renderer;
    comac_scan_converter_t *converter;
    comac_bool_t needs_clip;
    comac_int_status_t status;

    if (extents->is_bounded)
	needs_clip = extents->clip->path != NULL;
    else
	needs_clip =
	    ! _clip_is_region (extents->clip) || extents->clip->num_boxes > 1;
    TRACE ((stderr, "%s - needs_clip=%d\n", __FUNCTION__, needs_clip));
    if (needs_clip) {
	TRACE ((stderr, "%s: unsupported clip\n", __FUNCTION__));
	return COMAC_INT_STATUS_UNSUPPORTED;
	converter = _comac_clip_tor_scan_converter_create (extents->clip,
							   polygon,
							   fill_rule,
							   antialias);
    } else {
	const comac_rectangle_int_t *r = &extents->unbounded;

	if (antialias == COMAC_ANTIALIAS_FAST) {
	    converter = _comac_tor22_scan_converter_create (r->x,
							    r->y,
							    r->x + r->width,
							    r->y + r->height,
							    fill_rule,
							    antialias);
	    status =
		_comac_tor22_scan_converter_add_polygon (converter, polygon);
	} else if (antialias == COMAC_ANTIALIAS_NONE) {
	    converter = _comac_mono_scan_converter_create (r->x,
							   r->y,
							   r->x + r->width,
							   r->y + r->height,
							   fill_rule);
	    status =
		_comac_mono_scan_converter_add_polygon (converter, polygon);
	} else {
	    converter = _comac_tor_scan_converter_create (r->x,
							  r->y,
							  r->x + r->width,
							  r->y + r->height,
							  fill_rule,
							  antialias);
	    status = _comac_tor_scan_converter_add_polygon (converter, polygon);
	}
    }
    if (unlikely (status))
	goto cleanup_converter;

    status =
	compositor->renderer_init (&renderer, extents, antialias, needs_clip);
    if (likely (status == COMAC_INT_STATUS_SUCCESS))
	status = converter->generate (converter, &renderer.base);
    compositor->renderer_fini (&renderer, status);

cleanup_converter:
    converter->destroy (converter);
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

static comac_int_status_t
trim_extents_to_polygon (comac_composite_rectangles_t *extents,
			 comac_polygon_t *polygon)
{
    return _comac_composite_rectangles_intersect_mask_extents (
	extents,
	&polygon->extents);
}

static comac_int_status_t
clip_and_composite_boxes (const comac_spans_compositor_t *compositor,
			  comac_composite_rectangles_t *extents,
			  comac_boxes_t *boxes)
{
    comac_int_status_t status;
    comac_polygon_t polygon;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    status = trim_extents_to_boxes (extents, boxes);
    if (unlikely (status))
	return status;

    if (boxes->num_boxes == 0) {
	if (extents->is_bounded)
	    return COMAC_STATUS_SUCCESS;

	return fixup_unbounded_boxes (compositor, extents, boxes);
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
						 fill_rule,
						 antialias);

	    clip = extents->clip;
	    extents->clip = saved_clip;

	    _comac_polygon_fini (&polygon);
	}
	_comac_clip_destroy (clip);

	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    if (boxes->is_pixel_aligned) {
	status = composite_aligned_boxes (compositor, extents, boxes);
	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    status = composite_boxes (compositor, extents, boxes);
    if (status != COMAC_INT_STATUS_UNSUPPORTED)
	return status;

    status = _comac_polygon_init_boxes (&polygon, boxes);
    if (unlikely (status))
	return status;

    status = composite_polygon (compositor,
				extents,
				&polygon,
				COMAC_FILL_RULE_WINDING,
				COMAC_ANTIALIAS_DEFAULT);
    _comac_polygon_fini (&polygon);

    return status;
}

static comac_int_status_t
clip_and_composite_polygon (const comac_spans_compositor_t *compositor,
			    comac_composite_rectangles_t *extents,
			    comac_polygon_t *polygon,
			    comac_fill_rule_t fill_rule,
			    comac_antialias_t antialias)
{
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    /* XXX simply uses polygon limits.point extemities, tessellation? */
    status = trim_extents_to_polygon (extents, polygon);
    if (unlikely (status))
	return status;

    if (_comac_polygon_is_empty (polygon)) {
	comac_boxes_t boxes;

	if (extents->is_bounded)
	    return COMAC_STATUS_SUCCESS;

	_comac_boxes_init (&boxes);
	extents->bounded.width = extents->bounded.height = 0;
	return fixup_unbounded_boxes (compositor, extents, &boxes);
    }

    if (extents->is_bounded && extents->clip->path) {
	comac_polygon_t clipper;
	comac_antialias_t clip_antialias;
	comac_fill_rule_t clip_fill_rule;

	TRACE (
	    (stderr, "%s - combining shape with clip polygon\n", __FUNCTION__));

	status = _comac_clip_get_polygon (extents->clip,
					  &clipper,
					  &clip_fill_rule,
					  &clip_antialias);
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    comac_clip_t *old_clip;

	    if (clip_antialias == antialias) {
		status = _comac_polygon_intersect (polygon,
						   fill_rule,
						   &clipper,
						   clip_fill_rule);
		_comac_polygon_fini (&clipper);
		if (unlikely (status))
		    return status;

		old_clip = extents->clip;
		extents->clip = _comac_clip_copy_region (extents->clip);
		_comac_clip_destroy (old_clip);

		status = trim_extents_to_polygon (extents, polygon);
		if (unlikely (status))
		    return status;

		fill_rule = COMAC_FILL_RULE_WINDING;
	    } else {
		_comac_polygon_fini (&clipper);
	    }
	}
    }

    return composite_polygon (compositor,
			      extents,
			      polygon,
			      fill_rule,
			      antialias);
}

/* high-level compositor interface */

static comac_int_status_t
_comac_spans_compositor_paint (const comac_compositor_t *_compositor,
			       comac_composite_rectangles_t *extents)
{
    const comac_spans_compositor_t *compositor =
	(comac_spans_compositor_t *) _compositor;
    comac_boxes_t boxes;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    _comac_clip_steal_boxes (extents->clip, &boxes);
    status = clip_and_composite_boxes (compositor, extents, &boxes);
    _comac_clip_unsteal_boxes (extents->clip, &boxes);

    return status;
}

static comac_int_status_t
_comac_spans_compositor_mask (const comac_compositor_t *_compositor,
			      comac_composite_rectangles_t *extents)
{
    const comac_spans_compositor_t *compositor =
	(comac_spans_compositor_t *) _compositor;
    comac_int_status_t status;
    comac_boxes_t boxes;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    _comac_clip_steal_boxes (extents->clip, &boxes);
    status = clip_and_composite_boxes (compositor, extents, &boxes);
    _comac_clip_unsteal_boxes (extents->clip, &boxes);

    return status;
}

static comac_int_status_t
_comac_spans_compositor_stroke (const comac_compositor_t *_compositor,
				comac_composite_rectangles_t *extents,
				const comac_path_fixed_t *path,
				const comac_stroke_style_t *style,
				const comac_matrix_t *ctm,
				const comac_matrix_t *ctm_inverse,
				double tolerance,
				comac_antialias_t antialias)
{
    const comac_spans_compositor_t *compositor =
	(comac_spans_compositor_t *) _compositor;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    TRACE_ (_comac_debug_print_path (stderr, path));
    TRACE_ (_comac_debug_print_clip (stderr, extents->clip));

    status = COMAC_INT_STATUS_UNSUPPORTED;
    if (_comac_path_fixed_stroke_is_rectilinear (path)) {
	comac_boxes_t boxes;

	_comac_boxes_init (&boxes);
	if (! _comac_clip_contains_rectangle (extents->clip, &extents->mask))
	    _comac_boxes_limit (&boxes,
				extents->clip->boxes,
				extents->clip->num_boxes);

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
	comac_polygon_t polygon;
	comac_box_t limits;
	comac_fill_rule_t fill_rule = COMAC_FILL_RULE_WINDING;

	if (! _comac_rectangle_contains_rectangle (&extents->unbounded,
						   &extents->mask)) {
	    if (extents->clip->num_boxes == 1) {
		_comac_polygon_init (&polygon, extents->clip->boxes, 1);
	    } else {
		_comac_box_from_rectangle (&limits, &extents->unbounded);
		_comac_polygon_init (&polygon, &limits, 1);
	    }
	} else {
	    _comac_polygon_init (&polygon, NULL, 0);
	}
	status = _comac_path_fixed_stroke_to_polygon (path,
						      style,
						      ctm,
						      ctm_inverse,
						      tolerance,
						      &polygon);
	TRACE_ (_comac_debug_print_polygon (stderr, &polygon));
	polygon.num_limits = 0;

	if (status == COMAC_INT_STATUS_SUCCESS &&
	    extents->clip->num_boxes > 1) {
	    status =
		_comac_polygon_intersect_with_boxes (&polygon,
						     &fill_rule,
						     extents->clip->boxes,
						     extents->clip->num_boxes);
	}
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    comac_clip_t *saved_clip = extents->clip;

	    if (extents->is_bounded) {
		extents->clip = _comac_clip_copy_path (extents->clip);
		extents->clip =
		    _comac_clip_intersect_box (extents->clip, &polygon.extents);
	    }

	    status = clip_and_composite_polygon (compositor,
						 extents,
						 &polygon,
						 fill_rule,
						 antialias);

	    if (extents->is_bounded) {
		_comac_clip_destroy (extents->clip);
		extents->clip = saved_clip;
	    }
	}
	_comac_polygon_fini (&polygon);
    }

    return status;
}

static comac_int_status_t
_comac_spans_compositor_fill (const comac_compositor_t *_compositor,
			      comac_composite_rectangles_t *extents,
			      const comac_path_fixed_t *path,
			      comac_fill_rule_t fill_rule,
			      double tolerance,
			      comac_antialias_t antialias)
{
    const comac_spans_compositor_t *compositor =
	(comac_spans_compositor_t *) _compositor;
    comac_int_status_t status;

    TRACE ((stderr,
	    "%s op=%d, antialias=%d\n",
	    __FUNCTION__,
	    extents->op,
	    antialias));

    status = COMAC_INT_STATUS_UNSUPPORTED;
    if (_comac_path_fixed_fill_is_rectilinear (path)) {
	comac_boxes_t boxes;

	TRACE ((stderr, "%s - rectilinear\n", __FUNCTION__));

	_comac_boxes_init (&boxes);
	if (! _comac_clip_contains_rectangle (extents->clip, &extents->mask))
	    _comac_boxes_limit (&boxes,
				extents->clip->boxes,
				extents->clip->num_boxes);
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
	comac_box_t limits;

	TRACE ((stderr, "%s - polygon\n", __FUNCTION__));

	if (! _comac_rectangle_contains_rectangle (&extents->unbounded,
						   &extents->mask)) {
	    TRACE ((stderr, "%s - clipping to bounds\n", __FUNCTION__));
	    if (extents->clip->num_boxes == 1) {
		_comac_polygon_init (&polygon, extents->clip->boxes, 1);
	    } else {
		_comac_box_from_rectangle (&limits, &extents->unbounded);
		_comac_polygon_init (&polygon, &limits, 1);
	    }
	} else {
	    _comac_polygon_init (&polygon, NULL, 0);
	}

	status = _comac_path_fixed_fill_to_polygon (path, tolerance, &polygon);
	TRACE_ (_comac_debug_print_polygon (stderr, &polygon));
	polygon.num_limits = 0;

	if (status == COMAC_INT_STATUS_SUCCESS &&
	    extents->clip->num_boxes > 1) {
	    TRACE ((stderr,
		    "%s - polygon intersect with %d clip boxes\n",
		    __FUNCTION__,
		    extents->clip->num_boxes));
	    status =
		_comac_polygon_intersect_with_boxes (&polygon,
						     &fill_rule,
						     extents->clip->boxes,
						     extents->clip->num_boxes);
	}
	TRACE_ (_comac_debug_print_polygon (stderr, &polygon));
	if (likely (status == COMAC_INT_STATUS_SUCCESS)) {
	    comac_clip_t *saved_clip = extents->clip;

	    if (extents->is_bounded) {
		TRACE ((stderr,
			"%s - polygon discard clip boxes\n",
			__FUNCTION__));
		extents->clip = _comac_clip_copy_path (extents->clip);
		extents->clip =
		    _comac_clip_intersect_box (extents->clip, &polygon.extents);
	    }

	    status = clip_and_composite_polygon (compositor,
						 extents,
						 &polygon,
						 fill_rule,
						 antialias);

	    if (extents->is_bounded) {
		_comac_clip_destroy (extents->clip);
		extents->clip = saved_clip;
	    }
	}
	_comac_polygon_fini (&polygon);

	TRACE ((stderr, "%s - polygon status=%d\n", __FUNCTION__, status));
    }

    return status;
}

void
_comac_spans_compositor_init (comac_spans_compositor_t *compositor,
			      const comac_compositor_t *delegate)
{
    compositor->base.delegate = delegate;

    compositor->base.paint = _comac_spans_compositor_paint;
    compositor->base.mask = _comac_spans_compositor_mask;
    compositor->base.fill = _comac_spans_compositor_fill;
    compositor->base.stroke = _comac_spans_compositor_stroke;
    compositor->base.glyphs = NULL;
}
