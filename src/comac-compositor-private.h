/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_COMPOSITOR_PRIVATE_H
#define COMAC_COMPOSITOR_PRIVATE_H

#include "comac-composite-rectangles-private.h"

COMAC_BEGIN_DECLS

typedef struct {
    comac_scaled_font_t *font;
    comac_glyph_t *glyphs;
    int num_glyphs;
    comac_bool_t use_mask;
    comac_rectangle_int_t extents;
} comac_composite_glyphs_info_t;

struct comac_compositor {
    const comac_compositor_t *delegate;

    comac_warn comac_int_status_t
    (*paint)			(const comac_compositor_t	*compositor,
				 comac_composite_rectangles_t	*extents);

    comac_warn comac_int_status_t
    (*mask)			(const comac_compositor_t	*compositor,
				 comac_composite_rectangles_t	*extents);

    comac_warn comac_int_status_t
    (*stroke)			(const comac_compositor_t	*compositor,
				 comac_composite_rectangles_t	*extents,
				 const comac_path_fixed_t	*path,
				 const comac_stroke_style_t	*style,
				 const comac_matrix_t		*ctm,
				 const comac_matrix_t		*ctm_inverse,
				 double				 tolerance,
				 comac_antialias_t		 antialias);

    comac_warn comac_int_status_t
    (*fill)			(const comac_compositor_t	*compositor,
				 comac_composite_rectangles_t	*extents,
				 const comac_path_fixed_t	*path,
				 comac_fill_rule_t		 fill_rule,
				 double				 tolerance,
				 comac_antialias_t		 antialias);

    comac_warn comac_int_status_t
    (*glyphs)			(const comac_compositor_t	 *compositor,
				 comac_composite_rectangles_t	*extents,
				 comac_scaled_font_t		*scaled_font,
				 comac_glyph_t			*glyphs,
				 int				 num_glyphs,
				 comac_bool_t			 overlap);
};

struct comac_mask_compositor {
    comac_compositor_t base;

    comac_int_status_t (*acquire) (void *surface);
    comac_int_status_t (*release) (void *surface);

    comac_int_status_t (*set_clip_region) (void		 *surface,
					   comac_region_t	*clip_region);

    comac_surface_t * (*pattern_to_surface) (comac_surface_t *dst,
					     const comac_pattern_t *pattern,
					     comac_bool_t is_mask,
					     const comac_rectangle_int_t *extents,
					     const comac_rectangle_int_t *sample,
					     int *src_x, int *src_y);

    comac_int_status_t (*draw_image_boxes) (void *surface,
					    comac_image_surface_t *image,
					    comac_boxes_t *boxes,
					    int dx, int dy);

    comac_int_status_t (*copy_boxes) (void *surface,
				      comac_surface_t *src,
				      comac_boxes_t *boxes,
				      const comac_rectangle_int_t *extents,
				      int dx, int dy);

    comac_int_status_t
	(*fill_rectangles)	(void			 *surface,
				 comac_operator_t	  op,
				 const comac_color_t     *color,
				 comac_rectangle_int_t   *rectangles,
				 int			  num_rects);

    comac_int_status_t
	(*fill_boxes)		(void			*surface,
				 comac_operator_t	 op,
				 const comac_color_t	*color,
				 comac_boxes_t		*boxes);

    comac_int_status_t
	(*check_composite) (const comac_composite_rectangles_t *extents);

    comac_int_status_t
	(*composite)		(void			*dst,
				 comac_operator_t	 op,
				 comac_surface_t	*src,
				 comac_surface_t	*mask,
				 int			 src_x,
				 int			 src_y,
				 int			 mask_x,
				 int			 mask_y,
				 int			 dst_x,
				 int			 dst_y,
				 unsigned int		 width,
				 unsigned int		 height);

    comac_int_status_t
	(*composite_boxes)	(void			*surface,
				 comac_operator_t	 op,
				 comac_surface_t	*source,
				 comac_surface_t	*mask,
				 int			 src_x,
				 int			 src_y,
				 int			 mask_x,
				 int			 mask_y,
				 int			 dst_x,
				 int			 dst_y,
				 comac_boxes_t		*boxes,
				 const comac_rectangle_int_t  *extents);

    comac_int_status_t
	(*check_composite_glyphs) (const comac_composite_rectangles_t *extents,
				   comac_scaled_font_t *scaled_font,
				   comac_glyph_t *glyphs,
				   int *num_glyphs);
    comac_int_status_t
	(*composite_glyphs)	(void				*surface,
				 comac_operator_t		 op,
				 comac_surface_t		*src,
				 int				 src_x,
				 int				 src_y,
				 int				 dst_x,
				 int				 dst_y,
				 comac_composite_glyphs_info_t  *info);
};

struct comac_traps_compositor {
    comac_compositor_t base;

    comac_int_status_t
	(*acquire) (void *surface);

    comac_int_status_t
	(*release) (void *surface);

    comac_int_status_t
	(*set_clip_region) (void		 *surface,
			    comac_region_t	*clip_region);

    comac_surface_t *
	(*pattern_to_surface) (comac_surface_t *dst,
			       const comac_pattern_t *pattern,
			       comac_bool_t is_mask,
			       const comac_rectangle_int_t *extents,
			       const comac_rectangle_int_t *sample,
			       int *src_x, int *src_y);

