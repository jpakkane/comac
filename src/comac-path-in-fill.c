/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2008 Chris Wilson
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
 * The Initial Developer of the Original Code is Chris Wilson.
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"
#include "comac-path-fixed-private.h"

typedef struct comac_in_fill {
    double tolerance;
    comac_bool_t on_edge;
    int winding;

    comac_fixed_t x, y;

    comac_bool_t has_current_point;
    comac_point_t current_point;
    comac_point_t first_point;
} comac_in_fill_t;

static void
_comac_in_fill_init (comac_in_fill_t	*in_fill,
		     double		 tolerance,
		     double		 x,
		     double		 y)
{
    in_fill->on_edge = FALSE;
    in_fill->winding = 0;
    in_fill->tolerance = tolerance;

    in_fill->x = _comac_fixed_from_double (x);
    in_fill->y = _comac_fixed_from_double (y);

    in_fill->has_current_point = FALSE;
    in_fill->current_point.x = 0;
    in_fill->current_point.y = 0;
}

static void
_comac_in_fill_fini (comac_in_fill_t *in_fill)
{
}

static int
edge_compare_for_y_against_x (const comac_point_t *p1,
			      const comac_point_t *p2,
			      comac_fixed_t y,
			      comac_fixed_t x)
{
    comac_fixed_t adx, ady;
    comac_fixed_t dx, dy;
    comac_int64_t L, R;

    adx = p2->x - p1->x;
    dx = x - p1->x;

    if (adx == 0)
	return -dx;
    if ((adx ^ dx) < 0)
	return adx;

    dy = y - p1->y;
    ady = p2->y - p1->y;

    L = _comac_int32x32_64_mul (dy, adx);
    R = _comac_int32x32_64_mul (dx, ady);

    return _comac_int64_cmp (L, R);
}

static void
_comac_in_fill_add_edge (comac_in_fill_t *in_fill,
			 const comac_point_t *p1,
			 const comac_point_t *p2)
{
    int dir;

    if (in_fill->on_edge)
	return;

    /* count the number of edge crossing to -∞ */

    dir = 1;
    if (p2->y < p1->y) {
	const comac_point_t *tmp;

	tmp = p1;
	p1 = p2;
	p2 = tmp;

	dir = -1;
    }

    /* First check whether the query is on an edge */
    if ((p1->x == in_fill->x && p1->y == in_fill->y) ||
	(p2->x == in_fill->x && p2->y == in_fill->y) ||
	(! (p2->y < in_fill->y || p1->y > in_fill->y ||
	   (p1->x > in_fill->x && p2->x > in_fill->x) ||
	   (p1->x < in_fill->x && p2->x < in_fill->x)) &&
	 edge_compare_for_y_against_x (p1, p2, in_fill->y, in_fill->x) == 0))
    {
	in_fill->on_edge = TRUE;
	return;
    }

    /* edge is entirely above or below, note the shortening rule */
    if (p2->y <= in_fill->y || p1->y > in_fill->y)
	return;

    /* edge lies wholly to the right */
    if (p1->x >= in_fill->x && p2->x >= in_fill->x)
	return;

    if ((p1->x <= in_fill->x && p2->x <= in_fill->x) ||
	edge_compare_for_y_against_x (p1, p2, in_fill->y, in_fill->x) < 0)
    {
	in_fill->winding += dir;
    }
}

static comac_status_t
_comac_in_fill_move_to (void *closure,
			const comac_point_t *point)
{
    comac_in_fill_t *in_fill = closure;

    /* implicit close path */
    if (in_fill->has_current_point) {
	_comac_in_fill_add_edge (in_fill,
				 &in_fill->current_point,
				 &in_fill->first_point);
    }

    in_fill->first_point = *point;
    in_fill->current_point = *point;
    in_fill->has_current_point = TRUE;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_in_fill_line_to (void *closure,
			const comac_point_t *point)
{
    comac_in_fill_t *in_fill = closure;

    if (in_fill->has_current_point)
	_comac_in_fill_add_edge (in_fill, &in_fill->current_point, point);

    in_fill->current_point = *point;
    in_fill->has_current_point = TRUE;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_in_fill_add_point (void *closure,
                          const comac_point_t *point,
                          const comac_slope_t *tangent)
{
    return _comac_in_fill_line_to (closure, point);
};

static comac_status_t
_comac_in_fill_curve_to (void *closure,
			 const comac_point_t *b,
			 const comac_point_t *c,
			 const comac_point_t *d)
{
    comac_in_fill_t *in_fill = closure;
    comac_spline_t spline;
    comac_fixed_t top, bot, left;

    /* first reject based on bbox */
    bot = top = in_fill->current_point.y;
    if (b->y < top) top = b->y;
    if (b->y > bot) bot = b->y;
    if (c->y < top) top = c->y;
    if (c->y > bot) bot = c->y;
    if (d->y < top) top = d->y;
    if (d->y > bot) bot = d->y;
    if (bot < in_fill->y || top > in_fill->y) {
	in_fill->current_point = *d;
	return COMAC_STATUS_SUCCESS;
    }

    left = in_fill->current_point.x;
    if (b->x < left) left = b->x;
    if (c->x < left) left = c->x;
    if (d->x < left) left = d->x;
    if (left > in_fill->x) {
	in_fill->current_point = *d;
	return COMAC_STATUS_SUCCESS;
    }

    /* XXX Investigate direct inspection of the inflections? */
    if (! _comac_spline_init (&spline,
			      _comac_in_fill_add_point,
			      in_fill,
			      &in_fill->current_point, b, c, d))
    {
	return COMAC_STATUS_SUCCESS;
    }

    return _comac_spline_decompose (&spline, in_fill->tolerance);
}

static comac_status_t
_comac_in_fill_close_path (void *closure)
{
    comac_in_fill_t *in_fill = closure;

    if (in_fill->has_current_point) {
	_comac_in_fill_add_edge (in_fill,
				 &in_fill->current_point,
				 &in_fill->first_point);

	in_fill->has_current_point = FALSE;
    }

    return COMAC_STATUS_SUCCESS;
}

comac_bool_t
_comac_path_fixed_in_fill (const comac_path_fixed_t	*path,
			   comac_fill_rule_t	 fill_rule,
			   double		 tolerance,
			   double		 x,
			   double		 y)
{
    comac_in_fill_t in_fill;
    comac_status_t status;
    comac_bool_t is_inside;

    if (_comac_path_fixed_fill_is_empty (path))
	return FALSE;

    _comac_in_fill_init (&in_fill, tolerance, x, y);

    status = _comac_path_fixed_interpret (path,
					  _comac_in_fill_move_to,
					  _comac_in_fill_line_to,
					  _comac_in_fill_curve_to,
					  _comac_in_fill_close_path,
					  &in_fill);
    assert (status == COMAC_STATUS_SUCCESS);

    _comac_in_fill_close_path (&in_fill);

    if (in_fill.on_edge) {
	is_inside = TRUE;
    } else switch (fill_rule) {
    case COMAC_FILL_RULE_EVEN_ODD:
	is_inside = in_fill.winding & 1;
	break;
    case COMAC_FILL_RULE_WINDING:
	is_inside = in_fill.winding != 0;
	break;
    default:
	ASSERT_NOT_REACHED;
	is_inside = FALSE;
	break;
    }

    _comac_in_fill_fini (&in_fill);

    return is_inside;
}
