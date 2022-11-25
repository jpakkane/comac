/*
 * Copyright © 2004 Carl Worth
 * Copyright © 2006 Red Hat, Inc.
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
 * The Initial Developer of the Original Code is Carl Worth
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

/* Provide definitions for standalone compilation */
#include "comacint.h"

#include "comac-combsort-inline.h"
#include "comac-error-private.h"
#include "comac-freelist-private.h"
#include "comac-line-inline.h"
#include "comac-traps-private.h"

#define DEBUG_PRINT_STATE 0
#define DEBUG_EVENTS 0
#define DEBUG_TRAPS 0

typedef comac_point_t comac_bo_point32_t;

typedef struct _comac_bo_intersect_ordinate {
    int32_t ordinate;
    enum { EXACT, INEXACT } exactness;
} comac_bo_intersect_ordinate_t;

typedef struct _comac_bo_intersect_point {
    comac_bo_intersect_ordinate_t x;
    comac_bo_intersect_ordinate_t y;
} comac_bo_intersect_point_t;

typedef struct _comac_bo_edge comac_bo_edge_t;
typedef struct _comac_bo_trap comac_bo_trap_t;

/* A deferred trapezoid of an edge */
struct _comac_bo_trap {
    comac_bo_edge_t *right;
    int32_t top;
};

struct _comac_bo_edge {
    comac_edge_t edge;
    comac_bo_edge_t *prev;
    comac_bo_edge_t *next;
    comac_bo_edge_t *colinear;
    comac_bo_trap_t deferred_trap;
};

/* the parent is always given by index/2 */
#define PQ_PARENT_INDEX(i) ((i) >> 1)
#define PQ_FIRST_ENTRY 1

/* left and right children are index * 2 and (index * 2) +1 respectively */
#define PQ_LEFT_CHILD_INDEX(i) ((i) << 1)

typedef enum {
    COMAC_BO_EVENT_TYPE_STOP,
    COMAC_BO_EVENT_TYPE_INTERSECTION,
    COMAC_BO_EVENT_TYPE_START
} comac_bo_event_type_t;

typedef struct _comac_bo_event {
    comac_bo_event_type_t type;
    comac_point_t point;
} comac_bo_event_t;

typedef struct _comac_bo_start_event {
    comac_bo_event_type_t type;
    comac_point_t point;
    comac_bo_edge_t edge;
} comac_bo_start_event_t;

typedef struct _comac_bo_queue_event {
    comac_bo_event_type_t type;
    comac_point_t point;
    comac_bo_edge_t *e1;
    comac_bo_edge_t *e2;
} comac_bo_queue_event_t;

typedef struct _pqueue {
    int size, max_size;

    comac_bo_event_t **elements;
    comac_bo_event_t *elements_embedded[1024];
} pqueue_t;

typedef struct _comac_bo_event_queue {
    comac_freepool_t pool;
    pqueue_t pqueue;
    comac_bo_event_t **start_events;
} comac_bo_event_queue_t;

typedef struct _comac_bo_sweep_line {
    comac_bo_edge_t *head;
    comac_bo_edge_t *stopped;
    int32_t current_y;
    comac_bo_edge_t *current_edge;
} comac_bo_sweep_line_t;

#if DEBUG_TRAPS
static void
dump_traps (comac_traps_t *traps, const char *filename)
{
    FILE *file;
    comac_box_t extents;
    int n;

    if (getenv ("COMAC_DEBUG_TRAPS") == NULL)
	return;

#if 0
    if (traps->has_limits) {
	printf ("%s: limits=(%d, %d, %d, %d)\n",
		filename,
		traps->limits.p1.x, traps->limits.p1.y,
		traps->limits.p2.x, traps->limits.p2.y);
    }
#endif
    _comac_traps_extents (traps, &extents);
    printf ("%s: extents=(%d, %d, %d, %d)\n",
	    filename,
	    extents.p1.x, extents.p1.y,
	    extents.p2.x, extents.p2.y);

    file = fopen (filename, "a");
    if (file != NULL) {
	for (n = 0; n < traps->num_traps; n++) {
	    fprintf (file, "%d %d L:(%d, %d), (%d, %d) R:(%d, %d), (%d, %d)\n",
		     traps->traps[n].top,
		     traps->traps[n].bottom,
		     traps->traps[n].left.p1.x,
		     traps->traps[n].left.p1.y,
		     traps->traps[n].left.p2.x,
		     traps->traps[n].left.p2.y,
		     traps->traps[n].right.p1.x,
		     traps->traps[n].right.p1.y,
		     traps->traps[n].right.p2.x,
		     traps->traps[n].right.p2.y);
	}
	fprintf (file, "\n");
	fclose (file);
    }
}

static void
dump_edges (comac_bo_start_event_t *events,
	    int num_edges,
	    const char *filename)
{
    FILE *file;
    int n;

    if (getenv ("COMAC_DEBUG_TRAPS") == NULL)
	return;

    file = fopen (filename, "a");
    if (file != NULL) {
	for (n = 0; n < num_edges; n++) {
	    fprintf (file, "(%d, %d), (%d, %d) %d %d %d\n",
		     events[n].edge.edge.line.p1.x,
		     events[n].edge.edge.line.p1.y,
		     events[n].edge.edge.line.p2.x,
		     events[n].edge.edge.line.p2.y,
		     events[n].edge.edge.top,
		     events[n].edge.edge.bottom,
		     events[n].edge.edge.dir);
	}
	fprintf (file, "\n");
	fclose (file);
    }
}
#endif

static comac_fixed_t
_line_compute_intersection_x_for_y (const comac_line_t *line,
				    comac_fixed_t y)
{
    comac_fixed_t x, dy;

    if (y == line->p1.y)
	return line->p1.x;
    if (y == line->p2.y)
	return line->p2.x;

    x = line->p1.x;
    dy = line->p2.y - line->p1.y;
    if (dy != 0) {
	x += _comac_fixed_mul_div_floor (y - line->p1.y,
					 line->p2.x - line->p1.x,
					 dy);
    }

    return x;
}

static inline int
_comac_bo_point32_compare (comac_bo_point32_t const *a,
			   comac_bo_point32_t const *b)
{
    int cmp;

    cmp = a->y - b->y;
    if (cmp)
	return cmp;

    return a->x - b->x;
}

/* Compare the slope of a to the slope of b, returning 1, 0, -1 if the
 * slope a is respectively greater than, equal to, or less than the
 * slope of b.
 *
 * For each edge, consider the direction vector formed from:
 *
 *	top -> bottom
 *
 * which is:
 *
 *	(dx, dy) = (line.p2.x - line.p1.x, line.p2.y - line.p1.y)
 *
 * We then define the slope of each edge as dx/dy, (which is the
 * inverse of the slope typically used in math instruction). We never
 * compute a slope directly as the value approaches infinity, but we
 * can derive a slope comparison without division as follows, (where
 * the ? represents our compare operator).
 *
 * 1.	   slope(a) ? slope(b)
 * 2.	    adx/ady ? bdx/bdy
 * 3.	(adx * bdy) ? (bdx * ady)
 *
 * Note that from step 2 to step 3 there is no change needed in the
 * sign of the result since both ady and bdy are guaranteed to be
 * greater than or equal to 0.
 *
 * When using this slope comparison to sort edges, some care is needed
 * when interpreting the results. Since the slope compare operates on
 * distance vectors from top to bottom it gives a correct left to
 * right sort for edges that have a common top point, (such as two
 * edges with start events at the same location). On the other hand,
 * the sense of the result will be exactly reversed for two edges that
 * have a common stop point.
 */