    comac_int_status_t (*draw_image_boxes) (void *surface,
					    comac_image_surface_t *image,
					    comac_boxes_t *boxes,
					    int dx, int dy);

    comac_int_status_t (*copy_boxes) (void *surface,
				      comac_surface_t *src,
				      comac_boxes_t *boxes,
				      const comac_rectangle_int_t *extents,
				      int dx, int dy);

    comac_int_status_t
	(*fill_boxes)		(void			*surface,
				 comac_operator_t	 op,
				 const comac_color_t	*color,
				 comac_boxes_t		*boxes);

    comac_int_status_t
	(*check_composite) (const comac_composite_rectangles_t *extents);

    comac_int_status_t
	(*composite)		(void			*dst,
				 comac_operator_t	 op,
				 comac_surface_t	*src,
				 comac_surface_t	*mask,
				 int			 src_x,
				 int			 src_y,
				 int			 mask_x,
				 int			 mask_y,
				 int			 dst_x,
				 int			 dst_y,
				 unsigned int		 width,
				 unsigned int		 height);
    comac_int_status_t
	    (*lerp)		(void			*_dst,
				 comac_surface_t	*abstract_src,
				 comac_surface_t	*abstract_mask,
				 int			src_x,
				 int			src_y,
				 int			mask_x,
				 int			mask_y,
				 int			dst_x,
				 int			dst_y,
				 unsigned int		width,
				 unsigned int		height);

    comac_int_status_t
	(*composite_boxes)	(void			*surface,
				 comac_operator_t	 op,
				 comac_surface_t	*source,
				 comac_surface_t	*mask,
				 int			 src_x,
				 int			 src_y,
				 int			 mask_x,
				 int			 mask_y,
				 int			 dst_x,
				 int			 dst_y,
				 comac_boxes_t		*boxes,
				 const comac_rectangle_int_t  *extents);

    comac_int_status_t
	(*composite_traps)	(void			*dst,
				 comac_operator_t	 op,
				 comac_surface_t	*source,
				 int			 src_x,
				 int			 src_y,
				 int			 dst_x,
				 int			 dst_y,
				 const comac_rectangle_int_t *extents,
				 comac_antialias_t	 antialias,
				 comac_traps_t		*traps);

    comac_int_status_t
	(*composite_tristrip)	(void			*dst,
				 comac_operator_t	 op,
				 comac_surface_t	*source,
				 int			 src_x,
				 int			 src_y,
				 int			 dst_x,
				 int			 dst_y,
				 const comac_rectangle_int_t *extents,
				 comac_antialias_t	 antialias,
				 comac_tristrip_t	*tristrip);

    comac_int_status_t
	(*check_composite_glyphs) (const comac_composite_rectangles_t *extents,
				   comac_scaled_font_t *scaled_font,
				   comac_glyph_t *glyphs,
				   int *num_glyphs);
    comac_int_status_t
	(*composite_glyphs)	(void				*surface,
				 comac_operator_t		 op,
				 comac_surface_t		*src,
				 int				 src_x,
				 int				 src_y,
				 int				 dst_x,
				 int				 dst_y,
				 comac_composite_glyphs_info_t  *info);
};

comac_private extern const comac_compositor_t __comac_no_compositor;
comac_private extern const comac_compositor_t _comac_fallback_compositor;

comac_private void
_comac_mask_compositor_init (comac_mask_compositor_t *compositor,
			     const comac_compositor_t *delegate);

comac_private void
_comac_shape_mask_compositor_init (comac_compositor_t *compositor,
				   const comac_compositor_t  *delegate);

comac_private void
_comac_traps_compositor_init (comac_traps_compositor_t *compositor,
			      const comac_compositor_t *delegate);

comac_private comac_int_status_t
_comac_compositor_paint (const comac_compositor_t	*compositor,
			 comac_surface_t		*surface,
			 comac_operator_t		 op,
			 const comac_pattern_t		*source,
			 const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_compositor_mask (const comac_compositor_t	*compositor,
			comac_surface_t			*surface,
			comac_operator_t		 op,
			const comac_pattern_t		*source,
			const comac_pattern_t		*mask,
			const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_compositor_stroke (const comac_compositor_t	*compositor,
			  comac_surface_t		*surface,
			  comac_operator_t		 op,
			  const comac_pattern_t		*source,
			  const comac_path_fixed_t	*path,
			  const comac_stroke_style_t	*style,
			  const comac_matrix_t		*ctm,
			  const comac_matrix_t		*ctm_inverse,
			  double			 tolerance,
			  comac_antialias_t		 antialias,
			  const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_compositor_fill (const comac_compositor_t	*compositor,
			comac_surface_t			*surface,
			comac_operator_t		 op,
			const comac_pattern_t		*source,
			const comac_path_fixed_t	*path,
			comac_fill_rule_t		 fill_rule,
			double				 tolerance,
			comac_antialias_t		 antialias,
			const comac_clip_t		*clip);

comac_private comac_int_status_t
_comac_compositor_glyphs (const comac_compositor_t		*compositor,
			  comac_surface_t			*surface,
			  comac_operator_t			 op,
			  const comac_pattern_t			*source,
			  comac_glyph_t				*glyphs,
			  int					 num_glyphs,
			  comac_scaled_font_t			*scaled_font,
			  const comac_clip_t			*clip);

COMAC_END_DECLS

#endif /* COMAC_COMPOSITOR_PRIVATE_H */
