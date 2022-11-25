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

static void
_comac_clip_extract_region (comac_clip_t *clip)
{
    comac_rectangle_int_t
	stack_rects[COMAC_STACK_ARRAY_LENGTH (comac_rectangle_int_t)];
    comac_rectangle_int_t *r = stack_rects;
    comac_bool_t is_region;
    int i;

    if (clip->num_boxes == 0)
	return;

    if (clip->num_boxes > ARRAY_LENGTH (stack_rects)) {
	r = _comac_malloc_ab (clip->num_boxes, sizeof (comac_rectangle_int_t));
	if (r == NULL) {
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return;
	}
    }

    is_region = clip->path == NULL;
    for (i = 0; i < clip->num_boxes; i++) {
	comac_box_t *b = &clip->boxes[i];
	if (is_region)
	    is_region =
		_comac_fixed_is_integer (b->p1.x | b->p1.y | b->p2.x | b->p2.y);
	r[i].x = _comac_fixed_integer_floor (b->p1.x);
	r[i].y = _comac_fixed_integer_floor (b->p1.y);
	r[i].width = _comac_fixed_integer_ceil (b->p2.x) - r[i].x;
	r[i].height = _comac_fixed_integer_ceil (b->p2.y) - r[i].y;
    }
    clip->is_region = is_region;

    clip->region = comac_region_create_rectangles (r, i);

    if (r != stack_rects)
	free (r);
}

comac_region_t *
_comac_clip_get_region (const comac_clip_t *clip)
{
    if (clip == NULL)
	return NULL;

    if (clip->region == NULL)
	_comac_clip_extract_region ((comac_clip_t *) clip);

    return clip->region;
}

comac_bool_t
_comac_clip_is_region (const comac_clip_t *clip)
{
    if (clip == NULL)
	return TRUE;

    if (clip->is_region)
	return TRUE;

    /* XXX Geometric reduction? */

    if (clip->path)
	return FALSE;

    if (clip->num_boxes == 0)
	return TRUE;

    if (clip->region == NULL)
	_comac_clip_extract_region ((comac_clip_t *) clip);

    return clip->is_region;
}
