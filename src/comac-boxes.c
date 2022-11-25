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
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-box-inline.h"
#include "comac-boxes-private.h"
#include "comac-error-private.h"

void
_comac_boxes_init (comac_boxes_t *boxes)
{
    boxes->status = COMAC_STATUS_SUCCESS;
    boxes->num_limits = 0;
    boxes->num_boxes = 0;

    boxes->tail = &boxes->chunks;
    boxes->chunks.next = NULL;
    boxes->chunks.base = boxes->boxes_embedded;
    boxes->chunks.size = ARRAY_LENGTH (boxes->boxes_embedded);
    boxes->chunks.count = 0;

    boxes->is_pixel_aligned = TRUE;
}

void
_comac_boxes_init_from_rectangle (comac_boxes_t *boxes,
				  int x, int y, int w, int h)
{
    _comac_boxes_init (boxes);

    _comac_box_from_integers (&boxes->chunks.base[0], x, y, w, h);
    boxes->num_boxes = 1;
}

void
_comac_boxes_init_with_clip (comac_boxes_t *boxes,
			     comac_clip_t *clip)
{
    _comac_boxes_init (boxes);
    if (clip)
	_comac_boxes_limit (boxes, clip->boxes, clip->num_boxes);
}

void
_comac_boxes_init_for_array (comac_boxes_t *boxes,
			     comac_box_t *array,
			     int num_boxes)
{
    int n;

    boxes->status = COMAC_STATUS_SUCCESS;
    boxes->num_limits = 0;
    boxes->num_boxes = num_boxes;

    boxes->tail = &boxes->chunks;
    boxes->chunks.next = NULL;
    boxes->chunks.base = array;
    boxes->chunks.size = num_boxes;
    boxes->chunks.count = num_boxes;

    for (n = 0; n < num_boxes; n++) {
	if (! _comac_fixed_is_integer (array[n].p1.x) ||
	    ! _comac_fixed_is_integer (array[n].p1.y) ||
	    ! _comac_fixed_is_integer (array[n].p2.x) ||
	    ! _comac_fixed_is_integer (array[n].p2.y))
	{
	    break;
	}
    }

    boxes->is_pixel_aligned = n == num_boxes;
}

/** _comac_boxes_limit:
 *
 * Computes the minimum bounding box of the given list of boxes and assign
 * it to the given boxes set. It also assigns that list as the list of
 * limiting boxes in the box set.
 *
 * @param boxes        the box set to be filled (return buffer)
 * @param limits       array of the limiting boxes to compute the bounding
 *                     box from
 * @param num_limits   length of the limits array
 */
void
_comac_boxes_limit (comac_boxes_t	*boxes,
		    const comac_box_t	*limits,
		    int			 num_limits)
{
    int n;

    boxes->limits = limits;
    boxes->num_limits = num_limits;

    if (boxes->num_limits) {
	boxes->limit = limits[0];
	for (n = 1; n < num_limits; n++) {
	    if (limits[n].p1.x < boxes->limit.p1.x)
		boxes->limit.p1.x = limits[n].p1.x;

	    if (limits[n].p1.y < boxes->limit.p1.y)
		boxes->limit.p1.y = limits[n].p1.y;

	    if (limits[n].p2.x > boxes->limit.p2.x)
		boxes->limit.p2.x = limits[n].p2.x;

	    if (limits[n].p2.y > boxes->limit.p2.y)
		boxes->limit.p2.y = limits[n].p2.y;
	}
    }
}

static void
_comac_boxes_add_internal (comac_boxes_t *boxes,
			   const comac_box_t *box)
{
    struct _comac_boxes_chunk *chunk;

    if (unlikely (boxes->status))
	return;

    chunk = boxes->tail;
    if (unlikely (chunk->count == chunk->size)) {
	int size;

	size = chunk->size * 2;
	chunk->next = _comac_malloc_ab_plus_c (size,
					       sizeof (comac_box_t),
					       sizeof (struct _comac_boxes_chunk));

	if (unlikely (chunk->next == NULL)) {
	    boxes->status = _comac_error (COMAC_STATUS_NO_MEMORY);
	    return;
	}

	chunk = chunk->next;
	boxes->tail = chunk;

	chunk->next = NULL;
	chunk->count = 0;
	chunk->size = size;
	chunk->base = (comac_box_t *) (chunk + 1);
    }

    chunk->base[chunk->count++] = *box;
    boxes->num_boxes++;

    if (boxes->is_pixel_aligned)
	boxes->is_pixel_aligned = _comac_box_is_pixel_aligned (box);
}

