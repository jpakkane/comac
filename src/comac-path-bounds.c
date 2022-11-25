/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2003 University of Southern California
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
 */

#include "comacint.h"
#include "comac-box-inline.h"
#include "comac-error-private.h"
#include "comac-path-fixed-private.h"

typedef struct _comac_path_bounder {
    comac_point_t current_point;
    comac_bool_t has_extents;
    comac_box_t extents;
} comac_path_bounder_t;

static comac_status_t
_comac_path_bounder_move_to (void *closure, const comac_point_t *point)
{
    comac_path_bounder_t *bounder = closure;

    bounder->current_point = *point;

    if (likely (bounder->has_extents)) {
	_comac_box_add_point (&bounder->extents, point);
    } else {
	bounder->has_extents = TRUE;
	_comac_box_set (&bounder->extents, point, point);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_path_bounder_line_to (void *closure, const comac_point_t *point)
{
    comac_path_bounder_t *bounder = closure;

    bounder->current_point = *point;
    _comac_box_add_point (&bounder->extents, point);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_path_bounder_curve_to (void *closure,
			      const comac_point_t *b,
			      const comac_point_t *c,
			      const comac_point_t *d)
{
    comac_path_bounder_t *bounder = closure;

    _comac_box_add_curve_to (&bounder->extents,
			     &bounder->current_point,
			     b,
			     c,
			     d);
    bounder->current_point = *d;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_path_bounder_close_path (void *closure)
{
    return COMAC_STATUS_SUCCESS;
}

comac_bool_t
_comac_path_bounder_extents (const comac_path_fixed_t *path,
			     comac_box_t *extents)
{
    comac_path_bounder_t bounder;
    comac_status_t status;

    bounder.has_extents = FALSE;
    status = _comac_path_fixed_interpret (path,
					  _comac_path_bounder_move_to,
					  _comac_path_bounder_line_to,
					  _comac_path_bounder_curve_to,
					  _comac_path_bounder_close_path,
					  &bounder);
    assert (! status);

    if (bounder.has_extents)
	*extents = bounder.extents;

    return bounder.has_extents;
}

void
_comac_path_fixed_approximate_clip_extents (const comac_path_fixed_t *path,
					    comac_rectangle_int_t *extents)
{
    _comac_path_fixed_approximate_fill_extents (path, extents);
}

void
_comac_path_fixed_approximate_fill_extents (const comac_path_fixed_t *path,
					    comac_rectangle_int_t *extents)
{
    _comac_path_fixed_fill_extents (path, COMAC_FILL_RULE_WINDING, 0, extents);
}

void
_comac_path_fixed_fill_extents (const comac_path_fixed_t *path,
				comac_fill_rule_t fill_rule,
				double tolerance,
				comac_rectangle_int_t *extents)
{
    if (path->extents.p1.x < path->extents.p2.x &&
	path->extents.p1.y < path->extents.p2.y) {
	_comac_box_round_to_rectangle (&path->extents, extents);
    } else {
	extents->x = extents->y = 0;
	extents->width = extents->height = 0;
    }
}

/* Adjusts the fill extents (above) by the device-space pen.  */
void
_comac_path_fixed_approximate_stroke_extents (const comac_path_fixed_t *path,
					      const comac_stroke_style_t *style,
					      const comac_matrix_t *ctm,
					      comac_bool_t is_vector,
					      comac_rectangle_int_t *extents)
{
    if (path->has_extents) {
	comac_box_t box_extents;
	double dx, dy;

	_comac_stroke_style_max_distance_from_path (style, path, ctm, &dx, &dy);
	if (is_vector) {
	    /* When calculating extents for vector surfaces, ensure lines thinner
	     * than the fixed point resolution are not optimized away. */
	    double min = _comac_fixed_to_double (COMAC_FIXED_EPSILON * 2);
	    if (dx < min)
		dx = min;

	    if (dy < min)
		dy = min;
	}

	box_extents = path->extents;
	box_extents.p1.x -= _comac_fixed_from_double (dx);
	box_extents.p1.y -= _comac_fixed_from_double (dy);
	box_extents.p2.x += _comac_fixed_from_double (dx);
	box_extents.p2.y += _comac_fixed_from_double (dy);

	_comac_box_round_to_rectangle (&box_extents, extents);
    } else {
	extents->x = extents->y = 0;
	extents->width = extents->height = 0;
    }
}

comac_status_t
_comac_path_fixed_stroke_extents (const comac_path_fixed_t *path,
				  const comac_stroke_style_t *stroke_style,
				  const comac_matrix_t *ctm,
				  const comac_matrix_t *ctm_inverse,
				  double tolerance,
				  comac_rectangle_int_t *extents)
{
    comac_polygon_t polygon;
    comac_status_t status;
    comac_stroke_style_t style;

    /* When calculating extents for vector surfaces, ensure lines thinner
     * than one point are not optimized away. */
    double min_line_width =
	_comac_matrix_transformed_circle_major_axis (ctm_inverse, 1.0);
    if (stroke_style->line_width < min_line_width) {
	style = *stroke_style;
	style.line_width = min_line_width;
	stroke_style = &style;
    }

    _comac_polygon_init (&polygon, NULL, 0);
    status = _comac_path_fixed_stroke_to_polygon (path,
						  stroke_style,
						  ctm,
						  ctm_inverse,
						  tolerance,
						  &polygon);
    _comac_box_round_to_rectangle (&polygon.extents, extents);
    _comac_polygon_fini (&polygon);

    return status;
}

comac_bool_t
_comac_path_fixed_extents (const comac_path_fixed_t *path, comac_box_t *box)
{
    *box = path->extents;
    return path->has_extents;
}
