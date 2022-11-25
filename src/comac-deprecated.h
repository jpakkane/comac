/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2006 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@cworth.org>
 */

#ifndef COMAC_DEPRECATED_H
#define COMAC_DEPRECATED_H

#define COMAC_FONT_TYPE_ATSUI COMAC_FONT_TYPE_QUARTZ

/* Obsolete functions. These definitions exist to coerce the compiler
 * into providing a little bit of guidance with its error
 * messages. The idea is to help users port their old code without
 * having to dig through lots of documentation.
 *
 * The first set of REPLACED_BY functions is for functions whose names
 * have just been changed. So fixing these up is mechanical, (and
 * automated by means of the comac/util/comac-api-update script.
 *
 * The second set of DEPRECATED_BY functions is for functions where
 * the replacement is used in a different way, (ie. different
 * arguments, multiple functions instead of one, etc). Fixing these up
 * will require a bit more work on the user's part, (and hopefully we
 * can get comac-api-update to find these and print some guiding
 * information).
 */
#define comac_current_font_extents   comac_current_font_extents_REPLACED_BY_comac_font_extents
#define comac_get_font_extents       comac_get_font_extents_REPLACED_BY_comac_font_extents
#define comac_current_operator       comac_current_operator_REPLACED_BY_comac_get_operator
#define comac_current_tolerance	     comac_current_tolerance_REPLACED_BY_comac_get_tolerance
#define comac_current_point	     comac_current_point_REPLACED_BY_comac_get_current_point
#define comac_current_fill_rule	     comac_current_fill_rule_REPLACED_BY_comac_get_fill_rule
#define comac_current_line_width     comac_current_line_width_REPLACED_BY_comac_get_line_width
#define comac_current_line_cap       comac_current_line_cap_REPLACED_BY_comac_get_line_cap
#define comac_current_line_join      comac_current_line_join_REPLACED_BY_comac_get_line_join
#define comac_current_miter_limit    comac_current_miter_limit_REPLACED_BY_comac_get_miter_limit
#define comac_current_matrix         comac_current_matrix_REPLACED_BY_comac_get_matrix
#define comac_current_target_surface comac_current_target_surface_REPLACED_BY_comac_get_target
#define comac_get_status             comac_get_status_REPLACED_BY_comac_status
#define comac_concat_matrix		 comac_concat_matrix_REPLACED_BY_comac_transform
#define comac_scale_font                 comac_scale_font_REPLACED_BY_comac_set_font_size
#define comac_select_font                comac_select_font_REPLACED_BY_comac_select_font_face
#define comac_transform_font             comac_transform_font_REPLACED_BY_comac_set_font_matrix
#define comac_transform_point		 comac_transform_point_REPLACED_BY_comac_user_to_device
#define comac_transform_distance	 comac_transform_distance_REPLACED_BY_comac_user_to_device_distance
#define comac_inverse_transform_point	 comac_inverse_transform_point_REPLACED_BY_comac_device_to_user
#define comac_inverse_transform_distance comac_inverse_transform_distance_REPLACED_BY_comac_device_to_user_distance
#define comac_init_clip			 comac_init_clip_REPLACED_BY_comac_reset_clip
#define comac_surface_create_for_image	 comac_surface_create_for_image_REPLACED_BY_comac_image_surface_create_for_data
#define comac_default_matrix		 comac_default_matrix_REPLACED_BY_comac_identity_matrix
#define comac_matrix_set_affine		 comac_matrix_set_affine_REPLACED_BY_comac_matrix_init
#define comac_matrix_set_identity	 comac_matrix_set_identity_REPLACED_BY_comac_matrix_init_identity
#define comac_pattern_add_color_stop	 comac_pattern_add_color_stop_REPLACED_BY_comac_pattern_add_color_stop_rgba
#define comac_set_rgb_color		 comac_set_rgb_color_REPLACED_BY_comac_set_source_rgb
#define comac_set_pattern		 comac_set_pattern_REPLACED_BY_comac_set_source
#define comac_xlib_surface_create_for_pixmap_with_visual	comac_xlib_surface_create_for_pixmap_with_visual_REPLACED_BY_comac_xlib_surface_create
#define comac_xlib_surface_create_for_window_with_visual	comac_xlib_surface_create_for_window_with_visual_REPLACED_BY_comac_xlib_surface_create
#define comac_xcb_surface_create_for_pixmap_with_visual	comac_xcb_surface_create_for_pixmap_with_visual_REPLACED_BY_comac_xcb_surface_create
#define comac_xcb_surface_create_for_window_with_visual	comac_xcb_surface_create_for_window_with_visual_REPLACED_BY_comac_xcb_surface_create
#define comac_ps_surface_set_dpi	comac_ps_surface_set_dpi_REPLACED_BY_comac_surface_set_fallback_resolution
#define comac_pdf_surface_set_dpi	comac_pdf_surface_set_dpi_REPLACED_BY_comac_surface_set_fallback_resolution
#define comac_svg_surface_set_dpi	comac_svg_surface_set_dpi_REPLACED_BY_comac_surface_set_fallback_resolution
#define comac_atsui_font_face_create_for_atsu_font_id  comac_atsui_font_face_create_for_atsu_font_id_REPLACED_BY_comac_quartz_font_face_create_for_atsu_font_id

