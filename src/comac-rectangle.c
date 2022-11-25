/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Red Hat, Inc.
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

const comac_rectangle_int_t _comac_empty_rectangle = {0, 0, 0, 0};
const comac_rectangle_int_t _comac_unbounded_rectangle = {
    COMAC_RECT_INT_MIN,
    COMAC_RECT_INT_MIN,
    COMAC_RECT_INT_MAX - COMAC_RECT_INT_MIN,
    COMAC_RECT_INT_MAX - COMAC_RECT_INT_MIN,
};

comac_private void
_comac_box_from_doubles (
    comac_box_t *box, double *x1, double *y1, double *x2, double *y2)
{
    box->p1.x = _comac_fixed_from_double (*x1);
    box->p1.y = _comac_fixed_from_double (*y1);
    box->p2.x = _comac_fixed_from_double (*x2);
    box->p2.y = _comac_fixed_from_double (*y2);
}

comac_private void
_comac_box_to_doubles (
    const comac_box_t *box, double *x1, double *y1, double *x2, double *y2)
{
    *x1 = _comac_fixed_to_double (box->p1.x);
    *y1 = _comac_fixed_to_double (box->p1.y);
    *x2 = _comac_fixed_to_double (box->p2.x);
    *y2 = _comac_fixed_to_double (box->p2.y);
}

void
_comac_box_from_rectangle (comac_box_t *box, const comac_rectangle_int_t *rect)
{
    box->p1.x = _comac_fixed_from_int (rect->x);
    box->p1.y = _comac_fixed_from_int (rect->y);
    box->p2.x = _comac_fixed_from_int (rect->x + rect->width);
    box->p2.y = _comac_fixed_from_int (rect->y + rect->height);
}

void
_comac_boxes_get_extents (const comac_box_t *boxes,
			  int num_boxes,
			  comac_box_t *extents)
{
    assert (num_boxes > 0);
    *extents = *boxes;
    while (--num_boxes)
	_comac_box_add_box (extents, ++boxes);
}

/* XXX We currently have a confusing mix of boxes and rectangles as
 * exemplified by this function.  A #comac_box_t is a rectangular area
 * represented by the coordinates of the upper left and lower right
 * corners, expressed in fixed point numbers.  A #comac_rectangle_int_t is
 * also a rectangular area, but represented by the upper left corner
 * and the width and the height, as integer numbers.
 *
 * This function converts a #comac_box_t to a #comac_rectangle_int_t by
 * increasing the area to the nearest integer coordinates.  We should
 * standardize on #comac_rectangle_fixed_t and #comac_rectangle_int_t, and
 * this function could be renamed to the more reasonable
 * _comac_rectangle_fixed_round.
 */

void
_comac_box_round_to_rectangle (const comac_box_t *box,
			       comac_rectangle_int_t *rectangle)
{
    rectangle->x = _comac_fixed_integer_floor (box->p1.x);
    rectangle->y = _comac_fixed_integer_floor (box->p1.y);
    rectangle->width = _comac_fixed_integer_ceil (box->p2.x) - rectangle->x;
    rectangle->height = _comac_fixed_integer_ceil (box->p2.y) - rectangle->y;
}

comac_bool_t
_comac_rectangle_intersect (comac_rectangle_int_t *dst,
			    const comac_rectangle_int_t *src)
{
    int x1, y1, x2, y2;

    x1 = MAX (dst->x, src->x);
    y1 = MAX (dst->y, src->y);
    /* Beware the unsigned promotion, fortunately we have bits to spare
     * as (COMAC_RECT_INT_MAX - COMAC_RECT_INT_MIN) < UINT_MAX
     */
    x2 = MIN (dst->x + (int) dst->width, src->x + (int) src->width);
    y2 = MIN (dst->y + (int) dst->height, src->y + (int) src->height);

    if (x1 >= x2 || y1 >= y2) {
	dst->x = 0;
	dst->y = 0;
	dst->width = 0;
	dst->height = 0;

	return FALSE;
    } else {
	dst->x = x1;
	dst->y = y1;
	dst->width = x2 - x1;
	dst->height = y2 - y1;

	return TRUE;
    }
}

/* Extends the dst rectangle to also contain src.
 * If one of the rectangles is empty, the result is undefined
 */
