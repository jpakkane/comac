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
 *	Carl D. Worth <cworth@redhat.com>
 */

#ifndef COMAC_PATTERN_PRIVATE_H
#define COMAC_PATTERN_PRIVATE_H

#include "comac-error-private.h"
#include "comac-types-private.h"
#include "comac-list-private.h"
#include "comac-surface-private.h"

#include <stdio.h> /* FILE* */

COMAC_BEGIN_DECLS

typedef struct _comac_pattern_observer comac_pattern_observer_t;

enum {
    COMAC_PATTERN_NOTIFY_MATRIX = 0x1,
    COMAC_PATTERN_NOTIFY_FILTER = 0x2,
    COMAC_PATTERN_NOTIFY_EXTEND = 0x4,
    COMAC_PATTERN_NOTIFY_OPACITY = 0x9,
};

struct _comac_pattern_observer {
    void (*notify) (comac_pattern_observer_t *,
		    comac_pattern_t *pattern,
		    unsigned int flags);
    comac_list_t link;
};

struct _comac_pattern {
    comac_reference_count_t ref_count;
    comac_status_t status;
    comac_user_data_array_t user_data;
    comac_list_t observers;

    comac_pattern_type_t type;

    comac_filter_t filter;
    comac_extend_t extend;
    comac_bool_t has_component_alpha;
    comac_bool_t is_userfont_foreground;

    comac_matrix_t matrix;
    double opacity;
};

struct _comac_solid_pattern {
    comac_pattern_t base;
    comac_color_t color;
};

typedef struct _comac_surface_pattern {
    comac_pattern_t base;

    comac_surface_t *surface;
} comac_surface_pattern_t;

typedef struct _comac_gradient_stop {
    double offset;
    comac_color_stop_t color;
} comac_gradient_stop_t;

typedef struct _comac_gradient_pattern {
    comac_pattern_t base;

    unsigned int n_stops;
    unsigned int stops_size;
    comac_gradient_stop_t *stops;
    comac_gradient_stop_t stops_embedded[2];
} comac_gradient_pattern_t;

typedef struct _comac_linear_pattern {
    comac_gradient_pattern_t base;

    comac_point_double_t pd1;
    comac_point_double_t pd2;
} comac_linear_pattern_t;

typedef struct _comac_radial_pattern {
    comac_gradient_pattern_t base;

    comac_circle_double_t cd1;
    comac_circle_double_t cd2;
} comac_radial_pattern_t;

typedef union {
    comac_gradient_pattern_t base;

    comac_linear_pattern_t linear;
    comac_radial_pattern_t radial;
} comac_gradient_pattern_union_t;

/*
 * A mesh patch is a tensor-product patch (bicubic Bezier surface
 * patch). It has 16 control points. Each set of 4 points along the
 * sides of the 4x4 grid of control points is a Bezier curve that
 * defines one side of the patch. A color is assigned to each
 * corner. The inner 4 points provide additional control over the
 * shape and the color mapping.
 *
 * Comac uses the same convention as the PDF Reference for numbering
 * the points and side of the patch.
 *
 *
 *                      Side 1
 *
 *          p[0][3] p[1][3] p[2][3] p[3][3]
 * Side 0   p[0][2] p[1][2] p[2][2] p[3][2]  Side 2
 *          p[0][1] p[1][1] p[2][1] p[3][1]
 *          p[0][0] p[1][0] p[2][0] p[3][0]
 *
 *                      Side 3
 *
 *
 *   Point            Color
 *  -------------------------
 *  points[0][0]    colors[0]
 *  points[0][3]    colors[1]
 *  points[3][3]    colors[2]
 *  points[3][0]    colors[3]
 */

typedef struct _comac_mesh_patch {
    comac_point_double_t points[4][4];
    comac_color_t colors[4];
} comac_mesh_patch_t;

typedef struct _comac_mesh_pattern {
    comac_pattern_t base;

    comac_array_t patches;
    comac_mesh_patch_t *current_patch;
    int current_side;
    comac_bool_t has_control_point[4];
    comac_bool_t has_color[4];
} comac_mesh_pattern_t;

typedef struct _comac_raster_source_pattern {
    comac_pattern_t base;

    comac_content_t content;
    comac_rectangle_int_t extents;

    comac_raster_source_acquire_func_t acquire;
    comac_raster_source_release_func_t release;
    comac_raster_source_snapshot_func_t snapshot;
    comac_raster_source_copy_func_t copy;
    comac_raster_source_finish_func_t finish;

    /* an explicit pre-allocated member in preference to the general user-data */
    void *user_data;
} comac_raster_source_pattern_t;

typedef union {
    comac_pattern_t base;

    comac_solid_pattern_t solid;
    comac_surface_pattern_t surface;
    comac_gradient_pattern_union_t gradient;
    comac_mesh_pattern_t mesh;
    comac_raster_source_pattern_t raster_source;
} comac_pattern_union_t;

/* comac-pattern.c */

comac_private comac_pattern_t *
_comac_pattern_create_in_error (comac_status_t status);

comac_private comac_status_t
_comac_pattern_create_copy (comac_pattern_t **pattern,
			    const comac_pattern_t *other);

comac_private void
_comac_pattern_init (comac_pattern_t *pattern, comac_pattern_type_t type);

comac_private comac_status_t
_comac_pattern_init_copy (comac_pattern_t *pattern,
			  const comac_pattern_t *other);