static inline int
_slope_compare (const comac_bo_edge_t *a,
		const comac_bo_edge_t *b)
{
    /* XXX: We're assuming here that dx and dy will still fit in 32
     * bits. That's not true in general as there could be overflow. We
     * should prevent that before the tessellation algorithm
     * begins.
     */
    int32_t adx = a->edge.line.p2.x - a->edge.line.p1.x;
    int32_t bdx = b->edge.line.p2.x - b->edge.line.p1.x;

    /* Since the dy's are all positive by construction we can fast
     * path several common cases.
     */

    /* First check for vertical lines. */
    if (adx == 0)
	return -bdx;
    if (bdx == 0)
	return adx;

    /* Then where the two edges point in different directions wrt x. */
    if ((adx ^ bdx) < 0)
	return adx;

    /* Finally we actually need to do the general comparison. */
    {
	int32_t ady = a->edge.line.p2.y - a->edge.line.p1.y;
	int32_t bdy = b->edge.line.p2.y - b->edge.line.p1.y;
	comac_int64_t adx_bdy = _comac_int32x32_64_mul (adx, bdy);
	comac_int64_t bdx_ady = _comac_int32x32_64_mul (bdx, ady);

	return _comac_int64_cmp (adx_bdy, bdx_ady);
    }
}


/*
 * We need to compare the x-coordinate of a line for a particular y wrt to a
 * given x, without loss of precision.
 *
 * The x-coordinate along an edge for a given y is:
 *   X = A_x + (Y - A_y) * A_dx / A_dy
 *
 * So the inequality we wish to test is:
 *   A_x + (Y - A_y) * A_dx / A_dy ∘ X
 * where ∘ is our inequality operator.
 *
 * By construction, we know that A_dy (and (Y - A_y)) are
 * all positive, so we can rearrange it thus without causing a sign change:
 *   (Y - A_y) * A_dx ∘ (X - A_x) * A_dy
 *
 * Given the assumption that all the deltas fit within 32 bits, we can compute
 * this comparison directly using 64 bit arithmetic.
 *
 * See the similar discussion for _slope_compare() and
 * edges_compare_x_for_y_general().
 */
static int
edge_compare_for_y_against_x (const comac_bo_edge_t *a,
			      int32_t y,
			      int32_t x)
{
    int32_t adx, ady;
    int32_t dx, dy;
    comac_int64_t L, R;

    if (x < a->edge.line.p1.x && x < a->edge.line.p2.x)
	return 1;
    if (x > a->edge.line.p1.x && x > a->edge.line.p2.x)
	return -1;

    adx = a->edge.line.p2.x - a->edge.line.p1.x;
    dx = x - a->edge.line.p1.x;

    if (adx == 0)
	return -dx;
    if (dx == 0 || (adx ^ dx) < 0)
	return adx;

    dy = y - a->edge.line.p1.y;
    ady = a->edge.line.p2.y - a->edge.line.p1.y;

    L = _comac_int32x32_64_mul (dy, adx);
    R = _comac_int32x32_64_mul (dx, ady);

    return _comac_int64_cmp (L, R);
}

static inline int
_comac_bo_sweep_line_compare_edges (const comac_bo_sweep_line_t	*sweep_line,
				    const comac_bo_edge_t	*a,
				    const comac_bo_edge_t	*b)
{
    int cmp;

    cmp = _comac_lines_compare_at_y (&a->edge.line,
				    &b->edge.line,
				    sweep_line->current_y);
    if (cmp)
	    return cmp;

    /* We've got two collinear edges now. */
    return b->edge.bottom - a->edge.bottom;
}

static inline comac_int64_t
det32_64 (int32_t a, int32_t b,
	  int32_t c, int32_t d)
{
    /* det = a * d - b * c */
    return _comac_int64_sub (_comac_int32x32_64_mul (a, d),
			     _comac_int32x32_64_mul (b, c));
}

static inline comac_int128_t
det64x32_128 (comac_int64_t a, int32_t       b,
	      comac_int64_t c, int32_t       d)
{
    /* det = a * d - b * c */
    return _comac_int128_sub (_comac_int64x32_128_mul (a, d),
			      _comac_int64x32_128_mul (c, b));
}

/* Compute the intersection of two lines as defined by two edges. The
 * result is provided as a coordinate pair of 128-bit integers.
 *
 * Returns %COMAC_BO_STATUS_INTERSECTION if there is an intersection or
 * %COMAC_BO_STATUS_PARALLEL if the two lines are exactly parallel.
 */
static comac_bool_t
intersect_lines (comac_bo_edge_t		*a,
		 comac_bo_edge_t		*b,
		 comac_bo_intersect_point_t	*intersection)
{
    comac_int64_t a_det, b_det;

    /* XXX: We're assuming here that dx and dy will still fit in 32
     * bits. That's not true in general as there could be overflow. We
     * should prevent that before the tessellation algorithm begins.
     * What we're doing to mitigate this is to perform clamping in
     * comac_bo_tessellate_polygon().
     */
    int32_t dx1 = a->edge.line.p1.x - a->edge.line.p2.x;
    int32_t dy1 = a->edge.line.p1.y - a->edge.line.p2.y;

    int32_t dx2 = b->edge.line.p1.x - b->edge.line.p2.x;
    int32_t dy2 = b->edge.line.p1.y - b->edge.line.p2.y;

    comac_int64_t den_det;
    comac_int64_t R;
    comac_quorem64_t qr;

    den_det = det32_64 (dx1, dy1, dx2, dy2);

     /* Q: Can we determine that the lines do not intersect (within range)
      * much more cheaply than computing the intersection point i.e. by
      * avoiding the division?
      *
      *   X = ax + t * adx = bx + s * bdx;
      *   Y = ay + t * ady = by + s * bdy;
      *   ∴ t * (ady*bdx - bdy*adx) = bdx * (by - ay) + bdy * (ax - bx)
      *   => t * L = R
      *
      * Therefore we can reject any intersection (under the criteria for
      * valid intersection events) if:
      *   L^R < 0 => t < 0, or
      *   L<R => t > 1
      *
      * (where top/bottom must at least extend to the line endpoints).
      *
      * A similar substitution can be performed for s, yielding:
      *   s * (ady*bdx - bdy*adx) = ady * (ax - bx) - adx * (ay - by)
      */
    R = det32_64 (dx2, dy2,
		  b->edge.line.p1.x - a->edge.line.p1.x,
		  b->edge.line.p1.y - a->edge.line.p1.y);
    if (_comac_int64_negative (den_det)) {
	if (_comac_int64_ge (den_det, R))
	    return FALSE;
    } else {
	if (_comac_int64_le (den_det, R))
	    return FALSE;
    }

    R = det32_64 (dy1, dx1,
		  a->edge.line.p1.y - b->edge.line.p1.y,
		  a->edge.line.p1.x - b->edge.line.p1.x);
    if (_comac_int64_negative (den_det)) {
	if (_comac_int64_ge (den_det, R))
	    return FALSE;
    } else {
	if (_comac_int64_le (den_det, R))
	    return FALSE;
    }

    /* We now know that the two lines should intersect within range. */

    a_det = det32_64 (a->edge.line.p1.x, a->edge.line.p1.y,
		      a->edge.line.p2.x, a->edge.line.p2.y);
    b_det = det32_64 (b->edge.line.p1.x, b->edge.line.p1.y,
		      b->edge.line.p2.x, b->edge.line.p2.y);

    /* x = det (a_det, dx1, b_det, dx2) / den_det */
    qr = _comac_int_96by64_32x64_divrem (det64x32_128 (a_det, dx1,
						       b_det, dx2),
					 den_det);
    if (_comac_int64_eq (qr.rem, den_det))
	return FALSE;
#if 0
    intersection->x.exactness = _comac_int64_is_zero (qr.rem) ? EXACT : INEXACT;
#else
    intersection->x.exactness = EXACT;
    if (! _comac_int64_is_zero (qr.rem)) {
	if (_comac_int64_negative (den_det) ^ _comac_int64_negative (qr.rem))
	    qr.rem = _comac_int64_negate (qr.rem);
	qr.rem = _comac_int64_mul (qr.rem, _comac_int32_to_int64 (2));
	if (_comac_int64_ge (qr.rem, den_det)) {
	    qr.quo = _comac_int64_add (qr.quo,
				       _comac_int32_to_int64 (_comac_int64_negative (qr.quo) ? -1 : 1));
	} else
	    intersection->x.exactness = INEXACT;
    }
#endif
    intersection->x.ordinate = _comac_int64_to_int32 (qr.quo);

    /* y = det (a_det, dy1, b_det, dy2) / den_det */
    qr = _comac_int_96by64_32x64_divrem (det64x32_128 (a_det, dy1,
						       b_det, dy2),
					 den_det);
    if (_comac_int64_eq (qr.rem, den_det))
	return FALSE;
#if 0
    intersection->y.exactness = _comac_int64_is_zero (qr.rem) ? EXACT : INEXACT;
#else
    intersection->y.exactness = EXACT;
    if (! _comac_int64_is_zero (qr.rem)) {
	if (_comac_int64_negative (den_det) ^ _comac_int64_negative (qr.rem))
	    qr.rem = _comac_int64_negate (qr.rem);
	qr.rem = _comac_int64_mul (qr.rem, _comac_int32_to_int64 (2));
	if (_comac_int64_ge (qr.rem, den_det)) {
	    qr.quo = _comac_int64_add (qr.quo,
				       _comac_int32_to_int64 (_comac_int64_negative (qr.quo) ? -1 : 1));
	} else
	    intersection->y.exactness = INEXACT;
    }
#endif
    intersection->y.ordinate = _comac_int64_to_int32 (qr.quo);

    return TRUE;
}

