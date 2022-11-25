/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
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
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-compositor-private.h"
#include "comac-damage-private.h"
#include "comac-error-private.h"

comac_int_status_t
_comac_compositor_paint (const comac_compositor_t	*compositor,
			 comac_surface_t		*surface,
			 comac_operator_t		 op,
			 const comac_pattern_t		*source,
			 const comac_clip_t		*clip)
{
    comac_composite_rectangles_t extents;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    status = _comac_composite_rectangles_init_for_paint (&extents, surface,
							 op, source,
							 clip);
    if (unlikely (status))
	return status;

    do {
	while (compositor->paint == NULL)
	    compositor = compositor->delegate;

	status = compositor->paint (compositor, &extents);

	compositor = compositor->delegate;
    } while (status == COMAC_INT_STATUS_UNSUPPORTED);

    if (status == COMAC_INT_STATUS_SUCCESS && surface->damage) {
	TRACE ((stderr, "%s: applying damage (%d,%d)x(%d, %d)\n",
		__FUNCTION__,
		extents.unbounded.x, extents.unbounded.y,
		extents.unbounded.width, extents.unbounded.height));
	surface->damage = _comac_damage_add_rectangle (surface->damage,
						       &extents.unbounded);
    }

    _comac_composite_rectangles_fini (&extents);

    return status;
}

comac_int_status_t
_comac_compositor_mask (const comac_compositor_t	*compositor,
			comac_surface_t			*surface,
			comac_operator_t		 op,
			const comac_pattern_t		*source,
			const comac_pattern_t		*mask,
			const comac_clip_t		*clip)
{
    comac_composite_rectangles_t extents;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    status = _comac_composite_rectangles_init_for_mask (&extents, surface,
							op, source, mask,
							clip);
    if (unlikely (status))
	return status;

    do {
	while (compositor->mask == NULL)
	    compositor = compositor->delegate;

	status = compositor->mask (compositor, &extents);

	compositor = compositor->delegate;
    } while (status == COMAC_INT_STATUS_UNSUPPORTED);

    if (status == COMAC_INT_STATUS_SUCCESS && surface->damage) {
	TRACE ((stderr, "%s: applying damage (%d,%d)x(%d, %d)\n",
		__FUNCTION__,
		extents.unbounded.x, extents.unbounded.y,
		extents.unbounded.width, extents.unbounded.height));
	surface->damage = _comac_damage_add_rectangle (surface->damage,
						       &extents.unbounded);
    }

    _comac_composite_rectangles_fini (&extents);

    return status;
}

static comac_int_status_t
_comac_compositor_stroke_impl (const comac_compositor_t	*compositor,
			       comac_surface_t		*surface,
			       comac_operator_t		 op,
			       const comac_pattern_t		*source,
			       const comac_path_fixed_t	*path,
			       const comac_stroke_style_t	*style,
			       const comac_matrix_t		*ctm,
			       const comac_matrix_t		*ctm_inverse,
			       double			 tolerance,
			       comac_antialias_t		 antialias,
			       const comac_clip_t		*clip)
{
    comac_composite_rectangles_t extents;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (_comac_pen_vertices_needed (tolerance, style->line_width/2, ctm) <= 1)
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    status = _comac_composite_rectangles_init_for_stroke (&extents, surface,
							  op, source,
							  path, style, ctm,
							  clip);
    if (unlikely (status))
	return status;

    do {
	while (compositor->stroke == NULL)
	    compositor = compositor->delegate;

	status = compositor->stroke (compositor, &extents,
				     path, style, ctm, ctm_inverse,
				     tolerance, antialias);

	compositor = compositor->delegate;
    } while (status == COMAC_INT_STATUS_UNSUPPORTED);

    if (status == COMAC_INT_STATUS_SUCCESS && surface->damage) {
	TRACE ((stderr, "%s: applying damage (%d,%d)x(%d, %d)\n",
		__FUNCTION__,
		extents.unbounded.x, extents.unbounded.y,
		extents.unbounded.width, extents.unbounded.height));
	surface->damage = _comac_damage_add_rectangle (surface->damage,
						       &extents.unbounded);
    }

    _comac_composite_rectangles_fini (&extents);

    return status;
}

