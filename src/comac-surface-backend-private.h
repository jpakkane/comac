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

#ifndef COMAC_SURFACE_BACKEND_PRIVATE_H
#define COMAC_SURFACE_BACKEND_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-error-private.h"

COMAC_BEGIN_DECLS

struct _comac_surface_backend {
    comac_surface_type_t type;

    comac_warn comac_status_t (*finish) (void *surface);

    comac_t *(*create_context) (void *surface);

    comac_surface_t *(*create_similar) (void *surface,
					comac_content_t content,
					int width,
					int height);
    comac_surface_t *(*create_similar_image) (void *surface,
					      comac_format_t format,
					      int width,
					      int height);

    comac_image_surface_t *(*map_to_image) (
	void *surface, const comac_rectangle_int_t *extents);
    comac_int_status_t (*unmap_image) (void *surface,
				       comac_image_surface_t *image);

    comac_surface_t *(*source) (void *abstract_surface,
				comac_rectangle_int_t *extents);

    comac_warn comac_status_t (*acquire_source_image) (
	void *abstract_surface,
	comac_image_surface_t **image_out,
	void **image_extra);

    comac_warn void (*release_source_image) (void *abstract_surface,
					     comac_image_surface_t *image_out,
					     void *image_extra);

    comac_surface_t *(*snapshot) (void *surface);

    comac_warn comac_int_status_t (*copy_page) (void *surface);

    comac_warn comac_int_status_t (*show_page) (void *surface);

    /* Get the extents of the current surface. For many surface types
     * this will be as simple as { x=0, y=0, width=surface->width,
     * height=surface->height}.
     *
     * If this function is not implemented, or if it returns
     * FALSE the surface is considered to be
     * boundless and infinite bounds are used for it.
     */
    comac_bool_t (*get_extents) (void *surface, comac_rectangle_int_t *extents);

    void (*get_font_options) (void *surface, comac_font_options_t *options);

    comac_warn comac_status_t (*flush) (void *surface, unsigned flags);

    comac_warn comac_status_t (*mark_dirty_rectangle) (
	void *surface, int x, int y, int width, int height);

    comac_warn comac_int_status_t (*paint) (void *surface,
					    comac_operator_t op,
					    const comac_pattern_t *source,
					    const comac_clip_t *clip);

    comac_warn comac_int_status_t (*mask) (void *surface,
					   comac_operator_t op,
					   const comac_pattern_t *source,
					   const comac_pattern_t *mask,
					   const comac_clip_t *clip);

    comac_warn comac_int_status_t (*stroke) (void *surface,
					     comac_operator_t op,
					     const comac_pattern_t *source,
					     const comac_path_fixed_t *path,
					     const comac_stroke_style_t *style,
					     const comac_matrix_t *ctm,
					     const comac_matrix_t *ctm_inverse,
					     double tolerance,
					     comac_antialias_t antialias,
					     const comac_clip_t *clip);

    comac_warn comac_int_status_t (*fill) (void *surface,
					   comac_operator_t op,
					   const comac_pattern_t *source,
					   const comac_path_fixed_t *path,
					   comac_fill_rule_t fill_rule,
					   double tolerance,
					   comac_antialias_t antialias,
					   const comac_clip_t *clip);

    comac_warn comac_int_status_t (*fill_stroke) (
	void *surface,
	comac_operator_t fill_op,
	const comac_pattern_t *fill_source,
	comac_fill_rule_t fill_rule,
	double fill_tolerance,
	comac_antialias_t fill_antialias,
	const comac_path_fixed_t *path,
	comac_operator_t stroke_op,
	const comac_pattern_t *stroke_source,
	const comac_stroke_style_t *stroke_style,
	const comac_matrix_t *stroke_ctm,
	const comac_matrix_t *stroke_ctm_inverse,
	double stroke_tolerance,
	comac_antialias_t stroke_antialias,
	const comac_clip_t *clip);

    comac_warn
	comac_int_status_t (*show_glyphs) (void *surface,
					   comac_operator_t op,
					   const comac_pattern_t *source,
					   comac_glyph_t *glyphs,
					   int num_glyphs,
					   comac_scaled_font_t *scaled_font,
					   const comac_clip_t *clip);

    comac_bool_t (*has_show_text_glyphs) (void *surface);

    comac_warn comac_int_status_t (*show_text_glyphs) (
	void *surface,
	comac_operator_t op,
	const comac_pattern_t *source,
	const char *utf8,
	int utf8_len,
	comac_glyph_t *glyphs,
	int num_glyphs,
	const comac_text_cluster_t *clusters,
	int num_clusters,
	comac_text_cluster_flags_t cluster_flags,
	comac_scaled_font_t *scaled_font,
	const comac_clip_t *clip);

    const char **(*get_supported_mime_types) (void *surface);

    comac_warn comac_int_status_t (*tag) (void *surface,
					  comac_bool_t begin,
					  const char *tag_name,
					  const char *attributes);
};

comac_private comac_status_t
_comac_surface_default_acquire_source_image (void *surface,
					     comac_image_surface_t **image_out,
					     void **image_extra);

comac_private void
_comac_surface_default_release_source_image (void *surface,
					     comac_image_surface_t *image,
					     void *image_extra);

comac_private comac_surface_t *
_comac_surface_default_source (void *surface, comac_rectangle_int_t *extents);

COMAC_END_DECLS

#endif /* COMAC_SURFACE_BACKEND_PRIVATE_H */
