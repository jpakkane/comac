/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

#ifndef COMAC_TRAPS_PRIVATE_H
#define COMAC_TRAPS_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-error-private.h"
#include "comac-types-private.h"

COMAC_BEGIN_DECLS

struct _comac_traps {
    comac_status_t status;

    comac_box_t bounds;
    const comac_box_t *limits;
    int num_limits;

    unsigned int maybe_region : 1; /* hint: 0 implies that it cannot be */
    unsigned int has_intersections : 1;
    unsigned int is_rectilinear : 1;
    unsigned int is_rectangular : 1;

    int num_traps;
    int traps_size;
    comac_trapezoid_t *traps;
    comac_trapezoid_t traps_embedded[16];
};

/* comac-traps.c */
comac_private void
_comac_traps_init (comac_traps_t *traps);

comac_private void
_comac_traps_init_with_clip (comac_traps_t *traps, const comac_clip_t *clip);

comac_private void
_comac_traps_limit (comac_traps_t *traps,
		    const comac_box_t *boxes,
		    int num_boxes);

comac_private comac_status_t
_comac_traps_init_boxes (comac_traps_t *traps, const comac_boxes_t *boxes);

comac_private void
_comac_traps_clear (comac_traps_t *traps);

comac_private void
_comac_traps_fini (comac_traps_t *traps);

#define _comac_traps_status(T) (T)->status

comac_private void
_comac_traps_translate (comac_traps_t *traps, int x, int y);

comac_private void
_comac_traps_tessellate_triangle_with_edges (comac_traps_t *traps,
					     const comac_point_t t[3],
					     const comac_point_t edges[4]);

comac_private void
_comac_traps_tessellate_convex_quad (comac_traps_t *traps,
				     const comac_point_t q[4]);

comac_private comac_status_t
_comac_traps_tessellate_rectangle (comac_traps_t *traps,
				   const comac_point_t *top_left,
				   const comac_point_t *bottom_right);

comac_private void
_comac_traps_add_trap (comac_traps_t *traps,
		       comac_fixed_t top,
		       comac_fixed_t bottom,
		       const comac_line_t *left,
		       const comac_line_t *right);

comac_private int
_comac_traps_contain (const comac_traps_t *traps, double x, double y);

comac_private void
_comac_traps_extents (const comac_traps_t *traps, comac_box_t *extents);

comac_private comac_int_status_t
_comac_traps_extract_region (comac_traps_t *traps,
			     comac_antialias_t antialias,
			     comac_region_t **region);

comac_private comac_bool_t
_comac_traps_to_boxes (comac_traps_t *traps,
		       comac_antialias_t antialias,
		       comac_boxes_t *boxes);

comac_private comac_status_t
_comac_traps_path (const comac_traps_t *traps, comac_path_fixed_t *path);

comac_private comac_int_status_t
_comac_rasterise_polygon_to_traps (comac_polygon_t *polygon,
				   comac_fill_rule_t fill_rule,
				   comac_antialias_t antialias,
				   comac_traps_t *traps);

COMAC_END_DECLS

#endif /* COMAC_TRAPS_PRIVATE_H */
