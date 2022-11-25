/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@cworth.org>
 *      Joonas Pihlaja <jpihlaja@cc.helsinki.fi>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-compositor-private.h"

static comac_int_status_t
_comac_no_compositor_paint (const comac_compositor_t *_compositor,
			    comac_composite_rectangles_t *extents)
{
    ASSERT_NOT_REACHED;
    return COMAC_INT_STATUS_NOTHING_TO_DO;
}

static comac_int_status_t
_comac_no_compositor_mask (const comac_compositor_t *compositor,
			   comac_composite_rectangles_t *extents)
{
    ASSERT_NOT_REACHED;
    return COMAC_INT_STATUS_NOTHING_TO_DO;
}

static comac_int_status_t
_comac_no_compositor_stroke (const comac_compositor_t *_compositor,
			     comac_composite_rectangles_t *extents,
			     const comac_path_fixed_t	*path,
			     const comac_stroke_style_t	*style,
			     const comac_matrix_t	*ctm,
			     const comac_matrix_t	*ctm_inverse,
			     double		 tolerance,
			     comac_antialias_t	 antialias)
{
    ASSERT_NOT_REACHED;
    return COMAC_INT_STATUS_NOTHING_TO_DO;
}

static comac_int_status_t
_comac_no_compositor_fill (const comac_compositor_t *_compositor,
			   comac_composite_rectangles_t *extents,
			   const comac_path_fixed_t	*path,
			   comac_fill_rule_t	 fill_rule,
			   double			 tolerance,
			   comac_antialias_t	 antialias)
{
    ASSERT_NOT_REACHED;
    return COMAC_INT_STATUS_NOTHING_TO_DO;
}

static comac_int_status_t
_comac_no_compositor_glyphs (const comac_compositor_t *compositor,
			     comac_composite_rectangles_t *extents,
			     comac_scaled_font_t	*scaled_font,
			     comac_glyph_t		*glyphs,
			     int			 num_glyphs,
			     comac_bool_t overlap)
{
    ASSERT_NOT_REACHED;
    return COMAC_INT_STATUS_NOTHING_TO_DO;
}

const comac_compositor_t __comac_no_compositor = {
    NULL,
    _comac_no_compositor_paint,
    _comac_no_compositor_mask,
    _comac_no_compositor_stroke,
    _comac_no_compositor_fill,
    _comac_no_compositor_glyphs,
};