static int
_comac_bo_intersect_ordinate_32_compare (comac_bo_intersect_ordinate_t	a,
					 int32_t			b)
{
    /* First compare the quotient */
    if (a.ordinate > b)
	return +1;
    if (a.ordinate < b)
	return -1;
    /* With quotient identical, if remainder is 0 then compare equal */
    /* Otherwise, the non-zero remainder makes a > b */
    return INEXACT == a.exactness;
}

/* Does the given edge contain the given point. The point must already
 * be known to be contained within the line determined by the edge,
 * (most likely the point results from an intersection of this edge
 * with another).
 *
 * If we had exact arithmetic, then this function would simply be a
 * matter of examining whether the y value of the point lies within
 * the range of y values of the edge. But since intersection points
 * are not exact due to being rounded to the nearest integer within
 * the available precision, we must also examine the x value of the
 * point.
 *
 * The definition of "contains" here is that the given intersection
 * point will be seen by the sweep line after the start event for the
 * given edge and before the stop event for the edge. See the comments
 * in the implementation for more details.
 */
static comac_bool_t
_comac_bo_edge_contains_intersect_point (comac_bo_edge_t		*edge,
					 comac_bo_intersect_point_t	*point)
{
    int cmp_top, cmp_bottom;

    /* XXX: When running the actual algorithm, we don't actually need to
     * compare against edge->top at all here, since any intersection above
     * top is eliminated early via a slope comparison. We're leaving these
     * here for now only for the sake of the quadratic-time intersection
     * finder which needs them.
     */

    cmp_top = _comac_bo_intersect_ordinate_32_compare (point->y,
						       edge->edge.top);
    cmp_bottom = _comac_bo_intersect_ordinate_32_compare (point->y,
							  edge->edge.bottom);

    if (cmp_top < 0 || cmp_bottom > 0)
    {
	return FALSE;
    }

    if (cmp_top > 0 && cmp_bottom < 0)
    {
	return TRUE;
    }

    /* At this stage, the point lies on the same y value as either
     * edge->top or edge->bottom, so we have to examine the x value in
     * order to properly determine containment. */

    /* If the y value of the point is the same as the y value of the
     * top of the edge, then the x value of the point must be greater
     * to be considered as inside the edge. Similarly, if the y value
     * of the point is the same as the y value of the bottom of the
     * edge, then the x value of the point must be less to be
     * considered as inside. */

    if (cmp_top == 0) {
	comac_fixed_t top_x;

	top_x = _line_compute_intersection_x_for_y (&edge->edge.line,
						    edge->edge.top);
	return _comac_bo_intersect_ordinate_32_compare (point->x, top_x) > 0;
    } else { /* cmp_bottom == 0 */
	comac_fixed_t bot_x;

	bot_x = _line_compute_intersection_x_for_y (&edge->edge.line,
						    edge->edge.bottom);
	return _comac_bo_intersect_ordinate_32_compare (point->x, bot_x) < 0;
    }
}

/* Compute the intersection of two edges. The result is provided as a
 * coordinate pair of 128-bit integers.
 *
 * Returns %COMAC_BO_STATUS_INTERSECTION if there is an intersection
 * that is within both edges, %COMAC_BO_STATUS_NO_INTERSECTION if the
 * intersection of the lines defined by the edges occurs outside of
 * one or both edges, and %COMAC_BO_STATUS_PARALLEL if the two edges
 * are exactly parallel.
 *
 * Note that when determining if a candidate intersection is "inside"
 * an edge, we consider both the infinitesimal shortening and the
 * infinitesimal tilt rules described by John Hobby. Specifically, if
 * the intersection is exactly the same as an edge point, it is
 * effectively outside (no intersection is returned). Also, if the
 * intersection point has the same
 */
static comac_bool_t
_comac_bo_edge_intersect (comac_bo_edge_t	*a,
			  comac_bo_edge_t	*b,
			  comac_bo_point32_t	*intersection)
{
    comac_bo_intersect_point_t quorem;

    if (! intersect_lines (a, b, &quorem))
	return FALSE;

    if (! _comac_bo_edge_contains_intersect_point (a, &quorem))
	return FALSE;

    if (! _comac_bo_edge_contains_intersect_point (b, &quorem))
	return FALSE;

    /* Now that we've correctly compared the intersection point and
     * determined that it lies within the edge, then we know that we
     * no longer need any more bits of storage for the intersection
     * than we do for our edge coordinates. We also no longer need the
     * remainder from the division. */
    intersection->x = quorem.x.ordinate;
    intersection->y = quorem.y.ordinate;

    return TRUE;
}

static inline int
comac_bo_event_compare (const comac_bo_event_t *a,
			const comac_bo_event_t *b)
{
    int cmp;

    cmp = _comac_bo_point32_compare (&a->point, &b->point);
    if (cmp)
	return cmp;

    cmp = a->type - b->type;
    if (cmp)
	return cmp;

    return a - b;
}

static inline void
_pqueue_init (pqueue_t *pq)
{
    pq->max_size = ARRAY_LENGTH (pq->elements_embedded);
    pq->size = 0;

    pq->elements = pq->elements_embedded;
}

static inline void
_pqueue_fini (pqueue_t *pq)
{
    if (pq->elements != pq->elements_embedded)
	free (pq->elements);
}

