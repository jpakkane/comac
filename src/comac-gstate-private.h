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

#ifndef COMAC_GSTATE_PRIVATE_H
#define COMAC_GSTATE_PRIVATE_H

#include "comac-clip-private.h"

struct _comac_gstate {
    comac_operator_t op;

    double opacity;
    double tolerance;
    comac_antialias_t antialias;

    comac_stroke_style_t stroke_style;

    comac_fill_rule_t fill_rule;

    comac_font_face_t *font_face;
    comac_scaled_font_t *scaled_font;	/* Specific to the current CTM */
    comac_scaled_font_t *previous_scaled_font;	/* holdover */
    comac_matrix_t font_matrix;
    comac_font_options_t font_options;

    comac_clip_t *clip;

    comac_surface_t *target;		/* The target to which all rendering is directed */
    comac_surface_t *parent_target;	/* The previous target which was receiving rendering */
    comac_surface_t *original_target;	/* The original target the initial gstate was created with */

    /* the user is allowed to update the device after we have cached the matrices... */
    comac_observer_t device_transform_observer;

    comac_matrix_t ctm;
    comac_matrix_t ctm_inverse;
    comac_matrix_t source_ctm_inverse; /* At the time ->source was set */
    comac_bool_t is_identity;

    comac_pattern_t *source;

    struct _comac_gstate *next;
};

/* comac-gstate.c */
comac_private comac_status_t
_comac_gstate_init (comac_gstate_t  *gstate,
		    comac_surface_t *target);

