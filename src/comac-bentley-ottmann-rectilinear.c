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

#include "comac-boxes-private.h"
#include "comac-combsort-inline.h"
#include "comac-error-private.h"
#include "comac-traps-private.h"

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
    comac_bo_trap_t deferred_trap;
};

typedef enum {
    COMAC_BO_EVENT_TYPE_START,
    COMAC_BO_EVENT_TYPE_STOP
} comac_bo_event_type_t;

typedef struct _comac_bo_event {
    comac_bo_event_type_t type;
    comac_point_t point;
    comac_bo_edge_t *edge;
} comac_bo_event_t;

typedef struct _comac_bo_sweep_line {
    comac_bo_event_t **events;
    comac_bo_edge_t *head;
    comac_bo_edge_t *stopped;
    int32_t current_y;
    comac_bo_edge_t *current_edge;
} comac_bo_sweep_line_t;

static inline int
_comac_point_compare (const comac_point_t *a,
		      const comac_point_t *b)
{
    int cmp;

    cmp = a->y - b->y;
    if (likely (cmp))
	return cmp;

    return a->x - b->x;
}

static inline int
_comac_bo_edge_compare (const comac_bo_edge_t	*a,
			const comac_bo_edge_t	*b)
{
    int cmp;

    cmp = a->edge.line.p1.x - b->edge.line.p1.x;
    if (likely (cmp))
	return cmp;

    return b->edge.bottom - a->edge.bottom;
}

static inline int
comac_bo_event_compare (const comac_bo_event_t *a,
			const comac_bo_event_t *b)
{
    int cmp;

    cmp = _comac_point_compare (&a->point, &b->point);
    if (likely (cmp))
	return cmp;

    cmp = a->type - b->type;
    if (cmp)
	return cmp;

    return a - b;
}

static inline comac_bo_event_t *
_comac_bo_event_dequeue (comac_bo_sweep_line_t *sweep_line)
{
    return *sweep_line->events++;
}

COMAC_COMBSORT_DECLARE (_comac_bo_event_queue_sort,
			comac_bo_event_t *,
			comac_bo_event_compare)

