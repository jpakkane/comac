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
 *	Chris Wilson <chris@chris-wilson.co.u>
 */

#ifndef COMAC_SURFACE_WRAPPER_PRIVATE_H
#define COMAC_SURFACE_WRAPPER_PRIVATE_H

#include "comacint.h"
#include "comac-types-private.h"
#include "comac-surface-backend-private.h"

COMAC_BEGIN_DECLS

struct _comac_surface_wrapper {
    comac_surface_t *target;

    comac_matrix_t transform;

    comac_bool_t has_extents;
    comac_rectangle_int_t extents;
    const comac_clip_t *clip;
    comac_pattern_t *foreground_source;

    comac_bool_t needs_transform;
};

comac_private void
_comac_surface_wrapper_init (comac_surface_wrapper_t *wrapper,
			     comac_surface_t *target);

comac_private void
_comac_surface_wrapper_intersect_extents (comac_surface_wrapper_t *wrapper,
					  const comac_rectangle_int_t *extents);

comac_private void
_comac_surface_wrapper_set_inverse_transform (comac_surface_wrapper_t *wrapper,
					      const comac_matrix_t *transform);

comac_private void
_comac_surface_wrapper_set_clip (comac_surface_wrapper_t *wrapper,
				 const comac_clip_t *clip);

comac_private void
_comac_surface_wrapper_set_foreground_color (comac_surface_wrapper_t *wrapper,
                                             const comac_color_t *color);

comac_private void
_comac_surface_wrapper_fini (comac_surface_wrapper_t *wrapper);

static inline comac_bool_t
_comac_surface_wrapper_has_fill_stroke (comac_surface_wrapper_t *wrapper)
{
    return wrapper->target->backend->fill_stroke != NULL;
}

comac_private comac_status_t
_comac_surface_wrapper_acquire_source_image (comac_surface_wrapper_t *wrapper,
					     comac_image_surface_t  **image_out,
					     void                   **image_extra);

comac_private void
_comac_surface_wrapper_release_source_image (comac_surface_wrapper_t *wrapper,
					     comac_image_surface_t  *image,
					     void                   *image_extra);


comac_private comac_status_t
_comac_surface_wrapper_paint (comac_surface_wrapper_t *wrapper,
			      comac_operator_t	 op,
			      const comac_pattern_t *source,
			      const comac_clip_t	    *clip);

comac_private comac_status_t
_comac_surface_wrapper_mask (comac_surface_wrapper_t *wrapper,
			     comac_operator_t	 op,
			     const comac_pattern_t *source,
			     const comac_pattern_t *mask,
			     const comac_clip_t	    *clip);

comac_private comac_status_t
_comac_surface_wrapper_stroke (comac_surface_wrapper_t *wrapper,
			       comac_operator_t		 op,
			       const comac_pattern_t	*source,
			       const comac_path_fixed_t	*path,
			       const comac_stroke_style_t	*stroke_style,
			       const comac_matrix_t		*ctm,
			       const comac_matrix_t		*ctm_inverse,
			       double			 tolerance,
			       comac_antialias_t	 antialias,
			       const comac_clip_t		*clip);

comac_private comac_status_t
_comac_surface_wrapper_fill_stroke (comac_surface_wrapper_t *wrapper,
				    comac_operator_t	     fill_op,
				    const comac_pattern_t   *fill_source,
				    comac_fill_rule_t	     fill_rule,
				    double		     fill_tolerance,
				    comac_antialias_t	     fill_antialias,
				    const comac_path_fixed_t*path,
				    comac_operator_t	     stroke_op,
				    const comac_pattern_t   *stroke_source,
				    const comac_stroke_style_t    *stroke_style,
				    const comac_matrix_t	    *stroke_ctm,
				    const comac_matrix_t	    *stroke_ctm_inverse,
				    double		     stroke_tolerance,
				    comac_antialias_t	     stroke_antialias,
				    const comac_clip_t	    *clip);

comac_private comac_status_t
_comac_surface_wrapper_fill (comac_surface_wrapper_t *wrapper,
			     comac_operator_t	 op,
			     const comac_pattern_t *source,
			     const comac_path_fixed_t	*path,
			     comac_fill_rule_t	 fill_rule,
			     double		 tolerance,
			     comac_antialias_t	 antialias,
			     const comac_clip_t	*clip);

comac_private comac_status_t
_comac_surface_wrapper_show_text_glyphs (comac_surface_wrapper_t *wrapper,
					 comac_operator_t	     op,
					 const comac_pattern_t	    *source,
					 const char		    *utf8,
					 int			     utf8_len,
					 const comac_glyph_t	    *glyphs,
					 int			     num_glyphs,
					 const comac_text_cluster_t *clusters,
					 int			     num_clusters,
					 comac_text_cluster_flags_t  cluster_flags,
					 comac_scaled_font_t	    *scaled_font,
					 const comac_clip_t	    *clip);

comac_private comac_status_t
_comac_surface_wrapper_tag (comac_surface_wrapper_t     *wrapper,
			    comac_bool_t                 begin,
			    const char                  *tag_name,
			    const char                  *attributes);

comac_private comac_surface_t *
_comac_surface_wrapper_create_similar (comac_surface_wrapper_t *wrapper,
				       comac_content_t	content,
				       int		width,
				       int		height);
comac_private comac_bool_t
_comac_surface_wrapper_get_extents (comac_surface_wrapper_t *wrapper,
				    comac_rectangle_int_t   *extents);

comac_private void
_comac_surface_wrapper_get_font_options (comac_surface_wrapper_t    *wrapper,
					 comac_font_options_t	    *options);

comac_private comac_surface_t *
_comac_surface_wrapper_snapshot (comac_surface_wrapper_t *wrapper);

comac_private comac_bool_t
_comac_surface_wrapper_has_show_text_glyphs (comac_surface_wrapper_t *wrapper);

static inline comac_bool_t
_comac_surface_wrapper_is_active (comac_surface_wrapper_t *wrapper)
{
    return wrapper->target != (comac_surface_t *) 0;
}

comac_private comac_bool_t
_comac_surface_wrapper_get_target_extents (comac_surface_wrapper_t *wrapper,
					   comac_bool_t surface_is_unbounded,
					   comac_rectangle_int_t *extents);

COMAC_END_DECLS

#endif /* COMAC_SURFACE_WRAPPER_PRIVATE_H */