static comac_status_t
_pqueue_grow (pqueue_t *pq)
{
    comac_bo_event_t **new_elements;
    pq->max_size *= 2;

    if (pq->elements == pq->elements_embedded) {
	new_elements = _comac_malloc_ab (pq->max_size,
					 sizeof (comac_bo_event_t *));
	if (unlikely (new_elements == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	memcpy (new_elements, pq->elements_embedded,
		sizeof (pq->elements_embedded));
    } else {
	new_elements = _comac_realloc_ab (pq->elements,
					  pq->max_size,
					  sizeof (comac_bo_event_t *));
	if (unlikely (new_elements == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);
    }

    pq->elements = new_elements;
    return COMAC_STATUS_SUCCESS;
}

static inline comac_status_t
_pqueue_push (pqueue_t *pq, comac_bo_event_t *event)
{
    comac_bo_event_t **elements;
    int i, parent;

    if (unlikely (pq->size + 1 == pq->max_size)) {
	comac_status_t status;

	status = _pqueue_grow (pq);
	if (unlikely (status))
	    return status;
    }

    elements = pq->elements;

    for (i = ++pq->size;
	 i != PQ_FIRST_ENTRY &&
	 comac_bo_event_compare (event,
				 elements[parent = PQ_PARENT_INDEX (i)]) < 0;
	 i = parent)
    {
	elements[i] = elements[parent];
    }

    elements[i] = event;

    return COMAC_STATUS_SUCCESS;
}

static inline void
_pqueue_pop (pqueue_t *pq)
{
    comac_bo_event_t **elements = pq->elements;
    comac_bo_event_t *tail;
    int child, i;

    tail = elements[pq->size--];
    if (pq->size == 0) {
	elements[PQ_FIRST_ENTRY] = NULL;
	return;
    }

    for (i = PQ_FIRST_ENTRY;
	 (child = PQ_LEFT_CHILD_INDEX (i)) <= pq->size;
	 i = child)
    {
	if (child != pq->size &&
	    comac_bo_event_compare (elements[child+1],
				    elements[child]) < 0)
	{
	    child++;
	}

	if (comac_bo_event_compare (elements[child], tail) >= 0)
	    break;

	elements[i] = elements[child];
    }
    elements[i] = tail;
}

static inline comac_status_t
_comac_bo_event_queue_insert (comac_bo_event_queue_t	*queue,
			      comac_bo_event_type_t	 type,
			      comac_bo_edge_t		*e1,
			      comac_bo_edge_t		*e2,
			      const comac_point_t	 *point)
{
    comac_bo_queue_event_t *event;

    event = _comac_freepool_alloc (&queue->pool);
    if (unlikely (event == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    event->type = type;
    event->e1 = e1;
    event->e2 = e2;
    event->point = *point;

    return _pqueue_push (&queue->pqueue, (comac_bo_event_t *) event);
}

static void
_comac_bo_event_queue_delete (comac_bo_event_queue_t *queue,
			      comac_bo_event_t	     *event)
{
    _comac_freepool_free (&queue->pool, event);
}

static comac_bo_event_t *
_comac_bo_event_dequeue (comac_bo_event_queue_t *event_queue)
{
    comac_bo_event_t *event, *cmp;

    event = event_queue->pqueue.elements[PQ_FIRST_ENTRY];
    cmp = *event_queue->start_events;
    if (event == NULL ||
	(cmp != NULL && comac_bo_event_compare (cmp, event) < 0))
    {
	event = cmp;
	event_queue->start_events++;
    }
    else
    {
	_pqueue_pop (&event_queue->pqueue);
    }

    return event;
}

COMAC_COMBSORT_DECLARE (_comac_bo_event_queue_sort,
			comac_bo_event_t *,
			comac_bo_event_compare)

static void
_comac_bo_event_queue_init (comac_bo_event_queue_t	 *event_queue,
			    comac_bo_event_t		**start_events,
			    int				  num_events)
{
    event_queue->start_events = start_events;

    _comac_freepool_init (&event_queue->pool,
			  sizeof (comac_bo_queue_event_t));
    _pqueue_init (&event_queue->pqueue);
    event_queue->pqueue.elements[PQ_FIRST_ENTRY] = NULL;
}

static comac_status_t
_comac_bo_event_queue_insert_stop (comac_bo_event_queue_t	*event_queue,
				   comac_bo_edge_t		*edge)
{
    comac_bo_point32_t point;

    point.y = edge->edge.bottom;
    point.x = _line_compute_intersection_x_for_y (&edge->edge.line,
						  point.y);
    return _comac_bo_event_queue_insert (event_queue,
					 COMAC_BO_EVENT_TYPE_STOP,
					 edge, NULL,
					 &point);
}

static void
_comac_bo_event_queue_fini (comac_bo_event_queue_t *event_queue)
{
    _pqueue_fini (&event_queue->pqueue);
    _comac_freepool_fini (&event_queue->pool);
}

static inline comac_status_t
_comac_bo_event_queue_insert_if_intersect_below_current_y (comac_bo_event_queue_t	*event_queue,
							   comac_bo_edge_t	*left,
							   comac_bo_edge_t *right)
{
    comac_bo_point32_t intersection;

    if (MAX (left->edge.line.p1.x, left->edge.line.p2.x) <=
	MIN (right->edge.line.p1.x, right->edge.line.p2.x))
	return COMAC_STATUS_SUCCESS;

    if (comac_lines_equal (&left->edge.line, &right->edge.line))
	return COMAC_STATUS_SUCCESS;

    /* The names "left" and "right" here are correct descriptions of
     * the order of the two edges within the active edge list. So if a
     * slope comparison also puts left less than right, then we know
     * that the intersection of these two segments has already
     * occurred before the current sweep line position. */
    if (_slope_compare (left, right) <= 0)
	return COMAC_STATUS_SUCCESS;

    if (! _comac_bo_edge_intersect (left, right, &intersection))
	return COMAC_STATUS_SUCCESS;

    return _comac_bo_event_queue_insert (event_queue,
					 COMAC_BO_EVENT_TYPE_INTERSECTION,
					 left, right,
					 &intersection);
}

static void
_comac_bo_sweep_line_init (comac_bo_sweep_line_t *sweep_line)
{
    sweep_line->head = NULL;
    sweep_line->stopped = NULL;
    sweep_line->current_y = INT32_MIN;
    sweep_line->current_edge = NULL;
}

static void
_comac_bo_sweep_line_insert (comac_bo_sweep_line_t	*sweep_line,
			     comac_bo_edge_t		*edge)
{
    if (sweep_line->current_edge != NULL) {
	comac_bo_edge_t *prev, *next;
	int cmp;

	cmp = _comac_bo_sweep_line_compare_edges (sweep_line,
						  sweep_line->current_edge,
						  edge);
	if (cmp < 0) {
	    prev = sweep_line->current_edge;
	    next = prev->next;
	    while (next != NULL &&
		   _comac_bo_sweep_line_compare_edges (sweep_line,
						       next, edge) < 0)
	    {
		prev = next, next = prev->next;
	    }

	    prev->next = edge;
	    edge->prev = prev;
	    edge->next = next;
	    if (next != NULL)
		next->prev = edge;
	} else if (cmp > 0) {
	    next = sweep_line->current_edge;
	    prev = next->prev;
	    while (prev != NULL &&
		   _comac_bo_sweep_line_compare_edges (sweep_line,
						       prev, edge) > 0)
	    {
		next = prev, prev = next->prev;
	    }

	    next->prev = edge;
	    edge->next = next;
	    edge->prev = prev;
	    if (prev != NULL)
		prev->next = edge;
	    else
		sweep_line->head = edge;
	} else {
	    prev = sweep_line->current_edge;
	    edge->prev = prev;
	    edge->next = prev->next;
	    if (prev->next != NULL)
		prev->next->prev = edge;
	    prev->next = edge;
	}
    } else {
	sweep_line->head = edge;
	edge->next = NULL;
    }

    sweep_line->current_edge = edge;
}

static void
_comac_bo_sweep_line_delete (comac_bo_sweep_line_t	*sweep_line,
			     comac_bo_edge_t	*edge)
{
    if (edge->prev != NULL)
	edge->prev->next = edge->next;
    else
	sweep_line->head = edge->next;

    if (edge->next != NULL)
	edge->next->prev = edge->prev;

    if (sweep_line->current_edge == edge)
	sweep_line->current_edge = edge->prev ? edge->prev : edge->next;
}

static void
_comac_bo_sweep_line_swap (comac_bo_sweep_line_t	*sweep_line,
			   comac_bo_edge_t		*left,
			   comac_bo_edge_t		*right)
{
    if (left->prev != NULL)
	left->prev->next = right;
    else
	sweep_line->head = right;

    if (right->next != NULL)
	right->next->prev = left;

    left->next = right->next;
    right->next = left;

    right->prev = left->prev;
    left->prev = right;
}

#if DEBUG_PRINT_STATE
static void
_comac_bo_edge_print (comac_bo_edge_t *edge)
{
    printf ("(0x%x, 0x%x)-(0x%x, 0x%x)",
	    edge->edge.line.p1.x, edge->edge.line.p1.y,
	    edge->edge.line.p2.x, edge->edge.line.p2.y);
}

static void
_comac_bo_event_print (comac_bo_event_t *event)
{
    switch (event->type) {
    case COMAC_BO_EVENT_TYPE_START:
	printf ("Start: ");
	break;
    case COMAC_BO_EVENT_TYPE_STOP:
	printf ("Stop: ");
	break;
    case COMAC_BO_EVENT_TYPE_INTERSECTION:
	printf ("Intersection: ");
	break;
    }
    printf ("(%d, %d)\t", event->point.x, event->point.y);
    _comac_bo_edge_print (event->e1);
    if (event->type == COMAC_BO_EVENT_TYPE_INTERSECTION) {
	printf (" X ");
	_comac_bo_edge_print (event->e2);
    }
    printf ("\n");
}

static void
_comac_bo_event_queue_print (comac_bo_event_queue_t *event_queue)
{
    /* XXX: fixme to print the start/stop array too. */
    printf ("Event queue:\n");
}

static void
_comac_bo_sweep_line_print (comac_bo_sweep_line_t *sweep_line)
{
    comac_bool_t first = TRUE;
    comac_bo_edge_t *edge;

    printf ("Sweep line from edge list: ");
    first = TRUE;
    for (edge = sweep_line->head;
	 edge;
	 edge = edge->next)
    {
	if (!first)
	    printf (", ");
	_comac_bo_edge_print (edge);
	first = FALSE;
    }
    printf ("\n");
}

static void
print_state (const char			*msg,
	     comac_bo_event_t		*event,
	     comac_bo_event_queue_t	*event_queue,
	     comac_bo_sweep_line_t	*sweep_line)
{
    printf ("%s ", msg);
    _comac_bo_event_print (event);
    _comac_bo_event_queue_print (event_queue);
    _comac_bo_sweep_line_print (sweep_line);
    printf ("\n");
}
#endif

#if DEBUG_EVENTS
static void COMAC_PRINTF_FORMAT (1, 2)
event_log (const char *fmt, ...)
{
    FILE *file;

    if (getenv ("COMAC_DEBUG_EVENTS") == NULL)
	return;

    file = fopen ("bo-events.txt", "a");
    if (file != NULL) {
	va_list ap;

	va_start (ap, fmt);
	vfprintf (file, fmt, ap);
	va_end (ap);

	fclose (file);
    }
}
#endif

#define HAS_COLINEAR(a, b) ((comac_bo_edge_t *)(((uintptr_t)(a))&~1) == (b))
#define IS_COLINEAR(e) (((uintptr_t)(e))&1)
#define MARK_COLINEAR(e, v) ((comac_bo_edge_t *)(((uintptr_t)(e))|(v)))

static inline comac_bool_t
edges_colinear (comac_bo_edge_t *a, const comac_bo_edge_t *b)
{
    unsigned p;

    if (HAS_COLINEAR(a->colinear, b))
	return IS_COLINEAR(a->colinear);

    if (HAS_COLINEAR(b->colinear, a)) {
	p = IS_COLINEAR(b->colinear);
	a->colinear = MARK_COLINEAR(b, p);
	return p;
    }

    p = 0;
    p |= (a->edge.line.p1.x == b->edge.line.p1.x) << 0;
    p |= (a->edge.line.p1.y == b->edge.line.p1.y) << 1;
    p |= (a->edge.line.p2.x == b->edge.line.p2.x) << 3;
    p |= (a->edge.line.p2.y == b->edge.line.p2.y) << 4;
    if (p == ((1 << 0) | (1 << 1) | (1 << 3) | (1 << 4))) {
	a->colinear = MARK_COLINEAR(b, 1);
	return TRUE;
    }

    if (_slope_compare (a, b)) {
	a->colinear = MARK_COLINEAR(b, 0);
	return FALSE;
    }

    /* The choice of y is not truly arbitrary since we must guarantee that it
     * is greater than the start of either line.
     */
    if (p != 0) {
	/* colinear if either end-point are coincident */
	p = (((p >> 1) & p) & 5) != 0;
    } else if (a->edge.line.p1.y < b->edge.line.p1.y) {
	p = edge_compare_for_y_against_x (b,
					  a->edge.line.p1.y,
					  a->edge.line.p1.x) == 0;
    } else {
	p = edge_compare_for_y_against_x (a,
					  b->edge.line.p1.y,
					  b->edge.line.p1.x) == 0;
    }

    a->colinear = MARK_COLINEAR(b, p);
    return p;
}

/* Adds the trapezoid, if any, of the left edge to the #comac_traps_t */
static void
_comac_bo_edge_end_trap (comac_bo_edge_t	*left,
			 int32_t		 bot,
			 comac_traps_t	        *traps)
{
    comac_bo_trap_t *trap = &left->deferred_trap;

    /* Only emit (trivial) non-degenerate trapezoids with positive height. */
    if (likely (trap->top < bot)) {
	_comac_traps_add_trap (traps,
			       trap->top, bot,
			       &left->edge.line, &trap->right->edge.line);

#if DEBUG_PRINT_STATE
	printf ("Deferred trap: left=(%x, %x)-(%x,%x) "
		"right=(%x,%x)-(%x,%x) top=%x, bot=%x\n",
		left->edge.line.p1.x, left->edge.line.p1.y,
		left->edge.line.p2.x, left->edge.line.p2.y,
		trap->right->edge.line.p1.x, trap->right->edge.line.p1.y,
		trap->right->edge.line.p2.x, trap->right->edge.line.p2.y,
		trap->top, bot);
#endif
#if DEBUG_EVENTS
	event_log ("end trap: %lu %lu %d %d\n",
		   (long) left,
		   (long) trap->right,
		   trap->top,
		   bot);
#endif
    }

    trap->right = NULL;
}


/* Start a new trapezoid at the given top y coordinate, whose edges
 * are `edge' and `edge->next'. If `edge' already has a trapezoid,
 * then either add it to the traps in `traps', if the trapezoid's
 * right edge differs from `edge->next', or do nothing if the new
 * trapezoid would be a continuation of the existing one. */
static inline void
_comac_bo_edge_start_or_continue_trap (comac_bo_edge_t	*left,
				       comac_bo_edge_t  *right,
				       int               top,
				       comac_traps_t	*traps)
{
    if (left->deferred_trap.right == right)
	return;

    assert (right);
    if (left->deferred_trap.right != NULL) {
	if (edges_colinear (left->deferred_trap.right, right))
	{
	    /* continuation on right, so just swap edges */
	    left->deferred_trap.right = right;
	    return;
	}

	_comac_bo_edge_end_trap (left, top, traps);
    }

    if (! edges_colinear (left, right)) {
	left->deferred_trap.top = top;
	left->deferred_trap.right = right;

#if DEBUG_EVENTS
	event_log ("begin trap: %lu %lu %d\n",
		   (long) left,
		   (long) right,
		   top);
#endif
    }
}

static inline void
_active_edges_to_traps (comac_bo_edge_t	*pos,
			int32_t		 top,
			unsigned	 mask,
			comac_traps_t        *traps)
{
    comac_bo_edge_t *left;
    int in_out;


#if DEBUG_PRINT_STATE
    printf ("Processing active edges for %x\n", top);
#endif

    in_out = 0;
    left = pos;
    while (pos != NULL) {
	if (pos != left && pos->deferred_trap.right) {
	    /* XXX It shouldn't be possible to here with 2 deferred traps
	     * on colinear edges... See bug-bo-rictoz.
	     */
	    if (left->deferred_trap.right == NULL &&
		edges_colinear (left, pos))
	    {
		/* continuation on left */
		left->deferred_trap = pos->deferred_trap;
		pos->deferred_trap.right = NULL;
	    }
	    else
	    {
		_comac_bo_edge_end_trap (pos, top, traps);
	    }
	}

	in_out += pos->edge.dir;
	if ((in_out & mask) == 0) {
	    /* skip co-linear edges */
	    if (pos->next == NULL || ! edges_colinear (pos, pos->next)) {
		_comac_bo_edge_start_or_continue_trap (left, pos, top, traps);
		left = pos->next;
	    }
	}

	pos = pos->next;
    }
}

/* Execute a single pass of the Bentley-Ottmann algorithm on edges,
 * generating trapezoids according to the fill_rule and appending them
 * to traps. */
static comac_status_t
_comac_bentley_ottmann_tessellate_bo_edges (comac_bo_event_t   **start_events,
					    int			 num_events,
					    unsigned		 fill_rule,
					    comac_traps_t	*traps,
					    int			*num_intersections)
{
    comac_status_t status;
    int intersection_count = 0;
    comac_bo_event_queue_t event_queue;
    comac_bo_sweep_line_t sweep_line;
    comac_bo_event_t *event;
    comac_bo_edge_t *left, *right;
    comac_bo_edge_t *e1, *e2;

    /* convert the fill_rule into a winding mask */
    if (fill_rule == COMAC_FILL_RULE_WINDING)
	fill_rule = (unsigned) -1;
    else
	fill_rule = 1;

#if DEBUG_EVENTS
    {
	int i;

	for (i = 0; i < num_events; i++) {
	    comac_bo_start_event_t *event =
		((comac_bo_start_event_t **) start_events)[i];
	    event_log ("edge: %lu (%d, %d) (%d, %d) (%d, %d) %d\n",
		       (long) &events[i].edge,
		       event->edge.edge.line.p1.x,
		       event->edge.edge.line.p1.y,
		       event->edge.edge.line.p2.x,
		       event->edge.edge.line.p2.y,
		       event->edge.top,
		       event->edge.bottom,
		       event->edge.edge.dir);
	}
    }
#endif

    _comac_bo_event_queue_init (&event_queue, start_events, num_events);
    _comac_bo_sweep_line_init (&sweep_line);

    while ((event = _comac_bo_event_dequeue (&event_queue))) {
	if (event->point.y != sweep_line.current_y) {
	    for (e1 = sweep_line.stopped; e1; e1 = e1->next) {
		if (e1->deferred_trap.right != NULL) {
		    _comac_bo_edge_end_trap (e1,
					     e1->edge.bottom,
					     traps);
		}
	    }
	    sweep_line.stopped = NULL;

	    _active_edges_to_traps (sweep_line.head,
				    sweep_line.current_y,
				    fill_rule, traps);

	    sweep_line.current_y = event->point.y;
	}

#if DEBUG_EVENTS
	event_log ("event: %d (%ld, %ld) %lu, %lu\n",
		   event->type,
		   (long) event->point.x,
		   (long) event->point.y,
		   (long) event->e1,
		   (long) event->e2);
#endif

	switch (event->type) {
	case COMAC_BO_EVENT_TYPE_START:
	    e1 = &((comac_bo_start_event_t *) event)->edge;

	    _comac_bo_sweep_line_insert (&sweep_line, e1);

	    status = _comac_bo_event_queue_insert_stop (&event_queue, e1);
	    if (unlikely (status))
		goto unwind;

	    /* check to see if this is a continuation of a stopped edge */
	    /* XXX change to an infinitesimal lengthening rule */
	    for (left = sweep_line.stopped; left; left = left->next) {
		if (e1->edge.top <= left->edge.bottom &&
		    edges_colinear (e1, left))
		{
		    e1->deferred_trap = left->deferred_trap;
		    if (left->prev != NULL)
			left->prev = left->next;
		    else
			sweep_line.stopped = left->next;
		    if (left->next != NULL)
			left->next->prev = left->prev;
		    break;
		}
	    }

	    left = e1->prev;
	    right = e1->next;

	    if (left != NULL) {
		status = _comac_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, e1);
		if (unlikely (status))
		    goto unwind;
	    }

	    if (right != NULL) {
		status = _comac_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, e1, right);
		if (unlikely (status))
		    goto unwind;
	    }

	    break;

	case COMAC_BO_EVENT_TYPE_STOP:
	    e1 = ((comac_bo_queue_event_t *) event)->e1;
	    _comac_bo_event_queue_delete (&event_queue, event);

	    left = e1->prev;
	    right = e1->next;

	    _comac_bo_sweep_line_delete (&sweep_line, e1);

	    /* first, check to see if we have a continuation via a fresh edge */
	    if (e1->deferred_trap.right != NULL) {
		e1->next = sweep_line.stopped;
		if (sweep_line.stopped != NULL)
		    sweep_line.stopped->prev = e1;
		sweep_line.stopped = e1;
		e1->prev = NULL;
	    }

	    if (left != NULL && right != NULL) {
		status = _comac_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, right);
		if (unlikely (status))
		    goto unwind;
	    }

	    break;

	case COMAC_BO_EVENT_TYPE_INTERSECTION:
	    e1 = ((comac_bo_queue_event_t *) event)->e1;
	    e2 = ((comac_bo_queue_event_t *) event)->e2;
	    _comac_bo_event_queue_delete (&event_queue, event);

	    /* skip this intersection if its edges are not adjacent */
	    if (e2 != e1->next)
		break;

	    intersection_count++;

	    left = e1->prev;
	    right = e2->next;

	    _comac_bo_sweep_line_swap (&sweep_line, e1, e2);

	    /* after the swap e2 is left of e1 */

	    if (left != NULL) {
		status = _comac_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, left, e2);
		if (unlikely (status))
		    goto unwind;
	    }

	    if (right != NULL) {
		status = _comac_bo_event_queue_insert_if_intersect_below_current_y (&event_queue, e1, right);
		if (unlikely (status))
		    goto unwind;
	    }

	    break;
	}
    }

    *num_intersections = intersection_count;
    for (e1 = sweep_line.stopped; e1; e1 = e1->next) {
	if (e1->deferred_trap.right != NULL) {
	    _comac_bo_edge_end_trap (e1, e1->edge.bottom, traps);
	}
    }
    status = traps->status;
 unwind:
    _comac_bo_event_queue_fini (&event_queue);

#if DEBUG_EVENTS
    event_log ("\n");
#endif

    return status;
}

comac_status_t
_comac_bentley_ottmann_tessellate_polygon (comac_traps_t	 *traps,
					   const comac_polygon_t *polygon,
					   comac_fill_rule_t	  fill_rule)
{
    int intersections;
    comac_bo_start_event_t stack_events[COMAC_STACK_ARRAY_LENGTH (comac_bo_start_event_t)];
    comac_bo_start_event_t *events;
    comac_bo_event_t *stack_event_ptrs[ARRAY_LENGTH (stack_events) + 1];
    comac_bo_event_t **event_ptrs;
    comac_bo_start_event_t *stack_event_y[64];
    comac_bo_start_event_t **event_y = NULL;
    int i, num_events, y, ymin, ymax;
    comac_status_t status;

    num_events = polygon->num_edges;
    if (unlikely (0 == num_events))
	return COMAC_STATUS_SUCCESS;

    if (polygon->num_limits) {
	ymin = _comac_fixed_integer_floor (polygon->limit.p1.y);
	ymax = _comac_fixed_integer_ceil (polygon->limit.p2.y) - ymin;

	if (ymax > 64) {
	    event_y = _comac_malloc_ab(sizeof (comac_bo_event_t*), ymax);
	    if (unlikely (event_y == NULL))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	} else {
	    event_y = stack_event_y;
	}
	memset (event_y, 0, ymax * sizeof(comac_bo_event_t *));
    }

    events = stack_events;
    event_ptrs = stack_event_ptrs;
    if (num_events > ARRAY_LENGTH (stack_events)) {
	events = _comac_malloc_ab_plus_c (num_events,
					  sizeof (comac_bo_start_event_t) +
					  sizeof (comac_bo_event_t *),
					  sizeof (comac_bo_event_t *));
	if (unlikely (events == NULL)) {
	    if (event_y != stack_event_y)
		free (event_y);
	    return _comac_error (COMAC_STATUS_NO_MEMORY);
	}

	event_ptrs = (comac_bo_event_t **) (events + num_events);
    }

    for (i = 0; i < num_events; i++) {
	events[i].type = COMAC_BO_EVENT_TYPE_START;
	events[i].point.y = polygon->edges[i].top;
	events[i].point.x =
	    _line_compute_intersection_x_for_y (&polygon->edges[i].line,
						events[i].point.y);

	events[i].edge.edge = polygon->edges[i];
	events[i].edge.deferred_trap.right = NULL;
	events[i].edge.prev = NULL;
	events[i].edge.next = NULL;
	events[i].edge.colinear = NULL;

	if (event_y) {
	    y = _comac_fixed_integer_floor (events[i].point.y) - ymin;
	    events[i].edge.next = (comac_bo_edge_t *) event_y[y];
	    event_y[y] = (comac_bo_start_event_t *) &events[i];
	} else
	    event_ptrs[i] = (comac_bo_event_t *) &events[i];
    }

    if (event_y) {
	for (y = i = 0; y < ymax && i < num_events; y++) {
	    comac_bo_start_event_t *e;
	    int j = i;
	    for (e = event_y[y]; e; e = (comac_bo_start_event_t *)e->edge.next)
		event_ptrs[i++] = (comac_bo_event_t *) e;
	    if (i > j + 1)
		_comac_bo_event_queue_sort (event_ptrs+j, i-j);
	}
	if (event_y != stack_event_y)
	    free (event_y);
    } else
	_comac_bo_event_queue_sort (event_ptrs, i);
    event_ptrs[i] = NULL;

#if DEBUG_TRAPS
    dump_edges (events, num_events, "bo-polygon-edges.txt");
#endif

    /* XXX: This would be the convenient place to throw in multiple
     * passes of the Bentley-Ottmann algorithm. It would merely
     * require storing the results of each pass into a temporary
     * comac_traps_t. */
    status = _comac_bentley_ottmann_tessellate_bo_edges (event_ptrs, num_events,
							 fill_rule, traps,
							 &intersections);
#if DEBUG_TRAPS
    dump_traps (traps, "bo-polygon-out.txt");
#endif

    if (events != stack_events)
	free (events);

    return status;
}

comac_status_t
_comac_bentley_ottmann_tessellate_traps (comac_traps_t *traps,
					 comac_fill_rule_t fill_rule)
{
    comac_status_t status;
    comac_polygon_t polygon;
    int i;

    if (unlikely (0 == traps->num_traps))
	return COMAC_STATUS_SUCCESS;

#if DEBUG_TRAPS
    dump_traps (traps, "bo-traps-in.txt");
#endif

    _comac_polygon_init (&polygon, traps->limits, traps->num_limits);

    for (i = 0; i < traps->num_traps; i++) {
	status = _comac_polygon_add_line (&polygon,
					  &traps->traps[i].left,
					  traps->traps[i].top,
					  traps->traps[i].bottom,
					  1);
	if (unlikely (status))
	    goto CLEANUP;

	status = _comac_polygon_add_line (&polygon,
					  &traps->traps[i].right,
					  traps->traps[i].top,
					  traps->traps[i].bottom,
					  -1);
	if (unlikely (status))
	    goto CLEANUP;
    }

    _comac_traps_clear (traps);
    status = _comac_bentley_ottmann_tessellate_polygon (traps,
							&polygon,
							fill_rule);

#if DEBUG_TRAPS
    dump_traps (traps, "bo-traps-out.txt");
#endif

  CLEANUP:
    _comac_polygon_fini (&polygon);

    return status;
}

#if 0
static comac_bool_t
edges_have_an_intersection_quadratic (comac_bo_edge_t	*edges,
				      int		 num_edges)

{
    int i, j;
    comac_bo_edge_t *a, *b;
    comac_bo_point32_t intersection;

    /* We must not be given any upside-down edges. */
    for (i = 0; i < num_edges; i++) {
	assert (_comac_bo_point32_compare (&edges[i].top, &edges[i].bottom) < 0);
	edges[i].line.p1.x <<= COMAC_BO_GUARD_BITS;
	edges[i].line.p1.y <<= COMAC_BO_GUARD_BITS;
	edges[i].line.p2.x <<= COMAC_BO_GUARD_BITS;
	edges[i].line.p2.y <<= COMAC_BO_GUARD_BITS;
    }

    for (i = 0; i < num_edges; i++) {
	for (j = 0; j < num_edges; j++) {
	    if (i == j)
		continue;

	    a = &edges[i];
	    b = &edges[j];

	    if (! _comac_bo_edge_intersect (a, b, &intersection))
		continue;

	    printf ("Found intersection (%d,%d) between (%d,%d)-(%d,%d) and (%d,%d)-(%d,%d)\n",
		    intersection.x,
		    intersection.y,
		    a->line.p1.x, a->line.p1.y,
		    a->line.p2.x, a->line.p2.y,
		    b->line.p1.x, b->line.p1.y,
		    b->line.p2.x, b->line.p2.y);

	    return TRUE;
	}
    }
    return FALSE;
}

#define TEST_MAX_EDGES 10

typedef struct test {
    const char *name;
    const char *description;
    int num_edges;
    comac_bo_edge_t edges[TEST_MAX_EDGES];
} test_t;

static test_t
tests[] = {
    {
	"3 near misses",
	"3 edges all intersecting very close to each other",
	3,
	{
	    { { 4, 2}, {0, 0}, { 9, 9}, NULL, NULL },
	    { { 7, 2}, {0, 0}, { 2, 3}, NULL, NULL },
	    { { 5, 2}, {0, 0}, { 1, 7}, NULL, NULL }
	}
    },
    {
	"inconsistent data",
	"Derived from random testing---was leading to skip list and edge list disagreeing.",
	2,
	{
	    { { 2, 3}, {0, 0}, { 8, 9}, NULL, NULL },
	    { { 2, 3}, {0, 0}, { 6, 7}, NULL, NULL }
	}
    },
    {
	"failed sort",
	"A test derived from random testing that leads to an inconsistent sort --- looks like we just can't attempt to validate the sweep line with edge_compare?",
	3,
	{
	    { { 6, 2}, {0, 0}, { 6, 5}, NULL, NULL },
	    { { 3, 5}, {0, 0}, { 5, 6}, NULL, NULL },
	    { { 9, 2}, {0, 0}, { 5, 6}, NULL, NULL },
	}
    },
    {
	"minimal-intersection",
	"Intersection of a two from among the smallest possible edges.",
	2,
	{
	    { { 0, 0}, {0, 0}, { 1, 1}, NULL, NULL },
	    { { 1, 0}, {0, 0}, { 0, 1}, NULL, NULL }
	}
    },
    {
	"simple",
	"A simple intersection of two edges at an integer (2,2).",
	2,
	{
	    { { 1, 1}, {0, 0}, { 3, 3}, NULL, NULL },
	    { { 2, 1}, {0, 0}, { 2, 3}, NULL, NULL }
	}
    },
    {
	"bend-to-horizontal",
	"With intersection truncation one edge bends to horizontal",
	2,
	{
	    { { 9, 1}, {0, 0}, {3, 7}, NULL, NULL },
	    { { 3, 5}, {0, 0}, {9, 9}, NULL, NULL }
	}
    }
};

/*
    {
	"endpoint",
	"An intersection that occurs at the endpoint of a segment.",
	{
	    { { 4, 6}, { 5, 6}, NULL, { { NULL }} },
	    { { 4, 5}, { 5, 7}, NULL, { { NULL }} },
	    { { 0, 0}, { 0, 0}, NULL, { { NULL }} },
	}
    }
    {
	name = "overlapping",
	desc = "Parallel segments that share an endpoint, with different slopes.",
	edges = {
	    { top = { x = 2, y = 0}, bottom = { x = 1, y = 1}},
	    { top = { x = 2, y = 0}, bottom = { x = 0, y = 2}},
	    { top = { x = 0, y = 3}, bottom = { x = 1, y = 3}},
	    { top = { x = 0, y = 3}, bottom = { x = 2, y = 3}},
	    { top = { x = 0, y = 4}, bottom = { x = 0, y = 6}},
	    { top = { x = 0, y = 5}, bottom = { x = 0, y = 6}}
	}
    },
    {
	name = "hobby_stage_3",
	desc = "A particularly tricky part of the 3rd stage of the 'hobby' test below.",
	edges = {
	    { top = { x = -1, y = -2}, bottom = { x =  4, y = 2}},
	    { top = { x =  5, y =  3}, bottom = { x =  9, y = 5}},
	    { top = { x =  5, y =  3}, bottom = { x =  6, y = 3}},
	}
    },
    {
	name = "hobby",
	desc = "Example from John Hobby's paper. Requires 3 passes of the iterative algorithm.",
	edges = {
	    { top = { x =   0, y =   0}, bottom = { x =   9, y =   5}},
	    { top = { x =   0, y =   0}, bottom = { x =  13, y =   6}},
	    { top = { x =  -1, y =  -2}, bottom = { x =   9, y =   5}}
	}
    },
    {
	name = "slope",
	desc = "Edges with same start/stop points but different slopes",
	edges = {
	    { top = { x = 4, y = 1}, bottom = { x = 6, y = 3}},
	    { top = { x = 4, y = 1}, bottom = { x = 2, y = 3}},
	    { top = { x = 2, y = 4}, bottom = { x = 4, y = 6}},
	    { top = { x = 6, y = 4}, bottom = { x = 4, y = 6}}
	}
    },
    {
	name = "horizontal",
	desc = "Test of a horizontal edge",
	edges = {
	    { top = { x = 1, y = 1}, bottom = { x = 6, y = 6}},
	    { top = { x = 2, y = 3}, bottom = { x = 5, y = 3}}
	}
    },
    {
	name = "vertical",
	desc = "Test of a vertical edge",
	edges = {
	    { top = { x = 5, y = 1}, bottom = { x = 5, y = 7}},
	    { top = { x = 2, y = 4}, bottom = { x = 8, y = 5}}
	}
    },
    {
	name = "congruent",
	desc = "Two overlapping edges with the same slope",
	edges = {
	    { top = { x = 5, y = 1}, bottom = { x = 5, y = 7}},
	    { top = { x = 5, y = 2}, bottom = { x = 5, y = 6}},
	    { top = { x = 2, y = 4}, bottom = { x = 8, y = 5}}
	}
    },
    {
	name = "multi",
	desc = "Several segments with a common intersection point",
	edges = {
	    { top = { x = 1, y = 2}, bottom = { x = 5, y = 4} },
	    { top = { x = 1, y = 1}, bottom = { x = 5, y = 5} },
	    { top = { x = 2, y = 1}, bottom = { x = 4, y = 5} },
	    { top = { x = 4, y = 1}, bottom = { x = 2, y = 5} },
	    { top = { x = 5, y = 1}, bottom = { x = 1, y = 5} },
	    { top = { x = 5, y = 2}, bottom = { x = 1, y = 4} }
	}
    }
};
*/

static int
run_test (const char		*test_name,
          comac_bo_edge_t	*test_edges,
          int			 num_edges)
{
    int i, intersections, passes;
    comac_bo_edge_t *edges;
    comac_array_t intersected_edges;

    printf ("Testing: %s\n", test_name);

    _comac_array_init (&intersected_edges, sizeof (comac_bo_edge_t));

    intersections = _comac_bentley_ottmann_intersect_edges (test_edges, num_edges, &intersected_edges);
    if (intersections)
	printf ("Pass 1 found %d intersections:\n", intersections);


    /* XXX: Multi-pass Bentley-Ottmmann. Preferable would be to add a
     * pass of Hobby's tolerance-square algorithm instead. */
    passes = 1;
    while (intersections) {
	int num_edges = _comac_array_num_elements (&intersected_edges);
	passes++;
	edges = _comac_malloc_ab (num_edges, sizeof (comac_bo_edge_t));
	assert (edges != NULL);
	memcpy (edges, _comac_array_index (&intersected_edges, 0), num_edges * sizeof (comac_bo_edge_t));
	_comac_array_fini (&intersected_edges);
	_comac_array_init (&intersected_edges, sizeof (comac_bo_edge_t));
	intersections = _comac_bentley_ottmann_intersect_edges (edges, num_edges, &intersected_edges);
	free (edges);

	if (intersections){
	    printf ("Pass %d found %d remaining intersections:\n", passes, intersections);
	} else {
	    if (passes > 3)
		for (i = 0; i < passes; i++)
		    printf ("*");
	    printf ("No remainining intersections found after pass %d\n", passes);
	}
    }

    if (edges_have_an_intersection_quadratic (_comac_array_index (&intersected_edges, 0),
					      _comac_array_num_elements (&intersected_edges)))
	printf ("*** FAIL ***\n");
    else
	printf ("PASS\n");

    _comac_array_fini (&intersected_edges);

    return 0;
}

#define MAX_RANDOM 300

int
main (void)
{
    char random_name[] = "random-XX";
    comac_bo_edge_t random_edges[MAX_RANDOM], *edge;
    unsigned int i, num_random;
    test_t *test;

    for (i = 0; i < ARRAY_LENGTH (tests); i++) {
	test = &tests[i];
	run_test (test->name, test->edges, test->num_edges);
    }

    for (num_random = 0; num_random < MAX_RANDOM; num_random++) {
	srand (0);
	for (i = 0; i < num_random; i++) {
	    do {
		edge = &random_edges[i];
		edge->line.p1.x = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->line.p1.y = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->line.p2.x = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		edge->line.p2.y = (int32_t) (10.0 * (rand() / (RAND_MAX + 1.0)));
		if (edge->line.p1.y > edge->line.p2.y) {
		    int32_t tmp = edge->line.p1.y;
		    edge->line.p1.y = edge->line.p2.y;
		    edge->line.p2.y = tmp;
		}
	    } while (edge->line.p1.y == edge->line.p2.y);
	}

	sprintf (random_name, "random-%02d", num_random);

	run_test (random_name, random_edges, num_random);
    }

    return 0;
}
#endif