void
_comac_rectangle_union (comac_rectangle_int_t *dst,
			const comac_rectangle_int_t *src)
{
    int x1, y1, x2, y2;

    x1 = MIN (dst->x, src->x);
    y1 = MIN (dst->y, src->y);
    /* Beware the unsigned promotion, fortunately we have bits to spare
     * as (COMAC_RECT_INT_MAX - COMAC_RECT_INT_MIN) < UINT_MAX
     */
    x2 = MAX (dst->x + (int) dst->width, src->x + (int) src->width);
    y2 = MAX (dst->y + (int) dst->height, src->y + (int) src->height);

    dst->x = x1;
    dst->y = y1;
    dst->width = x2 - x1;
    dst->height = y2 - y1;
}

#define P1x (line->p1.x)
#define P1y (line->p1.y)
#define P2x (line->p2.x)
#define P2y (line->p2.y)
#define B1x (box->p1.x)
#define B1y (box->p1.y)
#define B2x (box->p2.x)
#define B2y (box->p2.y)

/*
 * Check whether any part of line intersects box.  This function essentially
 * computes whether the ray starting at line->p1 in the direction of line->p2
 * intersects the box before it reaches p2.  Normally, this is done
 * by dividing by the lengths of the line projected onto each axis.  Because
 * we're in fixed point, this function does a bit more work to avoid having to
 * do the division -- we don't care about the actual intersection point, so
 * it's of no interest to us.
 */

comac_bool_t
_comac_box_intersects_line_segment (const comac_box_t *box, comac_line_t *line)
{
    comac_fixed_t t1 = 0, t2 = 0, t3 = 0, t4 = 0;
    comac_int64_t t1y, t2y, t3x, t4x;

    comac_fixed_t xlen, ylen;

    if (_comac_box_contains_point (box, &line->p1) ||
	_comac_box_contains_point (box, &line->p2))
	return TRUE;

    xlen = P2x - P1x;
    ylen = P2y - P1y;

    if (xlen) {
	if (xlen > 0) {
	    t1 = B1x - P1x;
	    t2 = B2x - P1x;
	} else {
	    t1 = P1x - B2x;
	    t2 = P1x - B1x;
	    xlen = -xlen;
	}

	if ((t1 < 0 || t1 > xlen) && (t2 < 0 || t2 > xlen))
	    return FALSE;
    } else {
	/* Fully vertical line -- check that X is in bounds */
	if (P1x < B1x || P1x > B2x)
	    return FALSE;
    }

    if (ylen) {
	if (ylen > 0) {
	    t3 = B1y - P1y;
	    t4 = B2y - P1y;
	} else {
	    t3 = P1y - B2y;
	    t4 = P1y - B1y;
	    ylen = -ylen;
	}

	if ((t3 < 0 || t3 > ylen) && (t4 < 0 || t4 > ylen))
	    return FALSE;
    } else {
	/* Fully horizontal line -- check Y */
	if (P1y < B1y || P1y > B2y)
	    return FALSE;
    }

    /* If we had a horizontal or vertical line, then it's already been checked */
    if (P1x == P2x || P1y == P2y)
	return TRUE;

    /* Check overlap.  Note that t1 < t2 and t3 < t4 here. */
    t1y = _comac_int32x32_64_mul (t1, ylen);
    t2y = _comac_int32x32_64_mul (t2, ylen);
    t3x = _comac_int32x32_64_mul (t3, xlen);
    t4x = _comac_int32x32_64_mul (t4, xlen);

    if (_comac_int64_lt (t1y, t4x) && _comac_int64_lt (t3x, t2y))
	return TRUE;

    return FALSE;
}

static comac_status_t
_comac_box_add_spline_point (void *closure,
			     const comac_point_t *point,
			     const comac_slope_t *tangent)
{
    _comac_box_add_point (closure, point);

    return COMAC_STATUS_SUCCESS;
}

/* assumes a has been previously added */
void
_comac_box_add_curve_to (comac_box_t *extents,
			 const comac_point_t *a,
			 const comac_point_t *b,
			 const comac_point_t *c,
			 const comac_point_t *d)
{
    _comac_box_add_point (extents, d);
    if (! _comac_box_contains_point (extents, b) ||
	! _comac_box_contains_point (extents, c)) {
	comac_status_t status;

	status = _comac_spline_bound (_comac_box_add_spline_point,
				      extents,
				      a,
				      b,
				      c,
				      d);
	assert (status == COMAC_STATUS_SUCCESS);
    }
}

void
_comac_rectangle_int_from_double (comac_rectangle_int_t *recti,
				  const comac_rectangle_t *rectf)
{
    recti->x = floor (rectf->x);
    recti->y = floor (rectf->y);
    recti->width = ceil (rectf->x + rectf->width) - floor (rectf->x);
    recti->height = ceil (rectf->y + rectf->height) - floor (rectf->y);
}
