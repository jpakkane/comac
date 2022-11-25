/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
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
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#define _DEFAULT_SOURCE /* for hypot() */
#include "comacint.h"

#include "comac-box-inline.h"
#include "comac-boxes-private.h"
#include "comac-error-private.h"
#include "comac-path-fixed-private.h"
#include "comac-slope-private.h"
#include "comac-stroke-dash-private.h"

typedef struct _segment_t {
    comac_point_t p1, p2;
    unsigned flags;
#define HORIZONTAL 0x1
#define FORWARDS 0x2
#define JOIN 0x4
} segment_t;

typedef struct _comac_rectilinear_stroker {
    const comac_stroke_style_t *stroke_style;
    const comac_matrix_t *ctm;
    comac_antialias_t antialias;

    comac_fixed_t half_line_x, half_line_y;
    comac_boxes_t *boxes;
    comac_point_t current_point;
    comac_point_t first_point;
    comac_bool_t open_sub_path;

    comac_stroker_dash_t dash;

    comac_bool_t has_bounds;
    comac_box_t bounds;

    int num_segments;
    int segments_size;
    segment_t *segments;
    segment_t segments_embedded[8]; /* common case is a single rectangle */
} comac_rectilinear_stroker_t;

static void
_comac_rectilinear_stroker_limit (comac_rectilinear_stroker_t *stroker,
				  const comac_box_t *boxes,
				  int num_boxes)
{
    stroker->has_bounds = TRUE;
    _comac_boxes_get_extents (boxes, num_boxes, &stroker->bounds);

    stroker->bounds.p1.x -= stroker->half_line_x;
    stroker->bounds.p2.x += stroker->half_line_x;

    stroker->bounds.p1.y -= stroker->half_line_y;
    stroker->bounds.p2.y += stroker->half_line_y;
}

static comac_bool_t
_comac_rectilinear_stroker_init (comac_rectilinear_stroker_t *stroker,
				 const comac_stroke_style_t *stroke_style,
				 const comac_matrix_t *ctm,
				 comac_antialias_t antialias,
				 comac_boxes_t *boxes)
{
    /* This special-case rectilinear stroker only supports
     * miter-joined lines (not curves) and a translation-only matrix
     * (though it could probably be extended to support a matrix with
     * uniform, integer scaling).
     *
     * It also only supports horizontal and vertical line_to
     * elements. But we don't catch that here, but instead return
     * UNSUPPORTED from _comac_rectilinear_stroker_line_to if any
     * non-rectilinear line_to is encountered.
     */
    if (stroke_style->line_join != COMAC_LINE_JOIN_MITER)
	return FALSE;

    /* If the miter limit turns right angles into bevels, then we
     * can't use this optimization. Remember, the ratio is
     * 1/sin(ɸ/2). So the cutoff is 1/sin(π/4.0) or ⎷2,
     * which we round for safety. */
    if (stroke_style->miter_limit < M_SQRT2)
	return FALSE;

    if (! (stroke_style->line_cap == COMAC_LINE_CAP_BUTT ||
	   stroke_style->line_cap == COMAC_LINE_CAP_SQUARE)) {
	return FALSE;
    }

    if (! _comac_matrix_is_scale (ctm))
	return FALSE;

    stroker->stroke_style = stroke_style;
    stroker->ctm = ctm;
    stroker->antialias = antialias;

    stroker->half_line_x = _comac_fixed_from_double (
	fabs (ctm->xx) * stroke_style->line_width / 2.0);
    stroker->half_line_y = _comac_fixed_from_double (
	fabs (ctm->yy) * stroke_style->line_width / 2.0);

    stroker->open_sub_path = FALSE;
    stroker->segments = stroker->segments_embedded;
    stroker->segments_size = ARRAY_LENGTH (stroker->segments_embedded);
    stroker->num_segments = 0;

    _comac_stroker_dash_init (&stroker->dash, stroke_style);

    stroker->has_bounds = FALSE;

    stroker->boxes = boxes;

    return TRUE;
}

static void
_comac_rectilinear_stroker_fini (comac_rectilinear_stroker_t *stroker)
{
    if (stroker->segments != stroker->segments_embedded)
	free (stroker->segments);
}

