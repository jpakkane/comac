/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2009 Chris Wilson
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
 *	Kristian Høgsberg <krh@redhat.com>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"
#include "comac-clip-inline.h"
#include "comac-clip-private.h"
#include "comac-error-private.h"
#include "comac-freed-pool-private.h"
#include "comac-gstate-private.h"
#include "comac-path-fixed-private.h"
#include "comac-pattern-private.h"
#include "comac-composite-rectangles-private.h"
#include "comac-region-private.h"

static freed_pool_t clip_path_pool;
static freed_pool_t clip_pool;

const comac_clip_t __comac_clip_all;

static comac_clip_path_t *
_comac_clip_path_create (comac_clip_t *clip)
{
    comac_clip_path_t *clip_path;

    clip_path = _freed_pool_get (&clip_path_pool);
    if (unlikely (clip_path == NULL)) {
	clip_path = _comac_malloc (sizeof (comac_clip_path_t));
	if (unlikely (clip_path == NULL))
	    return NULL;
    }

    COMAC_REFERENCE_COUNT_INIT (&clip_path->ref_count, 1);

    clip_path->prev = clip->path;
    clip->path = clip_path;

    return clip_path;
}

comac_clip_path_t *
_comac_clip_path_reference (comac_clip_path_t *clip_path)
{
    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&clip_path->ref_count));

    _comac_reference_count_inc (&clip_path->ref_count);

    return clip_path;
}

void
_comac_clip_path_destroy (comac_clip_path_t *clip_path)
{
    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&clip_path->ref_count));

    if (! _comac_reference_count_dec_and_test (&clip_path->ref_count))
	return;

    _comac_path_fixed_fini (&clip_path->path);

    if (clip_path->prev != NULL)
	_comac_clip_path_destroy (clip_path->prev);

    _freed_pool_put (&clip_path_pool, clip_path);
}

comac_clip_t *
_comac_clip_create (void)
{
    comac_clip_t *clip;

    clip = _freed_pool_get (&clip_pool);
    if (unlikely (clip == NULL)) {
	clip = _comac_malloc (sizeof (comac_clip_t));
	if (unlikely (clip == NULL))
	    return NULL;
    }

    clip->extents = _comac_unbounded_rectangle;

    clip->path = NULL;
    clip->boxes = NULL;
    clip->num_boxes = 0;
    clip->region = NULL;
    clip->is_region = FALSE;

    return clip;
}

void
_comac_clip_destroy (comac_clip_t *clip)
{
    if (clip == NULL || _comac_clip_is_all_clipped (clip))
	return;

    if (clip->path != NULL)
	_comac_clip_path_destroy (clip->path);

    if (clip->boxes != &clip->embedded_box)
	free (clip->boxes);
    comac_region_destroy (clip->region);

    _freed_pool_put (&clip_pool, clip);
}

comac_clip_t *
_comac_clip_copy (const comac_clip_t *clip)
{
    comac_clip_t *copy;

    if (clip == NULL || _comac_clip_is_all_clipped (clip))
	return (comac_clip_t *) clip;

    copy = _comac_clip_create ();

    if (clip->path)
	copy->path = _comac_clip_path_reference (clip->path);

    if (clip->num_boxes) {
	if (clip->num_boxes == 1) {
	    copy->boxes = &copy->embedded_box;
	} else {
	    copy->boxes = _comac_malloc_ab (clip->num_boxes, sizeof (comac_box_t));
	    if (unlikely (copy->boxes == NULL))
		return _comac_clip_set_all_clipped (copy);
	}

	memcpy (copy->boxes, clip->boxes,
		clip->num_boxes * sizeof (comac_box_t));
	copy->num_boxes = clip->num_boxes;
    }

    copy->extents = clip->extents;
    copy->region = comac_region_reference (clip->region);
    copy->is_region = clip->is_region;

    return copy;
}

comac_clip_t *
_comac_clip_copy_path (const comac_clip_t *clip)
{
    comac_clip_t *copy;

    if (clip == NULL || _comac_clip_is_all_clipped (clip))
	return (comac_clip_t *) clip;

    assert (clip->num_boxes);

    copy = _comac_clip_create ();
    copy->extents = clip->extents;
    if (clip->path)
	copy->path = _comac_clip_path_reference (clip->path);

    return copy;
}

