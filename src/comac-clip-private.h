/* comac - a vector graphics library with display and print output
 *
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Kristian Høgsberg <krh@redhat.com>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_CLIP_PRIVATE_H
#define COMAC_CLIP_PRIVATE_H

#include "comac-types-private.h"

#include "comac-boxes-private.h"
#include "comac-error-private.h"
#include "comac-compiler-private.h"
#include "comac-error-private.h"
#include "comac-path-fixed-private.h"
#include "comac-reference-count-private.h"

extern const comac_private comac_rectangle_list_t _comac_rectangles_nil;

struct _comac_clip_path {
    comac_reference_count_t ref_count;
    comac_path_fixed_t path;
    comac_fill_rule_t fill_rule;
    double tolerance;
    comac_antialias_t antialias;
    comac_clip_path_t *prev;
};

struct _comac_clip {
    comac_rectangle_int_t extents;
    comac_clip_path_t *path;

    comac_box_t *boxes;
    int num_boxes;

    comac_region_t *region;
    comac_bool_t is_region;

    comac_box_t embedded_box;
};

comac_private comac_clip_t *
_comac_clip_create (void);

comac_private comac_clip_path_t *
_comac_clip_path_reference (comac_clip_path_t *clip_path);

comac_private void
_comac_clip_path_destroy (comac_clip_path_t *clip_path);

comac_private void
_comac_clip_destroy (comac_clip_t *clip);

comac_private extern const comac_clip_t __comac_clip_all;

comac_private comac_clip_t *
_comac_clip_copy (const comac_clip_t *clip);

comac_private comac_clip_t *
_comac_clip_copy_region (const comac_clip_t *clip);

comac_private comac_clip_t *
_comac_clip_copy_path (const comac_clip_t *clip);

comac_private comac_clip_t *
_comac_clip_translate (comac_clip_t *clip, int tx, int ty);

comac_private comac_clip_t *
_comac_clip_transform (comac_clip_t *clip, const comac_matrix_t *m);

comac_private comac_clip_t *
_comac_clip_copy_with_translation (const comac_clip_t *clip, int tx, int ty);

comac_private comac_bool_t
_comac_clip_equal (const comac_clip_t *clip_a, const comac_clip_t *clip_b);

comac_private comac_clip_t *
_comac_clip_intersect_rectangle (comac_clip_t *clip,
				 const comac_rectangle_int_t *rectangle);

comac_private comac_clip_t *
_comac_clip_intersect_clip (comac_clip_t *clip, const comac_clip_t *other);

comac_private comac_clip_t *
_comac_clip_intersect_box (comac_clip_t *clip, const comac_box_t *box);

comac_private comac_clip_t *
_comac_clip_intersect_boxes (comac_clip_t *clip, const comac_boxes_t *boxes);

comac_private comac_clip_t *
_comac_clip_intersect_rectilinear_path (comac_clip_t *clip,
					const comac_path_fixed_t *path,
					comac_fill_rule_t fill_rule,
					comac_antialias_t antialias);

comac_private comac_clip_t *
_comac_clip_intersect_path (comac_clip_t *clip,
			    const comac_path_fixed_t *path,
			    comac_fill_rule_t fill_rule,
			    double tolerance,
			    comac_antialias_t antialias);

comac_private const comac_rectangle_int_t *
_comac_clip_get_extents (const comac_clip_t *clip);

comac_private comac_surface_t *
_comac_clip_get_surface (const comac_clip_t *clip,
			 comac_surface_t *dst,
			 int *tx,
			 int *ty);

comac_private comac_surface_t *
_comac_clip_get_image (const comac_clip_t *clip,
		       comac_surface_t *target,
		       const comac_rectangle_int_t *extents);

comac_private comac_status_t
_comac_clip_combine_with_surface (const comac_clip_t *clip,
				  comac_surface_t *dst,
				  int dst_x,
				  int dst_y);

comac_private comac_clip_t *
_comac_clip_from_boxes (const comac_boxes_t *boxes);

comac_private comac_region_t *
_comac_clip_get_region (const comac_clip_t *clip);

comac_private comac_bool_t
_comac_clip_is_region (const comac_clip_t *clip);

comac_private comac_clip_t *
_comac_clip_reduce_to_rectangle (const comac_clip_t *clip,
				 const comac_rectangle_int_t *r);

comac_private comac_clip_t *
_comac_clip_reduce_for_composite (const comac_clip_t *clip,
				  comac_composite_rectangles_t *extents);

comac_private comac_bool_t
_comac_clip_contains_rectangle (const comac_clip_t *clip,
				const comac_rectangle_int_t *rect);

comac_private comac_bool_t
_comac_clip_contains_box (const comac_clip_t *clip, const comac_box_t *box);

comac_private comac_bool_t
_comac_clip_contains_extents (const comac_clip_t *clip,
			      const comac_composite_rectangles_t *extents);

comac_private comac_rectangle_list_t *
_comac_clip_copy_rectangle_list (comac_clip_t *clip, comac_gstate_t *gstate);

comac_private comac_rectangle_list_t *
_comac_rectangle_list_create_in_error (comac_status_t status);

comac_private comac_bool_t
_comac_clip_is_polygon (const comac_clip_t *clip);

comac_private comac_int_status_t
_comac_clip_get_polygon (const comac_clip_t *clip,
			 comac_polygon_t *polygon,
			 comac_fill_rule_t *fill_rule,
			 comac_antialias_t *antialias);

#endif /* COMAC_CLIP_PRIVATE_H */
