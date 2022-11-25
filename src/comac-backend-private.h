/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2010 Intel Corporation
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

#ifndef COMAC_BACKEND_PRIVATE_H
#define COMAC_BACKEND_PRIVATE_H

#include "comac-types-private.h"
#include "comac-private.h"

typedef enum _comac_backend_type {
    COMAC_TYPE_DEFAULT,
    COMAC_TYPE_SKIA,
} comac_backend_type_t;

struct _comac_backend {
    comac_backend_type_t type;
    void (*destroy) (void *cr);

    comac_surface_t *(*get_original_target) (void *cr);
    comac_surface_t *(*get_current_target) (void *cr);

    comac_status_t (*save) (void *cr);
    comac_status_t (*restore) (void *cr);

    comac_status_t (*push_group) (void *cr, comac_content_t content);
    comac_pattern_t *(*pop_group) (void *cr);

    comac_status_t (*set_source_rgba) (
	void *cr, double red, double green, double blue, double alpha);
    comac_status_t (*set_source_surface) (void *cr,
					  comac_surface_t *surface,
					  double x,
					  double y);
    comac_status_t (*set_source) (void *cr, comac_pattern_t *source);
    comac_pattern_t *(*get_source) (void *cr);

    comac_status_t (*set_antialias) (void *cr, comac_antialias_t antialias);
    comac_status_t (*set_dash) (void *cr,
				const double *dashes,
				int num_dashes,
				double offset);
    comac_status_t (*set_fill_rule) (void *cr, comac_fill_rule_t fill_rule);
    comac_status_t (*set_line_cap) (void *cr, comac_line_cap_t line_cap);
    comac_status_t (*set_line_join) (void *cr, comac_line_join_t line_join);
    comac_status_t (*set_line_width) (void *cr, double line_width);
    comac_status_t (*set_hairline) (void *cr, comac_bool_t set_hairline);
    comac_status_t (*set_miter_limit) (void *cr, double limit);
    comac_status_t (*set_opacity) (void *cr, double opacity);
    comac_status_t (*set_operator) (void *cr, comac_operator_t op);
    comac_status_t (*set_tolerance) (void *cr, double tolerance);

    comac_antialias_t (*get_antialias) (void *cr);
    void (*get_dash) (void *cr,
		      double *dashes,
		      int *num_dashes,
		      double *offset);
    comac_fill_rule_t (*get_fill_rule) (void *cr);
    comac_line_cap_t (*get_line_cap) (void *cr);
    comac_line_join_t (*get_line_join) (void *cr);
    double (*get_line_width) (void *cr);
    comac_bool_t (*get_hairline) (void *cr);
    double (*get_miter_limit) (void *cr);
    double (*get_opacity) (void *cr);
    comac_operator_t (*get_operator) (void *cr);
    double (*get_tolerance) (void *cr);

    comac_status_t (*translate) (void *cr, double tx, double ty);
    comac_status_t (*scale) (void *cr, double sx, double sy);
    comac_status_t (*rotate) (void *cr, double theta);
    comac_status_t (*transform) (void *cr, const comac_matrix_t *matrix);
    comac_status_t (*set_matrix) (void *cr, const comac_matrix_t *matrix);
    comac_status_t (*set_identity_matrix) (void *cr);
    void (*get_matrix) (void *cr, comac_matrix_t *matrix);

    void (*user_to_device) (void *cr, double *x, double *y);
    void (*user_to_device_distance) (void *cr, double *x, double *y);
    void (*device_to_user) (void *cr, double *x, double *y);
    void (*device_to_user_distance) (void *cr, double *x, double *y);

    void (*user_to_backend) (void *cr, double *x, double *y);
    void (*user_to_backend_distance) (void *cr, double *x, double *y);
    void (*backend_to_user) (void *cr, double *x, double *y);
    void (*backend_to_user_distance) (void *cr, double *x, double *y);

    comac_status_t (*new_path) (void *cr);
    comac_status_t (*new_sub_path) (void *cr);
    comac_status_t (*move_to) (void *cr, double x, double y);
    comac_status_t (*rel_move_to) (void *cr, double dx, double dy);
    comac_status_t (*line_to) (void *cr, double x, double y);
    comac_status_t (*rel_line_to) (void *cr, double dx, double dy);
    comac_status_t (*curve_to) (void *cr,
				double x1,
				double y1,
				double x2,
				double y2,
				double x3,
				double y3);
    comac_status_t (*rel_curve_to) (void *cr,
				    double dx1,
				    double dy1,
				    double dx2,
				    double dy2,
				    double dx3,
				    double dy3);
    comac_status_t (*arc_to) (
	void *cr, double x1, double y1, double x2, double y2, double radius);
    comac_status_t (*rel_arc_to) (void *cr,
				  double dx1,
				  double dy1,
				  double dx2,
				  double dy2,
				  double radius);
    comac_status_t (*close_path) (void *cr);

