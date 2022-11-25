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

#include "comacint.h"

#if COMAC_HAS_FONT_SUBSET

#include "comac-type3-glyph-surface-private.h"
#include "comac-output-stream-private.h"
#include "comac-recording-surface-private.h"
#include "comac-analysis-surface-private.h"
#include "comac-default-context-private.h"
#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-surface-clipper-private.h"

static const comac_surface_backend_t comac_type3_glyph_surface_backend;

static comac_status_t
_comac_type3_glyph_surface_clipper_intersect_clip_path (
    comac_surface_clipper_t *clipper,
    comac_path_fixed_t *path,
    comac_fill_rule_t fill_rule,
    double tolerance,
    comac_antialias_t antialias)
{
    comac_type3_glyph_surface_t *surface =
	comac_container_of (clipper, comac_type3_glyph_surface_t, clipper);

    if (path == NULL) {
	_comac_output_stream_printf (surface->stream, "Q q\n");
	return COMAC_STATUS_SUCCESS;
    }

    return _comac_pdf_operators_clip (&surface->pdf_operators, path, fill_rule);
}

comac_surface_t *
_comac_type3_glyph_surface_create (
    comac_scaled_font_t *scaled_font,
    comac_output_stream_t *stream,
    comac_type3_glyph_surface_emit_image_t emit_image,
    comac_scaled_font_subsets_t *font_subsets,
    comac_bool_t ps)
{
    comac_type3_glyph_surface_t *surface;

    if (unlikely (stream != NULL && stream->status))
	return _comac_surface_create_in_error (stream->status);

    surface = _comac_malloc (sizeof (comac_type3_glyph_surface_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_surface_init (&surface->base,
			 &comac_type3_glyph_surface_backend,
			 NULL, /* device */
			 COMAC_CONTENT_COLOR_ALPHA,
			 TRUE, /* is_vector */
			 COMAC_COLORSPACE_RGB);

    surface->scaled_font = scaled_font;
    surface->stream = stream;
    surface->emit_image = emit_image;

    /* Setup the transform from the user-font device space to Type 3
     * font space. The Type 3 font space is defined by the FontMatrix
     * entry in the Type 3 dictionary. In the PDF backend this is an
     * identity matrix. */
    surface->comac_to_pdf = scaled_font->scale_inverse;

    _comac_pdf_operators_init (&surface->pdf_operators,
			       surface->stream,
			       &surface->comac_to_pdf,
			       font_subsets,
			       ps);

    _comac_surface_clipper_init (
	&surface->clipper,
	_comac_type3_glyph_surface_clipper_intersect_clip_path);

    return &surface->base;
}

static comac_status_t
_comac_type3_glyph_surface_emit_image (comac_type3_glyph_surface_t *surface,
				       comac_image_surface_t *image,
				       comac_matrix_t *image_matrix)
{
    comac_status_t status;

    /* The only image type supported by Type 3 fonts are 1-bit masks */
    image = _comac_image_surface_coerce_to_format (image, COMAC_FORMAT_A1);
    status = image->base.status;
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (surface->stream,
				 "q %f %f %f %f %f %f cm\n",
				 image_matrix->xx,
				 image_matrix->xy,
				 image_matrix->yx,
				 image_matrix->yy,
				 image_matrix->x0,
				 image_matrix->y0);

    status = surface->emit_image (image, surface->stream);
    comac_surface_destroy (&image->base);

    _comac_output_stream_printf (surface->stream, "Q\n");

    return status;
}

static comac_status_t
_comac_type3_glyph_surface_emit_image_pattern (
    comac_type3_glyph_surface_t *surface,
    comac_image_surface_t *image,
    const comac_matrix_t *pattern_matrix)
{
    comac_matrix_t mat, upside_down;
    comac_status_t status;

    if (image->width == 0 || image->height == 0)
	return COMAC_STATUS_SUCCESS;

    mat = *pattern_matrix;

    /* Get the pattern space to user space matrix  */
    status = comac_matrix_invert (&mat);

    /* comac_pattern_set_matrix ensures the matrix is invertible */
    assert (status == COMAC_STATUS_SUCCESS);

    /* Make this a pattern space to Type 3 font space matrix */
    comac_matrix_multiply (&mat, &mat, &surface->comac_to_pdf);

    /* PDF images are in a 1 unit by 1 unit image space. Turn the 1 by
     * 1 image upside down to convert to flip the Y-axis going from
     * comac to PDF. Then scale the image up to the required size. */
    comac_matrix_scale (&mat, image->width, image->height);
    comac_matrix_init (&upside_down, 1, 0, 0, -1, 0, 1);
    comac_matrix_multiply (&mat, &upside_down, &mat);

    return _comac_type3_glyph_surface_emit_image (surface, image, &mat);
}

static comac_status_t
_comac_type3_glyph_surface_finish (void *abstract_surface)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;

    return _comac_pdf_operators_fini (&surface->pdf_operators);
}

static comac_int_status_t
_comac_type3_glyph_surface_paint (void *abstract_surface,
				  comac_operator_t op,
				  const comac_pattern_t *source,
				  const comac_clip_t *clip)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;
    const comac_surface_pattern_t *pattern;
    comac_image_surface_t *image;
    void *image_extra;
    comac_status_t status;

    if (source->type != COMAC_PATTERN_TYPE_SURFACE)
	return COMAC_INT_STATUS_IMAGE_FALLBACK;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	return status;

    pattern = (const comac_surface_pattern_t *) source;
    status = _comac_surface_acquire_source_image (pattern->surface,
						  &image,
						  &image_extra);
    if (unlikely (status))
	goto fail;

    status =
	_comac_type3_glyph_surface_emit_image_pattern (surface,
						       image,
						       &pattern->base.matrix);

fail:
    _comac_surface_release_source_image (pattern->surface, image, image_extra);

    return status;
}

