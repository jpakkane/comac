/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc
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
 *	Kristian Høgsberg <krh@redhat.com>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#ifndef COMAC_RECORDING_SURFACE_H
#define COMAC_RECORDING_SURFACE_H

#include "comacint.h"
#include "comac-path-fixed-private.h"
#include "comac-pattern-private.h"
#include "comac-surface-backend-private.h"

typedef enum {
    /* The 5 basic drawing operations. */
    COMAC_COMMAND_PAINT,
    COMAC_COMMAND_MASK,
    COMAC_COMMAND_STROKE,
    COMAC_COMMAND_FILL,
    COMAC_COMMAND_SHOW_TEXT_GLYPHS,

    /* comac_tag_begin()/comac_tag_end() */
    COMAC_COMMAND_TAG,
} comac_command_type_t;

typedef enum {
    COMAC_RECORDING_REGION_ALL,
    COMAC_RECORDING_REGION_NATIVE,
    COMAC_RECORDING_REGION_IMAGE_FALLBACK
} comac_recording_region_type_t;

typedef struct _comac_command_header {
    comac_command_type_t	 type;
    comac_recording_region_type_t region;
    comac_operator_t		 op;
    comac_rectangle_int_t	 extents;
    comac_clip_t		*clip;

    int index;
    struct _comac_command_header *chain;
} comac_command_header_t;

typedef struct _comac_command_paint {
    comac_command_header_t       header;
    comac_pattern_union_t	 source;
} comac_command_paint_t;

typedef struct _comac_command_mask {
    comac_command_header_t       header;
    comac_pattern_union_t	 source;
    comac_pattern_union_t	 mask;
} comac_command_mask_t;

typedef struct _comac_command_stroke {
    comac_command_header_t       header;
    comac_pattern_union_t	 source;
    comac_path_fixed_t		 path;
    comac_stroke_style_t	 style;
    comac_matrix_t		 ctm;
    comac_matrix_t		 ctm_inverse;
    double			 tolerance;
    comac_antialias_t		 antialias;
} comac_command_stroke_t;

typedef struct _comac_command_fill {
    comac_command_header_t       header;
    comac_pattern_union_t	 source;
    comac_path_fixed_t		 path;
    comac_fill_rule_t		 fill_rule;
    double			 tolerance;
    comac_antialias_t		 antialias;
} comac_command_fill_t;

typedef struct _comac_command_show_text_glyphs {
    comac_command_header_t       header;
    comac_pattern_union_t	 source;
    char			*utf8;
    int				 utf8_len;
    comac_glyph_t		*glyphs;
    unsigned int		 num_glyphs;
    comac_text_cluster_t	*clusters;
    int				 num_clusters;
    comac_text_cluster_flags_t   cluster_flags;
    comac_scaled_font_t		*scaled_font;
} comac_command_show_text_glyphs_t;

typedef struct _comac_command_tag {
    comac_command_header_t       header;
    comac_bool_t                 begin;
    char                        *tag_name;
    char                        *attributes;
} comac_command_tag_t;

typedef union _comac_command {
    comac_command_header_t      header;

    comac_command_paint_t			paint;
    comac_command_mask_t			mask;
    comac_command_stroke_t			stroke;
    comac_command_fill_t			fill;
    comac_command_show_text_glyphs_t		show_text_glyphs;
    comac_command_tag_t                         tag;
} comac_command_t;

typedef struct _comac_recording_surface {
    comac_surface_t base;

    /* A recording-surface is logically unbounded, but when used as a
     * source we need to render it to an image, so we need a size at
     * which to create that image. */
    comac_rectangle_t extents_pixels;
    comac_rectangle_int_t extents;
    comac_bool_t unbounded;

    comac_array_t commands;
    unsigned int *indices;
    unsigned int num_indices;
    comac_bool_t optimize_clears;
    comac_bool_t has_bilevel_alpha;
    comac_bool_t has_only_op_over;

    struct bbtree {
	comac_box_t extents;
	struct bbtree *left, *right;
	comac_command_header_t *chain;
    } bbtree;
} comac_recording_surface_t;


comac_private comac_int_status_t
_comac_recording_surface_get_path (comac_surface_t	 *surface,
				   comac_path_fixed_t *path);

comac_private comac_status_t
_comac_recording_surface_replay_one (comac_recording_surface_t	*surface,
				     long unsigned index,
				     comac_surface_t *target);

comac_private comac_status_t
_comac_recording_surface_replay (comac_surface_t *surface,
				 comac_surface_t *target);

comac_private comac_status_t
_comac_recording_surface_replay_with_foreground_color (comac_surface_t *surface,
                                                       comac_surface_t *target,
                                                       const comac_color_t *color);

comac_private comac_status_t
_comac_recording_surface_replay_with_clip (comac_surface_t *surface,
					   const comac_matrix_t *surface_transform,
					   comac_surface_t *target,
					   const comac_clip_t *target_clip);

comac_private comac_status_t
_comac_recording_surface_replay_and_create_regions (comac_surface_t *surface,
						    const comac_matrix_t *surface_transform,
						    comac_surface_t *target,
						    comac_bool_t surface_is_unbounded);
comac_private comac_status_t
_comac_recording_surface_replay_region (comac_surface_t			*surface,
					const comac_rectangle_int_t *surface_extents,
					comac_surface_t			*target,
					comac_recording_region_type_t	region);

comac_private comac_status_t
_comac_recording_surface_get_bbox (comac_recording_surface_t *recording,
				   comac_box_t *bbox,
				   const comac_matrix_t *transform);

comac_private comac_status_t
_comac_recording_surface_get_ink_bbox (comac_recording_surface_t *surface,
				       comac_box_t *bbox,
				       const comac_matrix_t *transform);

comac_private comac_bool_t
_comac_recording_surface_has_only_bilevel_alpha (comac_recording_surface_t *surface);

comac_private comac_bool_t
_comac_recording_surface_has_only_op_over (comac_recording_surface_t *surface);

#endif /* COMAC_RECORDING_SURFACE_H */
