/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
#include "comac-list-inline.h"
#include "comac-path-fixed-private.h"
#include "comac-slope-private.h"

static comac_status_t
_comac_path_fixed_add (comac_path_fixed_t  *path,
		       comac_path_op_t	    op,
		       const comac_point_t *points,
		       int		    num_points);

static void
_comac_path_fixed_add_buf (comac_path_fixed_t *path,
			   comac_path_buf_t   *buf);

static comac_path_buf_t *
_comac_path_buf_create (int size_ops, int size_points);

static void
_comac_path_buf_destroy (comac_path_buf_t *buf);

static void
_comac_path_buf_add_op (comac_path_buf_t *buf,
			comac_path_op_t   op);

static void
_comac_path_buf_add_points (comac_path_buf_t       *buf,
			    const comac_point_t    *points,
			    int		            num_points);

void
_comac_path_fixed_init (comac_path_fixed_t *path)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (path, sizeof (comac_path_fixed_t)));

    comac_list_init (&path->buf.base.link);

    path->buf.base.num_ops = 0;
    path->buf.base.num_points = 0;
    path->buf.base.size_ops = ARRAY_LENGTH (path->buf.op);
    path->buf.base.size_points = ARRAY_LENGTH (path->buf.points);
    path->buf.base.op = path->buf.op;
    path->buf.base.points = path->buf.points;

    path->current_point.x = 0;
    path->current_point.y = 0;
    path->last_move_point = path->current_point;

    path->has_current_point = FALSE;
    path->needs_move_to = TRUE;
    path->has_extents = FALSE;
    path->has_curve_to = FALSE;
    path->stroke_is_rectilinear = TRUE;
    path->fill_is_rectilinear = TRUE;
    path->fill_maybe_region = TRUE;
    path->fill_is_empty = TRUE;

    path->extents.p1.x = path->extents.p1.y = 0;
    path->extents.p2.x = path->extents.p2.y = 0;
}

comac_status_t
_comac_path_fixed_init_copy (comac_path_fixed_t *path,
			     const comac_path_fixed_t *other)
{
    comac_path_buf_t *buf, *other_buf;
    unsigned int num_points, num_ops;

    VG (VALGRIND_MAKE_MEM_UNDEFINED (path, sizeof (comac_path_fixed_t)));

    comac_list_init (&path->buf.base.link);

    path->buf.base.op = path->buf.op;
    path->buf.base.points = path->buf.points;
    path->buf.base.size_ops = ARRAY_LENGTH (path->buf.op);
    path->buf.base.size_points = ARRAY_LENGTH (path->buf.points);

    path->current_point = other->current_point;
    path->last_move_point = other->last_move_point;

    path->has_current_point = other->has_current_point;
    path->needs_move_to = other->needs_move_to;
    path->has_extents = other->has_extents;
    path->has_curve_to = other->has_curve_to;
    path->stroke_is_rectilinear = other->stroke_is_rectilinear;
    path->fill_is_rectilinear = other->fill_is_rectilinear;
    path->fill_maybe_region = other->fill_maybe_region;
    path->fill_is_empty = other->fill_is_empty;

    path->extents = other->extents;

    path->buf.base.num_ops = other->buf.base.num_ops;
    path->buf.base.num_points = other->buf.base.num_points;
    memcpy (path->buf.op, other->buf.base.op,
	    other->buf.base.num_ops * sizeof (other->buf.op[0]));
    memcpy (path->buf.points, other->buf.points,
	    other->buf.base.num_points * sizeof (other->buf.points[0]));

    num_points = num_ops = 0;
    for (other_buf = comac_path_buf_next (comac_path_head (other));
	 other_buf != comac_path_head (other);
	 other_buf = comac_path_buf_next (other_buf))
    {
	num_ops    += other_buf->num_ops;
	num_points += other_buf->num_points;
    }

    if (num_ops) {
	buf = _comac_path_buf_create (num_ops, num_points);
	if (unlikely (buf == NULL)) {
	    _comac_path_fixed_fini (path);
	    return _comac_error (COMAC_STATUS_NO_MEMORY);
	}

	for (other_buf = comac_path_buf_next (comac_path_head (other));
	     other_buf != comac_path_head (other);
	     other_buf = comac_path_buf_next (other_buf))
	{
	    memcpy (buf->op + buf->num_ops, other_buf->op,
		    other_buf->num_ops * sizeof (buf->op[0]));
	    buf->num_ops += other_buf->num_ops;

	    memcpy (buf->points + buf->num_points, other_buf->points,
		    other_buf->num_points * sizeof (buf->points[0]));
	    buf->num_points += other_buf->num_points;
	}

	_comac_path_fixed_add_buf (path, buf);
    }

    return COMAC_STATUS_SUCCESS;
}

uintptr_t
_comac_path_fixed_hash (const comac_path_fixed_t *path)
{
    uintptr_t hash = _COMAC_HASH_INIT_VALUE;
    const comac_path_buf_t *buf;
    unsigned int count;

    count = 0;
    comac_path_foreach_buf_start (buf, path) {
	hash = _comac_hash_bytes (hash, buf->op,
			          buf->num_ops * sizeof (buf->op[0]));
	count += buf->num_ops;
    } comac_path_foreach_buf_end (buf, path);
    hash = _comac_hash_bytes (hash, &count, sizeof (count));

    count = 0;
    comac_path_foreach_buf_start (buf, path) {
	hash = _comac_hash_bytes (hash, buf->points,
			          buf->num_points * sizeof (buf->points[0]));
	count += buf->num_points;
    } comac_path_foreach_buf_end (buf, path);
    hash = _comac_hash_bytes (hash, &count, sizeof (count));

    return hash;
}

unsigned long
_comac_path_fixed_size (const comac_path_fixed_t *path)
{
    const comac_path_buf_t *buf;
    int num_points, num_ops;

    num_ops = num_points = 0;
    comac_path_foreach_buf_start (buf, path) {
	num_ops    += buf->num_ops;
	num_points += buf->num_points;
    } comac_path_foreach_buf_end (buf, path);

    return num_ops * sizeof (buf->op[0]) +
	   num_points * sizeof (buf->points[0]);
}