comac_private void
_comac_gstate_fini (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_save (comac_gstate_t **gstate, comac_gstate_t **freelist);

comac_private comac_status_t
_comac_gstate_restore (comac_gstate_t **gstate, comac_gstate_t **freelist);

comac_private comac_bool_t
_comac_gstate_is_group (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_redirect_target (comac_gstate_t *gstate, comac_surface_t *child);

comac_private comac_surface_t *
_comac_gstate_get_target (comac_gstate_t *gstate);

comac_private comac_surface_t *
_comac_gstate_get_original_target (comac_gstate_t *gstate);

comac_private comac_clip_t *
_comac_gstate_get_clip (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_source (comac_gstate_t *gstate, comac_pattern_t *source);

comac_private comac_pattern_t *
_comac_gstate_get_source (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_operator (comac_gstate_t *gstate, comac_operator_t op);

comac_private comac_operator_t
_comac_gstate_get_operator (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_opacity (comac_gstate_t *gstate, double opacity);

comac_private double
_comac_gstate_get_opacity (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_tolerance (comac_gstate_t *gstate, double tolerance);

comac_private double
_comac_gstate_get_tolerance (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_fill_rule (comac_gstate_t *gstate, comac_fill_rule_t fill_rule);

comac_private comac_fill_rule_t
_comac_gstate_get_fill_rule (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_line_width (comac_gstate_t *gstate, double width);

comac_private double
_comac_gstate_get_line_width (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_hairline (comac_gstate_t *gstate, comac_bool_t set_hairline);

comac_private comac_bool_t
_comac_gstate_get_hairline (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_line_cap (comac_gstate_t *gstate, comac_line_cap_t line_cap);

comac_private comac_line_cap_t
_comac_gstate_get_line_cap (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_line_join (comac_gstate_t *gstate, comac_line_join_t line_join);

comac_private comac_line_join_t
_comac_gstate_get_line_join (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_set_dash (comac_gstate_t *gstate, const double *dash, int num_dashes, double offset);

comac_private void
_comac_gstate_get_dash (comac_gstate_t *gstate, double *dash, int *num_dashes, double *offset);

comac_private comac_status_t
_comac_gstate_set_miter_limit (comac_gstate_t *gstate, double limit);

comac_private double
_comac_gstate_get_miter_limit (comac_gstate_t *gstate);

comac_private void
_comac_gstate_get_matrix (comac_gstate_t *gstate, comac_matrix_t *matrix);

comac_private comac_status_t
_comac_gstate_translate (comac_gstate_t *gstate, double tx, double ty);

comac_private comac_status_t
_comac_gstate_scale (comac_gstate_t *gstate, double sx, double sy);

comac_private comac_status_t
_comac_gstate_rotate (comac_gstate_t *gstate, double angle);

comac_private comac_status_t
_comac_gstate_transform (comac_gstate_t	      *gstate,
			 const comac_matrix_t *matrix);

comac_private comac_status_t
_comac_gstate_set_matrix (comac_gstate_t       *gstate,
			  const comac_matrix_t *matrix);

comac_private void
_comac_gstate_identity_matrix (comac_gstate_t *gstate);

comac_private void
_comac_gstate_user_to_device (comac_gstate_t *gstate, double *x, double *y);

comac_private void
_comac_gstate_user_to_device_distance (comac_gstate_t *gstate, double *dx, double *dy);

comac_private void
_comac_gstate_device_to_user (comac_gstate_t *gstate, double *x, double *y);

comac_private void
_comac_gstate_device_to_user_distance (comac_gstate_t *gstate, double *dx, double *dy);

comac_private void
_do_comac_gstate_user_to_backend (comac_gstate_t *gstate, double *x, double *y);

static inline void
_comac_gstate_user_to_backend (comac_gstate_t *gstate, double *x, double *y)
{
    if (! gstate->is_identity)
	_do_comac_gstate_user_to_backend (gstate, x, y);
}

comac_private void
_do_comac_gstate_user_to_backend_distance (comac_gstate_t *gstate, double *x, double *y);

static inline void
_comac_gstate_user_to_backend_distance (comac_gstate_t *gstate, double *x, double *y)
{
    if (! gstate->is_identity)
	_do_comac_gstate_user_to_backend_distance (gstate, x, y);
}

comac_private void
_do_comac_gstate_backend_to_user (comac_gstate_t *gstate, double *x, double *y);

static inline void
_comac_gstate_backend_to_user (comac_gstate_t *gstate, double *x, double *y)
{
    if (! gstate->is_identity)
	_do_comac_gstate_backend_to_user (gstate, x, y);
}

comac_private void
_do_comac_gstate_backend_to_user_distance (comac_gstate_t *gstate, double *x, double *y);

static inline void
_comac_gstate_backend_to_user_distance (comac_gstate_t *gstate, double *x, double *y)
{
    if (! gstate->is_identity)
	_do_comac_gstate_backend_to_user_distance (gstate, x, y);
}

comac_private void
_comac_gstate_backend_to_user_rectangle (comac_gstate_t *gstate,
                                         double *x1, double *y1,
                                         double *x2, double *y2,
                                         comac_bool_t *is_tight);

comac_private void
_comac_gstate_path_extents (comac_gstate_t     *gstate,
			    comac_path_fixed_t *path,
			    double *x1, double *y1,
			    double *x2, double *y2);

comac_private comac_status_t
_comac_gstate_paint (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_mask (comac_gstate_t  *gstate,
		    comac_pattern_t *mask);

comac_private comac_status_t
_comac_gstate_stroke (comac_gstate_t *gstate, comac_path_fixed_t *path);

comac_private comac_status_t
_comac_gstate_fill (comac_gstate_t *gstate, comac_path_fixed_t *path);

comac_private comac_status_t
_comac_gstate_copy_page (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_show_page (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_stroke_extents (comac_gstate_t	 *gstate,
			      comac_path_fixed_t *path,
                              double *x1, double *y1,
			      double *x2, double *y2);

comac_private comac_status_t
_comac_gstate_fill_extents (comac_gstate_t     *gstate,
			    comac_path_fixed_t *path,
                            double *x1, double *y1,
			    double *x2, double *y2);

comac_private comac_status_t
_comac_gstate_in_stroke (comac_gstate_t	    *gstate,
			 comac_path_fixed_t *path,
			 double		     x,
			 double		     y,
			 comac_bool_t	    *inside_ret);

comac_private comac_bool_t
_comac_gstate_in_fill (comac_gstate_t	  *gstate,
		       comac_path_fixed_t *path,
		       double		   x,
		       double		   y);

comac_private comac_bool_t
_comac_gstate_in_clip (comac_gstate_t	  *gstate,
		       double		   x,
		       double		   y);

comac_private comac_status_t
_comac_gstate_clip (comac_gstate_t *gstate, comac_path_fixed_t *path);

comac_private comac_status_t
_comac_gstate_reset_clip (comac_gstate_t *gstate);

comac_private comac_bool_t
_comac_gstate_clip_extents (comac_gstate_t *gstate,
		            double         *x1,
		            double         *y1,
			    double         *x2,
			    double         *y2);

comac_private comac_rectangle_list_t*
_comac_gstate_copy_clip_rectangle_list (comac_gstate_t *gstate);

comac_private comac_status_t
_comac_gstate_show_surface (comac_gstate_t	*gstate,
			    comac_surface_t	*surface,
			    double		 x,
			    double		 y,
			    double		width,
			    double		height);

comac_private comac_status_t
_comac_gstate_tag_begin (comac_gstate_t	*gstate,
			 const char     *tag_name,
			 const char     *attributes);

comac_private comac_status_t
_comac_gstate_tag_end (comac_gstate_t	*gstate,
		       const char       *tag_name);

comac_private comac_status_t
_comac_gstate_set_font_size (comac_gstate_t *gstate,
			     double          size);

comac_private void
_comac_gstate_get_font_matrix (comac_gstate_t *gstate,
			       comac_matrix_t *matrix);

comac_private comac_status_t
_comac_gstate_set_font_matrix (comac_gstate_t	    *gstate,
			       const comac_matrix_t *matrix);

comac_private void
_comac_gstate_get_font_options (comac_gstate_t       *gstate,
				comac_font_options_t *options);

comac_private void
_comac_gstate_set_font_options (comac_gstate_t	           *gstate,
				const comac_font_options_t *options);

comac_private comac_status_t
_comac_gstate_get_font_face (comac_gstate_t     *gstate,
			     comac_font_face_t **font_face);

comac_private comac_status_t
_comac_gstate_get_scaled_font (comac_gstate_t       *gstate,
			       comac_scaled_font_t **scaled_font);

comac_private comac_status_t
_comac_gstate_get_font_extents (comac_gstate_t *gstate,
				comac_font_extents_t *extents);

comac_private comac_status_t
_comac_gstate_set_font_face (comac_gstate_t    *gstate,
			     comac_font_face_t *font_face);

comac_private comac_status_t
_comac_gstate_glyph_extents (comac_gstate_t *gstate,
			     const comac_glyph_t *glyphs,
			     int num_glyphs,
			     comac_text_extents_t *extents);

comac_private comac_status_t
_comac_gstate_show_text_glyphs (comac_gstate_t		   *gstate,
				const comac_glyph_t	   *glyphs,
				int			    num_glyphs,
				comac_glyph_text_info_t    *info);

comac_private comac_status_t
_comac_gstate_glyph_path (comac_gstate_t      *gstate,
			  const comac_glyph_t *glyphs,
			  int		       num_glyphs,
			  comac_path_fixed_t  *path);

comac_private comac_status_t
_comac_gstate_set_antialias (comac_gstate_t *gstate,
			     comac_antialias_t antialias);

comac_private comac_antialias_t
_comac_gstate_get_antialias (comac_gstate_t *gstate);

#endif /* COMAC_GSTATE_PRIVATE_H */