static comac_int_status_t
_comac_type3_glyph_surface_mask (void *abstract_surface,
				 comac_operator_t op,
				 const comac_pattern_t *source,
				 const comac_pattern_t *mask,
				 const comac_clip_t *clip)
{
    return _comac_type3_glyph_surface_paint (abstract_surface, op, mask, clip);
}

static comac_int_status_t
_comac_type3_glyph_surface_stroke (void *abstract_surface,
				   comac_operator_t op,
				   const comac_pattern_t *source,
				   const comac_path_fixed_t *path,
				   const comac_stroke_style_t *style,
				   const comac_matrix_t *ctm,
				   const comac_matrix_t *ctm_inverse,
				   double tolerance,
				   comac_antialias_t antialias,
				   const comac_clip_t *clip)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;
    comac_int_status_t status;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	return status;

    return _comac_pdf_operators_stroke (&surface->pdf_operators,
					path,
					style,
					ctm,
					ctm_inverse);
}

static comac_int_status_t
_comac_type3_glyph_surface_fill (void *abstract_surface,
				 comac_operator_t op,
				 const comac_pattern_t *source,
				 const comac_path_fixed_t *path,
				 comac_fill_rule_t fill_rule,
				 double tolerance,
				 comac_antialias_t antialias,
				 const comac_clip_t *clip)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;
    comac_int_status_t status;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	return status;

    return _comac_pdf_operators_fill (&surface->pdf_operators, path, fill_rule);
}

static comac_int_status_t
_comac_type3_glyph_surface_show_glyphs (void *abstract_surface,
					comac_operator_t op,
					const comac_pattern_t *source,
					comac_glyph_t *glyphs,
					int num_glyphs,
					comac_scaled_font_t *scaled_font,
					const comac_clip_t *clip)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;
    comac_int_status_t status;
    comac_scaled_font_t *font;
    comac_matrix_t new_ctm;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	return status;

    comac_matrix_multiply (&new_ctm, &surface->comac_to_pdf, &scaled_font->ctm);
    font = comac_scaled_font_create (scaled_font->font_face,
				     &scaled_font->font_matrix,
				     &new_ctm,
				     &scaled_font->options);
    if (unlikely (font->status))
	return font->status;

    status = _comac_pdf_operators_show_text_glyphs (&surface->pdf_operators,
						    NULL,
						    0,
						    glyphs,
						    num_glyphs,
						    NULL,
						    0,
						    FALSE,
						    font);

    comac_scaled_font_destroy (font);

    return status;
}