comac_bool_t
_comac_path_fixed_equal (const comac_path_fixed_t *a,
			 const comac_path_fixed_t *b)
{
    const comac_path_buf_t *buf_a, *buf_b;
    const comac_path_op_t *ops_a, *ops_b;
    const comac_point_t *points_a, *points_b;
    int num_points_a, num_ops_a;
    int num_points_b, num_ops_b;

    if (a == b)
	return TRUE;

    /* use the flags to quickly differentiate based on contents */
    if (a->has_curve_to != b->has_curve_to)
    {
	return FALSE;
    }

    if (a->extents.p1.x != b->extents.p1.x ||
	a->extents.p1.y != b->extents.p1.y ||
	a->extents.p2.x != b->extents.p2.x ||
	a->extents.p2.y != b->extents.p2.y)
    {
	return FALSE;
    }

    num_ops_a = num_points_a = 0;
    comac_path_foreach_buf_start (buf_a, a) {
	num_ops_a    += buf_a->num_ops;
	num_points_a += buf_a->num_points;
    } comac_path_foreach_buf_end (buf_a, a);

    num_ops_b = num_points_b = 0;
    comac_path_foreach_buf_start (buf_b, b) {
	num_ops_b    += buf_b->num_ops;
	num_points_b += buf_b->num_points;
    } comac_path_foreach_buf_end (buf_b, b);

    if (num_ops_a == 0 && num_ops_b == 0)
	return TRUE;

    if (num_ops_a != num_ops_b || num_points_a != num_points_b)
	return FALSE;

    buf_a = comac_path_head (a);
    num_points_a = buf_a->num_points;
    num_ops_a = buf_a->num_ops;
    ops_a = buf_a->op;
    points_a = buf_a->points;

    buf_b = comac_path_head (b);
    num_points_b = buf_b->num_points;
    num_ops_b = buf_b->num_ops;
    ops_b = buf_b->op;
    points_b = buf_b->points;

    while (TRUE) {
	int num_ops = MIN (num_ops_a, num_ops_b);
	int num_points = MIN (num_points_a, num_points_b);

	if (memcmp (ops_a, ops_b, num_ops * sizeof (comac_path_op_t)))
	    return FALSE;
	if (memcmp (points_a, points_b, num_points * sizeof (comac_point_t)))
	    return FALSE;

	num_ops_a -= num_ops;
	ops_a += num_ops;
	num_points_a -= num_points;
	points_a += num_points;
	if (num_ops_a == 0 || num_points_a == 0) {
	    if (num_ops_a || num_points_a)
		return FALSE;

	    buf_a = comac_path_buf_next (buf_a);
	    if (buf_a == comac_path_head (a))
		break;

	    num_points_a = buf_a->num_points;
	    num_ops_a = buf_a->num_ops;
	    ops_a = buf_a->op;
	    points_a = buf_a->points;
	}

	num_ops_b -= num_ops;
	ops_b += num_ops;
	num_points_b -= num_points;
	points_b += num_points;
	if (num_ops_b == 0 || num_points_b == 0) {
	    if (num_ops_b || num_points_b)
		return FALSE;

	    buf_b = comac_path_buf_next (buf_b);
	    if (buf_b == comac_path_head (b))
		break;

	    num_points_b = buf_b->num_points;
	    num_ops_b = buf_b->num_ops;
	    ops_b = buf_b->op;
	    points_b = buf_b->points;
	}
    }

    return TRUE;
}

comac_path_fixed_t *
_comac_path_fixed_create (void)
{
    comac_path_fixed_t	*path;

    path = _comac_malloc (sizeof (comac_path_fixed_t));
    if (!path) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return NULL;
    }

    _comac_path_fixed_init (path);
    return path;
}

void
_comac_path_fixed_fini (comac_path_fixed_t *path)
{
    comac_path_buf_t *buf;

    buf = comac_path_buf_next (comac_path_head (path));
    while (buf != comac_path_head (path)) {
	comac_path_buf_t *this = buf;
	buf = comac_path_buf_next (buf);
	_comac_path_buf_destroy (this);
    }

    VG (VALGRIND_MAKE_MEM_UNDEFINED (path, sizeof (comac_path_fixed_t)));
}

void
_comac_path_fixed_destroy (comac_path_fixed_t *path)
{
    _comac_path_fixed_fini (path);
    free (path);
}

static comac_path_op_t
_comac_path_fixed_last_op (comac_path_fixed_t *path)
{
    comac_path_buf_t *buf;

    buf = comac_path_tail (path);
    assert (buf->num_ops != 0);

    return buf->op[buf->num_ops - 1];
}

static inline const comac_point_t *
_comac_path_fixed_penultimate_point (comac_path_fixed_t *path)
{
    comac_path_buf_t *buf;

    buf = comac_path_tail (path);
    if (likely (buf->num_points >= 2)) {
	return &buf->points[buf->num_points - 2];
    } else {
	comac_path_buf_t *prev_buf = comac_path_buf_prev (buf);

	assert (prev_buf->num_points >= 2 - buf->num_points);
	return &prev_buf->points[prev_buf->num_points - (2 - buf->num_points)];
    }
}

static void
_comac_path_fixed_drop_line_to (comac_path_fixed_t *path)
{
    comac_path_buf_t *buf;

    assert (_comac_path_fixed_last_op (path) == COMAC_PATH_OP_LINE_TO);

    buf = comac_path_tail (path);
    buf->num_points--;
    buf->num_ops--;
}

