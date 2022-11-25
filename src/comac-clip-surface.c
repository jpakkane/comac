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
#include "comac-clip-private.h"
#include "comac-error-private.h"
#include "comac-freed-pool-private.h"
#include "comac-gstate-private.h"
#include "comac-path-fixed-private.h"
#include "comac-pattern-private.h"
#include "comac-composite-rectangles-private.h"
#include "comac-region-private.h"

comac_status_t
_comac_clip_combine_with_surface (const comac_clip_t *clip,
				  comac_surface_t *dst,
				  int dst_x, int dst_y)
{
    comac_clip_path_t *copy_path;
    comac_clip_path_t *clip_path;
    comac_clip_t *copy;
    comac_status_t status = COMAC_STATUS_SUCCESS;

    copy = _comac_clip_copy_with_translation (clip, -dst_x, -dst_y);
    copy_path = copy->path;
    copy->path = NULL;

    if (copy->boxes) {
	status = _comac_surface_paint (dst,
				       COMAC_OPERATOR_IN,
				       &_comac_pattern_white.base,
				       copy);
    }

    clip = NULL;
    if (_comac_clip_is_region (copy))
	clip = copy;
    clip_path = copy_path;
    while (status == COMAC_STATUS_SUCCESS && clip_path) {
	status = _comac_surface_fill (dst,
				      COMAC_OPERATOR_IN,
				      &_comac_pattern_white.base,
				      &clip_path->path,
				      clip_path->fill_rule,
				      clip_path->tolerance,
				      clip_path->antialias,
				      clip);
	clip_path = clip_path->prev;
    }

    copy->path = copy_path;
    _comac_clip_destroy (copy);
    return status;
}

static comac_status_t
_comac_path_fixed_add_box (comac_path_fixed_t *path,
			   const comac_box_t *box,
			   comac_fixed_t fx,
			   comac_fixed_t fy)
{
    comac_status_t status;

    status = _comac_path_fixed_move_to (path, box->p1.x + fx, box->p1.y + fy);
    if (unlikely (status))
	return status;

    status = _comac_path_fixed_line_to (path, box->p2.x + fx, box->p1.y + fy);
    if (unlikely (status))
	return status;

    status = _comac_path_fixed_line_to (path, box->p2.x + fx, box->p2.y + fy);
    if (unlikely (status))
	return status;

    status = _comac_path_fixed_line_to (path, box->p1.x + fx, box->p2.y + fy);
    if (unlikely (status))
	return status;

    return _comac_path_fixed_close_path (path);
}

comac_surface_t *
_comac_clip_get_surface (const comac_clip_t *clip,
			 comac_surface_t *target,
			 int *tx, int *ty)
{
    comac_surface_t *surface;
    comac_status_t status;
    comac_clip_t *copy, *region;
    comac_clip_path_t *copy_path, *clip_path;

    if (clip->num_boxes) {
	comac_path_fixed_t path;
	int i;

	surface = _comac_surface_create_scratch (target,
						 COMAC_CONTENT_ALPHA,
						 clip->extents.width,
						 clip->extents.height,
						 COMAC_COLOR_TRANSPARENT);
	if (unlikely (surface->status))
	    return surface;

	_comac_path_fixed_init (&path);
	status = COMAC_STATUS_SUCCESS;
	for (i = 0; status == COMAC_STATUS_SUCCESS && i < clip->num_boxes; i++) {
	    status = _comac_path_fixed_add_box (&path, &clip->boxes[i],
						-_comac_fixed_from_int (clip->extents.x),
						-_comac_fixed_from_int (clip->extents.y));
	}
	if (status == COMAC_STATUS_SUCCESS)
	    status = _comac_surface_fill (surface,
					  COMAC_OPERATOR_ADD,
					  &_comac_pattern_white.base,
					  &path,
					  COMAC_FILL_RULE_WINDING,
					  1.,
					  COMAC_ANTIALIAS_DEFAULT,
					  NULL);
	_comac_path_fixed_fini (&path);
	if (unlikely (status)) {
	    comac_surface_destroy (surface);
	    return _comac_surface_create_in_error (status);
	}
    } else {
	surface = _comac_surface_create_scratch (target,
						 COMAC_CONTENT_ALPHA,
						 clip->extents.width,
						 clip->extents.height,
						 COMAC_COLOR_WHITE);
	if (unlikely (surface->status))
	    return surface;
    }

    copy = _comac_clip_copy_with_translation (clip,
					      -clip->extents.x,
					      -clip->extents.y);
    copy_path = copy->path;
    copy->path = NULL;

    region = copy;
    if (! _comac_clip_is_region (copy))
	region = _comac_clip_copy_region (copy);

    status = COMAC_STATUS_SUCCESS;
    clip_path = copy_path;
    while (status == COMAC_STATUS_SUCCESS && clip_path) {
	status = _comac_surface_fill (surface,
				      COMAC_OPERATOR_IN,
				      &_comac_pattern_white.base,
				      &clip_path->path,
				      clip_path->fill_rule,
				      clip_path->tolerance,
				      clip_path->antialias,
				      region);
	clip_path = clip_path->prev;
    }

    copy->path = copy_path;
    _comac_clip_destroy (copy);
    if (region != copy)
	_comac_clip_destroy (region);

    if (unlikely (status)) {
	comac_surface_destroy (surface);
	return _comac_surface_create_in_error (status);
    }

    *tx = clip->extents.x;
    *ty = clip->extents.y;
    return surface;
}

comac_surface_t *
_comac_clip_get_image (const comac_clip_t *clip,
		       comac_surface_t *target,
		       const comac_rectangle_int_t *extents)
{
    comac_surface_t *surface;
    comac_status_t status;

    surface = comac_surface_create_similar_image (target,
						  COMAC_FORMAT_A8,
						  extents->width,
						  extents->height);
    if (unlikely (surface->status))
	return surface;

    status = _comac_surface_paint (surface, COMAC_OPERATOR_SOURCE,
				   &_comac_pattern_white.base, NULL);
    if (likely (status == COMAC_STATUS_SUCCESS))
	status = _comac_clip_combine_with_surface (clip, surface,
						   extents->x, extents->y);

    if (unlikely (status)) {
	comac_surface_destroy (surface);
	surface = _comac_surface_create_in_error (status);
    }

    return surface;
}
