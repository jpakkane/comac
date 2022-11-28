/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2009 Intel Corporation
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-clip-inline.h"
#include "comac-error-private.h"
#include "comac-composite-rectangles-private.h"
#include "comac-pattern-private.h"

/* A collection of routines to facilitate writing compositors. */

void
_comac_composite_rectangles_fini (comac_composite_rectangles_t *extents)
{
    /* If adding further free() code here, make sure those fields are inited by
     * _comac_composite_rectangles_init IN ALL CASES
     */
    _comac_clip_destroy (extents->clip);
    extents->clip = NULL;
}

static void
_comac_composite_reduce_pattern (const comac_pattern_t *src,
				 comac_pattern_union_t *dst)
{
    int tx, ty;

    _comac_pattern_init_static_copy (&dst->base, src);
    if (dst->base.type == COMAC_PATTERN_TYPE_SOLID)
	return;

    dst->base.filter = _comac_pattern_analyze_filter (&dst->base);

    tx = ty = 0;
    if (_comac_matrix_is_pixman_translation (&dst->base.matrix,
					     dst->base.filter,
					     &tx,
					     &ty)) {
	dst->base.matrix.x0 = tx;
	dst->base.matrix.y0 = ty;
    }
}

static inline comac_bool_t
_comac_composite_rectangles_init (comac_composite_rectangles_t *extents,
				  comac_surface_t *surface,
				  comac_operator_t op,
				  const comac_pattern_t *source,
				  const comac_clip_t *clip)
{
    /* Always set the clip so that a _comac_composite_rectangles_init can ALWAYS be
     * balanced by a _comac_composite_rectangles_fini */
    extents->clip = NULL;

    if (_comac_clip_is_all_clipped (clip))
	return FALSE;
    extents->surface = surface;
    extents->op = op;

    _comac_surface_get_extents (surface, &extents->destination);

    extents->unbounded = extents->destination;
    if (clip && ! _comac_rectangle_intersect (&extents->unbounded,
					      _comac_clip_get_extents (clip)))
	return FALSE;

    extents->bounded = extents->unbounded;
    extents->is_bounded = _comac_operator_bounded_by_either (op);

    extents->original_source_pattern = source;
    _comac_composite_reduce_pattern (source, &extents->source_pattern);

    _comac_pattern_get_extents (&extents->source_pattern.base,
				&extents->source,
				surface->is_vector);
    if (extents->is_bounded & COMAC_OPERATOR_BOUND_BY_SOURCE) {
	if (! _comac_rectangle_intersect (&extents->bounded, &extents->source))
	    return FALSE;
    }

    extents->original_mask_pattern = NULL;
    extents->mask_pattern.base.type = COMAC_PATTERN_TYPE_SOLID;
    extents->mask_pattern.solid.color.colorspace = COMAC_COLORSPACE_RGB;
    extents->mask_pattern.solid.color.c.rgb.alpha =
	1.; /* XXX full initialisation? */
    extents->mask_pattern.solid.color.c.rgb.alpha_short = 0xffff;

    return TRUE;
}

comac_int_status_t
_comac_composite_rectangles_init_for_paint (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_clip_t *clip)
{
    if (! _comac_composite_rectangles_init (extents,
					    surface,
					    op,
					    source,
					    clip)) {
	goto NOTHING_TO_DO;
    }

    extents->mask = extents->destination;

    extents->clip = _comac_clip_reduce_for_composite (clip, extents);
    if (_comac_clip_is_all_clipped (extents->clip))
	goto NOTHING_TO_DO;

    if (! _comac_rectangle_intersect (&extents->unbounded,
				      _comac_clip_get_extents (extents->clip)))
	goto NOTHING_TO_DO;

    if (extents->source_pattern.base.type != COMAC_PATTERN_TYPE_SOLID)
	_comac_pattern_sampled_area (&extents->source_pattern.base,
				     &extents->bounded,
				     &extents->source_sample_area);

    return COMAC_INT_STATUS_SUCCESS;
NOTHING_TO_DO:
    _comac_composite_rectangles_fini (extents);
    return COMAC_INT_STATUS_NOTHING_TO_DO;
}