comac_clip_t *
_comac_clip_copy_region (const comac_clip_t *clip)
{
    comac_clip_t *copy;
    int i;

    if (clip == NULL || _comac_clip_is_all_clipped (clip))
	return (comac_clip_t *) clip;

    assert (clip->num_boxes);

    copy = _comac_clip_create ();
    copy->extents = clip->extents;

    if (clip->num_boxes == 1) {
	copy->boxes = &copy->embedded_box;
    } else {
	copy->boxes = _comac_malloc_ab (clip->num_boxes, sizeof (comac_box_t));
	if (unlikely (copy->boxes == NULL))
	    return _comac_clip_set_all_clipped (copy);
    }

    for (i = 0; i < clip->num_boxes; i++) {
	copy->boxes[i].p1.x = _comac_fixed_floor (clip->boxes[i].p1.x);
	copy->boxes[i].p1.y = _comac_fixed_floor (clip->boxes[i].p1.y);
	copy->boxes[i].p2.x = _comac_fixed_ceil (clip->boxes[i].p2.x);
	copy->boxes[i].p2.y = _comac_fixed_ceil (clip->boxes[i].p2.y);
    }
    copy->num_boxes = clip->num_boxes;

    copy->region = comac_region_reference (clip->region);
    copy->is_region = TRUE;

    return copy;
}

comac_clip_t *
_comac_clip_intersect_path (comac_clip_t       *clip,
			    const comac_path_fixed_t *path,
			    comac_fill_rule_t   fill_rule,
			    double              tolerance,
			    comac_antialias_t   antialias)
{
    comac_clip_path_t *clip_path;
    comac_status_t status;
    comac_rectangle_int_t extents;
    comac_box_t box;

    if (_comac_clip_is_all_clipped (clip))
	return clip;

    /* catch the empty clip path */
    if (_comac_path_fixed_fill_is_empty (path))
	return _comac_clip_set_all_clipped (clip);

    if (_comac_path_fixed_is_box (path, &box)) {
	if (antialias == COMAC_ANTIALIAS_NONE) {
	    box.p1.x = _comac_fixed_round_down (box.p1.x);
	    box.p1.y = _comac_fixed_round_down (box.p1.y);
	    box.p2.x = _comac_fixed_round_down (box.p2.x);
	    box.p2.y = _comac_fixed_round_down (box.p2.y);
	}

	return _comac_clip_intersect_box (clip, &box);
    }
    if (_comac_path_fixed_fill_is_rectilinear (path))
	return _comac_clip_intersect_rectilinear_path (clip, path,
						       fill_rule, antialias);

    _comac_path_fixed_approximate_clip_extents (path, &extents);
    if (extents.width == 0 || extents.height == 0)
	return _comac_clip_set_all_clipped (clip);

    clip = _comac_clip_intersect_rectangle (clip, &extents);
    if (_comac_clip_is_all_clipped (clip))
	return clip;

    clip_path = _comac_clip_path_create (clip);
    if (unlikely (clip_path == NULL))
	return _comac_clip_set_all_clipped (clip);

    status = _comac_path_fixed_init_copy (&clip_path->path, path);
    if (unlikely (status))
	return _comac_clip_set_all_clipped (clip);

    clip_path->fill_rule = fill_rule;
    clip_path->tolerance = tolerance;
    clip_path->antialias = antialias;

    if (clip->region) {
	comac_region_destroy (clip->region);
	clip->region = NULL;
    }

    clip->is_region = FALSE;
    return clip;
}

static comac_clip_t *
_comac_clip_intersect_clip_path (comac_clip_t *clip,
				 const comac_clip_path_t *clip_path)
{
    if (clip_path->prev)
	clip = _comac_clip_intersect_clip_path (clip, clip_path->prev);

    return _comac_clip_intersect_path (clip,
				       &clip_path->path,
				       clip_path->fill_rule,
				       clip_path->tolerance,
				       clip_path->antialias);
}

