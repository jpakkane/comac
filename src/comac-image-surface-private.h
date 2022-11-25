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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

#ifndef COMAC_IMAGE_SURFACE_PRIVATE_H
#define COMAC_IMAGE_SURFACE_PRIVATE_H

#include "comac-surface-private.h"

#include <stddef.h>
#include <pixman.h>

COMAC_BEGIN_DECLS

/* The canonical image backend */
struct _comac_image_surface {
    comac_surface_t base;

    pixman_image_t *pixman_image;
    const comac_compositor_t *compositor;

    /* Parenting is tricky wrt lifetime tracking...
     *
     * One use for tracking the parent of an image surface is for
     * create_similar_image() where we wish to create a device specific
     * surface but return an image surface to the user. In such a case,
     * the image may be owned by the device specific surface, its parent,
     * but the user lifetime tracking is then performed on the image. So
     * when the image is then finalized we call comac_surface_destroy()
     * on the parent. However, for normal usage where the lifetime tracking
     * is done on the parent surface, we need to be careful to unhook
     * the image->parent pointer before finalizing the image.
     */
    comac_surface_t *parent;

    pixman_format_code_t pixman_format;
    comac_format_t format;
    unsigned char *data;

    int width;
    int height;
    ptrdiff_t stride;
    int depth;

    unsigned owns_data : 1;
    unsigned transparency : 2;
    unsigned color : 2;
};
#define to_image_surface(S) ((comac_image_surface_t *)(S))

/* A wrapper for holding pixman images returned by create_for_pattern */
typedef struct _comac_image_source {
    comac_surface_t base;

    pixman_image_t *pixman_image;
    unsigned is_opaque_solid : 1;
} comac_image_source_t;

comac_private extern const comac_surface_backend_t _comac_image_surface_backend;
comac_private extern const comac_surface_backend_t _comac_image_source_backend;

comac_private const comac_compositor_t *
_comac_image_mask_compositor_get (void);

comac_private const comac_compositor_t *
_comac_image_traps_compositor_get (void);

comac_private const comac_compositor_t *
_comac_image_spans_compositor_get (void);

#define _comac_image_default_compositor_get _comac_image_spans_compositor_get

comac_private comac_int_status_t
_comac_image_surface_paint (void			*abstract_surface,
			    comac_operator_t		 op,
			    const comac_pattern_t	*source,
			    const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_image_surface_mask (void				*abstract_surface,
			   comac_operator_t		 op,
			   const comac_pattern_t	*source,
			   const comac_pattern_t	*mask,
			   const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_image_surface_stroke (void			*abstract_surface,
			     comac_operator_t		 op,
			     const comac_pattern_t	*source,
			     const comac_path_fixed_t	*path,
			     const comac_stroke_style_t	*style,
			     const comac_matrix_t	*ctm,
			     const comac_matrix_t	*ctm_inverse,
			     double			 tolerance,
			     comac_antialias_t		 antialias,
			     const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_image_surface_fill (void				*abstract_surface,
			   comac_operator_t		 op,
			   const comac_pattern_t	*source,
			   const comac_path_fixed_t	*path,
			   comac_fill_rule_t		 fill_rule,
			   double			 tolerance,
			   comac_antialias_t		 antialias,
			   const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_image_surface_glyphs (void			*abstract_surface,
			     comac_operator_t		 op,
			     const comac_pattern_t	*source,
			     comac_glyph_t		*glyphs,
			     int			 num_glyphs,
			     comac_scaled_font_t	*scaled_font,
			     const comac_clip_t		*clip);

comac_private void
_comac_image_surface_init (comac_image_surface_t *surface,
			   pixman_image_t	*pixman_image,
			   pixman_format_code_t	 pixman_format);

comac_private comac_surface_t *
_comac_image_surface_create_similar (void	       *abstract_other,
				     comac_content_t	content,
				     int		width,
				     int		height);

comac_private comac_image_surface_t *
_comac_image_surface_map_to_image (void *abstract_other,
				   const comac_rectangle_int_t *extents);

comac_private comac_int_status_t
_comac_image_surface_unmap_image (void *abstract_surface,
				  comac_image_surface_t *image);

comac_private comac_surface_t *
_comac_image_surface_source (void			*abstract_surface,
			     comac_rectangle_int_t	*extents);

comac_private comac_status_t
_comac_image_surface_acquire_source_image (void                    *abstract_surface,
					   comac_image_surface_t  **image_out,
					   void                   **image_extra);

comac_private void
_comac_image_surface_release_source_image (void                   *abstract_surface,
					   comac_image_surface_t  *image,
					   void                   *image_extra);

comac_private comac_surface_t *
_comac_image_surface_snapshot (void *abstract_surface);

comac_private_no_warn comac_bool_t
_comac_image_surface_get_extents (void			  *abstract_surface,
				  comac_rectangle_int_t   *rectangle);

comac_private void
_comac_image_surface_get_font_options (void                  *abstract_surface,
				       comac_font_options_t  *options);

comac_private comac_surface_t *
_comac_image_source_create_for_pattern (comac_surface_t *dst,
					const comac_pattern_t *pattern,
					comac_bool_t is_mask,
					const comac_rectangle_int_t *extents,
					const comac_rectangle_int_t *sample,
					int *src_x, int *src_y);

comac_private comac_status_t
_comac_image_surface_finish (void *abstract_surface);

comac_private pixman_image_t *
_pixman_image_for_color (const comac_color_t *comac_color);

comac_private pixman_image_t *
_pixman_image_for_pattern (comac_image_surface_t *dst,
			   const comac_pattern_t *pattern,
			   comac_bool_t is_mask,
			   const comac_rectangle_int_t *extents,
			   const comac_rectangle_int_t *sample,
			   int *tx, int *ty);

comac_private void
_pixman_image_add_traps (pixman_image_t *image,
			 int dst_x, int dst_y,
			 comac_traps_t *traps);

comac_private void
_pixman_image_add_tristrip (pixman_image_t *image,
			    int dst_x, int dst_y,
			    comac_tristrip_t *strip);

comac_private comac_image_surface_t *
_comac_image_surface_clone_subimage (comac_surface_t             *surface,
				     const comac_rectangle_int_t *extents);

/* Similar to clone; but allow format conversion */
comac_private comac_image_surface_t *
_comac_image_surface_create_from_image (comac_image_surface_t *other,
					pixman_format_code_t format,
					int x, int y, int width, int height,
					int stride);

COMAC_END_DECLS

#endif /* COMAC_IMAGE_SURFACE_PRIVATE_H */
