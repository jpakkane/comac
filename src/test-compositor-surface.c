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
 * The Initial Developer of the Original Code is Intel Corporation
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "test-compositor-surface-private.h"

#include "comac-compositor-private.h"
#include "comac-default-context-private.h"
#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-surface-backend-private.h"

typedef struct _test_compositor_surface {
    comac_image_surface_t base;
} test_compositor_surface_t;

static const comac_surface_backend_t test_compositor_surface_backend;

comac_surface_t *
test_compositor_surface_create (const comac_compositor_t *compositor,
				comac_content_t content,
				int width,
				int height)
{
    test_compositor_surface_t *surface;
    pixman_image_t *pixman_image;
    pixman_format_code_t pixman_format;

    switch (content) {
    case COMAC_CONTENT_ALPHA:
	pixman_format = PIXMAN_a8;
	break;
    case COMAC_CONTENT_COLOR:
	pixman_format = PIXMAN_x8r8g8b8;
	break;
    case COMAC_CONTENT_COLOR_ALPHA:
	pixman_format = PIXMAN_a8r8g8b8;
	break;
    default:
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_INVALID_CONTENT));
    }

    pixman_image =
	pixman_image_create_bits (pixman_format, width, height, NULL, 0);
    if (unlikely (pixman_image == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    surface = _comac_malloc (sizeof (test_compositor_surface_t));
    if (unlikely (surface == NULL)) {
	pixman_image_unref (pixman_image);
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));
    }

    _comac_surface_init (&surface->base.base,
			 &test_compositor_surface_backend,
			 NULL, /* device */
			 content,
			 FALSE); /* is_vector */
    _comac_image_surface_init (&surface->base, pixman_image, pixman_format);

    surface->base.compositor = compositor;

    return &surface->base.base;
}

static comac_surface_t *
test_compositor_surface_create_similar (void *abstract_surface,
					comac_content_t content,
					int width,
					int height)
{
    test_compositor_surface_t *surface = abstract_surface;

    return test_compositor_surface_create (surface->base.compositor,
					   content,
					   width,
					   height);
}

static comac_int_status_t
test_compositor_surface_paint (void *_surface,
			       comac_operator_t op,
			       const comac_pattern_t *source,
			       const comac_clip_t *clip)
{
    test_compositor_surface_t *surface = _surface;
    return _comac_compositor_paint (surface->base.compositor,
				    _surface,
				    op,
				    source,
				    clip);
}

static comac_int_status_t
test_compositor_surface_mask (void *_surface,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_pattern_t *mask,
			      const comac_clip_t *clip)
{
    test_compositor_surface_t *surface = _surface;
    return _comac_compositor_mask (surface->base.compositor,
				   _surface,
				   op,
				   source,
				   mask,
				   clip);
}

static comac_int_status_t
test_compositor_surface_stroke (void *_surface,
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
    test_compositor_surface_t *surface = _surface;
    if (antialias == COMAC_ANTIALIAS_DEFAULT)
	antialias = COMAC_ANTIALIAS_BEST;
    return _comac_compositor_stroke (surface->base.compositor,
				     _surface,
				     op,
				     source,
				     path,
				     style,
				     ctm,
				     ctm_inverse,
				     tolerance,
				     antialias,
				     clip);
}

static comac_int_status_t
test_compositor_surface_fill (void *_surface,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_path_fixed_t *path,
			      comac_fill_rule_t fill_rule,
			      double tolerance,
			      comac_antialias_t antialias,
			      const comac_clip_t *clip)
{
    test_compositor_surface_t *surface = _surface;
    if (antialias == COMAC_ANTIALIAS_DEFAULT)
	antialias = COMAC_ANTIALIAS_BEST;
    return _comac_compositor_fill (surface->base.compositor,
				   _surface,
				   op,
				   source,
				   path,
				   fill_rule,
				   tolerance,
				   antialias,
				   clip);
}

static comac_int_status_t
test_compositor_surface_glyphs (void *_surface,
				comac_operator_t op,
				const comac_pattern_t *source,
				comac_glyph_t *glyphs,
				int num_glyphs,
				comac_scaled_font_t *scaled_font,
				const comac_clip_t *clip)
{
    test_compositor_surface_t *surface = _surface;
    return _comac_compositor_glyphs (surface->base.compositor,
				     _surface,
				     op,
				     source,
				     glyphs,
				     num_glyphs,
				     scaled_font,
				     clip);
}

static const comac_surface_backend_t test_compositor_surface_backend = {
    COMAC_SURFACE_TYPE_IMAGE,
    _comac_image_surface_finish,
    _comac_default_context_create,

    test_compositor_surface_create_similar,
    NULL, /* create similar image */
    _comac_image_surface_map_to_image,
    _comac_image_surface_unmap_image,

    _comac_image_surface_source,
    _comac_image_surface_acquire_source_image,
    _comac_image_surface_release_source_image,
    _comac_image_surface_snapshot,

    NULL, /* copy_page */
    NULL, /* show_page */

    _comac_image_surface_get_extents,
    _comac_image_surface_get_font_options,

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    test_compositor_surface_paint,
    test_compositor_surface_mask,
    test_compositor_surface_stroke,
    test_compositor_surface_fill,
    NULL, /* fill/stroke */
    test_compositor_surface_glyphs,
};

static const comac_compositor_t *
get_fallback_compositor (void)
{
    return &_comac_fallback_compositor;
}

comac_surface_t *
_comac_test_fallback_compositor_surface_create (comac_content_t content,
						int width,
						int height)
{
    return test_compositor_surface_create (get_fallback_compositor (),
					   content,
					   width,
					   height);
}

comac_surface_t *
_comac_test_mask_compositor_surface_create (comac_content_t content,
					    int width,
					    int height)
{
    return test_compositor_surface_create (_comac_image_mask_compositor_get (),
					   content,
					   width,
					   height);
}

comac_surface_t *
_comac_test_traps_compositor_surface_create (comac_content_t content,
					     int width,
					     int height)
{
    return test_compositor_surface_create (_comac_image_traps_compositor_get (),
					   content,
					   width,
					   height);
}

comac_surface_t *
_comac_test_spans_compositor_surface_create (comac_content_t content,
					     int width,
					     int height)
{
    return test_compositor_surface_create (_comac_image_spans_compositor_get (),
					   content,
					   width,
					   height);
}