static void
_comac_bo_sweep_line_init (comac_bo_sweep_line_t *sweep_line,
			   comac_bo_event_t	**events,
			   int			  num_events)
{
    _comac_bo_event_queue_sort (events, num_events);
    events[num_events] = NULL;
    sweep_line->events = events;

    sweep_line->head = NULL;
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

	cmp = _comac_bo_edge_compare (sweep_line->current_edge, edge);
	if (cmp < 0) {
	    prev = sweep_line->current_edge;
	    next = prev->next;
	    while (next != NULL && _comac_bo_edge_compare (next, edge) < 0)
		prev = next, next = prev->next;

	    prev->next = edge;
	    edge->prev = prev;
	    edge->next = next;
	    if (next != NULL)
		next->prev = edge;
	} else if (cmp > 0) {
	    next = sweep_line->current_edge;
	    prev = next->prev;
	    while (prev != NULL && _comac_bo_edge_compare (prev, edge) > 0)
		next = prev, prev = next->prev;

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

static inline comac_bool_t
edges_collinear (const comac_bo_edge_t *a, const comac_bo_edge_t *b)
{
    return a->edge.line.p1.x == b->edge.line.p1.x;
}

static comac_status_t
_comac_bo_edge_end_trap (comac_bo_edge_t	*left,
			 int32_t		 bot,
			 comac_bool_t		 do_traps,
			 void			*container)
{
    comac_bo_trap_t *trap = &left->deferred_trap;
    comac_status_t status = COMAC_STATUS_SUCCESS;

    /* Only emit (trivial) non-degenerate trapezoids with positive height. */
    if (likely (trap->top < bot)) {
	if (do_traps) {
	    _comac_traps_add_trap (container,
				   trap->top, bot,
				   &left->edge.line, &trap->right->edge.line);
	    status =  _comac_traps_status ((comac_traps_t *) container);
	} else {
	    comac_box_t box;

	    box.p1.x = left->edge.line.p1.x;
	    box.p1.y = trap->top;
	    box.p2.x = trap->right->edge.line.p1.x;
	    box.p2.y = bot;
	    status = _comac_boxes_add (container, COMAC_ANTIALIAS_DEFAULT, &box);
	}
    }

    trap->right = NULL;

    return status;
}

/* Start a new trapezoid at the given top y coordinate, whose edges
 * are `edge' and `edge->next'. If `edge' already has a trapezoid,
 * then either add it to the traps in `traps', if the trapezoid's
 * right edge differs from `edge->next', or do nothing if the new
 * trapezoid would be a continuation of the existing one. */
static inline comac_status_t
_comac_bo_edge_start_or_continue_trap (comac_bo_edge_t	*left,
				       comac_bo_edge_t  *right,
				       int               top,
				       comac_bool_t	 do_traps,
				       void		*container)
{
    comac_status_t status;

    if (left->deferred_trap.right == right)
	return COMAC_STATUS_SUCCESS;

    if (left->deferred_trap.right != NULL) {
	if (right != NULL && edges_collinear (left->deferred_trap.right, right))
	{
	    /* continuation on right, so just swap edges */
	    left->deferred_trap.right = right;
	    return COMAC_STATUS_SUCCESS;
	}

	status = _comac_bo_edge_end_trap (left, top, do_traps, container);
	if (unlikely (status))
	    return status;
    }

    if (right != NULL && ! edges_collinear (left, right)) {
	left->deferred_trap.top = top;
	left->deferred_trap.right = right;
    }

    return COMAC_STATUS_SUCCESS;
}

static inline comac_status_t
_active_edges_to_traps (comac_bo_edge_t		*left,
			int32_t			 top,
			comac_fill_rule_t	 fill_rule,
			comac_bool_t		 do_traps,
			void			*container)
{
    comac_bo_edge_t *right;
    comac_status_t status;

    if (fill_rule == COMAC_FILL_RULE_WINDING) {
	while (left != NULL) {
	    int in_out;

	    /* Greedily search for the closing edge, so that we generate the
	     * maximal span width with the minimal number of trapezoids.
	     */
	    in_out = left->edge.dir;

	    /* Check if there is a co-linear edge with an existing trap */
	    right = left->next;
	    if (left->deferred_trap.right == NULL) {
		while (right != NULL && right->deferred_trap.right == NULL)
		    right = right->next;

		if (right != NULL && edges_collinear (left, right)) {
		    /* continuation on left */
		    left->deferred_trap = right->deferred_trap;
		    right->deferred_trap.right = NULL;
		}
	    }

	    /* End all subsumed traps */
	    right = left->next;
	    while (right != NULL) {
		if (right->deferred_trap.right != NULL) {
		    status = _comac_bo_edge_end_trap (right, top, do_traps, container);
		    if (unlikely (status))
			return status;
		}

		in_out += right->edge.dir;
		if (in_out == 0) {
		    /* skip co-linear edges */
		    if (right->next == NULL ||
			! edges_collinear (right, right->next))
		    {
			break;
		    }
		}

		right = right->next;
	    }

	    status = _comac_bo_edge_start_or_continue_trap (left, right, top,
							    do_traps, container);
	    if (unlikely (status))
		return status;

	    left = right;
	    if (left != NULL)
		left = left->next;
	}
    } else {
	while (left != NULL) {
	    int in_out = 0;

	    right = left->next;
	    while (right != NULL) {
		if (right->deferred_trap.right != NULL) {
		    status = _comac_bo_edge_end_trap (right, top, do_traps, container);
		    if (unlikely (status))
			return status;
		}

		if ((in_out++ & 1) == 0) {
		    comac_bo_edge_t *next;
		    comac_bool_t skip = FALSE;

		    /* skip co-linear edges */
		    next = right->next;
		    if (next != NULL)
			skip = edges_collinear (right, next);

		    if (! skip)
			break;
		}

		right = right->next;
	    }

	    status = _comac_bo_edge_start_or_continue_trap (left, right, top,
							    do_traps, container);
	    if (unlikely (status))
		return status;

	    left = right;
	    if (left != NULL)
		left = left->next;
	}
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_bentley_ottmann_tessellate_rectilinear (comac_bo_event_t   **start_events,
					       int			 num_events,
					       comac_fill_rule_t	 fill_rule,
					       comac_bool_t		 do_traps,
					       void			*container)
{
    comac_bo_sweep_line_t sweep_line;
    comac_bo_event_t *event;
    comac_status_t status;

    _comac_bo_sweep_line_init (&sweep_line, start_events, num_events);

    while ((event = _comac_bo_event_dequeue (&sweep_line))) {
	if (event->point.y != sweep_line.current_y) {
	    status = _active_edges_to_traps (sweep_line.head,
					     sweep_line.current_y,
					     fill_rule, do_traps, container);
	    if (unlikely (status))
		return status;

	    sweep_line.current_y = event->point.y;
	}

	switch (event->type) {
	case COMAC_BO_EVENT_TYPE_START:
	    _comac_bo_sweep_line_insert (&sweep_line, event->edge);
	    break;

	case COMAC_BO_EVENT_TYPE_STOP:
	    _comac_bo_sweep_line_delete (&sweep_line, event->edge);

	    if (event->edge->deferred_trap.right != NULL) {
		status = _comac_bo_edge_end_trap (event->edge,
						  sweep_line.current_y,
						  do_traps, container);
		if (unlikely (status))
		    return status;
	    }

	    break;
	}
    }

    return COMAC_STATUS_SUCCESS;
}

comac_status_t
_comac_bentley_ottmann_tessellate_rectilinear_polygon_to_boxes (const comac_polygon_t *polygon,
								comac_fill_rule_t	  fill_rule,
								comac_boxes_t *boxes)
{
    comac_status_t status;
    comac_bo_event_t stack_events[COMAC_STACK_ARRAY_LENGTH (comac_bo_event_t)];
    comac_bo_event_t *events;
    comac_bo_event_t *stack_event_ptrs[ARRAY_LENGTH (stack_events) + 1];
    comac_bo_event_t **event_ptrs;
    comac_bo_edge_t stack_edges[ARRAY_LENGTH (stack_events)];
    comac_bo_edge_t *edges;
    int num_events;
    int i, j;

    if (unlikely (polygon->num_edges == 0))
	return COMAC_STATUS_SUCCESS;

    num_events = 2 * polygon->num_edges;

    events = stack_events;
    event_ptrs = stack_event_ptrs;
    edges = stack_edges;
    if (num_events > ARRAY_LENGTH (stack_events)) {
	events = _comac_malloc_ab_plus_c (num_events,
					  sizeof (comac_bo_event_t) +
					  sizeof (comac_bo_edge_t) +
					  sizeof (comac_bo_event_t *),
					  sizeof (comac_bo_event_t *));
	if (unlikely (events == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	event_ptrs = (comac_bo_event_t **) (events + num_events);
	edges = (comac_bo_edge_t *) (event_ptrs + num_events + 1);
    }

    for (i = j = 0; i < polygon->num_edges; i++) {
	edges[i].edge = polygon->edges[i];
	edges[i].deferred_trap.right = NULL;
	edges[i].prev = NULL;
	edges[i].next = NULL;

	event_ptrs[j] = &events[j];
	events[j].type = COMAC_BO_EVENT_TYPE_START;
	events[j].point.y = polygon->edges[i].top;
	events[j].point.x = polygon->edges[i].line.p1.x;
	events[j].edge = &edges[i];
	j++;

	event_ptrs[j] = &events[j];
	events[j].type = COMAC_BO_EVENT_TYPE_STOP;
	events[j].point.y = polygon->edges[i].bottom;
	events[j].point.x = polygon->edges[i].line.p1.x;
	events[j].edge = &edges[i];
	j++;
    }

    status = _comac_bentley_ottmann_tessellate_rectilinear (event_ptrs, j,
							    fill_rule,
							    FALSE, boxes);
    if (events != stack_events)
	free (events);

    return status;
}

comac_status_t
_comac_bentley_ottmann_tessellate_rectilinear_traps (comac_traps_t *traps,
						     comac_fill_rule_t fill_rule)
{
    comac_bo_event_t stack_events[COMAC_STACK_ARRAY_LENGTH (comac_bo_event_t)];
    comac_bo_event_t *events;
    comac_bo_event_t *stack_event_ptrs[ARRAY_LENGTH (stack_events) + 1];
    comac_bo_event_t **event_ptrs;
    comac_bo_edge_t stack_edges[ARRAY_LENGTH (stack_events)];
    comac_bo_edge_t *edges;
    comac_status_t status;
    int i, j, k;

    if (unlikely (traps->num_traps == 0))
	return COMAC_STATUS_SUCCESS;

    assert (traps->is_rectilinear);

    i = 4 * traps->num_traps;

    events = stack_events;
    event_ptrs = stack_event_ptrs;
    edges = stack_edges;
    if (i > ARRAY_LENGTH (stack_events)) {
	events = _comac_malloc_ab_plus_c (i,
					  sizeof (comac_bo_event_t) +
					  sizeof (comac_bo_edge_t) +
					  sizeof (comac_bo_event_t *),
					  sizeof (comac_bo_event_t *));
	if (unlikely (events == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	event_ptrs = (comac_bo_event_t **) (events + i);
	edges = (comac_bo_edge_t *) (event_ptrs + i + 1);
    }

    for (i = j = k = 0; i < traps->num_traps; i++) {
	edges[k].edge.top = traps->traps[i].top;
	edges[k].edge.bottom = traps->traps[i].bottom;
	edges[k].edge.line = traps->traps[i].left;
	edges[k].edge.dir = 1;
	edges[k].deferred_trap.right = NULL;
	edges[k].prev = NULL;
	edges[k].next = NULL;

	event_ptrs[j] = &events[j];
	events[j].type = COMAC_BO_EVENT_TYPE_START;
	events[j].point.y = traps->traps[i].top;
	events[j].point.x = traps->traps[i].left.p1.x;
	events[j].edge = &edges[k];
	j++;

	event_ptrs[j] = &events[j];
	events[j].type = COMAC_BO_EVENT_TYPE_STOP;
	events[j].point.y = traps->traps[i].bottom;
	events[j].point.x = traps->traps[i].left.p1.x;
	events[j].edge = &edges[k];
	j++;
	k++;

	edges[k].edge.top = traps->traps[i].top;
	edges[k].edge.bottom = traps->traps[i].bottom;
	edges[k].edge.line = traps->traps[i].right;
	edges[k].edge.dir = -1;
	edges[k].deferred_trap.right = NULL;
	edges[k].prev = NULL;
	edges[k].next = NULL;

	event_ptrs[j] = &events[j];
	events[j].type = COMAC_BO_EVENT_TYPE_START;
	events[j].point.y = traps->traps[i].top;
	events[j].point.x = traps->traps[i].right.p1.x;
	events[j].edge = &edges[k];
	j++;

	event_ptrs[j] = &events[j];
	events[j].type = COMAC_BO_EVENT_TYPE_STOP;
	events[j].point.y = traps->traps[i].bottom;
	events[j].point.x = traps->traps[i].right.p1.x;
	events[j].edge = &edges[k];
	j++;
	k++;
    }

    _comac_traps_clear (traps);
    status = _comac_bentley_ottmann_tessellate_rectilinear (event_ptrs, j,
							    fill_rule,
							    TRUE, traps);
    traps->is_rectilinear = TRUE;

    if (events != stack_events)
	free (events);

    return status;
}
