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

#ifndef COMAC_BOXES_H
#define COMAC_BOXES_H

#include "comac-types-private.h"
#include "comac-compiler-private.h"

#include <stdio.h>
#include <stdlib.h>

struct _comac_boxes_t {
    comac_status_t status;

    comac_box_t limit;
    const comac_box_t *limits;
    int num_limits;

    int num_boxes;

    unsigned int is_pixel_aligned;

    struct _comac_boxes_chunk {
	struct _comac_boxes_chunk *next;
	comac_box_t *base;
	int count;
	int size;
    } chunks, *tail;
    comac_box_t boxes_embedded[32];
};

comac_private void
_comac_boxes_init (comac_boxes_t *boxes);

comac_private void
_comac_boxes_init_with_clip (comac_boxes_t *boxes, comac_clip_t *clip);

comac_private void
_comac_boxes_init_for_array (comac_boxes_t *boxes,
			     comac_box_t *array,
			     int num_boxes);

comac_private void
_comac_boxes_init_from_rectangle (
    comac_boxes_t *boxes, int x, int y, int w, int h);

comac_private void
_comac_boxes_limit (comac_boxes_t *boxes,
		    const comac_box_t *limits,
		    int num_limits);

comac_private comac_status_t
_comac_boxes_add (comac_boxes_t *boxes,
		  comac_antialias_t antialias,
		  const comac_box_t *box);

comac_private void
_comac_boxes_extents (const comac_boxes_t *boxes, comac_box_t *box);

comac_private comac_box_t *
_comac_boxes_to_array (const comac_boxes_t *boxes, int *num_boxes);

comac_private comac_status_t
_comac_boxes_intersect (const comac_boxes_t *a,
			const comac_boxes_t *b,
			comac_boxes_t *out);

comac_private void
_comac_boxes_clear (comac_boxes_t *boxes);

comac_private_no_warn comac_bool_t
_comac_boxes_for_each_box (comac_boxes_t *boxes,
			   comac_bool_t (*func) (comac_box_t *box, void *data),
			   void *data);

comac_private comac_status_t
_comac_rasterise_polygon_to_boxes (comac_polygon_t *polygon,
				   comac_fill_rule_t fill_rule,
				   comac_boxes_t *boxes);

comac_private void
_comac_boxes_fini (comac_boxes_t *boxes);

comac_private void
_comac_debug_print_boxes (FILE *stream, const comac_boxes_t *boxes);

#endif /* COMAC_BOXES_H */
