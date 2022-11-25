/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2009 Intel Corporation
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

#ifndef COMAC_COMPOSITE_RECTANGLES_PRIVATE_H
#define COMAC_COMPOSITE_RECTANGLES_PRIVATE_H

#include "comac-types-private.h"
#include "comac-error-private.h"
#include "comac-pattern-private.h"

COMAC_BEGIN_DECLS

/* Rectangles that take part in a composite operation.
 *
 * The source and mask track the extents of the respective patterns in device
 * space. The unbounded rectangle is essentially the clip rectangle. And the
 * intersection of all is the bounded rectangle, which is the minimum extents
 * the operation may require. Whether or not the operation is actually bounded
 * is tracked in the is_bounded boolean.
 *
 */
struct _comac_composite_rectangles {
    comac_surface_t *surface;
    comac_operator_t op;

    comac_rectangle_int_t source;
    comac_rectangle_int_t mask;
    comac_rectangle_int_t destination;

    comac_rectangle_int_t bounded;   /* source? IN mask? IN unbounded */
    comac_rectangle_int_t unbounded; /* destination IN clip */
    uint32_t is_bounded;

    comac_rectangle_int_t source_sample_area;
    comac_rectangle_int_t mask_sample_area;

    comac_pattern_union_t source_pattern;
    comac_pattern_union_t mask_pattern;
    const comac_pattern_t *original_source_pattern;
    const comac_pattern_t *original_mask_pattern;

    comac_clip_t *clip; /* clip will be reduced to the minimal container */
};

comac_private comac_int_status_t
_comac_composite_rectangles_init_for_paint (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_clip_t *clip);

comac_private comac_int_status_t
_comac_composite_rectangles_init_for_mask (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_pattern_t *mask,
    const comac_clip_t *clip);

comac_private comac_int_status_t
_comac_composite_rectangles_init_for_stroke (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_path_fixed_t *path,
    const comac_stroke_style_t *style,
    const comac_matrix_t *ctm,
    const comac_clip_t *clip);

comac_private comac_int_status_t
_comac_composite_rectangles_init_for_fill (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_path_fixed_t *path,
    const comac_clip_t *clip);

comac_private comac_int_status_t
_comac_composite_rectangles_init_for_boxes (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_boxes_t *boxes,
    const comac_clip_t *clip);

comac_private comac_int_status_t
_comac_composite_rectangles_init_for_polygon (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const comac_polygon_t *polygon,
    const comac_clip_t *clip);

comac_private comac_int_status_t
_comac_composite_rectangles_init_for_glyphs (
    comac_composite_rectangles_t *extents,
    comac_surface_t *surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    comac_scaled_font_t *scaled_font,
    comac_glyph_t *glyphs,
    int num_glyphs,
    const comac_clip_t *clip,
    comac_bool_t *overlap);

comac_private comac_int_status_t
_comac_composite_rectangles_intersect_source_extents (
    comac_composite_rectangles_t *extents, const comac_box_t *box);

comac_private comac_int_status_t
_comac_composite_rectangles_intersect_mask_extents (
    comac_composite_rectangles_t *extents, const comac_box_t *box);

comac_private comac_bool_t
_comac_composite_rectangles_can_reduce_clip (
    comac_composite_rectangles_t *composite, comac_clip_t *clip);

comac_private comac_int_status_t
_comac_composite_rectangles_add_to_damage (
    comac_composite_rectangles_t *composite, comac_boxes_t *damage);

comac_private void
_comac_composite_rectangles_fini (comac_composite_rectangles_t *extents);

COMAC_END_DECLS

#endif /* COMAC_COMPOSITE_RECTANGLES_PRIVATE_H */