comac_int_status_t
_comac_compositor_stroke (const comac_compositor_t	*compositor,
			  comac_surface_t		*surface,
			  comac_operator_t		 op,
			  const comac_pattern_t	*source,
			  const comac_path_fixed_t	*path,
			  const comac_stroke_style_t	*style,
			  const comac_matrix_t		*ctm,
			  const comac_matrix_t		*ctm_inverse,
			  double			 tolerance,
			  comac_antialias_t		 antialias,
			  const comac_clip_t		*clip)
{
    if (!style->is_hairline)
	return _comac_compositor_stroke_impl (compositor, surface,
				              op, source, path,
					      style, ctm, ctm_inverse,
					      tolerance, antialias, clip);
    else {
	comac_stroke_style_t hairline_style;
	comac_status_t status;
	comac_matrix_t identity;

	status = _comac_stroke_style_init_copy (&hairline_style, style);
	if (unlikely (status))
	    return status;
	
	hairline_style.line_width = 1.0;

	comac_matrix_init_identity (&identity);

	status = _comac_compositor_stroke_impl (compositor, surface,
					        op, source, path,
					        &hairline_style, &identity, &identity,
					        tolerance, antialias, clip);

	_comac_stroke_style_fini (&hairline_style);

	return status;
    }
}

comac_int_status_t
_comac_compositor_fill (const comac_compositor_t	*compositor,
			comac_surface_t			*surface,
			comac_operator_t		 op,
			const comac_pattern_t		*source,
			const comac_path_fixed_t	*path,
			comac_fill_rule_t		 fill_rule,
			double				 tolerance,
			comac_antialias_t		 antialias,
			const comac_clip_t		*clip)
{
    comac_composite_rectangles_t extents;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    status = _comac_composite_rectangles_init_for_fill (&extents, surface,
							op, source, path,
							clip);
    if (unlikely (status))
	return status;

    do {
	while (compositor->fill == NULL)
	    compositor = compositor->delegate;

	status = compositor->fill (compositor, &extents,
				   path, fill_rule, tolerance, antialias);

	compositor = compositor->delegate;
    } while (status == COMAC_INT_STATUS_UNSUPPORTED);

    if (status == COMAC_INT_STATUS_SUCCESS && surface->damage) {
	TRACE ((stderr, "%s: applying damage (%d,%d)x(%d, %d)\n",
		__FUNCTION__,
		extents.unbounded.x, extents.unbounded.y,
		extents.unbounded.width, extents.unbounded.height));
	surface->damage = _comac_damage_add_rectangle (surface->damage,
						       &extents.unbounded);
    }

    _comac_composite_rectangles_fini (&extents);

    return status;
}

comac_int_status_t
_comac_compositor_glyphs (const comac_compositor_t		*compositor,
			  comac_surface_t			*surface,
			  comac_operator_t			 op,
			  const comac_pattern_t			*source,
			  comac_glyph_t				*glyphs,
			  int					 num_glyphs,
			  comac_scaled_font_t			*scaled_font,
			  const comac_clip_t			*clip)
{
    comac_composite_rectangles_t extents;
    comac_bool_t overlap;
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    status = _comac_composite_rectangles_init_for_glyphs (&extents, surface,
							  op, source,
							  scaled_font,
							  glyphs, num_glyphs,
							  clip, &overlap);
    if (unlikely (status))
	return status;

    do {
	while (compositor->glyphs == NULL)
	    compositor = compositor->delegate;

	status = compositor->glyphs (compositor, &extents,
				     scaled_font, glyphs, num_glyphs, overlap);

	compositor = compositor->delegate;
    } while (status == COMAC_INT_STATUS_UNSUPPORTED);

    if (status == COMAC_INT_STATUS_SUCCESS && surface->damage) {
	TRACE ((stderr, "%s: applying damage (%d,%d)x(%d, %d)\n",
		__FUNCTION__,
		extents.unbounded.x, extents.unbounded.y,
		extents.unbounded.width, extents.unbounded.height));
	surface->damage = _comac_damage_add_rectangle (surface->damage,
						       &extents.unbounded);
    }

    _comac_composite_rectangles_fini (&extents);

    return status;
}