comac_clip_t *
_comac_clip_intersect_clip (comac_clip_t *clip,
			    const comac_clip_t *other)
{
    if (_comac_clip_is_all_clipped (clip))
	return clip;

    if (other == NULL)
	return clip;

    if (clip == NULL)
	return _comac_clip_copy (other);

    if (_comac_clip_is_all_clipped (other))
	return _comac_clip_set_all_clipped (clip);

    if (! _comac_rectangle_intersect (&clip->extents, &other->extents))
	return _comac_clip_set_all_clipped (clip);

    if (other->num_boxes) {
	comac_boxes_t boxes;

	_comac_boxes_init_for_array (&boxes, other->boxes, other->num_boxes);
	clip = _comac_clip_intersect_boxes (clip, &boxes);
    }

    if (! _comac_clip_is_all_clipped (clip)) {
	if (other->path) {
	    if (clip->path == NULL)
		clip->path = _comac_clip_path_reference (other->path);
	    else
		clip = _comac_clip_intersect_clip_path (clip, other->path);
	}
    }

    if (clip->region) {
	comac_region_destroy (clip->region);
	clip->region = NULL;
    }
    clip->is_region = FALSE;

    return clip;
}

comac_bool_t
_comac_clip_equal (const comac_clip_t *clip_a,
		   const comac_clip_t *clip_b)
{
    const comac_clip_path_t *cp_a, *cp_b;

    /* are both all-clipped or no-clip? */
    if (clip_a == clip_b)
	return TRUE;

    /* or just one of them? */
    if (clip_a == NULL || clip_b == NULL ||
	_comac_clip_is_all_clipped (clip_a) ||
	_comac_clip_is_all_clipped (clip_b))
    {
	return FALSE;
    }

    /* We have a pair of normal clips, check their contents */

    if (clip_a->num_boxes != clip_b->num_boxes)
	return FALSE;

    if (memcmp (clip_a->boxes, clip_b->boxes,
		sizeof (comac_box_t) * clip_a->num_boxes))
	return FALSE;

    cp_a = clip_a->path;
    cp_b = clip_b->path;
    while (cp_a && cp_b) {
	if (cp_a == cp_b)
	    return TRUE;

	/* XXX compare reduced polygons? */

	if (cp_a->antialias != cp_b->antialias)
	    return FALSE;

	if (cp_a->tolerance != cp_b->tolerance)
	    return FALSE;

	if (cp_a->fill_rule != cp_b->fill_rule)
	    return FALSE;

	if (! _comac_path_fixed_equal (&cp_a->path,
				       &cp_b->path))
	    return FALSE;

	cp_a = cp_a->prev;
	cp_b = cp_b->prev;
    }

    return cp_a == NULL && cp_b == NULL;
}

static comac_clip_t *
_comac_clip_path_copy_with_translation (comac_clip_t      *clip,
					comac_clip_path_t *other_path,
					int fx, int fy)
{
    comac_status_t status;
    comac_clip_path_t *clip_path;

    if (other_path->prev != NULL)
	clip = _comac_clip_path_copy_with_translation (clip, other_path->prev,
						       fx, fy);
    if (_comac_clip_is_all_clipped (clip))
	return clip;

    clip_path = _comac_clip_path_create (clip);
    if (unlikely (clip_path == NULL))
	return _comac_clip_set_all_clipped (clip);

    status = _comac_path_fixed_init_copy (&clip_path->path,
					  &other_path->path);
    if (unlikely (status))
	return _comac_clip_set_all_clipped (clip);

    _comac_path_fixed_translate (&clip_path->path, fx, fy);

    clip_path->fill_rule = other_path->fill_rule;
    clip_path->tolerance = other_path->tolerance;
    clip_path->antialias = other_path->antialias;

    return clip;
}