static comac_int_status_t
_comac_composite_rectangles_intersect (comac_composite_rectangles_t *extents,
				       const comac_clip_t *clip)
{
    if ((! _comac_rectangle_intersect (&extents->bounded, &extents->mask)) &&
	(extents->is_bounded & COMAC_OPERATOR_BOUND_BY_MASK))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (extents->is_bounded ==
	(COMAC_OPERATOR_BOUND_BY_MASK | COMAC_OPERATOR_BOUND_BY_SOURCE)) {
	extents->unbounded = extents->bounded;
    } else if (extents->is_bounded & COMAC_OPERATOR_BOUND_BY_MASK) {
	if (! _comac_rectangle_intersect (&extents->unbounded, &extents->mask))
	    return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    extents->clip = _comac_clip_reduce_for_composite (clip, extents);
    if (_comac_clip_is_all_clipped (extents->clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (! _comac_rectangle_intersect (&extents->unbounded,
				      _comac_clip_get_extents (extents->clip)))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (! _comac_rectangle_intersect (
	    &extents->bounded,
	    _comac_clip_get_extents (extents->clip)) &&
	extents->is_bounded & COMAC_OPERATOR_BOUND_BY_MASK) {
	return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    if (extents->source_pattern.base.type != COMAC_PATTERN_TYPE_SOLID)
	_comac_pattern_sampled_area (&extents->source_pattern.base,
				     &extents->bounded,
				     &extents->source_sample_area);
    if (extents->mask_pattern.base.type != COMAC_PATTERN_TYPE_SOLID) {
	_comac_pattern_sampled_area (&extents->mask_pattern.base,
				     &extents->bounded,
				     &extents->mask_sample_area);
	if (extents->mask_sample_area.width == 0 ||
	    extents->mask_sample_area.height == 0) {
	    _comac_composite_rectangles_fini (extents);
	    return COMAC_INT_STATUS_NOTHING_TO_DO;
	}
    }

    return COMAC_INT_STATUS_SUCCESS;
}

comac_int_status_t
_comac_composite_rectangles_intersect_source_extents (
    comac_composite_rectangles_t *extents, const comac_box_t *box)
{
    comac_rectangle_int_t rect;
    comac_clip_t *clip;

    _comac_box_round_to_rectangle (box, &rect);
    if (rect.x == extents->source.x && rect.y == extents->source.y &&
	rect.width == extents->source.width &&
	rect.height == extents->source.height) {
	return COMAC_INT_STATUS_SUCCESS;
    }

    _comac_rectangle_intersect (&extents->source, &rect);

    rect = extents->bounded;
    if (! _comac_rectangle_intersect (&extents->bounded, &extents->source) &&
	extents->is_bounded & COMAC_OPERATOR_BOUND_BY_SOURCE)
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (rect.width == extents->bounded.width &&
	rect.height == extents->bounded.height)
	return COMAC_INT_STATUS_SUCCESS;

    if (extents->is_bounded ==
	(COMAC_OPERATOR_BOUND_BY_MASK | COMAC_OPERATOR_BOUND_BY_SOURCE)) {
	extents->unbounded = extents->bounded;
    } else if (extents->is_bounded & COMAC_OPERATOR_BOUND_BY_MASK) {
	if (! _comac_rectangle_intersect (&extents->unbounded, &extents->mask))
	    return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    clip = extents->clip;
    extents->clip = _comac_clip_reduce_for_composite (clip, extents);
    if (clip != extents->clip)
	_comac_clip_destroy (clip);

    if (_comac_clip_is_all_clipped (extents->clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (! _comac_rectangle_intersect (&extents->unbounded,
				      _comac_clip_get_extents (extents->clip)))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (extents->source_pattern.base.type != COMAC_PATTERN_TYPE_SOLID)
	_comac_pattern_sampled_area (&extents->source_pattern.base,
				     &extents->bounded,
				     &extents->source_sample_area);
    if (extents->mask_pattern.base.type != COMAC_PATTERN_TYPE_SOLID) {
	_comac_pattern_sampled_area (&extents->mask_pattern.base,
				     &extents->bounded,
				     &extents->mask_sample_area);
	if (extents->mask_sample_area.width == 0 ||
	    extents->mask_sample_area.height == 0)
	    return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    return COMAC_INT_STATUS_SUCCESS;
}

comac_int_status_t
_comac_composite_rectangles_intersect_mask_extents (
    comac_composite_rectangles_t *extents, const comac_box_t *box)
{
    comac_rectangle_int_t mask;
    comac_clip_t *clip;

    _comac_box_round_to_rectangle (box, &mask);
    if (mask.x == extents->mask.x && mask.y == extents->mask.y &&
	mask.width == extents->mask.width &&
	mask.height == extents->mask.height) {
	return COMAC_INT_STATUS_SUCCESS;
    }

    _comac_rectangle_intersect (&extents->mask, &mask);

    mask = extents->bounded;
    if (! _comac_rectangle_intersect (&extents->bounded, &extents->mask) &&
	extents->is_bounded & COMAC_OPERATOR_BOUND_BY_MASK)
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (mask.width == extents->bounded.width &&
	mask.height == extents->bounded.height)
	return COMAC_INT_STATUS_SUCCESS;

    if (extents->is_bounded ==
	(COMAC_OPERATOR_BOUND_BY_MASK | COMAC_OPERATOR_BOUND_BY_SOURCE)) {
	extents->unbounded = extents->bounded;
    } else if (extents->is_bounded & COMAC_OPERATOR_BOUND_BY_MASK) {
	if (! _comac_rectangle_intersect (&extents->unbounded, &extents->mask))
	    return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    clip = extents->clip;
    extents->clip = _comac_clip_reduce_for_composite (clip, extents);
    if (clip != extents->clip)
	_comac_clip_destroy (clip);

    if (_comac_clip_is_all_clipped (extents->clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (! _comac_rectangle_intersect (&extents->unbounded,
				      _comac_clip_get_extents (extents->clip)))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (extents->source_pattern.base.type != COMAC_PATTERN_TYPE_SOLID)
	_comac_pattern_sampled_area (&extents->source_pattern.base,
				     &extents->bounded,
				     &extents->source_sample_area);
    if (extents->mask_pattern.base.type != COMAC_PATTERN_TYPE_SOLID) {
	_comac_pattern_sampled_area (&extents->mask_pattern.base,
				     &extents->bounded,
				     &extents->mask_sample_area);
	if (extents->mask_sample_area.width == 0 ||
	    extents->mask_sample_area.height == 0)
	    return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    return COMAC_INT_STATUS_SUCCESS;
}

comac_int_status_t
_comac_composite_rectangles_init_for_mask (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_pattern_t *mask,
    const comac_clip_t *clip)
{
    comac_int_status_t status;
    if (! _comac_composite_rectangles_init (extents,
					    surface,
					    op,
					    source,
					    clip)) {
	_comac_composite_rectangles_fini (extents);
	return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    extents->original_mask_pattern = mask;
    _comac_composite_reduce_pattern (mask, &extents->mask_pattern);
    _comac_pattern_get_extents (&extents->mask_pattern.base,
				&extents->mask,
				surface->is_vector);

    status = _comac_composite_rectangles_intersect (extents, clip);
    if (status == COMAC_INT_STATUS_NOTHING_TO_DO) {
	_comac_composite_rectangles_fini (extents);
    }
    return status;
}

comac_int_status_t
_comac_composite_rectangles_init_for_stroke (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_path_fixed_t *path,
    const comac_stroke_style_t *style,
    const comac_matrix_t *ctm,
    const comac_clip_t *clip)
{
    comac_int_status_t status;
    if (! _comac_composite_rectangles_init (extents,
					    surface,
					    op,
					    source,
					    clip)) {
	_comac_composite_rectangles_fini (extents);
	return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    _comac_path_fixed_approximate_stroke_extents (path,
						  style,
						  ctm,
						  surface->is_vector,
						  &extents->mask);

    status = _comac_composite_rectangles_intersect (extents, clip);
    if (status == COMAC_INT_STATUS_NOTHING_TO_DO) {
	_comac_composite_rectangles_fini (extents);
    }
    return status;
}

comac_int_status_t
_comac_composite_rectangles_init_for_fill (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_path_fixed_t *path,
    const comac_clip_t *clip)
{
    comac_int_status_t status;
    if (! _comac_composite_rectangles_init (extents,
					    surface,
					    op,
					    source,
					    clip)) {
	_comac_composite_rectangles_fini (extents);
	return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    _comac_path_fixed_approximate_fill_extents (path, &extents->mask);

    status = _comac_composite_rectangles_intersect (extents, clip);
    if (status == COMAC_INT_STATUS_NOTHING_TO_DO) {
	_comac_composite_rectangles_fini (extents);
    }
    return status;
}

comac_int_status_t
_comac_composite_rectangles_init_for_polygon (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_polygon_t *polygon,
    const comac_clip_t *clip)
{
    comac_int_status_t status;
    if (! _comac_composite_rectangles_init (extents,
					    surface,
					    op,
					    source,
					    clip)) {
	_comac_composite_rectangles_fini (extents);
	return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    _comac_box_round_to_rectangle (&polygon->extents, &extents->mask);
    status = _comac_composite_rectangles_intersect (extents, clip);
    if (status == COMAC_INT_STATUS_NOTHING_TO_DO) {
	_comac_composite_rectangles_fini (extents);
    }
    return status;
}

comac_int_status_t
_comac_composite_rectangles_init_for_boxes (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_boxes_t *boxes,
    const comac_clip_t *clip)
{
    comac_box_t box;
    comac_int_status_t status;

    if (! _comac_composite_rectangles_init (extents,
					    surface,
					    op,
					    source,
					    clip)) {
	_comac_composite_rectangles_fini (extents);
	return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    _comac_boxes_extents (boxes, &box);
    _comac_box_round_to_rectangle (&box, &extents->mask);
    status = _comac_composite_rectangles_intersect (extents, clip);
    if (status == COMAC_INT_STATUS_NOTHING_TO_DO) {
	_comac_composite_rectangles_fini (extents);
    }
    return status;
}

comac_int_status_t
_comac_composite_rectangles_init_for_glyphs (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    comac_scaled_font_t *scaled_font,
    comac_glyph_t *glyphs,
    int num_glyphs,
    const comac_clip_t *clip,
    comac_bool_t *overlap)
{
    comac_status_t status;
    comac_int_status_t int_status;

    if (! _comac_composite_rectangles_init (extents,
					    surface,
					    op,
					    source,
					    clip)) {
	_comac_composite_rectangles_fini (extents);
	return COMAC_INT_STATUS_NOTHING_TO_DO;
    }

    status = _comac_scaled_font_glyph_device_extents (scaled_font,
						      glyphs,
						      num_glyphs,
						      &extents->mask,
						      overlap);
    if (unlikely (status)) {
	_comac_composite_rectangles_fini (extents);
	return status;
    }
    if (overlap && *overlap &&
	scaled_font->options.antialias == COMAC_ANTIALIAS_NONE &&
	_comac_pattern_is_opaque_solid (&extents->source_pattern.base)) {
	*overlap = FALSE;
    }

    int_status = _comac_composite_rectangles_intersect (extents, clip);
    if (int_status == COMAC_INT_STATUS_NOTHING_TO_DO) {
	_comac_composite_rectangles_fini (extents);
    }
    return int_status;
}

comac_bool_t
_comac_composite_rectangles_can_reduce_clip (
    comac_composite_rectangles_t *composite, comac_clip_t *clip)
{
    comac_rectangle_int_t extents;
    comac_box_t box;

    if (clip == NULL)
	return TRUE;

    extents = composite->destination;
    if (composite->is_bounded & COMAC_OPERATOR_BOUND_BY_SOURCE)
	_comac_rectangle_intersect (&extents, &composite->source);
    if (composite->is_bounded & COMAC_OPERATOR_BOUND_BY_MASK)
	_comac_rectangle_intersect (&extents, &composite->mask);

    _comac_box_from_rectangle (&box, &extents);
    return _comac_clip_contains_box (clip, &box);
}

comac_int_status_t
_comac_composite_rectangles_add_to_damage (
    comac_composite_rectangles_t *composite, comac_boxes_t *damage)
{
    comac_int_status_t status;
    int n;

    for (n = 0; n < composite->clip->num_boxes; n++) {
	status = _comac_boxes_add (damage,
				   COMAC_ANTIALIAS_NONE,
				   &composite->clip->boxes[n]);
	if (unlikely (status))
	    return status;
    }

    return COMAC_INT_STATUS_SUCCESS;
}