static const comac_surface_backend_t comac_type3_glyph_surface_backend = {
    COMAC_INTERNAL_SURFACE_TYPE_TYPE3_GLYPH,
    _comac_type3_glyph_surface_finish,

    _comac_default_context_create, /* XXX usable through a context? */

    NULL, /* create similar */
    NULL, /* create similar image */
    NULL, /* map to image */
    NULL, /* unmap image */

    NULL, /* source */
    NULL, /* acquire_source_image */
    NULL, /* release_source_image */
    NULL, /* snapshot */

    NULL, /* copy page */
    NULL, /* show page */

    NULL, /* _comac_type3_glyph_surface_get_extents */
    NULL, /* _comac_type3_glyph_surface_get_font_options */

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    _comac_type3_glyph_surface_paint,
    _comac_type3_glyph_surface_mask,
    _comac_type3_glyph_surface_stroke,
    _comac_type3_glyph_surface_fill,
    NULL, /* fill-stroke */
    _comac_type3_glyph_surface_show_glyphs,
};

static void
_comac_type3_glyph_surface_set_stream (comac_type3_glyph_surface_t *surface,
				       comac_output_stream_t *stream)
{
    surface->stream = stream;
    _comac_pdf_operators_set_stream (&surface->pdf_operators, stream);
}

static comac_status_t
_comac_type3_glyph_surface_emit_fallback_image (
    comac_type3_glyph_surface_t *surface, unsigned long glyph_index)
{
    comac_scaled_glyph_t *scaled_glyph;
    comac_status_t status;
    comac_image_surface_t *image;
    comac_matrix_t mat;
    double x, y;

    status = _comac_scaled_glyph_lookup (surface->scaled_font,
					 glyph_index,
					 COMAC_SCALED_GLYPH_INFO_METRICS |
					     COMAC_SCALED_GLYPH_INFO_SURFACE,
					 NULL, /* foreground color */
					 &scaled_glyph);
    if (unlikely (status))
	return status;

    image = scaled_glyph->surface;
    if (image->width == 0 || image->height == 0)
	return COMAC_STATUS_SUCCESS;

    x = _comac_fixed_to_double (scaled_glyph->bbox.p1.x);
    y = _comac_fixed_to_double (scaled_glyph->bbox.p2.y);
    comac_matrix_init (&mat, image->width, 0, 0, -image->height, x, y);
    comac_matrix_multiply (&mat, &mat, &surface->scaled_font->scale_inverse);

    return _comac_type3_glyph_surface_emit_image (surface, image, &mat);
}

void
_comac_type3_glyph_surface_set_font_subsets_callback (
    void *abstract_surface,
    comac_pdf_operators_use_font_subset_t use_font_subset,
    void *closure)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;

    if (unlikely (surface->base.status))
	return;

    _comac_pdf_operators_set_font_subsets_callback (&surface->pdf_operators,
						    use_font_subset,
						    closure);
}

comac_status_t
_comac_type3_glyph_surface_analyze_glyph (void *abstract_surface,
					  unsigned long glyph_index)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;
    comac_scaled_glyph_t *scaled_glyph;
    comac_int_status_t status, status2;
    comac_output_stream_t *null_stream;

    if (unlikely (surface->base.status))
	return surface->base.status;

    null_stream = _comac_null_stream_create ();
    if (unlikely (null_stream->status))
	return null_stream->status;

    _comac_type3_glyph_surface_set_stream (surface, null_stream);

    _comac_scaled_font_freeze_cache (surface->scaled_font);
    status =
	_comac_scaled_glyph_lookup (surface->scaled_font,
				    glyph_index,
				    COMAC_SCALED_GLYPH_INFO_RECORDING_SURFACE,
				    NULL, /* foreground color */
				    &scaled_glyph);

    if (_comac_int_status_is_error (status))
	goto cleanup;

    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	status = COMAC_INT_STATUS_SUCCESS;
	goto cleanup;
    }

    status = _comac_recording_surface_replay (scaled_glyph->recording_surface,
					      &surface->base);
    if (unlikely (status))
	goto cleanup;

    status = _comac_pdf_operators_flush (&surface->pdf_operators);
    if (status == COMAC_INT_STATUS_IMAGE_FALLBACK)
	status = COMAC_INT_STATUS_SUCCESS;