comac_clip_t *
_comac_clip_translate (comac_clip_t *clip, int tx, int ty)
{
    int fx, fy, i;
    comac_clip_path_t *clip_path;

    if (clip == NULL || _comac_clip_is_all_clipped (clip))
	return clip;

    if (tx == 0 && ty == 0)
	return clip;

    fx = _comac_fixed_from_int (tx);
    fy = _comac_fixed_from_int (ty);

    for (i = 0; i < clip->num_boxes; i++) {
	clip->boxes[i].p1.x += fx;
	clip->boxes[i].p2.x += fx;
	clip->boxes[i].p1.y += fy;
	clip->boxes[i].p2.y += fy;
    }

    clip->extents.x += tx;
    clip->extents.y += ty;

    if (clip->path == NULL)
	return clip;

    clip_path = clip->path;
    clip->path = NULL;
    clip = _comac_clip_path_copy_with_translation (clip, clip_path, fx, fy);
    _comac_clip_path_destroy (clip_path);

    return clip;
}

static comac_status_t
_comac_path_fixed_add_box (comac_path_fixed_t *path,
			   const comac_box_t *box)
{
    comac_status_t status;

    status = _comac_path_fixed_move_to (path, box->p1.x, box->p1.y);
    if (unlikely (status))
	return status;

    status = _comac_path_fixed_line_to (path, box->p2.x, box->p1.y);
    if (unlikely (status))
	return status;

    status = _comac_path_fixed_line_to (path, box->p2.x, box->p2.y);
    if (unlikely (status))
	return status;

    status = _comac_path_fixed_line_to (path, box->p1.x, box->p2.y);
    if (unlikely (status))
	return status;

    return _comac_path_fixed_close_path (path);
}

