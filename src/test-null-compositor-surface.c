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

#include "test-null-compositor-surface.h"

#include "comac-compositor-private.h"
#include "comac-default-context-private.h"
#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-surface-backend-private.h"
#include "comac-spans-compositor-private.h"
#include "comac-spans-private.h"

typedef struct _test_compositor_surface {
    comac_image_surface_t base;
} test_compositor_surface_t;

static const comac_surface_backend_t test_compositor_surface_backend;

static comac_surface_t *
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
    NULL, /* snapshot */

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

static comac_int_status_t
acquire (void *abstract_dst)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
release (void *abstract_dst)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
set_clip_region (void *_surface, comac_region_t *region)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
pattern_to_surface (comac_surface_t *dst,
		    const comac_pattern_t *pattern,
		    comac_bool_t is_mask,
		    const comac_rectangle_int_t *extents,
		    const comac_rectangle_int_t *sample,
		    int *src_x,
		    int *src_y)
{
    return comac_image_surface_create (COMAC_FORMAT_ARGB32, 0, 0);
}

static comac_int_status_t
fill_boxes (void *_dst,
	    comac_operator_t op,
	    const comac_color_t *color,
	    comac_boxes_t *boxes)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
draw_image_boxes (void *_dst,
		  comac_image_surface_t *image,
		  comac_boxes_t *boxes,
		  int dx,
		  int dy)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite (void *_dst,
	   comac_operator_t op,
	   comac_surface_t *abstract_src,
	   comac_surface_t *abstract_mask,
	   int src_x,
	   int src_y,
	   int mask_x,
	   int mask_y,
	   int dst_x,
	   int dst_y,
	   unsigned int width,
	   unsigned int height)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
lerp (void *_dst,
      comac_surface_t *abstract_src,
      comac_surface_t *abstract_mask,
      int src_x,
      int src_y,
      int mask_x,
      int mask_y,
      int dst_x,
      int dst_y,
      unsigned int width,
      unsigned int height)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite_boxes (void *_dst,
		 comac_operator_t op,
		 comac_surface_t *abstract_src,
		 comac_surface_t *abstract_mask,
		 int src_x,
		 int src_y,
		 int mask_x,
		 int mask_y,
		 int dst_x,
		 int dst_y,
		 comac_boxes_t *boxes,
		 const comac_rectangle_int_t *extents)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite_traps (void *_dst,
		 comac_operator_t op,
		 comac_surface_t *abstract_src,
		 int src_x,
		 int src_y,
		 int dst_x,
		 int dst_y,
		 const comac_rectangle_int_t *extents,
		 comac_antialias_t antialias,
		 comac_traps_t *traps)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
check_composite_glyphs (const comac_composite_rectangles_t *extents,
			comac_scaled_font_t *scaled_font,
			comac_glyph_t *glyphs,
			int *num_glyphs)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite_glyphs (void *_dst,
		  comac_operator_t op,
		  comac_surface_t *_src,
		  int src_x,
		  int src_y,
		  int dst_x,
		  int dst_y,
		  comac_composite_glyphs_info_t *info)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
spans (void *abstract_renderer,
       int y,
       int height,
       const comac_half_open_span_t *spans,
       unsigned num_spans)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
finish_spans (void *abstract_renderer)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
span_renderer_init (comac_abstract_span_renderer_t *_r,
		    const comac_composite_rectangles_t *composite,
		    comac_antialias_t antialias,
		    comac_bool_t needs_clip)
{
    comac_span_renderer_t *r = (comac_span_renderer_t *) _r;
    r->render_rows = spans;
    r->finish = finish_spans;
    return COMAC_STATUS_SUCCESS;
}

static void
span_renderer_fini (comac_abstract_span_renderer_t *_r,
		    comac_int_status_t status)
{
}

static const comac_compositor_t *
no_fallback_compositor_get (void)
{
    return &__comac_no_compositor;
}

static comac_int_status_t
check_composite (const comac_composite_rectangles_t *extents)
{
    return COMAC_STATUS_SUCCESS;
}

static const comac_compositor_t *
no_traps_compositor_get (void)
{
    static comac_atomic_once_t once = COMAC_ATOMIC_ONCE_INIT;
    static comac_traps_compositor_t compositor;

    if (_comac_atomic_init_once_enter (&once)) {
	_comac_traps_compositor_init (&compositor,
				      no_fallback_compositor_get ());

	compositor.acquire = acquire;
	compositor.release = release;
	compositor.set_clip_region = set_clip_region;
	compositor.pattern_to_surface = pattern_to_surface;
	compositor.draw_image_boxes = draw_image_boxes;
	//compositor.copy_boxes = copy_boxes;
	compositor.fill_boxes = fill_boxes;
	compositor.check_composite = check_composite;
	compositor.composite = composite;
	compositor.lerp = lerp;
	//compositor.check_composite_boxes = check_composite_boxes;
	compositor.composite_boxes = composite_boxes;
	//compositor.check_composite_traps = check_composite_traps;
	compositor.composite_traps = composite_traps;
	compositor.check_composite_glyphs = check_composite_glyphs;
	compositor.composite_glyphs = composite_glyphs;

	_comac_atomic_init_once_leave (&once);
    }

    return &compositor.base;
}

static const comac_compositor_t *
no_spans_compositor_get (void)
{
    static comac_atomic_once_t once = COMAC_ATOMIC_ONCE_INIT;
    static comac_spans_compositor_t compositor;

    if (_comac_atomic_init_once_enter (&once)) {
	_comac_spans_compositor_init (&compositor, no_traps_compositor_get ());

	//compositor.acquire = acquire;
	//compositor.release = release;
	compositor.fill_boxes = fill_boxes;
	//compositor.check_composite_boxes = check_composite_boxes;
	compositor.composite_boxes = composite_boxes;
	//compositor.check_span_renderer = check_span_renderer;
	compositor.renderer_init = span_renderer_init;
	compositor.renderer_fini = span_renderer_fini;

	_comac_atomic_init_once_leave (&once);
    }

    return &compositor.base;
}

comac_surface_t *
_comac_test_no_fallback_compositor_surface_create (comac_content_t content,
						   int width,
						   int height)
{
    return test_compositor_surface_create (no_fallback_compositor_get (),
					   content,
					   width,
					   height);
}

comac_surface_t *
_comac_test_no_traps_compositor_surface_create (comac_content_t content,
						int width,
						int height)
{
    return test_compositor_surface_create (no_traps_compositor_get (),
					   content,
					   width,
					   height);
}

comac_surface_t *
_comac_test_no_spans_compositor_surface_create (comac_content_t content,
						int width,
						int height)
{
    return test_compositor_surface_create (no_spans_compositor_get (),
					   content,
					   width,
					   height);
}
