/*
 * Copyright © 2012 Intel Corporation
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
 * The Initial Developer of the Original Code is Chris Wilson
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-damage-private.h"
#include "comac-region-private.h"

static const comac_damage_t __comac_damage__nil = { COMAC_STATUS_NO_MEMORY };

comac_damage_t *
_comac_damage_create_in_error (comac_status_t status)
{
    _comac_error_throw (status);
    return (comac_damage_t *) &__comac_damage__nil;
}

comac_damage_t *
_comac_damage_create (void)
{
    comac_damage_t *damage;

    damage = _comac_malloc (sizeof (*damage));
    if (unlikely (damage == NULL)) {
	_comac_error_throw(COMAC_STATUS_NO_MEMORY);
	return (comac_damage_t *) &__comac_damage__nil;
    }

    damage->status = COMAC_STATUS_SUCCESS;
    damage->region = NULL;
    damage->dirty = 0;
    damage->tail = &damage->chunks;
    damage->chunks.base = damage->boxes;
    damage->chunks.size = ARRAY_LENGTH(damage->boxes);
    damage->chunks.count = 0;
    damage->chunks.next = NULL;

    damage->remain = damage->chunks.size;

    return damage;
}

void
_comac_damage_destroy (comac_damage_t *damage)
{
    struct _comac_damage_chunk *chunk, *next;

    if (damage == (comac_damage_t *) &__comac_damage__nil)
	return;

    for (chunk = damage->chunks.next; chunk != NULL; chunk = next) {
	next = chunk->next;
	free (chunk);
    }
    comac_region_destroy (damage->region);
    free (damage);
}

static comac_damage_t *
_comac_damage_add_boxes(comac_damage_t *damage,
			const comac_box_t *boxes,
			int count)
{
    struct _comac_damage_chunk *chunk;
    int n, size;

    TRACE ((stderr, "%s x%d\n", __FUNCTION__, count));

    if (damage == NULL)
	damage = _comac_damage_create ();
    if (damage->status)
	return damage;

    damage->dirty += count;

    n = count;
    if (n > damage->remain)
	n = damage->remain;

    memcpy (damage->tail->base + damage->tail->count, boxes,
	    n * sizeof (comac_box_t));

    count -= n;
    damage->tail->count += n;
    damage->remain -= n;

    if (count == 0)
	return damage;

    size = 2 * damage->tail->size;
    if (size < count)
	size = (count + 64) & ~63;

    chunk = _comac_malloc (sizeof (*chunk) + sizeof (comac_box_t) * size);
    if (unlikely (chunk == NULL)) {
	_comac_damage_destroy (damage);
	return (comac_damage_t *) &__comac_damage__nil;
    }

    chunk->next = NULL;
    chunk->base = (comac_box_t *) (chunk + 1);
    chunk->size = size;
    chunk->count = count;

    damage->tail->next = chunk;
    damage->tail = chunk;

    memcpy (damage->tail->base, boxes + n,
	    count * sizeof (comac_box_t));
    damage->remain = size - count;

    return damage;
}

comac_damage_t *
_comac_damage_add_box(comac_damage_t *damage,
		      const comac_box_t *box)
{
    TRACE ((stderr, "%s: (%d, %d),(%d, %d)\n", __FUNCTION__,
	    box->p1.x, box->p1.y, box->p2.x, box->p2.y));

    return _comac_damage_add_boxes(damage, box, 1);
}

comac_damage_t *
_comac_damage_add_rectangle(comac_damage_t *damage,
			    const comac_rectangle_int_t *r)
{
    comac_box_t box;

    TRACE ((stderr, "%s: (%d, %d)x(%d, %d)\n", __FUNCTION__,
	    r->x, r->y, r->width, r->height));

    box.p1.x = r->x;
    box.p1.y = r->y;
    box.p2.x = r->x + r->width;
    box.p2.y = r->y + r->height;

    return _comac_damage_add_boxes(damage, &box, 1);
}

comac_damage_t *
_comac_damage_add_region (comac_damage_t *damage,
			  const comac_region_t *region)
{
    comac_box_t *boxes;
    int nbox;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    boxes = _comac_region_get_boxes (region, &nbox);
    return _comac_damage_add_boxes(damage, boxes, nbox);
}

comac_damage_t *
_comac_damage_reduce (comac_damage_t *damage)
{
    comac_box_t *free_boxes = NULL;
    comac_box_t *boxes, *b;
    struct _comac_damage_chunk *chunk, *last;

    TRACE ((stderr, "%s: dirty=%d\n", __FUNCTION__,
	    damage ? damage->dirty : -1));
    if (damage == NULL || damage->status || !damage->dirty)
	return damage;

    if (damage->region) {
	comac_region_t *region;

	region = damage->region;
	damage->region = NULL;

	damage = _comac_damage_add_region (damage, region);
	comac_region_destroy (region);

	if (unlikely (damage->status))
	    return damage;
    }

    boxes = damage->tail->base;
    if (damage->dirty > damage->tail->size) {
	boxes = free_boxes = _comac_malloc (damage->dirty * sizeof (comac_box_t));
	if (unlikely (boxes == NULL)) {
	    _comac_damage_destroy (damage);
	    return (comac_damage_t *) &__comac_damage__nil;
	}

	b = boxes;
	last = NULL;
    } else {
	b = boxes + damage->tail->count;
	last = damage->tail;
    }

    for (chunk = &damage->chunks; chunk != last; chunk = chunk->next) {
	memcpy (b, chunk->base, chunk->count * sizeof (comac_box_t));
	b += chunk->count;
    }

    damage->region = _comac_region_create_from_boxes (boxes, damage->dirty);
    free (free_boxes);

    if (unlikely (damage->region->status)) {
	_comac_damage_destroy (damage);
	return (comac_damage_t *) &__comac_damage__nil;
    }

    damage->dirty = 0;
    return damage;
}