static comac_status_t
_comac_rectilinear_stroker_add_segment (comac_rectilinear_stroker_t *stroker,
					const comac_point_t *p1,
					const comac_point_t *p2,
					unsigned flags)
{
    if (COMAC_INJECT_FAULT ())
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    if (stroker->num_segments == stroker->segments_size) {
	int new_size = stroker->segments_size * 2;
	segment_t *new_segments;

	if (stroker->segments == stroker->segments_embedded) {
	    new_segments = _comac_malloc_ab (new_size, sizeof (segment_t));
	    if (unlikely (new_segments == NULL))
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	    memcpy (new_segments,
		    stroker->segments,
		    stroker->num_segments * sizeof (segment_t));
	} else {
	    new_segments = _comac_realloc_ab (stroker->segments,
					      new_size,
					      sizeof (segment_t));
	    if (unlikely (new_segments == NULL))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	}

	stroker->segments_size = new_size;
	stroker->segments = new_segments;
    }

    stroker->segments[stroker->num_segments].p1 = *p1;
    stroker->segments[stroker->num_segments].p2 = *p2;
    stroker->segments[stroker->num_segments].flags = flags;
    stroker->num_segments++;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_rectilinear_stroker_emit_segments (comac_rectilinear_stroker_t *stroker)
{
    comac_line_cap_t line_cap = stroker->stroke_style->line_cap;
    comac_fixed_t half_line_x = stroker->half_line_x;
    comac_fixed_t half_line_y = stroker->half_line_y;
    comac_status_t status;
    int i, j;

    /* For each segment we generate a single rectangle.
     * This rectangle is based on a perpendicular extension (by half the
     * line width) of the segment endpoints * after some adjustments of the
     * endpoints to account for caps and joins.
     */
    for (i = 0; i < stroker->num_segments; i++) {
	comac_bool_t lengthen_initial, lengthen_final;
	comac_point_t *a, *b;
	comac_box_t box;

	a = &stroker->segments[i].p1;
	b = &stroker->segments[i].p2;

	/* We adjust the initial point of the segment to extend the
	 * rectangle to include the previous cap or join, (this
	 * adjustment applies to all segments except for the first
	 * segment of open, butt-capped paths). However, we must be
	 * careful not to emit a miter join across a degenerate segment
	 * which has been elided.
	 *
	 * Overlapping segments will be eliminated by the tessellation.
	 * Ideally, we would not emit these self-intersections at all,
	 * but that is tricky with segments shorter than half_line_width.
	 */
	j = i == 0 ? stroker->num_segments - 1 : i - 1;
	lengthen_initial =
	    (stroker->segments[i].flags ^ stroker->segments[j].flags) &
	    HORIZONTAL;
	j = i == stroker->num_segments - 1 ? 0 : i + 1;
	lengthen_final =
	    (stroker->segments[i].flags ^ stroker->segments[j].flags) &
	    HORIZONTAL;
	if (stroker->open_sub_path) {
	    if (i == 0)
		lengthen_initial = line_cap != COMAC_LINE_CAP_BUTT;

	    if (i == stroker->num_segments - 1)
		lengthen_final = line_cap != COMAC_LINE_CAP_BUTT;
	}

	/* Perform the adjustments of the endpoints. */
	if (lengthen_initial | lengthen_final) {
	    if (a->y == b->y) {
		if (a->x < b->x) {
		    if (lengthen_initial)
			a->x -= half_line_x;
		    if (lengthen_final)
			b->x += half_line_x;
		} else {
		    if (lengthen_initial)
			a->x += half_line_x;
		    if (lengthen_final)
			b->x -= half_line_x;
		}
	    } else {
		if (a->y < b->y) {
		    if (lengthen_initial)
			a->y -= half_line_y;
		    if (lengthen_final)
			b->y += half_line_y;
		} else {
		    if (lengthen_initial)
			a->y += half_line_y;
		    if (lengthen_final)
			b->y -= half_line_y;
		}
	    }
	}

	/* Form the rectangle by expanding by half the line width in
	 * either perpendicular direction. */
	if (a->y == b->y) {
	    a->y -= half_line_y;
	    b->y += half_line_y;
	} else {
	    a->x -= half_line_x;
	    b->x += half_line_x;
	}

	if (a->x < b->x) {
	    box.p1.x = a->x;
	    box.p2.x = b->x;
	} else {
	    box.p1.x = b->x;
	    box.p2.x = a->x;
	}
	if (a->y < b->y) {
	    box.p1.y = a->y;
	    box.p2.y = b->y;
	} else {
	    box.p1.y = b->y;
	    box.p2.y = a->y;
	}

	status = _comac_boxes_add (stroker->boxes, stroker->antialias, &box);
	if (unlikely (status))
	    return status;
    }

    stroker->num_segments = 0;
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_rectilinear_stroker_emit_segments_dashed (
    comac_rectilinear_stroker_t *stroker)
{
    comac_status_t status;
    comac_line_cap_t line_cap = stroker->stroke_style->line_cap;
    comac_fixed_t half_line_x = stroker->half_line_x;
    comac_fixed_t half_line_y = stroker->half_line_y;
    int i;

    for (i = 0; i < stroker->num_segments; i++) {
	comac_point_t *a, *b;
	comac_bool_t is_horizontal;
	comac_box_t box;

	a = &stroker->segments[i].p1;
	b = &stroker->segments[i].p2;

	is_horizontal = stroker->segments[i].flags & HORIZONTAL;

	/* Handle the joins for a potentially degenerate segment. */
	if (line_cap == COMAC_LINE_CAP_BUTT &&
	    stroker->segments[i].flags & JOIN &&
	    (i != stroker->num_segments - 1 ||
	     (! stroker->open_sub_path && stroker->dash.dash_starts_on))) {
	    comac_slope_t out_slope;
	    int j = (i + 1) % stroker->num_segments;
	    comac_bool_t forwards = ! ! (stroker->segments[i].flags & FORWARDS);

	    _comac_slope_init (&out_slope,
			       &stroker->segments[j].p1,
			       &stroker->segments[j].p2);
	    box.p2 = box.p1 = stroker->segments[i].p2;

	    if (is_horizontal) {
		if (forwards)
		    box.p2.x += half_line_x;
		else
		    box.p1.x -= half_line_x;

		if (out_slope.dy > 0)
		    box.p1.y -= half_line_y;
		else
		    box.p2.y += half_line_y;
	    } else {
		if (forwards)
		    box.p2.y += half_line_y;
		else
		    box.p1.y -= half_line_y;

		if (out_slope.dx > 0)
		    box.p1.x -= half_line_x;
		else
		    box.p2.x += half_line_x;
	    }

	    status =
		_comac_boxes_add (stroker->boxes, stroker->antialias, &box);
	    if (unlikely (status))
		return status;
	}

	/* Perform the adjustments of the endpoints. */
	if (is_horizontal) {
	    if (line_cap == COMAC_LINE_CAP_SQUARE) {
		if (a->x <= b->x) {
		    a->x -= half_line_x;
		    b->x += half_line_x;
		} else {
		    a->x += half_line_x;
		    b->x -= half_line_x;
		}
	    }

	    a->y += half_line_y;
	    b->y -= half_line_y;
	} else {
	    if (line_cap == COMAC_LINE_CAP_SQUARE) {
		if (a->y <= b->y) {
		    a->y -= half_line_y;
		    b->y += half_line_y;
		} else {
		    a->y += half_line_y;
		    b->y -= half_line_y;
		}
	    }

	    a->x += half_line_x;
	    b->x -= half_line_x;
	}

	if (a->x == b->x && a->y == b->y)
	    continue;

	if (a->x < b->x) {
	    box.p1.x = a->x;
	    box.p2.x = b->x;
	} else {
	    box.p1.x = b->x;
	    box.p2.x = a->x;
	}
	if (a->y < b->y) {
	    box.p1.y = a->y;
	    box.p2.y = b->y;
	} else {
	    box.p1.y = b->y;
	    box.p2.y = a->y;
	}

	status = _comac_boxes_add (stroker->boxes, stroker->antialias, &box);
	if (unlikely (status))
	    return status;
    }

    stroker->num_segments = 0;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_rectilinear_stroker_move_to (void *closure, const comac_point_t *point)
{
    comac_rectilinear_stroker_t *stroker = closure;
    comac_status_t status;

    if (stroker->dash.dashed)
	status = _comac_rectilinear_stroker_emit_segments_dashed (stroker);
    else
	status = _comac_rectilinear_stroker_emit_segments (stroker);
    if (unlikely (status))
	return status;

    /* reset the dash pattern for new sub paths */
    _comac_stroker_dash_start (&stroker->dash);

    stroker->current_point = *point;
    stroker->first_point = *point;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_rectilinear_stroker_line_to (void *closure, const comac_point_t *b)
{
    comac_rectilinear_stroker_t *stroker = closure;
    comac_point_t *a = &stroker->current_point;
    comac_status_t status;

    /* We only support horizontal or vertical elements. */
    assert (a->x == b->x || a->y == b->y);

    /* We don't draw anything for degenerate paths. */
    if (a->x == b->x && a->y == b->y)
	return COMAC_STATUS_SUCCESS;

    status = _comac_rectilinear_stroker_add_segment (stroker,
						     a,
						     b,
						     (a->y == b->y) | JOIN);

    stroker->current_point = *b;
    stroker->open_sub_path = TRUE;

    return status;
}

static comac_status_t
_comac_rectilinear_stroker_line_to_dashed (void *closure,
					   const comac_point_t *point)
{
    comac_rectilinear_stroker_t *stroker = closure;
    const comac_point_t *a = &stroker->current_point;
    const comac_point_t *b = point;
    comac_bool_t fully_in_bounds;
    double sf, sign, remain;
    comac_fixed_t mag;
    comac_status_t status;
    comac_line_t segment;
    comac_bool_t dash_on = FALSE;
    unsigned is_horizontal;

    /* We don't draw anything for degenerate paths. */
    if (a->x == b->x && a->y == b->y)
	return COMAC_STATUS_SUCCESS;

    /* We only support horizontal or vertical elements. */
    assert (a->x == b->x || a->y == b->y);

    fully_in_bounds = TRUE;
    if (stroker->has_bounds &&
	(! _comac_box_contains_point (&stroker->bounds, a) ||
	 ! _comac_box_contains_point (&stroker->bounds, b))) {
	fully_in_bounds = FALSE;
    }

    is_horizontal = a->y == b->y;
    if (is_horizontal) {
	mag = b->x - a->x;
	sf = fabs (stroker->ctm->xx);
    } else {
	mag = b->y - a->y;
	sf = fabs (stroker->ctm->yy);
    }
    if (mag < 0) {
	remain = _comac_fixed_to_double (-mag);
	sign = 1.;
    } else {
	remain = _comac_fixed_to_double (mag);
	is_horizontal |= FORWARDS;
	sign = -1.;
    }

    segment.p2 = segment.p1 = *a;
    while (remain > 0.) {
	double step_length;

	step_length = MIN (sf * stroker->dash.dash_remain, remain);
	remain -= step_length;

	mag = _comac_fixed_from_double (sign * remain);
	if (is_horizontal & 0x1)
	    segment.p2.x = b->x + mag;
	else
	    segment.p2.y = b->y + mag;

	if (stroker->dash.dash_on &&
	    (fully_in_bounds ||
	     _comac_box_intersects_line_segment (&stroker->bounds, &segment))) {
	    status = _comac_rectilinear_stroker_add_segment (
		stroker,
		&segment.p1,
		&segment.p2,
		is_horizontal | (remain <= 0.) << 2);
	    if (unlikely (status))
		return status;

	    dash_on = TRUE;
	} else {
	    dash_on = FALSE;
	}

	_comac_stroker_dash_step (&stroker->dash, step_length / sf);
	segment.p1 = segment.p2;
    }

    if (stroker->dash.dash_on && ! dash_on &&
	(fully_in_bounds ||
	 _comac_box_intersects_line_segment (&stroker->bounds, &segment))) {

	/* This segment ends on a transition to dash_on, compute a new face
	 * and add cap for the beginning of the next dash_on step.
	 */

	status = _comac_rectilinear_stroker_add_segment (stroker,
							 &segment.p1,
							 &segment.p1,
							 is_horizontal | JOIN);
	if (unlikely (status))
	    return status;
    }

    stroker->current_point = *point;
    stroker->open_sub_path = TRUE;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_rectilinear_stroker_close_path (void *closure)
{
    comac_rectilinear_stroker_t *stroker = closure;
    comac_status_t status;

    /* We don't draw anything for degenerate paths. */
    if (! stroker->open_sub_path)
	return COMAC_STATUS_SUCCESS;

    if (stroker->dash.dashed) {
	status =
	    _comac_rectilinear_stroker_line_to_dashed (stroker,
						       &stroker->first_point);
    } else {
	status =
	    _comac_rectilinear_stroker_line_to (stroker, &stroker->first_point);
    }
    if (unlikely (status))
	return status;

    stroker->open_sub_path = FALSE;

    if (stroker->dash.dashed)
	status = _comac_rectilinear_stroker_emit_segments_dashed (stroker);
    else
	status = _comac_rectilinear_stroker_emit_segments (stroker);
    if (unlikely (status))
	return status;

    return COMAC_STATUS_SUCCESS;
}

comac_int_status_t
_comac_path_fixed_stroke_rectilinear_to_boxes (
    const comac_path_fixed_t *path,
    const comac_stroke_style_t *stroke_style,
    const comac_matrix_t *ctm,
    comac_antialias_t antialias,
    comac_boxes_t *boxes)
{
    comac_rectilinear_stroker_t rectilinear_stroker;
    comac_int_status_t status;
    comac_box_t box;

    assert (_comac_path_fixed_stroke_is_rectilinear (path));

    if (! _comac_rectilinear_stroker_init (&rectilinear_stroker,
					   stroke_style,
					   ctm,
					   antialias,
					   boxes)) {
	return COMAC_INT_STATUS_UNSUPPORTED;
    }

    if (! rectilinear_stroker.dash.dashed &&
	_comac_path_fixed_is_stroke_box (path, &box) &&
	/* if the segments overlap we need to feed them into the tessellator */
	box.p2.x - box.p1.x > 2 * rectilinear_stroker.half_line_x &&
	box.p2.y - box.p1.y > 2 * rectilinear_stroker.half_line_y) {
	comac_box_t b;

	/* top */
	b.p1.x = box.p1.x - rectilinear_stroker.half_line_x;
	b.p2.x = box.p2.x + rectilinear_stroker.half_line_x;
	b.p1.y = box.p1.y - rectilinear_stroker.half_line_y;
	b.p2.y = box.p1.y + rectilinear_stroker.half_line_y;
	status = _comac_boxes_add (boxes, antialias, &b);
	assert (status == COMAC_INT_STATUS_SUCCESS);

	/* left  (excluding top/bottom) */
	b.p1.x = box.p1.x - rectilinear_stroker.half_line_x;
	b.p2.x = box.p1.x + rectilinear_stroker.half_line_x;
	b.p1.y = box.p1.y + rectilinear_stroker.half_line_y;
	b.p2.y = box.p2.y - rectilinear_stroker.half_line_y;
	status = _comac_boxes_add (boxes, antialias, &b);
	assert (status == COMAC_INT_STATUS_SUCCESS);

	/* right  (excluding top/bottom) */
	b.p1.x = box.p2.x - rectilinear_stroker.half_line_x;
	b.p2.x = box.p2.x + rectilinear_stroker.half_line_x;
	b.p1.y = box.p1.y + rectilinear_stroker.half_line_y;
	b.p2.y = box.p2.y - rectilinear_stroker.half_line_y;
	status = _comac_boxes_add (boxes, antialias, &b);
	assert (status == COMAC_INT_STATUS_SUCCESS);

	/* bottom */
	b.p1.x = box.p1.x - rectilinear_stroker.half_line_x;
	b.p2.x = box.p2.x + rectilinear_stroker.half_line_x;
	b.p1.y = box.p2.y - rectilinear_stroker.half_line_y;
	b.p2.y = box.p2.y + rectilinear_stroker.half_line_y;
	status = _comac_boxes_add (boxes, antialias, &b);
	assert (status == COMAC_INT_STATUS_SUCCESS);

	goto done;
    }

    if (boxes->num_limits) {
	_comac_rectilinear_stroker_limit (&rectilinear_stroker,
					  boxes->limits,
					  boxes->num_limits);
    }

    status = _comac_path_fixed_interpret (
	path,
	_comac_rectilinear_stroker_move_to,
	rectilinear_stroker.dash.dashed
	    ? _comac_rectilinear_stroker_line_to_dashed
	    : _comac_rectilinear_stroker_line_to,
	NULL,
	_comac_rectilinear_stroker_close_path,
	&rectilinear_stroker);
    if (unlikely (status))
	goto BAIL;

    if (rectilinear_stroker.dash.dashed)
	status = _comac_rectilinear_stroker_emit_segments_dashed (
	    &rectilinear_stroker);
    else
	status =
	    _comac_rectilinear_stroker_emit_segments (&rectilinear_stroker);
    if (unlikely (status))
	goto BAIL;

    /* As we incrementally tessellate, we do not eliminate self-intersections */
    status = _comac_bentley_ottmann_tessellate_boxes (boxes,
						      COMAC_FILL_RULE_WINDING,
						      boxes);
    if (unlikely (status))
	goto BAIL;

done:
    _comac_rectilinear_stroker_fini (&rectilinear_stroker);
    return COMAC_STATUS_SUCCESS;

BAIL:
    _comac_rectilinear_stroker_fini (&rectilinear_stroker);
    _comac_boxes_clear (boxes);
    return status;
}