comac_private void
_comac_pattern_init_static_copy (comac_pattern_t *pattern,
				 const comac_pattern_t *other);

comac_private comac_status_t
_comac_pattern_init_snapshot (comac_pattern_t *pattern,
			      const comac_pattern_t *other);

comac_private void
_comac_pattern_init_solid (comac_solid_pattern_t *pattern,
			   const comac_color_t *color);

comac_private void
_comac_pattern_init_for_surface (comac_surface_pattern_t *pattern,
				 comac_surface_t *surface);

comac_private void
_comac_pattern_fini (comac_pattern_t *pattern);

comac_private comac_pattern_t *
_comac_pattern_create_solid (const comac_color_t *color);

comac_private void
_comac_pattern_transform (comac_pattern_t *pattern,
			  const comac_matrix_t *ctm_inverse);

comac_private void
_comac_pattern_pretransform (comac_pattern_t *pattern,
			     const comac_matrix_t *ctm);

comac_private comac_bool_t
_comac_pattern_is_opaque_solid (const comac_pattern_t *pattern);

comac_private comac_bool_t
_comac_pattern_is_opaque (const comac_pattern_t *pattern,
			  const comac_rectangle_int_t *extents);

comac_private comac_bool_t
_comac_pattern_is_clear (const comac_pattern_t *pattern);

comac_private comac_bool_t
_comac_gradient_pattern_is_solid (const comac_gradient_pattern_t *gradient,
				  const comac_rectangle_int_t *extents,
				  comac_color_t *color);

comac_private comac_bool_t
_comac_pattern_is_constant_alpha (const comac_pattern_t *abstract_pattern,
				  const comac_rectangle_int_t *extents,
				  double *alpha);

comac_private void
_comac_gradient_pattern_fit_to_range (const comac_gradient_pattern_t *gradient,
				      double max_value,
				      comac_matrix_t *out_matrix,
				      comac_circle_double_t out_circle[2]);

comac_private comac_bool_t
_comac_radial_pattern_focus_is_inside (const comac_radial_pattern_t *radial);

comac_private void
_comac_gradient_pattern_box_to_parameter (
    const comac_gradient_pattern_t *gradient,
    double x0,
    double y0,
    double x1,
    double y1,
    double tolerance,
    double out_range[2]);

comac_private void
_comac_gradient_pattern_interpolate (const comac_gradient_pattern_t *gradient,
				     double t,
				     comac_circle_double_t *out_circle);

comac_private void
_comac_pattern_alpha_range (const comac_pattern_t *pattern,
			    double *out_min,
			    double *out_max);

comac_private comac_bool_t
_comac_mesh_pattern_coord_box (const comac_mesh_pattern_t *mesh,
			       double *out_xmin,
			       double *out_ymin,
			       double *out_xmax,
			       double *out_ymax);

comac_private void
_comac_pattern_sampled_area (const comac_pattern_t *pattern,
			     const comac_rectangle_int_t *extents,
			     comac_rectangle_int_t *sample);

comac_private void
_comac_pattern_get_extents (const comac_pattern_t *pattern,
			    comac_rectangle_int_t *extents,
			    comac_bool_t is_vector);

comac_private comac_int_status_t
_comac_pattern_get_ink_extents (const comac_pattern_t *pattern,
				comac_rectangle_int_t *extents);

comac_private uintptr_t
_comac_pattern_hash (const comac_pattern_t *pattern);

comac_private uintptr_t
_comac_linear_pattern_hash (uintptr_t hash,
			    const comac_linear_pattern_t *linear);

comac_private uintptr_t
_comac_radial_pattern_hash (uintptr_t hash,
			    const comac_radial_pattern_t *radial);

comac_private comac_bool_t
_comac_linear_pattern_equal (const comac_linear_pattern_t *a,
			     const comac_linear_pattern_t *b);

comac_private unsigned long
_comac_pattern_size (const comac_pattern_t *pattern);

comac_private comac_bool_t
_comac_radial_pattern_equal (const comac_radial_pattern_t *a,
			     const comac_radial_pattern_t *b);

comac_private comac_bool_t
_comac_pattern_equal (const comac_pattern_t *a, const comac_pattern_t *b);

comac_private comac_filter_t
_comac_pattern_analyze_filter (const comac_pattern_t *pattern);

/* comac-mesh-pattern-rasterizer.c */

comac_private void
_comac_mesh_pattern_rasterize (const comac_mesh_pattern_t *mesh,
			       void *data,
			       int width,
			       int height,
			       int stride,
			       double x_offset,
			       double y_offset);

comac_private comac_surface_t *
_comac_raster_source_pattern_acquire (const comac_pattern_t *abstract_pattern,
				      comac_surface_t *target,
				      const comac_rectangle_int_t *extents);

comac_private void
_comac_raster_source_pattern_release (const comac_pattern_t *abstract_pattern,
				      comac_surface_t *surface);

comac_private comac_status_t
_comac_raster_source_pattern_snapshot (comac_pattern_t *abstract_pattern);

comac_private comac_status_t
_comac_raster_source_pattern_init_copy (comac_pattern_t *pattern,
					const comac_pattern_t *other);

comac_private void
_comac_raster_source_pattern_finish (comac_pattern_t *abstract_pattern);

comac_private void
_comac_debug_print_pattern (FILE *file, const comac_pattern_t *pattern);

COMAC_END_DECLS

#endif /* COMAC_PATTERN_PRIVATE */