comac_status_t
_comac_boxes_add (comac_boxes_t *boxes,
		  comac_antialias_t antialias,
		  const comac_box_t *box)
{
    comac_box_t b;

    if (antialias == COMAC_ANTIALIAS_NONE) {
	b.p1.x = _comac_fixed_round_down (box->p1.x);
	b.p1.y = _comac_fixed_round_down (box->p1.y);
	b.p2.x = _comac_fixed_round_down (box->p2.x);
	b.p2.y = _comac_fixed_round_down (box->p2.y);
	box = &b;
    }

    if (box->p1.y == box->p2.y)
	return COMAC_STATUS_SUCCESS;

    if (box->p1.x == box->p2.x)
	return COMAC_STATUS_SUCCESS;

    if (boxes->num_limits) {
	comac_point_t p1, p2;
	comac_bool_t reversed = FALSE;
	int n;

	/* support counter-clockwise winding for rectangular tessellation */
	if (box->p1.x < box->p2.x) {
	    p1.x = box->p1.x;
	    p2.x = box->p2.x;
	} else {
	    p2.x = box->p1.x;
	    p1.x = box->p2.x;
	    reversed = ! reversed;
	}

	if (p1.x >= boxes->limit.p2.x || p2.x <= boxes->limit.p1.x)
	    return COMAC_STATUS_SUCCESS;

	if (box->p1.y < box->p2.y) {
	    p1.y = box->p1.y;
	    p2.y = box->p2.y;
	} else {
	    p2.y = box->p1.y;
	    p1.y = box->p2.y;
	    reversed = ! reversed;
	}

	if (p1.y >= boxes->limit.p2.y || p2.y <= boxes->limit.p1.y)
	    return COMAC_STATUS_SUCCESS;

	for (n = 0; n < boxes->num_limits; n++) {
	    const comac_box_t *limits = &boxes->limits[n];
	    comac_box_t _box;
	    comac_point_t _p1, _p2;

	    if (p1.x >= limits->p2.x || p2.x <= limits->p1.x)
		continue;
	    if (p1.y >= limits->p2.y || p2.y <= limits->p1.y)
		continue;

	    /* Otherwise, clip the box to the limits. */
	    _p1 = p1;
	    if (_p1.x < limits->p1.x)
		_p1.x = limits->p1.x;
	    if (_p1.y < limits->p1.y)
		_p1.y = limits->p1.y;

	    _p2 = p2;
	    if (_p2.x > limits->p2.x)
		_p2.x = limits->p2.x;
	    if (_p2.y > limits->p2.y)
		_p2.y = limits->p2.y;

	    if (_p2.y <= _p1.y || _p2.x <= _p1.x)
		continue;

	    _box.p1.y = _p1.y;
	    _box.p2.y = _p2.y;
	    if (reversed) {
		_box.p1.x = _p2.x;
		_box.p2.x = _p1.x;
	    } else {
		_box.p1.x = _p1.x;
		_box.p2.x = _p2.x;
	    }

	    _comac_boxes_add_internal (boxes, &_box);
	}
    } else {
	_comac_boxes_add_internal (boxes, box);
    }

    return boxes->status;
}

/** _comac_boxes_extents:
 *
 * Computes the minimum bounding box of the given box set and stores
 * it in the given box.
 *
 * @param boxes      The box set whose minimum bounding is computed.
 * @param box        Return buffer for the computed result.
 */
void
_comac_boxes_extents (const comac_boxes_t *boxes,
		      comac_box_t *box)
{
    const struct _comac_boxes_chunk *chunk;
    comac_box_t b;
    int i;

    if (boxes->num_boxes == 0) {
	box->p1.x = box->p1.y = box->p2.x = box->p2.y = 0;
	return;
    }

    b = boxes->chunks.base[0];
    for (chunk = &boxes->chunks; chunk != NULL; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++) {
	    if (chunk->base[i].p1.x < b.p1.x)
		b.p1.x = chunk->base[i].p1.x;

	    if (chunk->base[i].p1.y < b.p1.y)
		b.p1.y = chunk->base[i].p1.y;

	    if (chunk->base[i].p2.x > b.p2.x)
		b.p2.x = chunk->base[i].p2.x;

	    if (chunk->base[i].p2.y > b.p2.y)
		b.p2.y = chunk->base[i].p2.y;
	}
    }
    *box = b;
}

void
_comac_boxes_clear (comac_boxes_t *boxes)
{
    struct _comac_boxes_chunk *chunk, *next;

    for (chunk = boxes->chunks.next; chunk != NULL; chunk = next) {
	next = chunk->next;
	free (chunk);
    }

    boxes->tail = &boxes->chunks;
    boxes->chunks.next = 0;
    boxes->chunks.count = 0;
    boxes->chunks.base = boxes->boxes_embedded;
    boxes->chunks.size = ARRAY_LENGTH (boxes->boxes_embedded);
    boxes->num_boxes = 0;

    boxes->is_pixel_aligned = TRUE;
}

