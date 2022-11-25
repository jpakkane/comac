/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2011 Intel Corporation
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
 * The Initial Developer of the Original Code is Intel Corporation
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_CONTOUR_PRIVATE_H
#define COMAC_CONTOUR_PRIVATE_H

#include "comac-types-private.h"
#include "comac-compiler-private.h"
#include "comac-error-private.h"
#include "comac-list-private.h"

#include <stdio.h>

COMAC_BEGIN_DECLS

/* A contour is simply a closed chain of points that divide the infinite plane
 * into inside and outside. Each contour is a simple polygon, that is it
 * contains no holes or self-intersections, but maybe either concave or convex.
 */

struct _comac_contour_chain {
    comac_point_t *points;
    int num_points, size_points;
    struct _comac_contour_chain *next;
};

struct _comac_contour_iter {
    comac_point_t *point;
    comac_contour_chain_t *chain;
};

struct _comac_contour {
    comac_list_t next;
    int direction;
    comac_contour_chain_t chain, *tail;

    comac_point_t embedded_points[64];
};

/* Initial definition of a shape is a set of contours (some representing holes) */
struct _comac_shape {
    comac_list_t contours;
};

typedef struct _comac_shape comac_shape_t;

#if 0
comac_private comac_status_t
_comac_shape_init_from_polygon (comac_shape_t *shape,
				const comac_polygon_t *polygon);

comac_private comac_status_t
_comac_shape_reduce (comac_shape_t *shape, double tolerance);
#endif

comac_private void
_comac_contour_init (comac_contour_t *contour, int direction);

comac_private comac_int_status_t
__comac_contour_add_point (comac_contour_t *contour,
			   const comac_point_t *point);

comac_private void
_comac_contour_simplify (comac_contour_t *contour, double tolerance);

comac_private void
_comac_contour_reverse (comac_contour_t *contour);

comac_private comac_int_status_t
_comac_contour_add (comac_contour_t *dst, const comac_contour_t *src);

comac_private comac_int_status_t
_comac_contour_add_reversed (comac_contour_t *dst, const comac_contour_t *src);

comac_private void
__comac_contour_remove_last_chain (comac_contour_t *contour);

comac_private void
_comac_contour_reset (comac_contour_t *contour);

comac_private void
_comac_contour_fini (comac_contour_t *contour);

comac_private void
_comac_debug_print_contour (FILE *file, comac_contour_t *contour);

COMAC_END_DECLS

#endif /* COMAC_CONTOUR_PRIVATE_H */
