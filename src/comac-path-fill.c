/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
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
#include "comac-boxes-private.h"
#include "comac-error-private.h"
#include "comac-path-fixed-private.h"
#include "comac-region-private.h"
#include "comac-traps-private.h"

typedef struct comac_filler {
    comac_polygon_t *polygon;
    double tolerance;

    comac_box_t limit;
    comac_bool_t has_limits;

    comac_point_t current_point;
    comac_point_t last_move_to;
} comac_filler_t;

static comac_status_t
_comac_filler_line_to (void *closure,
		       const comac_point_t *point)
{
    comac_filler_t *filler = closure;
    comac_status_t status;

    status = _comac_polygon_add_external_edge (filler->polygon,
					       &filler->current_point,
					       point);

    filler->current_point = *point;

    return status;
}

static comac_status_t
_comac_filler_add_point (void *closure,
			 const comac_point_t *point,
			 const comac_slope_t *tangent)
{
    return _comac_filler_line_to (closure, point);
};

static comac_status_t
_comac_filler_close (void *closure)
{
    comac_filler_t *filler = closure;

    /* close the subpath */
    return _comac_filler_line_to (closure, &filler->last_move_to);
}

static comac_status_t
_comac_filler_move_to (void *closure,
		       const comac_point_t *point)
{
    comac_filler_t *filler = closure;
    comac_status_t status;

    /* close current subpath */
    status = _comac_filler_close (closure);
    if (unlikely (status))
	return status;

        /* make sure that the closure represents a degenerate path */
    filler->current_point = *point;
    filler->last_move_to = *point;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_filler_curve_to (void		*closure,
			const comac_point_t	*p1,
			const comac_point_t	*p2,
			const comac_point_t	*p3)
{
    comac_filler_t *filler = closure;
    comac_spline_t spline;

    if (filler->has_limits) {
	if (! _comac_spline_intersects (&filler->current_point, p1, p2, p3,
					&filler->limit))
	    return _comac_filler_line_to (filler, p3);
    }

    if (! _comac_spline_init (&spline,
			      _comac_filler_add_point, filler,
			      &filler->current_point, p1, p2, p3))
    {
	return _comac_filler_line_to (closure, p3);
    }

    return _comac_spline_decompose (&spline, filler->tolerance);
}

comac_status_t
_comac_path_fixed_fill_to_polygon (const comac_path_fixed_t *path,
				   double tolerance,
				   comac_polygon_t *polygon)
{
    comac_filler_t filler;
    comac_status_t status;

    filler.polygon = polygon;
    filler.tolerance = tolerance;

    filler.has_limits = FALSE;
    if (polygon->num_limits) {
	filler.has_limits = TRUE;
	filler.limit = polygon->limit;
    }

    /* make sure that the closure represents a degenerate path */
    filler.current_point.x = 0;
    filler.current_point.y = 0;
    filler.last_move_to = filler.current_point;

    status = _comac_path_fixed_interpret (path,
					  _comac_filler_move_to,
					  _comac_filler_line_to,
					  _comac_filler_curve_to,
					  _comac_filler_close,
					  &filler);
    if (unlikely (status))
	return status;

    return _comac_filler_close (&filler);
}

typedef struct comac_filler_rectilinear_aligned {
    comac_polygon_t *polygon;

    comac_point_t current_point;
    comac_point_t last_move_to;
} comac_filler_ra_t;

static comac_status_t
_comac_filler_ra_line_to (void *closure,
			  const comac_point_t *point)
{
    comac_filler_ra_t *filler = closure;
    comac_status_t status;
    comac_point_t p;

    p.x = _comac_fixed_round_down (point->x);
    p.y = _comac_fixed_round_down (point->y);

    status = _comac_polygon_add_external_edge (filler->polygon,
					       &filler->current_point,
					       &p);

    filler->current_point = p;

    return status;
}

static comac_status_t
_comac_filler_ra_close (void *closure)
{
    comac_filler_ra_t *filler = closure;
    return _comac_filler_ra_line_to (closure, &filler->last_move_to);
}

static comac_status_t
_comac_filler_ra_move_to (void *closure,
			  const comac_point_t *point)
{
    comac_filler_ra_t *filler = closure;
    comac_status_t status;
    comac_point_t p;

    /* close current subpath */
    status = _comac_filler_ra_close (closure);
    if (unlikely (status))
	return status;

    p.x = _comac_fixed_round_down (point->x);
    p.y = _comac_fixed_round_down (point->y);

    /* make sure that the closure represents a degenerate path */
    filler->current_point = p;
    filler->last_move_to = p;

    return COMAC_STATUS_SUCCESS;
}

comac_status_t
_comac_path_fixed_fill_rectilinear_to_polygon (const comac_path_fixed_t *path,
					       comac_antialias_t antialias,
					       comac_polygon_t *polygon)
{
    comac_filler_ra_t filler;
    comac_status_t status;

    if (antialias != COMAC_ANTIALIAS_NONE)
	return _comac_path_fixed_fill_to_polygon (path, 0., polygon);

    filler.polygon = polygon;

    /* make sure that the closure represents a degenerate path */
    filler.current_point.x = 0;
    filler.current_point.y = 0;
    filler.last_move_to = filler.current_point;

    status = _comac_path_fixed_interpret_flat (path,
					       _comac_filler_ra_move_to,
					       _comac_filler_ra_line_to,
					       _comac_filler_ra_close,
					       &filler,
					       0.);
    if (unlikely (status))
	return status;

    return _comac_filler_ra_close (&filler);
}

comac_status_t
_comac_path_fixed_fill_to_traps (const comac_path_fixed_t *path,
				 comac_fill_rule_t fill_rule,
				 double tolerance,
				 comac_traps_t *traps)
{
    comac_polygon_t polygon;
    comac_status_t status;

    if (_comac_path_fixed_fill_is_empty (path))
	return COMAC_STATUS_SUCCESS;

    _comac_polygon_init (&polygon, traps->limits, traps->num_limits);
    status = _comac_path_fixed_fill_to_polygon (path, tolerance, &polygon);
    if (unlikely (status || polygon.num_edges == 0))
	goto CLEANUP;

    status = _comac_bentley_ottmann_tessellate_polygon (traps,
							&polygon, fill_rule);

  CLEANUP:
    _comac_polygon_fini (&polygon);
    return status;
}

static comac_status_t
_comac_path_fixed_fill_rectilinear_tessellate_to_boxes (const comac_path_fixed_t *path,
							comac_fill_rule_t fill_rule,
							comac_antialias_t antialias,
							comac_boxes_t *boxes)
{
    comac_polygon_t polygon;
    comac_status_t status;

    _comac_polygon_init (&polygon, boxes->limits, boxes->num_limits);
    boxes->num_limits = 0;

    /* tolerance will be ignored as the path is rectilinear */
    status = _comac_path_fixed_fill_rectilinear_to_polygon (path, antialias, &polygon);
    if (likely (status == COMAC_STATUS_SUCCESS)) {
	status =
	    _comac_bentley_ottmann_tessellate_rectilinear_polygon_to_boxes (&polygon,
									    fill_rule,
									    boxes);
    }

    _comac_polygon_fini (&polygon);

    return status;
}

comac_status_t
_comac_path_fixed_fill_rectilinear_to_boxes (const comac_path_fixed_t *path,
					     comac_fill_rule_t fill_rule,
					     comac_antialias_t antialias,
					     comac_boxes_t *boxes)
{
    comac_path_fixed_iter_t iter;
    comac_status_t status;
    comac_box_t box;

    if (_comac_path_fixed_is_box (path, &box))
	return _comac_boxes_add (boxes, antialias, &box);

    _comac_path_fixed_iter_init (&iter, path);
    while (_comac_path_fixed_iter_is_fill_box (&iter, &box)) {
	if (box.p1.y == box.p2.y || box.p1.x == box.p2.x)
	    continue;

	if (box.p1.y > box.p2.y) {
	    comac_fixed_t t;

	    t = box.p1.y;
	    box.p1.y = box.p2.y;
	    box.p2.y = t;

	    t = box.p1.x;
	    box.p1.x = box.p2.x;
	    box.p2.x = t;
	}

	status = _comac_boxes_add (boxes, antialias, &box);
	if (unlikely (status))
	    return status;
    }

    if (_comac_path_fixed_iter_at_end (&iter))
	return _comac_bentley_ottmann_tessellate_boxes (boxes, fill_rule, boxes);

    /* path is not rectangular, try extracting clipped rectilinear edges */
    _comac_boxes_clear (boxes);
    return _comac_path_fixed_fill_rectilinear_tessellate_to_boxes (path,
								   fill_rule,
								   antialias,
								   boxes);
}