comac_status_t
_comac_path_fixed_move_to (comac_path_fixed_t  *path,
			   comac_fixed_t	x,
			   comac_fixed_t	y)
{
    _comac_path_fixed_new_sub_path (path);

    path->has_current_point = TRUE;
    path->current_point.x = x;
    path->current_point.y = y;
    path->last_move_point = path->current_point;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_path_fixed_move_to_apply (comac_path_fixed_t  *path)
{
    if (likely (! path->needs_move_to))
	return COMAC_STATUS_SUCCESS;

    path->needs_move_to = FALSE;

    if (path->has_extents) {
	_comac_box_add_point (&path->extents, &path->current_point);
    } else {
	_comac_box_set (&path->extents, &path->current_point, &path->current_point);
	path->has_extents = TRUE;
    }

    if (path->fill_maybe_region) {
	path->fill_maybe_region = _comac_fixed_is_integer (path->current_point.x) &&
				  _comac_fixed_is_integer (path->current_point.y);
    }

    path->last_move_point = path->current_point;

    return _comac_path_fixed_add (path, COMAC_PATH_OP_MOVE_TO, &path->current_point, 1);
}

void
_comac_path_fixed_new_sub_path (comac_path_fixed_t *path)
{
    if (! path->needs_move_to) {
	/* If the current subpath doesn't need_move_to, it contains at least one command */
	if (path->fill_is_rectilinear) {
	    /* Implicitly close for fill */
	    path->fill_is_rectilinear = path->current_point.x == path->last_move_point.x ||
					path->current_point.y == path->last_move_point.y;
	    path->fill_maybe_region &= path->fill_is_rectilinear;
	}
	path->needs_move_to = TRUE;
    }

    path->has_current_point = FALSE;
}

comac_status_t
_comac_path_fixed_rel_move_to (comac_path_fixed_t *path,
			       comac_fixed_t	   dx,
			       comac_fixed_t	   dy)
{
    if (unlikely (! path->has_current_point))
	return _comac_error (COMAC_STATUS_NO_CURRENT_POINT);

    return _comac_path_fixed_move_to (path,
				      path->current_point.x + dx,
				      path->current_point.y + dy);

}

comac_status_t
_comac_path_fixed_line_to (comac_path_fixed_t *path,
			   comac_fixed_t	x,
			   comac_fixed_t	y)
{
    comac_status_t status;
    comac_point_t point;

    point.x = x;
    point.y = y;

    /* When there is not yet a current point, the line_to operation
     * becomes a move_to instead. Note: We have to do this by
     * explicitly calling into _comac_path_fixed_move_to to ensure
     * that the last_move_point state is updated properly.
     */
    if (! path->has_current_point)
	return _comac_path_fixed_move_to (path, point.x, point.y);

    status = _comac_path_fixed_move_to_apply (path);
    if (unlikely (status))
	return status;

    /* If the previous op was but the initial MOVE_TO and this segment
     * is degenerate, then we can simply skip this point. Note that
     * a move-to followed by a degenerate line-to is a valid path for
     * stroking, but at all other times is simply a degenerate segment.
     */
    if (_comac_path_fixed_last_op (path) != COMAC_PATH_OP_MOVE_TO) {
	if (x == path->current_point.x && y == path->current_point.y)
	    return COMAC_STATUS_SUCCESS;
    }

    /* If the previous op was also a LINE_TO with the same gradient,
     * then just change its end-point rather than adding a new op.
     */
    if (_comac_path_fixed_last_op (path) == COMAC_PATH_OP_LINE_TO) {
	const comac_point_t *p;

	p = _comac_path_fixed_penultimate_point (path);
	if (p->x == path->current_point.x && p->y == path->current_point.y) {
	    /* previous line element was degenerate, replace */
	    _comac_path_fixed_drop_line_to (path);
	} else {
	    comac_slope_t prev, self;

	    _comac_slope_init (&prev, p, &path->current_point);
	    _comac_slope_init (&self, &path->current_point, &point);
	    if (_comac_slope_equal (&prev, &self) &&
		/* cannot trim anti-parallel segments whilst stroking */
		! _comac_slope_backwards (&prev, &self))
	    {
		_comac_path_fixed_drop_line_to (path);
		/* In this case the flags might be more restrictive than
		 * what we actually need.
		 * When changing the flags definition we should check if
		 * changing the line_to point can affect them.
		*/
	    }
	}
    }

    if (path->stroke_is_rectilinear) {
	path->stroke_is_rectilinear = path->current_point.x == x ||
				      path->current_point.y == y;
	path->fill_is_rectilinear &= path->stroke_is_rectilinear;
	path->fill_maybe_region &= path->fill_is_rectilinear;
	if (path->fill_maybe_region) {
	    path->fill_maybe_region = _comac_fixed_is_integer (x) &&
				      _comac_fixed_is_integer (y);
	}
	if (path->fill_is_empty) {
	    path->fill_is_empty = path->current_point.x == x &&
				  path->current_point.y == y;
	}
    }

    path->current_point = point;

    _comac_box_add_point (&path->extents, &point);

    return _comac_path_fixed_add (path, COMAC_PATH_OP_LINE_TO, &point, 1);
}

comac_status_t
_comac_path_fixed_rel_line_to (comac_path_fixed_t *path,
			       comac_fixed_t	   dx,
			       comac_fixed_t	   dy)
{
    if (unlikely (! path->has_current_point))
	return _comac_error (COMAC_STATUS_NO_CURRENT_POINT);

    return _comac_path_fixed_line_to (path,
				      path->current_point.x + dx,
				      path->current_point.y + dy);
}

comac_status_t
_comac_path_fixed_curve_to (comac_path_fixed_t	*path,
			    comac_fixed_t x0, comac_fixed_t y0,
			    comac_fixed_t x1, comac_fixed_t y1,
			    comac_fixed_t x2, comac_fixed_t y2)
{
    comac_status_t status;
    comac_point_t point[3];

    /* If this curves does not move, replace it with a line-to.
     * This frequently happens with rounded-rectangles and r==0.
    */
    if (path->current_point.x == x2 && path->current_point.y == y2) {
	if (x1 == x2 && x0 == x2 && y1 == y2 && y0 == y2)
	    return _comac_path_fixed_line_to (path, x2, y2);

	/* We may want to check for the absence of a cusp, in which case
	 * we can also replace the curve-to with a line-to.
	 */
    }

    /* make sure subpaths are started properly */
    if (! path->has_current_point) {
	status = _comac_path_fixed_move_to (path, x0, y0);
	assert (status == COMAC_STATUS_SUCCESS);
    }

    status = _comac_path_fixed_move_to_apply (path);
    if (unlikely (status))
	return status;

    /* If the previous op was a degenerate LINE_TO, drop it. */
    if (_comac_path_fixed_last_op (path) == COMAC_PATH_OP_LINE_TO) {
	const comac_point_t *p;

	p = _comac_path_fixed_penultimate_point (path);
	if (p->x == path->current_point.x && p->y == path->current_point.y) {
	    /* previous line element was degenerate, replace */
	    _comac_path_fixed_drop_line_to (path);
	}
    }

    point[0].x = x0; point[0].y = y0;
    point[1].x = x1; point[1].y = y1;
    point[2].x = x2; point[2].y = y2;

    _comac_box_add_curve_to (&path->extents, &path->current_point,
			     &point[0], &point[1], &point[2]);

    path->current_point = point[2];
    path->has_curve_to = TRUE;
    path->stroke_is_rectilinear = FALSE;
    path->fill_is_rectilinear = FALSE;
    path->fill_maybe_region = FALSE;
    path->fill_is_empty = FALSE;

    return _comac_path_fixed_add (path, COMAC_PATH_OP_CURVE_TO, point, 3);
}

comac_status_t
_comac_path_fixed_rel_curve_to (comac_path_fixed_t *path,
				comac_fixed_t dx0, comac_fixed_t dy0,
				comac_fixed_t dx1, comac_fixed_t dy1,
				comac_fixed_t dx2, comac_fixed_t dy2)
{
    if (unlikely (! path->has_current_point))
	return _comac_error (COMAC_STATUS_NO_CURRENT_POINT);

    return _comac_path_fixed_curve_to (path,
				       path->current_point.x + dx0,
				       path->current_point.y + dy0,

				       path->current_point.x + dx1,
				       path->current_point.y + dy1,

				       path->current_point.x + dx2,
				       path->current_point.y + dy2);
}

comac_status_t
_comac_path_fixed_close_path (comac_path_fixed_t *path)
{
    comac_status_t status;

    if (! path->has_current_point)
	return COMAC_STATUS_SUCCESS;

    /*
     * Add a line_to, to compute flags and solve any degeneracy.
     * It will be removed later (if it was actually added).
     */
    status = _comac_path_fixed_line_to (path,
					path->last_move_point.x,
					path->last_move_point.y);
    if (unlikely (status))
	return status;

    /*
     * If the command used to close the path is a line_to, drop it.
     * We must check that last command is actually a line_to,
     * because the path could have been closed with a curve_to (and
     * the previous line_to not added as it would be degenerate).
     */
    if (_comac_path_fixed_last_op (path) == COMAC_PATH_OP_LINE_TO)
	    _comac_path_fixed_drop_line_to (path);

    path->needs_move_to = TRUE; /* After close_path, add an implicit move_to */

    return _comac_path_fixed_add (path, COMAC_PATH_OP_CLOSE_PATH, NULL, 0);
}

comac_bool_t
_comac_path_fixed_get_current_point (comac_path_fixed_t *path,
				     comac_fixed_t	*x,
				     comac_fixed_t	*y)
{
    if (! path->has_current_point)
	return FALSE;

    *x = path->current_point.x;
    *y = path->current_point.y;

    return TRUE;
}

static comac_status_t
_comac_path_fixed_add (comac_path_fixed_t   *path,
		       comac_path_op_t	     op,
		       const comac_point_t  *points,
		       int		     num_points)
{
    comac_path_buf_t *buf = comac_path_tail (path);

    if (buf->num_ops + 1 > buf->size_ops ||
	buf->num_points + num_points > buf->size_points)
    {
	buf = _comac_path_buf_create (buf->num_ops * 2, buf->num_points * 2);
	if (unlikely (buf == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	_comac_path_fixed_add_buf (path, buf);
    }

    if (WATCH_PATH) {
	const char *op_str[] = {
	    "move-to",
	    "line-to",
	    "curve-to",
	    "close-path",
	};
	char buf[1024];
	int len = 0;
	int i;

	len += snprintf (buf + len, sizeof (buf), "[");
	for (i = 0; i < num_points; i++) {
	    if (i != 0)
		len += snprintf (buf + len, sizeof (buf), " ");
	    len += snprintf (buf + len, sizeof (buf), "(%f, %f)",
			     _comac_fixed_to_double (points[i].x),
			     _comac_fixed_to_double (points[i].y));
	}
	len += snprintf (buf + len, sizeof (buf), "]");

#define STRINGIFYFLAG(x)  (path->x ? #x " " : "")
	fprintf (stderr,
		 "_comac_path_fixed_add (%s, %s) [%s%s%s%s%s%s%s%s]\n",
		 op_str[(int) op], buf,
		 STRINGIFYFLAG(has_current_point),
		 STRINGIFYFLAG(needs_move_to),
		 STRINGIFYFLAG(has_extents),
		 STRINGIFYFLAG(has_curve_to),
		 STRINGIFYFLAG(stroke_is_rectilinear),
		 STRINGIFYFLAG(fill_is_rectilinear),
		 STRINGIFYFLAG(fill_is_empty),
		 STRINGIFYFLAG(fill_maybe_region)
		 );
#undef STRINGIFYFLAG
    }

    _comac_path_buf_add_op (buf, op);
    _comac_path_buf_add_points (buf, points, num_points);

    return COMAC_STATUS_SUCCESS;
}

static void
_comac_path_fixed_add_buf (comac_path_fixed_t *path,
			   comac_path_buf_t   *buf)
{
    comac_list_add_tail (&buf->link, &comac_path_head (path)->link);
}

COMPILE_TIME_ASSERT (sizeof (comac_path_op_t) == 1);
static comac_path_buf_t *
_comac_path_buf_create (int size_ops, int size_points)
{
    comac_path_buf_t *buf;

    /* adjust size_ops to ensure that buf->points is naturally aligned */
    size_ops += sizeof (double) - ((sizeof (comac_path_buf_t) + size_ops) % sizeof (double));
    buf = _comac_malloc_ab_plus_c (size_points, sizeof (comac_point_t), size_ops + sizeof (comac_path_buf_t));
    if (buf) {
	buf->num_ops = 0;
	buf->num_points = 0;
	buf->size_ops = size_ops;
	buf->size_points = size_points;

	buf->op = (comac_path_op_t *) (buf + 1);
	buf->points = (comac_point_t *) (buf->op + size_ops);
    }

    return buf;
}

static void
_comac_path_buf_destroy (comac_path_buf_t *buf)
{
    free (buf);
}

static void
_comac_path_buf_add_op (comac_path_buf_t *buf,
			comac_path_op_t	  op)
{
    buf->op[buf->num_ops++] = op;
}

static void
_comac_path_buf_add_points (comac_path_buf_t       *buf,
			    const comac_point_t    *points,
			    int		            num_points)
{
    if (num_points == 0)
	return;

    memcpy (buf->points + buf->num_points,
	    points,
	    sizeof (points[0]) * num_points);
    buf->num_points += num_points;
}

comac_status_t
_comac_path_fixed_interpret (const comac_path_fixed_t		*path,
			     comac_path_fixed_move_to_func_t	*move_to,
			     comac_path_fixed_line_to_func_t	*line_to,
			     comac_path_fixed_curve_to_func_t	*curve_to,
			     comac_path_fixed_close_path_func_t	*close_path,
			     void				*closure)
{
    const comac_path_buf_t *buf;
    comac_status_t status;

    comac_path_foreach_buf_start (buf, path) {
	const comac_point_t *points = buf->points;
	unsigned int i;

	for (i = 0; i < buf->num_ops; i++) {
	    switch (buf->op[i]) {
	    case COMAC_PATH_OP_MOVE_TO:
		status = (*move_to) (closure, &points[0]);
		points += 1;
		break;
	    case COMAC_PATH_OP_LINE_TO:
		status = (*line_to) (closure, &points[0]);
		points += 1;
		break;
	    case COMAC_PATH_OP_CURVE_TO:
		status = (*curve_to) (closure, &points[0], &points[1], &points[2]);
		points += 3;
		break;
	    default:
		ASSERT_NOT_REACHED;
	    case COMAC_PATH_OP_CLOSE_PATH:
		status = (*close_path) (closure);
		break;
	    }

	    if (unlikely (status))
		return status;
	}
    } comac_path_foreach_buf_end (buf, path);

    if (path->needs_move_to && path->has_current_point)
	return (*move_to) (closure, &path->current_point);

    return COMAC_STATUS_SUCCESS;
}

typedef struct _comac_path_fixed_append_closure {
    comac_point_t	    offset;
    comac_path_fixed_t	    *path;
} comac_path_fixed_append_closure_t;

static comac_status_t
_append_move_to (void		 *abstract_closure,
		 const comac_point_t  *point)
{
    comac_path_fixed_append_closure_t	*closure = abstract_closure;

    return _comac_path_fixed_move_to (closure->path,
				      point->x + closure->offset.x,
				      point->y + closure->offset.y);
}

static comac_status_t
_append_line_to (void		 *abstract_closure,
		 const comac_point_t *point)
{
    comac_path_fixed_append_closure_t	*closure = abstract_closure;

    return _comac_path_fixed_line_to (closure->path,
				      point->x + closure->offset.x,
				      point->y + closure->offset.y);
}

static comac_status_t
_append_curve_to (void	  *abstract_closure,
		  const comac_point_t *p0,
		  const comac_point_t *p1,
		  const comac_point_t *p2)
{
    comac_path_fixed_append_closure_t	*closure = abstract_closure;

    return _comac_path_fixed_curve_to (closure->path,
				       p0->x + closure->offset.x,
				       p0->y + closure->offset.y,
				       p1->x + closure->offset.x,
				       p1->y + closure->offset.y,
				       p2->x + closure->offset.x,
				       p2->y + closure->offset.y);
}

static comac_status_t
_append_close_path (void *abstract_closure)
{
    comac_path_fixed_append_closure_t	*closure = abstract_closure;

    return _comac_path_fixed_close_path (closure->path);
}

comac_status_t
_comac_path_fixed_append (comac_path_fixed_t		    *path,
			  const comac_path_fixed_t	    *other,
			  comac_fixed_t			     tx,
			  comac_fixed_t			     ty)
{
    comac_path_fixed_append_closure_t closure;

    closure.path = path;
    closure.offset.x = tx;
    closure.offset.y = ty;

    return _comac_path_fixed_interpret (other,
					_append_move_to,
					_append_line_to,
					_append_curve_to,
					_append_close_path,
					&closure);
}

static void
_comac_path_fixed_offset_and_scale (comac_path_fixed_t *path,
				    comac_fixed_t offx,
				    comac_fixed_t offy,
				    comac_fixed_t scalex,
				    comac_fixed_t scaley)
{
    comac_path_buf_t *buf;
    unsigned int i;

    if (scalex == COMAC_FIXED_ONE && scaley == COMAC_FIXED_ONE) {
	_comac_path_fixed_translate (path, offx, offy);
	return;
    }

    path->last_move_point.x = _comac_fixed_mul (scalex, path->last_move_point.x) + offx;
    path->last_move_point.y = _comac_fixed_mul (scaley, path->last_move_point.y) + offy;
    path->current_point.x   = _comac_fixed_mul (scalex, path->current_point.x) + offx;
    path->current_point.y   = _comac_fixed_mul (scaley, path->current_point.y) + offy;

    path->fill_maybe_region = TRUE;

    comac_path_foreach_buf_start (buf, path) {
	 for (i = 0; i < buf->num_points; i++) {
	     if (scalex != COMAC_FIXED_ONE)
		 buf->points[i].x = _comac_fixed_mul (buf->points[i].x, scalex);
	     buf->points[i].x += offx;

	     if (scaley != COMAC_FIXED_ONE)
		 buf->points[i].y = _comac_fixed_mul (buf->points[i].y, scaley);
	     buf->points[i].y += offy;

	    if (path->fill_maybe_region) {
		path->fill_maybe_region = _comac_fixed_is_integer (buf->points[i].x) &&
					  _comac_fixed_is_integer (buf->points[i].y);
	    }
	 }
    } comac_path_foreach_buf_end (buf, path);

    path->fill_maybe_region &= path->fill_is_rectilinear;

    path->extents.p1.x = _comac_fixed_mul (scalex, path->extents.p1.x) + offx;
    path->extents.p2.x = _comac_fixed_mul (scalex, path->extents.p2.x) + offx;
    if (scalex < 0) {
	comac_fixed_t t = path->extents.p1.x;
	path->extents.p1.x = path->extents.p2.x;
	path->extents.p2.x = t;
    }

    path->extents.p1.y = _comac_fixed_mul (scaley, path->extents.p1.y) + offy;
    path->extents.p2.y = _comac_fixed_mul (scaley, path->extents.p2.y) + offy;
    if (scaley < 0) {
	comac_fixed_t t = path->extents.p1.y;
	path->extents.p1.y = path->extents.p2.y;
	path->extents.p2.y = t;
    }
}

void
_comac_path_fixed_translate (comac_path_fixed_t *path,
			     comac_fixed_t offx,
			     comac_fixed_t offy)
{
    comac_path_buf_t *buf;
    unsigned int i;

    if (offx == 0 && offy == 0)
	return;

    path->last_move_point.x += offx;
    path->last_move_point.y += offy;
    path->current_point.x += offx;
    path->current_point.y += offy;

    path->fill_maybe_region = TRUE;

    comac_path_foreach_buf_start (buf, path) {
	for (i = 0; i < buf->num_points; i++) {
	    buf->points[i].x += offx;
	    buf->points[i].y += offy;

	    if (path->fill_maybe_region) {
		path->fill_maybe_region = _comac_fixed_is_integer (buf->points[i].x) &&
					  _comac_fixed_is_integer (buf->points[i].y);
	    }
	 }
    } comac_path_foreach_buf_end (buf, path);

    path->fill_maybe_region &= path->fill_is_rectilinear;

    path->extents.p1.x += offx;
    path->extents.p1.y += offy;
    path->extents.p2.x += offx;
    path->extents.p2.y += offy;
}


static inline void
_comac_path_fixed_transform_point (comac_point_t *p,
				   const comac_matrix_t *matrix)
{
    double dx, dy;

    dx = _comac_fixed_to_double (p->x);
    dy = _comac_fixed_to_double (p->y);
    comac_matrix_transform_point (matrix, &dx, &dy);
    p->x = _comac_fixed_from_double (dx);
    p->y = _comac_fixed_from_double (dy);
}

/**
 * _comac_path_fixed_transform:
 * @path: a #comac_path_fixed_t to be transformed
 * @matrix: a #comac_matrix_t
 *
 * Transform the fixed-point path according to the given matrix.
 * There is a fast path for the case where @matrix has no rotation
 * or shear.
 **/
void
_comac_path_fixed_transform (comac_path_fixed_t	*path,
			     const comac_matrix_t     *matrix)
{
    comac_box_t extents;
    comac_point_t point;
    comac_path_buf_t *buf;
    unsigned int i;

    if (matrix->yx == 0.0 && matrix->xy == 0.0) {
	/* Fast path for the common case of scale+transform */
	_comac_path_fixed_offset_and_scale (path,
					    _comac_fixed_from_double (matrix->x0),
					    _comac_fixed_from_double (matrix->y0),
					    _comac_fixed_from_double (matrix->xx),
					    _comac_fixed_from_double (matrix->yy));
	return;
    }

    _comac_path_fixed_transform_point (&path->last_move_point, matrix);
    _comac_path_fixed_transform_point (&path->current_point, matrix);

    buf = comac_path_head (path);
    if (buf->num_points == 0)
	return;

    extents = path->extents;
    point = buf->points[0];
    _comac_path_fixed_transform_point (&point, matrix);
    _comac_box_set (&path->extents, &point, &point);

    comac_path_foreach_buf_start (buf, path) {
	for (i = 0; i < buf->num_points; i++) {
	    _comac_path_fixed_transform_point (&buf->points[i], matrix);
	    _comac_box_add_point (&path->extents, &buf->points[i]);
	}
    } comac_path_foreach_buf_end (buf, path);

    if (path->has_curve_to) {
	comac_bool_t is_tight;

	_comac_matrix_transform_bounding_box_fixed (matrix, &extents, &is_tight);
	if (!is_tight) {
	    comac_bool_t has_extents;

	    has_extents = _comac_path_bounder_extents (path, &extents);
	    assert (has_extents);
	}
	path->extents = extents;
    }

    /* flags might become more strict than needed */
    path->stroke_is_rectilinear = FALSE;
    path->fill_is_rectilinear = FALSE;
    path->fill_is_empty = FALSE;
    path->fill_maybe_region = FALSE;
}

/* Closure for path flattening */
typedef struct comac_path_flattener {
    double tolerance;
    comac_point_t current_point;
    comac_path_fixed_move_to_func_t	*move_to;
    comac_path_fixed_line_to_func_t	*line_to;
    comac_path_fixed_close_path_func_t	*close_path;
    void *closure;
} cpf_t;

static comac_status_t
_cpf_move_to (void *closure,
	      const comac_point_t *point)
{
    cpf_t *cpf = closure;

    cpf->current_point = *point;

    return cpf->move_to (cpf->closure, point);
}

static comac_status_t
_cpf_line_to (void *closure,
	      const comac_point_t *point)
{
    cpf_t *cpf = closure;

    cpf->current_point = *point;

    return cpf->line_to (cpf->closure, point);
}

static comac_status_t
_cpf_add_point (void *closure,
		const comac_point_t *point,
		const comac_slope_t *tangent)
{
    return _cpf_line_to (closure, point);
};

static comac_status_t
_cpf_curve_to (void		*closure,
	       const comac_point_t	*p1,
	       const comac_point_t	*p2,
	       const comac_point_t	*p3)
{
    cpf_t *cpf = closure;
    comac_spline_t spline;

    comac_point_t *p0 = &cpf->current_point;

    if (! _comac_spline_init (&spline,
			      _cpf_add_point,
			      cpf,
			      p0, p1, p2, p3))
    {
	return _cpf_line_to (closure, p3);
    }

    cpf->current_point = *p3;

    return _comac_spline_decompose (&spline, cpf->tolerance);
}

static comac_status_t
_cpf_close_path (void *closure)
{
    cpf_t *cpf = closure;

    return cpf->close_path (cpf->closure);
}

comac_status_t
_comac_path_fixed_interpret_flat (const comac_path_fixed_t		*path,
				  comac_path_fixed_move_to_func_t	*move_to,
				  comac_path_fixed_line_to_func_t	*line_to,
				  comac_path_fixed_close_path_func_t	*close_path,
				  void					*closure,
				  double				tolerance)
{
    cpf_t flattener;

    if (! path->has_curve_to) {
	return _comac_path_fixed_interpret (path,
					    move_to,
					    line_to,
					    NULL,
					    close_path,
					    closure);
    }

    flattener.tolerance = tolerance;
    flattener.move_to = move_to;
    flattener.line_to = line_to;
    flattener.close_path = close_path;
    flattener.closure = closure;
    return _comac_path_fixed_interpret (path,
					_cpf_move_to,
					_cpf_line_to,
					_cpf_curve_to,
					_cpf_close_path,
					&flattener);
}

static inline void
_canonical_box (comac_box_t *box,
		const comac_point_t *p1,
		const comac_point_t *p2)
{
    if (p1->x <= p2->x) {
	box->p1.x = p1->x;
	box->p2.x = p2->x;
    } else {
	box->p1.x = p2->x;
	box->p2.x = p1->x;
    }

    if (p1->y <= p2->y) {
	box->p1.y = p1->y;
	box->p2.y = p2->y;
    } else {
	box->p1.y = p2->y;
	box->p2.y = p1->y;
    }
}

static inline comac_bool_t
_path_is_quad (const comac_path_fixed_t *path)
{
    const comac_path_buf_t *buf = comac_path_head (path);

    /* Do we have the right number of ops? */
    if (buf->num_ops < 4 || buf->num_ops > 6)
	return FALSE;

    /* Check whether the ops are those that would be used for a rectangle */
    if (buf->op[0] != COMAC_PATH_OP_MOVE_TO ||
	buf->op[1] != COMAC_PATH_OP_LINE_TO ||
	buf->op[2] != COMAC_PATH_OP_LINE_TO ||
	buf->op[3] != COMAC_PATH_OP_LINE_TO)
    {
	return FALSE;
    }

    /* we accept an implicit close for filled paths */
    if (buf->num_ops > 4) {
	/* Now, there are choices. The rectangle might end with a LINE_TO
	 * (to the original point), but this isn't required. If it
	 * doesn't, then it must end with a CLOSE_PATH. */
	if (buf->op[4] == COMAC_PATH_OP_LINE_TO) {
	    if (buf->points[4].x != buf->points[0].x ||
		buf->points[4].y != buf->points[0].y)
		return FALSE;
	} else if (buf->op[4] != COMAC_PATH_OP_CLOSE_PATH) {
	    return FALSE;
	}

	if (buf->num_ops == 6) {
	    /* A trailing CLOSE_PATH or MOVE_TO is ok */
	    if (buf->op[5] != COMAC_PATH_OP_MOVE_TO &&
		buf->op[5] != COMAC_PATH_OP_CLOSE_PATH)
		return FALSE;
	}
    }

    return TRUE;
}

static inline comac_bool_t
_points_form_rect (const comac_point_t *points)
{
    if (points[0].y == points[1].y &&
	points[1].x == points[2].x &&
	points[2].y == points[3].y &&
	points[3].x == points[0].x)
	return TRUE;
    if (points[0].x == points[1].x &&
	points[1].y == points[2].y &&
	points[2].x == points[3].x &&
	points[3].y == points[0].y)
	return TRUE;
    return FALSE;
}

/*
 * Check whether the given path contains a single rectangle.
 */
comac_bool_t
_comac_path_fixed_is_box (const comac_path_fixed_t *path,
			  comac_box_t *box)
{
    const comac_path_buf_t *buf;

    if (! path->fill_is_rectilinear)
	return FALSE;

    if (! _path_is_quad (path))
	return FALSE;

    buf = comac_path_head (path);
    if (_points_form_rect (buf->points)) {
	_canonical_box (box, &buf->points[0], &buf->points[2]);
	return TRUE;
    }

    return FALSE;
}

/* Determine whether two lines A->B and C->D intersect based on the 
 * algorithm described here: http://paulbourke.net/geometry/pointlineplane/ */
static inline comac_bool_t
_lines_intersect_or_are_coincident (comac_point_t a,
				    comac_point_t b,
				    comac_point_t c,
				    comac_point_t d)
{
    comac_int64_t numerator_a, numerator_b, denominator;
    comac_bool_t denominator_negative;

    denominator = _comac_int64_sub (_comac_int32x32_64_mul (d.y - c.y, b.x - a.x),
				    _comac_int32x32_64_mul (d.x - c.x, b.y - a.y));
    numerator_a = _comac_int64_sub (_comac_int32x32_64_mul (d.x - c.x, a.y - c.y),
				    _comac_int32x32_64_mul (d.y - c.y, a.x - c.x));
    numerator_b = _comac_int64_sub (_comac_int32x32_64_mul (b.x - a.x, a.y - c.y),
				    _comac_int32x32_64_mul (b.y - a.y, a.x - c.x));

    if (_comac_int64_is_zero (denominator)) {
	/* If the denominator and numerators are both zero,
	 * the lines are coincident. */
	if (_comac_int64_is_zero (numerator_a) && _comac_int64_is_zero (numerator_b))
	    return TRUE;

	/* Otherwise, a zero denominator indicates the lines are
	*  parallel and never intersect. */
	return FALSE;
    }

    /* The lines intersect if both quotients are between 0 and 1 (exclusive). */

     /* We first test whether either quotient is a negative number. */
    denominator_negative = _comac_int64_negative (denominator);
    if (_comac_int64_negative (numerator_a) ^ denominator_negative)
	return FALSE;
    if (_comac_int64_negative (numerator_b) ^ denominator_negative)
	return FALSE;

    /* A zero quotient indicates an "intersection" at an endpoint, which
     * we aren't considering a true intersection. */
    if (_comac_int64_is_zero (numerator_a) || _comac_int64_is_zero (numerator_b))
	return FALSE;

    /* If the absolute value of the numerator is larger than or equal to the
     * denominator the result of the division would be greater than or equal
     * to one. */
    if (! denominator_negative) {
        if (! _comac_int64_lt (numerator_a, denominator) ||
	    ! _comac_int64_lt (numerator_b, denominator))
	    return FALSE;
    } else {
        if (! _comac_int64_lt (denominator, numerator_a) ||
	    ! _comac_int64_lt (denominator, numerator_b))
	    return FALSE;
    }

    return TRUE;
}

comac_bool_t
_comac_path_fixed_is_simple_quad (const comac_path_fixed_t *path)
{
    const comac_point_t *points;

    if (! _path_is_quad (path))
	return FALSE;

    points = comac_path_head (path)->points;
    if (_points_form_rect (points))
	return TRUE;

    if (_lines_intersect_or_are_coincident (points[0], points[1],
					    points[3], points[2]))
	return FALSE;

    if (_lines_intersect_or_are_coincident (points[0], points[3],
					    points[1], points[2]))
	return FALSE;

    return TRUE;
}

comac_bool_t
_comac_path_fixed_is_stroke_box (const comac_path_fixed_t *path,
				 comac_box_t *box)
{
    const comac_path_buf_t *buf = comac_path_head (path);

    if (! path->fill_is_rectilinear)
	return FALSE;

    /* Do we have the right number of ops? */
    if (buf->num_ops != 5)
	return FALSE;

    /* Check whether the ops are those that would be used for a rectangle */
    if (buf->op[0] != COMAC_PATH_OP_MOVE_TO ||
	buf->op[1] != COMAC_PATH_OP_LINE_TO ||
	buf->op[2] != COMAC_PATH_OP_LINE_TO ||
	buf->op[3] != COMAC_PATH_OP_LINE_TO ||
	buf->op[4] != COMAC_PATH_OP_CLOSE_PATH)
    {
	return FALSE;
    }

    /* Ok, we may have a box, if the points line up */
    if (buf->points[0].y == buf->points[1].y &&
	buf->points[1].x == buf->points[2].x &&
	buf->points[2].y == buf->points[3].y &&
	buf->points[3].x == buf->points[0].x)
    {
	_canonical_box (box, &buf->points[0], &buf->points[2]);
	return TRUE;
    }

    if (buf->points[0].x == buf->points[1].x &&
	buf->points[1].y == buf->points[2].y &&
	buf->points[2].x == buf->points[3].x &&
	buf->points[3].y == buf->points[0].y)
    {
	_canonical_box (box, &buf->points[0], &buf->points[2]);
	return TRUE;
    }

    return FALSE;
}

/*
 * Check whether the given path contains a single rectangle
 * that is logically equivalent to:
 * <informalexample><programlisting>
 *   comac_move_to (cr, x, y);
 *   comac_rel_line_to (cr, width, 0);
 *   comac_rel_line_to (cr, 0, height);
 *   comac_rel_line_to (cr, -width, 0);
 *   comac_close_path (cr);
 * </programlisting></informalexample>
 */
comac_bool_t
_comac_path_fixed_is_rectangle (const comac_path_fixed_t *path,
				comac_box_t        *box)
{
    const comac_path_buf_t *buf;

    if (! _comac_path_fixed_is_box (path, box))
	return FALSE;

    /* This check is valid because the current implementation of
     * _comac_path_fixed_is_box () only accepts rectangles like:
     * move,line,line,line[,line|close[,close|move]]. */
    buf = comac_path_head (path);
    if (buf->num_ops > 4)
	return TRUE;

    return FALSE;
}

void
_comac_path_fixed_iter_init (comac_path_fixed_iter_t *iter,
			     const comac_path_fixed_t *path)
{
    iter->first = iter->buf = comac_path_head (path);
    iter->n_op = 0;
    iter->n_point = 0;
}

static comac_bool_t
_comac_path_fixed_iter_next_op (comac_path_fixed_iter_t *iter)
{
    if (++iter->n_op >= iter->buf->num_ops) {
	iter->buf = comac_path_buf_next (iter->buf);
	if (iter->buf == iter->first) {
	    iter->buf = NULL;
	    return FALSE;
	}

	iter->n_op = 0;
	iter->n_point = 0;
    }

    return TRUE;
}

comac_bool_t
_comac_path_fixed_iter_is_fill_box (comac_path_fixed_iter_t *_iter,
				    comac_box_t *box)
{
    comac_point_t points[5];
    comac_path_fixed_iter_t iter;

    if (_iter->buf == NULL)
	return FALSE;

    iter = *_iter;

    if (iter.n_op == iter.buf->num_ops && ! _comac_path_fixed_iter_next_op (&iter))
	return FALSE;

    /* Check whether the ops are those that would be used for a rectangle */
    if (iter.buf->op[iter.n_op] != COMAC_PATH_OP_MOVE_TO)
	return FALSE;
    points[0] = iter.buf->points[iter.n_point++];
    if (! _comac_path_fixed_iter_next_op (&iter))
	return FALSE;

    if (iter.buf->op[iter.n_op] != COMAC_PATH_OP_LINE_TO)
	return FALSE;
    points[1] = iter.buf->points[iter.n_point++];
    if (! _comac_path_fixed_iter_next_op (&iter))
	return FALSE;

    /* a horizontal/vertical closed line is also a degenerate rectangle */
    switch (iter.buf->op[iter.n_op]) {
    case COMAC_PATH_OP_CLOSE_PATH:
	_comac_path_fixed_iter_next_op (&iter); /* fall through */
    case COMAC_PATH_OP_MOVE_TO: /* implicit close */
	box->p1 = box->p2 = points[0];
	*_iter = iter;
	return TRUE;
    default:
	return FALSE;
    case COMAC_PATH_OP_LINE_TO:
	break;
    }

    points[2] = iter.buf->points[iter.n_point++];
    if (! _comac_path_fixed_iter_next_op (&iter))
	return FALSE;

    if (iter.buf->op[iter.n_op] != COMAC_PATH_OP_LINE_TO)
	return FALSE;
    points[3] = iter.buf->points[iter.n_point++];

    /* Now, there are choices. The rectangle might end with a LINE_TO
     * (to the original point), but this isn't required. If it
     * doesn't, then it must end with a CLOSE_PATH (which may be implicit). */
    if (! _comac_path_fixed_iter_next_op (&iter)) {
	/* implicit close due to fill */
    } else if (iter.buf->op[iter.n_op] == COMAC_PATH_OP_LINE_TO) {
	points[4] = iter.buf->points[iter.n_point++];
	if (points[4].x != points[0].x || points[4].y != points[0].y)
	    return FALSE;
	_comac_path_fixed_iter_next_op (&iter);
    } else if (iter.buf->op[iter.n_op] == COMAC_PATH_OP_CLOSE_PATH) {
	_comac_path_fixed_iter_next_op (&iter);
    } else if (iter.buf->op[iter.n_op] == COMAC_PATH_OP_MOVE_TO) {
	/* implicit close-path due to new-sub-path */
    } else {
	return FALSE;
    }

    /* Ok, we may have a box, if the points line up */
    if (points[0].y == points[1].y &&
	points[1].x == points[2].x &&
	points[2].y == points[3].y &&
	points[3].x == points[0].x)
    {
	box->p1 = points[0];
	box->p2 = points[2];
	*_iter = iter;
	return TRUE;
    }

    if (points[0].x == points[1].x &&
	points[1].y == points[2].y &&
	points[2].x == points[3].x &&
	points[3].y == points[0].y)
    {
	box->p1 = points[1];
	box->p2 = points[3];
	*_iter = iter;
	return TRUE;
    }

    return FALSE;
}

comac_bool_t
_comac_path_fixed_iter_at_end (const comac_path_fixed_iter_t *iter)
{
    if (iter->buf == NULL)
	return TRUE;

    return iter->n_op == iter->buf->num_ops;
}