cleanup:
    _comac_scaled_font_thaw_cache (surface->scaled_font);

    status2 = _comac_output_stream_destroy (null_stream);
    if (status == COMAC_INT_STATUS_SUCCESS)
	status = status2;

    return status;
}

comac_status_t
_comac_type3_glyph_surface_emit_glyph (void *abstract_surface,
				       comac_output_stream_t *stream,
				       unsigned long glyph_index,
				       comac_box_t *bbox,
				       double *width)
{
    comac_type3_glyph_surface_t *surface = abstract_surface;
    comac_scaled_glyph_t *scaled_glyph;
    comac_int_status_t status, status2;
    double x_advance, y_advance;
    comac_matrix_t font_matrix_inverse;

    if (unlikely (surface->base.status))
	return surface->base.status;

    _comac_type3_glyph_surface_set_stream (surface, stream);

    _comac_scaled_font_freeze_cache (surface->scaled_font);
    status = _comac_scaled_glyph_lookup (
	surface->scaled_font,
	glyph_index,
	COMAC_SCALED_GLYPH_INFO_METRICS |
	    COMAC_SCALED_GLYPH_INFO_RECORDING_SURFACE,
	NULL, /* foreground color */
	&scaled_glyph);
    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	status = _comac_scaled_glyph_lookup (surface->scaled_font,
					     glyph_index,
					     COMAC_SCALED_GLYPH_INFO_METRICS,
					     NULL, /* foreground color */
					     &scaled_glyph);
	if (status == COMAC_INT_STATUS_SUCCESS)
	    status = COMAC_INT_STATUS_IMAGE_FALLBACK;
    }
    if (_comac_int_status_is_error (status)) {
	_comac_scaled_font_thaw_cache (surface->scaled_font);
	return status;
    }

    x_advance = scaled_glyph->metrics.x_advance;
    y_advance = scaled_glyph->metrics.y_advance;
    font_matrix_inverse = surface->scaled_font->font_matrix;
    status2 = comac_matrix_invert (&font_matrix_inverse);

    /* The invertability of font_matrix is tested in
     * pdf_operators_show_glyphs before any glyphs are mapped to the
     * subset. */
    assert (status2 == COMAC_INT_STATUS_SUCCESS);

    comac_matrix_transform_distance (&font_matrix_inverse,
				     &x_advance,
				     &y_advance);
    *width = x_advance;

    *bbox = scaled_glyph->bbox;
    _comac_matrix_transform_bounding_box_fixed (
	&surface->scaled_font->scale_inverse,
	bbox,
	NULL);

    _comac_output_stream_printf (surface->stream,
				 "%f 0 %f %f %f %f d1\n",
				 x_advance,
				 _comac_fixed_to_double (bbox->p1.x),
				 _comac_fixed_to_double (bbox->p1.y),
				 _comac_fixed_to_double (bbox->p2.x),
				 _comac_fixed_to_double (bbox->p2.y));

    if (status == COMAC_INT_STATUS_SUCCESS) {
	comac_output_stream_t *mem_stream;

	mem_stream = _comac_memory_stream_create ();
	status = mem_stream->status;
	if (unlikely (status))
	    goto FAIL;

	_comac_type3_glyph_surface_set_stream (surface, mem_stream);

	_comac_output_stream_printf (surface->stream, "q\n");
	status =
	    _comac_recording_surface_replay (scaled_glyph->recording_surface,
					     &surface->base);

	status2 = _comac_pdf_operators_flush (&surface->pdf_operators);
	if (status == COMAC_INT_STATUS_SUCCESS)
	    status = status2;

	_comac_output_stream_printf (surface->stream, "Q\n");

	_comac_type3_glyph_surface_set_stream (surface, stream);
	if (status == COMAC_INT_STATUS_SUCCESS)
	    _comac_memory_stream_copy (mem_stream, stream);

	status2 = _comac_output_stream_destroy (mem_stream);
	if (status == COMAC_INT_STATUS_SUCCESS)
	    status = status2;
    }

    if (status == COMAC_INT_STATUS_IMAGE_FALLBACK)
	status = _comac_type3_glyph_surface_emit_fallback_image (surface,
								 glyph_index);

FAIL:
    _comac_scaled_font_thaw_cache (surface->scaled_font);

    return status;
}

#endif /* COMAC_HAS_FONT_SUBSET */