#define comac_current_path	     comac_current_path_DEPRECATED_BY_comac_copy_path
#define comac_current_path_flat	     comac_current_path_flat_DEPRECATED_BY_comac_copy_path_flat
#define comac_get_path		     comac_get_path_DEPRECATED_BY_comac_copy_path
#define comac_get_path_flat	     comac_get_path_flat_DEPRECATED_BY_comac_get_path_flat
#define comac_set_alpha		     comac_set_alpha_DEPRECATED_BY_comac_set_source_rgba_OR_comac_paint_with_alpha
#define comac_show_surface	     comac_show_surface_DEPRECATED_BY_comac_set_source_surface_AND_comac_paint
#define comac_copy		     comac_copy_DEPRECATED_BY_comac_create_AND_MANY_INDIVIDUAL_FUNCTIONS
#define comac_surface_set_repeat	comac_surface_set_repeat_DEPRECATED_BY_comac_pattern_set_extend
#define comac_surface_set_matrix	comac_surface_set_matrix_DEPRECATED_BY_comac_pattern_set_matrix
#define comac_surface_get_matrix	comac_surface_get_matrix_DEPRECATED_BY_comac_pattern_get_matrix
#define comac_surface_set_filter	comac_surface_set_filter_DEPRECATED_BY_comac_pattern_set_filter
#define comac_surface_get_filter	comac_surface_get_filter_DEPRECATED_BY_comac_pattern_get_filter
#define comac_matrix_create		comac_matrix_create_DEPRECATED_BY_comac_matrix_t
#define comac_matrix_destroy		comac_matrix_destroy_DEPRECATED_BY_comac_matrix_t
#define comac_matrix_copy		comac_matrix_copy_DEPRECATED_BY_comac_matrix_t
#define comac_matrix_get_affine		comac_matrix_get_affine_DEPRECATED_BY_comac_matrix_t
#define comac_set_target_surface	comac_set_target_surface_DEPRECATED_BY_comac_create
#define comac_set_target_image		comac_set_target_image_DEPRECATED_BY_comac_image_surface_create_for_data
#define comac_set_target_pdf		comac_set_target_pdf_DEPRECATED_BY_comac_pdf_surface_create
#define comac_set_target_png		comac_set_target_png_DEPRECATED_BY_comac_surface_write_to_png
#define comac_set_target_ps		comac_set_target_ps_DEPRECATED_BY_comac_ps_surface_create
#define comac_set_target_quartz		comac_set_target_quartz_DEPRECATED_BY_comac_quartz_surface_create
#define comac_set_target_win32		comac_set_target_win32_DEPRECATED_BY_comac_win32_surface_create
#define comac_set_target_xcb		comac_set_target_xcb_DEPRECATED_BY_comac_xcb_surface_create
#define comac_set_target_drawable	comac_set_target_drawable_DEPRECATED_BY_comac_xlib_surface_create
#define comac_get_status_string		comac_get_status_string_DEPRECATED_BY_comac_status_AND_comac_status_to_string
#define comac_status_string		comac_status_string_DEPRECATED_BY_comac_status_AND_comac_status_to_string

#endif /* COMAC_DEPRECATED_H */
