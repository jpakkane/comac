/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2008 Adrian Johnson
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
 * The Initial Developer of the Original Code is Adrian Johnson.
 *
 * Contributor(s):
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#ifndef COMAC_TYPE3_GLYPH_SURFACE_PRIVATE_H
#define COMAC_TYPE3_GLYPH_SURFACE_PRIVATE_H

#include "comacint.h"

#if COMAC_HAS_FONT_SUBSET

#include "comac-surface-private.h"
#include "comac-surface-clipper-private.h"
#include "comac-pdf-operators-private.h"

typedef comac_int_status_t (*comac_type3_glyph_surface_emit_image_t) (
    comac_image_surface_t *image, comac_output_stream_t *stream);

typedef struct comac_type3_glyph_surface {
    comac_surface_t base;

    comac_scaled_font_t *scaled_font;
    comac_output_stream_t *stream;
    comac_pdf_operators_t pdf_operators;
    comac_matrix_t comac_to_pdf;
    comac_type3_glyph_surface_emit_image_t emit_image;

    comac_surface_clipper_t clipper;
} comac_type3_glyph_surface_t;

comac_private comac_surface_t *
_comac_type3_glyph_surface_create (
    comac_scaled_font_t *scaled_font,
    comac_output_stream_t *stream,
    comac_type3_glyph_surface_emit_image_t emit_image,
    comac_scaled_font_subsets_t *font_subsets,
    comac_bool_t ps_output);

comac_private void
_comac_type3_glyph_surface_set_font_subsets_callback (
    void *abstract_surface,
    comac_pdf_operators_use_font_subset_t use_font_subset,
    void *closure);

comac_private comac_status_t
_comac_type3_glyph_surface_analyze_glyph (void *abstract_surface,
					  unsigned long glyph_index);

comac_private comac_status_t
_comac_type3_glyph_surface_emit_glyph (void *abstract_surface,
				       comac_output_stream_t *stream,
				       unsigned long glyph_index,
				       comac_box_t *bbox,
				       double *width);

#endif /* COMAC_HAS_FONT_SUBSET */

#endif /* COMAC_TYPE3_GLYPH_SURFACE_PRIVATE_H */