    comac_status_t (*arc) (void *cr,
			   double xc,
			   double yc,
			   double radius,
			   double angle1,
			   double angle2,
			   comac_bool_t forward);
    comac_status_t (*rectangle) (
	void *cr, double x, double y, double width, double height);

    void (*path_extents) (
	void *cr, double *x1, double *y1, double *x2, double *y2);
    comac_bool_t (*has_current_point) (void *cr);
    comac_bool_t (*get_current_point) (void *cr, double *x, double *y);

    comac_path_t *(*copy_path) (void *cr);
    comac_path_t *(*copy_path_flat) (void *cr);
    comac_status_t (*append_path) (void *cr, const comac_path_t *path);

    comac_status_t (*stroke_to_path) (void *cr);

    comac_status_t (*clip) (void *cr);
    comac_status_t (*clip_preserve) (void *cr);
    comac_status_t (*in_clip) (void *cr,
			       double x,
			       double y,
			       comac_bool_t *inside);
    comac_status_t (*clip_extents) (
	void *cr, double *x1, double *y1, double *x2, double *y2);
    comac_status_t (*reset_clip) (void *cr);
    comac_rectangle_list_t *(*clip_copy_rectangle_list) (void *cr);

    comac_status_t (*paint) (void *cr);
    comac_status_t (*paint_with_alpha) (void *cr, double opacity);
    comac_status_t (*mask) (void *cr, comac_pattern_t *pattern);

    comac_status_t (*stroke) (void *cr);
    comac_status_t (*stroke_preserve) (void *cr);
    comac_status_t (*in_stroke) (void *cr,
				 double x,
				 double y,
				 comac_bool_t *inside);
    comac_status_t (*stroke_extents) (
	void *cr, double *x1, double *y1, double *x2, double *y2);

    comac_status_t (*fill) (void *cr);
    comac_status_t (*fill_preserve) (void *cr);
    comac_status_t (*in_fill) (void *cr,
			       double x,
			       double y,
			       comac_bool_t *inside);
    comac_status_t (*fill_extents) (
	void *cr, double *x1, double *y1, double *x2, double *y2);

    comac_status_t (*set_font_face) (void *cr, comac_font_face_t *font_face);
    comac_font_face_t *(*get_font_face) (void *cr);
    comac_status_t (*set_font_size) (void *cr, double size);
    comac_status_t (*set_font_matrix) (void *cr, const comac_matrix_t *matrix);
    void (*get_font_matrix) (void *cr, comac_matrix_t *matrix);
    comac_status_t (*set_font_options) (void *cr,
					const comac_font_options_t *options);
    void (*get_font_options) (void *cr, comac_font_options_t *options);
    comac_status_t (*set_scaled_font) (void *cr,
				       comac_scaled_font_t *scaled_font);
    comac_scaled_font_t *(*get_scaled_font) (void *cr);
    comac_status_t (*font_extents) (void *cr, comac_font_extents_t *extents);

    comac_status_t (*glyphs) (void *cr,
			      const comac_glyph_t *glyphs,
			      int num_glyphs,
			      comac_glyph_text_info_t *info);
    comac_status_t (*glyph_path) (void *cr,
				  const comac_glyph_t *glyphs,
				  int num_glyphs);

    comac_status_t (*glyph_extents) (void *cr,
				     const comac_glyph_t *glyphs,
				     int num_glyphs,
				     comac_text_extents_t *extents);

    comac_status_t (*copy_page) (void *cr);
    comac_status_t (*show_page) (void *cr);

    comac_status_t (*tag_begin) (void *cr,
				 const char *tag_name,
				 const char *attributes);
    comac_status_t (*tag_end) (void *cr, const char *tag_name);
};

static inline void
_comac_backend_to_user (comac_t *cr, double *x, double *y)
{
    cr->backend->backend_to_user (cr, x, y);
}

static inline void
_comac_backend_to_user_distance (comac_t *cr, double *x, double *y)
{
    cr->backend->backend_to_user_distance (cr, x, y);
}

static inline void
_comac_user_to_backend (comac_t *cr, double *x, double *y)
{
    cr->backend->user_to_backend (cr, x, y);
}

static inline void
_comac_user_to_backend_distance (comac_t *cr, double *x, double *y)
{
    cr->backend->user_to_backend_distance (cr, x, y);
}

#endif /* COMAC_BACKEND_PRIVATE_H */