static comac_status_t
_comac_path_fixed_init_from_boxes (comac_path_fixed_t *path,
				   const comac_boxes_t *boxes)
{
    comac_status_t status;
    const struct _comac_boxes_chunk *chunk;
    int i;

    _comac_path_fixed_init (path);
    if (boxes->num_boxes == 0)
	return COMAC_STATUS_SUCCESS;

    for (chunk = &boxes->chunks; chunk; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++) {
	    status = _comac_path_fixed_add_box (path, &chunk->base[i]);
	    if (unlikely (status)) {
		_comac_path_fixed_fini (path);
		return status;
	    }
	}
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_clip_t *
_comac_clip_intersect_clip_path_transformed (comac_clip_t *clip,
					     const comac_clip_path_t *clip_path,
					     const comac_matrix_t *m)
{
    comac_path_fixed_t path;

    if (clip_path->prev)
	clip = _comac_clip_intersect_clip_path_transformed (clip,
							    clip_path->prev,
							    m);

    if (_comac_path_fixed_init_copy (&path, &clip_path->path))
	return _comac_clip_set_all_clipped (clip);

    _comac_path_fixed_transform (&path, m);

    clip =  _comac_clip_intersect_path (clip,
				       &path,
				       clip_path->fill_rule,
				       clip_path->tolerance,
				       clip_path->antialias);
    _comac_path_fixed_fini (&path);

    return clip;
}

comac_clip_t *
_comac_clip_transform (comac_clip_t *clip, const comac_matrix_t *m)
{
    comac_clip_t *copy;

    if (clip == NULL || _comac_clip_is_all_clipped (clip))
	return clip;

    if (_comac_matrix_is_translation (m))
	return _comac_clip_translate (clip, m->x0, m->y0);

    copy = _comac_clip_create ();

    if (clip->num_boxes) {
	comac_path_fixed_t path;
	comac_boxes_t boxes;

	_comac_boxes_init_for_array (&boxes, clip->boxes, clip->num_boxes);
	_comac_path_fixed_init_from_boxes (&path, &boxes);
	_comac_path_fixed_transform (&path, m);

	copy = _comac_clip_intersect_path (copy, &path,
					   COMAC_FILL_RULE_WINDING,
					   0.1,
					   COMAC_ANTIALIAS_DEFAULT);

	_comac_path_fixed_fini (&path);
    }

    if (clip->path)
	copy = _comac_clip_intersect_clip_path_transformed (copy, clip->path,m);

    _comac_clip_destroy (clip);
    return copy;
}

comac_clip_t *
_comac_clip_copy_with_translation (const comac_clip_t *clip, int tx, int ty)
{
    comac_clip_t *copy;
    int fx, fy, i;

    if (clip == NULL || _comac_clip_is_all_clipped (clip))
	return (comac_clip_t *)clip;

    if (tx == 0 && ty == 0)
	return _comac_clip_copy (clip);

    copy = _comac_clip_create ();
    if (copy == NULL)
	    return _comac_clip_set_all_clipped (copy);

    fx = _comac_fixed_from_int (tx);
    fy = _comac_fixed_from_int (ty);

    if (clip->num_boxes) {
	if (clip->num_boxes == 1) {
	    copy->boxes = &copy->embedded_box;
	} else {
	    copy->boxes = _comac_malloc_ab (clip->num_boxes, sizeof (comac_box_t));
	    if (unlikely (copy->boxes == NULL))
		return _comac_clip_set_all_clipped (copy);
	}

	for (i = 0; i < clip->num_boxes; i++) {
	    copy->boxes[i].p1.x = clip->boxes[i].p1.x + fx;
	    copy->boxes[i].p2.x = clip->boxes[i].p2.x + fx;
	    copy->boxes[i].p1.y = clip->boxes[i].p1.y + fy;
	    copy->boxes[i].p2.y = clip->boxes[i].p2.y + fy;
	}
	copy->num_boxes = clip->num_boxes;
    }

    copy->extents = clip->extents;
    copy->extents.x += tx;
    copy->extents.y += ty;

    if (clip->path == NULL)
	return copy;

    return _comac_clip_path_copy_with_translation (copy, clip->path, fx, fy);
}

comac_bool_t
_comac_clip_contains_extents (const comac_clip_t *clip,
			      const comac_composite_rectangles_t *extents)
{
    const comac_rectangle_int_t *rect;

    rect = extents->is_bounded ? &extents->bounded : &extents->unbounded;
    return _comac_clip_contains_rectangle (clip, rect);
}

void
_comac_debug_print_clip (FILE *stream, const comac_clip_t *clip)
{
    int i;

    if (clip == NULL) {
	fprintf (stream, "no clip\n");
	return;
    }

    if (_comac_clip_is_all_clipped (clip)) {
	fprintf (stream, "clip: all-clipped\n");
	return;
    }

    fprintf (stream, "clip:\n");
    fprintf (stream, "  extents: (%d, %d) x (%d, %d), is-region? %d",
	     clip->extents.x, clip->extents.y,
	     clip->extents.width, clip->extents.height,
	     clip->is_region);

    fprintf (stream, "  num_boxes = %d\n", clip->num_boxes);
    for (i = 0; i < clip->num_boxes; i++) {
	fprintf (stream, "  [%d] = (%f, %f), (%f, %f)\n", i,
		 _comac_fixed_to_double (clip->boxes[i].p1.x),
		 _comac_fixed_to_double (clip->boxes[i].p1.y),
		 _comac_fixed_to_double (clip->boxes[i].p2.x),
		 _comac_fixed_to_double (clip->boxes[i].p2.y));
    }

    if (clip->path) {
	comac_clip_path_t *clip_path = clip->path;
	do {
	    fprintf (stream, "path: aa=%d, tolerance=%f, rule=%d: ",
		     clip_path->antialias,
		     clip_path->tolerance,
		     clip_path->fill_rule);
	    _comac_debug_print_path (stream, &clip_path->path);
	    fprintf (stream, "\n");
	} while ((clip_path = clip_path->prev) != NULL);
    }
}

const comac_rectangle_int_t *
_comac_clip_get_extents (const comac_clip_t *clip)
{
    if (clip == NULL)
	return &_comac_unbounded_rectangle;

    if (_comac_clip_is_all_clipped (clip))
	return &_comac_empty_rectangle;

    return &clip->extents;
}

const comac_rectangle_list_t _comac_rectangles_nil =
  { COMAC_STATUS_NO_MEMORY, NULL, 0 };
static const comac_rectangle_list_t _comac_rectangles_not_representable =
  { COMAC_STATUS_CLIP_NOT_REPRESENTABLE, NULL, 0 };

static comac_bool_t
_comac_clip_int_rect_to_user (comac_gstate_t *gstate,
			      comac_rectangle_int_t *clip_rect,
			      comac_rectangle_t *user_rect)
{
    comac_bool_t is_tight;

    double x1 = clip_rect->x;
    double y1 = clip_rect->y;
    double x2 = clip_rect->x + (int) clip_rect->width;
    double y2 = clip_rect->y + (int) clip_rect->height;

    _comac_gstate_backend_to_user_rectangle (gstate,
					     &x1, &y1, &x2, &y2,
					     &is_tight);

    user_rect->x = x1;
    user_rect->y = y1;
    user_rect->width  = x2 - x1;
    user_rect->height = y2 - y1;

    return is_tight;
}

comac_rectangle_list_t *
_comac_rectangle_list_create_in_error (comac_status_t status)
{
    comac_rectangle_list_t *list;

    if (status == COMAC_STATUS_NO_MEMORY)
	return (comac_rectangle_list_t*) &_comac_rectangles_nil;
    if (status == COMAC_STATUS_CLIP_NOT_REPRESENTABLE)
	return (comac_rectangle_list_t*) &_comac_rectangles_not_representable;

    list = _comac_malloc (sizeof (*list));
    if (unlikely (list == NULL)) {
	status = _comac_error (COMAC_STATUS_NO_MEMORY);
	return (comac_rectangle_list_t*) &_comac_rectangles_nil;
    }

    list->status = status;
    list->rectangles = NULL;
    list->num_rectangles = 0;

    return list;
}

comac_rectangle_list_t *
_comac_clip_copy_rectangle_list (comac_clip_t *clip, comac_gstate_t *gstate)
{
#define ERROR_LIST(S) _comac_rectangle_list_create_in_error (_comac_error (S))

    comac_rectangle_list_t *list;
    comac_rectangle_t *rectangles = NULL;
    comac_region_t *region = NULL;
    int n_rects = 0;
    int i;

    if (clip == NULL)
	return ERROR_LIST (COMAC_STATUS_CLIP_NOT_REPRESENTABLE);

    if (_comac_clip_is_all_clipped (clip))
	goto DONE;

    if (! _comac_clip_is_region (clip))
	return ERROR_LIST (COMAC_STATUS_CLIP_NOT_REPRESENTABLE);

    region = _comac_clip_get_region (clip);
    if (region == NULL)
	return ERROR_LIST (COMAC_STATUS_NO_MEMORY);

    n_rects = comac_region_num_rectangles (region);
    if (n_rects) {
	rectangles = _comac_malloc_ab (n_rects, sizeof (comac_rectangle_t));
	if (unlikely (rectangles == NULL)) {
	    return ERROR_LIST (COMAC_STATUS_NO_MEMORY);
	}

	for (i = 0; i < n_rects; ++i) {
	    comac_rectangle_int_t clip_rect;

	    comac_region_get_rectangle (region, i, &clip_rect);

	    if (! _comac_clip_int_rect_to_user (gstate,
						&clip_rect,
						&rectangles[i]))
	    {
		free (rectangles);
		return ERROR_LIST (COMAC_STATUS_CLIP_NOT_REPRESENTABLE);
	    }
	}
    }

 DONE:
    list = _comac_malloc (sizeof (comac_rectangle_list_t));
    if (unlikely (list == NULL)) {
        free (rectangles);
	return ERROR_LIST (COMAC_STATUS_NO_MEMORY);
    }

    list->status = COMAC_STATUS_SUCCESS;
    list->rectangles = rectangles;
    list->num_rectangles = n_rects;
    return list;

#undef ERROR_LIST
}

/**
 * comac_rectangle_list_destroy:
 * @rectangle_list: a rectangle list, as obtained from comac_copy_clip_rectangle_list()
 *
 * Unconditionally frees @rectangle_list and all associated
 * references. After this call, the @rectangle_list pointer must not
 * be dereferenced.
 *
 * Since: 1.4
 **/
void
comac_rectangle_list_destroy (comac_rectangle_list_t *rectangle_list)
{
    if (rectangle_list == NULL || rectangle_list == &_comac_rectangles_nil ||
        rectangle_list == &_comac_rectangles_not_representable)
        return;

    free (rectangle_list->rectangles);
    free (rectangle_list);
}

void
_comac_clip_reset_static_data (void)
{
    _freed_pool_reset (&clip_path_pool);
    _freed_pool_reset (&clip_pool);
}