/** _comac_boxes_to_array:
 *
 * Linearize a box set of possibly multiple chunks into one big chunk
 * and returns an array of boxes
 *
 * @param boxes      The box set to be converted.
 * @param num_boxes  Return buffer for the number of boxes (array count).
 * @return           Pointer to the newly allocated array of boxes
 *                   (the number o elements is given in num_boxes).
 */
comac_box_t *
_comac_boxes_to_array (const comac_boxes_t *boxes,
		       int *num_boxes)
{
    const struct _comac_boxes_chunk *chunk;
    comac_box_t *box;
    int i, j;

    *num_boxes = boxes->num_boxes;

    box = _comac_malloc_ab (boxes->num_boxes, sizeof (comac_box_t));
    if (box == NULL) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return NULL;
    }

    j = 0;
    for (chunk = &boxes->chunks; chunk != NULL; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++)
	    box[j++] = chunk->base[i];
    }

    return box;
}

void
_comac_boxes_fini (comac_boxes_t *boxes)
{
    struct _comac_boxes_chunk *chunk, *next;

    for (chunk = boxes->chunks.next; chunk != NULL; chunk = next) {
	next = chunk->next;
	free (chunk);
    }
}

comac_bool_t
_comac_boxes_for_each_box (comac_boxes_t *boxes,
			   comac_bool_t (*func) (comac_box_t *box, void *data),
			   void *data)
{
    struct _comac_boxes_chunk *chunk;
    int i;

    for (chunk = &boxes->chunks; chunk != NULL; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++)
	    if (! func (&chunk->base[i], data))
		return FALSE;
    }

    return TRUE;
}

struct comac_box_renderer {
    comac_span_renderer_t base;
    comac_boxes_t *boxes;
};

static comac_status_t
span_to_boxes (void *abstract_renderer, int y, int h,
	       const comac_half_open_span_t *spans, unsigned num_spans)
{
    struct comac_box_renderer *r = abstract_renderer;
    comac_status_t status = COMAC_STATUS_SUCCESS;
    comac_box_t box;

    if (num_spans == 0)
	return COMAC_STATUS_SUCCESS;

    box.p1.y = _comac_fixed_from_int (y);
    box.p2.y = _comac_fixed_from_int (y + h);
    do {
	if (spans[0].coverage) {
	    box.p1.x = _comac_fixed_from_int(spans[0].x);
	    box.p2.x = _comac_fixed_from_int(spans[1].x);
	    status = _comac_boxes_add (r->boxes, COMAC_ANTIALIAS_DEFAULT, &box);
	}
	spans++;
    } while (--num_spans > 1 && status == COMAC_STATUS_SUCCESS);

    return status;
}

comac_status_t
_comac_rasterise_polygon_to_boxes (comac_polygon_t			*polygon,
				   comac_fill_rule_t			 fill_rule,
				   comac_boxes_t *boxes)
{
    struct comac_box_renderer renderer;
    comac_scan_converter_t *converter;
    comac_int_status_t status;
    comac_rectangle_int_t r;

    TRACE ((stderr, "%s: fill_rule=%d\n", __FUNCTION__, fill_rule));

    _comac_box_round_to_rectangle (&polygon->extents, &r);
    converter = _comac_mono_scan_converter_create (r.x, r.y,
						   r.x + r.width,
						   r.y + r.height,
						   fill_rule);
    status = _comac_mono_scan_converter_add_polygon (converter, polygon);
    if (unlikely (status))
	goto cleanup_converter;

    renderer.boxes = boxes;
    renderer.base.render_rows = span_to_boxes;

    status = converter->generate (converter, &renderer.base);
cleanup_converter:
    converter->destroy (converter);
    return status;
}

void
_comac_debug_print_boxes (FILE *stream, const comac_boxes_t *boxes)
{
    const struct _comac_boxes_chunk *chunk;
    comac_box_t extents;
    int i;

    _comac_boxes_extents (boxes, &extents);
    fprintf (stream, "boxes x %d: (%f, %f) x (%f, %f)\n",
	     boxes->num_boxes,
	     _comac_fixed_to_double (extents.p1.x),
	     _comac_fixed_to_double (extents.p1.y),
	     _comac_fixed_to_double (extents.p2.x),
	     _comac_fixed_to_double (extents.p2.y));

    for (chunk = &boxes->chunks; chunk != NULL; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++) {
	    fprintf (stderr, "  box[%d]: (%f, %f), (%f, %f)\n", i,
		     _comac_fixed_to_double (chunk->base[i].p1.x),
		     _comac_fixed_to_double (chunk->base[i].p1.y),
		     _comac_fixed_to_double (chunk->base[i].p2.x),
		     _comac_fixed_to_double (chunk->base[i].p2.y));
	}
    }
}
