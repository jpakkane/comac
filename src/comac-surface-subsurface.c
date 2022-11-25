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
 * The Initial Developer of the Original Code is Intel Corporation.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-clip-inline.h"
#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-recording-surface-private.h"
#include "comac-surface-offset-private.h"
#include "comac-surface-snapshot-private.h"
#include "comac-surface-subsurface-private.h"

static const comac_surface_backend_t _comac_surface_subsurface_backend;

static comac_status_t
_comac_surface_subsurface_finish (void *abstract_surface)
{
    comac_surface_subsurface_t *surface = abstract_surface;

    comac_surface_destroy (surface->target);
    comac_surface_destroy (surface->snapshot);

    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
_comac_surface_subsurface_create_similar (void *other,
					  comac_content_t content,
					  int width,
					  int height)
{
    comac_surface_subsurface_t *surface = other;

    if (surface->target->backend->create_similar == NULL)
	return NULL;

    return surface->target->backend->create_similar (surface->target,
						     content,
						     width,
						     height);
}

static comac_surface_t *
_comac_surface_subsurface_create_similar_image (void *other,
						comac_format_t format,
						int width,
						int height)
{
    comac_surface_subsurface_t *surface = other;

    if (surface->target->backend->create_similar_image == NULL)
	return NULL;

    return surface->target->backend->create_similar_image (surface->target,
							   format,
							   width,
							   height);
}

static comac_image_surface_t *
_comac_surface_subsurface_map_to_image (void *abstract_surface,
					const comac_rectangle_int_t *extents)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_rectangle_int_t target_extents;

    target_extents.x = extents->x + surface->extents.x;
    target_extents.y = extents->y + surface->extents.y;
    target_extents.width = extents->width;
    target_extents.height = extents->height;

    return _comac_surface_map_to_image (surface->target, &target_extents);
}

static comac_int_status_t
_comac_surface_subsurface_unmap_image (void *abstract_surface,
				       comac_image_surface_t *image)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    return _comac_surface_unmap_image (surface->target, image);
}

static comac_int_status_t
_comac_surface_subsurface_paint (void *abstract_surface,
				 comac_operator_t op,
				 const comac_pattern_t *source,
				 const comac_clip_t *clip)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_rectangle_int_t rect = {0,
				  0,
				  surface->extents.width,
				  surface->extents.height};
    comac_status_t status;
    comac_clip_t *target_clip;

    target_clip = _comac_clip_copy_intersect_rectangle (clip, &rect);
    status = _comac_surface_offset_paint (surface->target,
					  -surface->extents.x,
					  -surface->extents.y,
					  op,
					  source,
					  target_clip);
    _comac_clip_destroy (target_clip);
    return status;
}

static comac_int_status_t
_comac_surface_subsurface_mask (void *abstract_surface,
				comac_operator_t op,
				const comac_pattern_t *source,
				const comac_pattern_t *mask,
				const comac_clip_t *clip)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_rectangle_int_t rect = {0,
				  0,
				  surface->extents.width,
				  surface->extents.height};
    comac_status_t status;
    comac_clip_t *target_clip;

    target_clip = _comac_clip_copy_intersect_rectangle (clip, &rect);
    status = _comac_surface_offset_mask (surface->target,
					 -surface->extents.x,
					 -surface->extents.y,
					 op,
					 source,
					 mask,
					 target_clip);
    _comac_clip_destroy (target_clip);
    return status;
}

static comac_int_status_t
_comac_surface_subsurface_fill (void *abstract_surface,
				comac_operator_t op,
				const comac_pattern_t *source,
				const comac_path_fixed_t *path,
				comac_fill_rule_t fill_rule,
				double tolerance,
				comac_antialias_t antialias,
				const comac_clip_t *clip)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_rectangle_int_t rect = {0,
				  0,
				  surface->extents.width,
				  surface->extents.height};
    comac_status_t status;
    comac_clip_t *target_clip;

    target_clip = _comac_clip_copy_intersect_rectangle (clip, &rect);
    status = _comac_surface_offset_fill (surface->target,
					 -surface->extents.x,
					 -surface->extents.y,
					 op,
					 source,
					 path,
					 fill_rule,
					 tolerance,
					 antialias,
					 target_clip);
    _comac_clip_destroy (target_clip);
    return status;
}

static comac_int_status_t
_comac_surface_subsurface_stroke (void *abstract_surface,
				  comac_operator_t op,
				  const comac_pattern_t *source,
				  const comac_path_fixed_t *path,
				  const comac_stroke_style_t *stroke_style,
				  const comac_matrix_t *ctm,
				  const comac_matrix_t *ctm_inverse,
				  double tolerance,
				  comac_antialias_t antialias,
				  const comac_clip_t *clip)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_rectangle_int_t rect = {0,
				  0,
				  surface->extents.width,
				  surface->extents.height};
    comac_status_t status;
    comac_clip_t *target_clip;

    target_clip = _comac_clip_copy_intersect_rectangle (clip, &rect);
    status = _comac_surface_offset_stroke (surface->target,
					   -surface->extents.x,
					   -surface->extents.y,
					   op,
					   source,
					   path,
					   stroke_style,
					   ctm,
					   ctm_inverse,
					   tolerance,
					   antialias,
					   target_clip);
    _comac_clip_destroy (target_clip);
    return status;
}

static comac_int_status_t
_comac_surface_subsurface_glyphs (void *abstract_surface,
				  comac_operator_t op,
				  const comac_pattern_t *source,
				  comac_glyph_t *glyphs,
				  int num_glyphs,
				  comac_scaled_font_t *scaled_font,
				  const comac_clip_t *clip)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_rectangle_int_t rect = {0,
				  0,
				  surface->extents.width,
				  surface->extents.height};
    comac_status_t status;
    comac_clip_t *target_clip;

    target_clip = _comac_clip_copy_intersect_rectangle (clip, &rect);
    status = _comac_surface_offset_glyphs (surface->target,
					   -surface->extents.x,
					   -surface->extents.y,
					   op,
					   source,
					   scaled_font,
					   glyphs,
					   num_glyphs,
					   target_clip);
    _comac_clip_destroy (target_clip);
    return status;
}

static comac_status_t
_comac_surface_subsurface_flush (void *abstract_surface, unsigned flags)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    return _comac_surface_flush (surface->target, flags);
}

static comac_status_t
_comac_surface_subsurface_mark_dirty (
    void *abstract_surface, int x, int y, int width, int height)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_status_t status;

    status = COMAC_STATUS_SUCCESS;
    if (surface->target->backend->mark_dirty_rectangle != NULL) {
	comac_rectangle_int_t rect, extents;

	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;

	extents.x = extents.y = 0;
	extents.width = surface->extents.width;
	extents.height = surface->extents.height;

	if (_comac_rectangle_intersect (&rect, &extents)) {
	    status = surface->target->backend->mark_dirty_rectangle (
		surface->target,
		rect.x + surface->extents.x,
		rect.y + surface->extents.y,
		rect.width,
		rect.height);
	}
    }

    return status;
}

static comac_bool_t
_comac_surface_subsurface_get_extents (void *abstract_surface,
				       comac_rectangle_int_t *extents)
{
    comac_surface_subsurface_t *surface = abstract_surface;

    extents->x = 0;
    extents->y = 0;
    extents->width = surface->extents.width;
    extents->height = surface->extents.height;

    return TRUE;
}

static void
_comac_surface_subsurface_get_font_options (void *abstract_surface,
					    comac_font_options_t *options)
{
    comac_surface_subsurface_t *surface = abstract_surface;

    if (surface->target->backend->get_font_options != NULL)
	surface->target->backend->get_font_options (surface->target, options);
}

static comac_surface_t *
_comac_surface_subsurface_source (void *abstract_surface,
				  comac_rectangle_int_t *extents)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_surface_t *source;

    source = _comac_surface_get_source (surface->target, extents);
    if (extents)
	*extents = surface->extents;

    return source;
}

static comac_status_t
_comac_surface_subsurface_acquire_source_image (
    void *abstract_surface, comac_image_surface_t **image_out, void **extra_out)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_surface_pattern_t pattern;
    comac_surface_t *image;
    comac_status_t status;

    image = _comac_image_surface_create_with_content (surface->base.content,
						      surface->extents.width,
						      surface->extents.height);
    if (unlikely (image->status))
	return image->status;

    _comac_pattern_init_for_surface (&pattern, surface->target);
    comac_matrix_init_translate (&pattern.base.matrix,
				 surface->extents.x,
				 surface->extents.y);
    pattern.base.filter = COMAC_FILTER_NEAREST;
    status = _comac_surface_paint (image,
				   COMAC_OPERATOR_SOURCE,
				   &pattern.base,
				   NULL);
    _comac_pattern_fini (&pattern.base);
    if (unlikely (status)) {
	comac_surface_destroy (image);
	return status;
    }

    *image_out = (comac_image_surface_t *) image;
    *extra_out = NULL;
    return COMAC_STATUS_SUCCESS;
}

static void
_comac_surface_subsurface_release_source_image (void *abstract_surface,
						comac_image_surface_t *image,
						void *abstract_extra)
{
    comac_surface_destroy (&image->base);
}

static comac_surface_t *
_comac_surface_subsurface_snapshot (void *abstract_surface)
{
    comac_surface_subsurface_t *surface = abstract_surface;
    comac_surface_pattern_t pattern;
    comac_surface_t *clone;
    comac_status_t status;

    TRACE (
	(stderr, "%s: target=%d\n", __FUNCTION__, surface->target->unique_id));

    clone = _comac_surface_create_scratch (surface->target,
					   surface->target->content,
					   surface->extents.width,
					   surface->extents.height,
					   NULL);
    if (unlikely (clone->status))
	return clone;

    _comac_pattern_init_for_surface (&pattern, surface->target);
    comac_matrix_init_translate (&pattern.base.matrix,
				 surface->extents.x,
				 surface->extents.y);
    pattern.base.filter = COMAC_FILTER_NEAREST;
    status = _comac_surface_paint (clone,
				   COMAC_OPERATOR_SOURCE,
				   &pattern.base,
				   NULL);
    _comac_pattern_fini (&pattern.base);

    if (unlikely (status)) {
	comac_surface_destroy (clone);
	clone = _comac_surface_create_in_error (status);
    }

    return clone;
}

static comac_t *
_comac_surface_subsurface_create_context (void *target)
{
    comac_surface_subsurface_t *surface = target;
    return surface->target->backend->create_context (&surface->base);
}

static const comac_surface_backend_t _comac_surface_subsurface_backend = {
    COMAC_SURFACE_TYPE_SUBSURFACE,
    _comac_surface_subsurface_finish,

    _comac_surface_subsurface_create_context,

    _comac_surface_subsurface_create_similar,
    _comac_surface_subsurface_create_similar_image,
    _comac_surface_subsurface_map_to_image,
    _comac_surface_subsurface_unmap_image,

    _comac_surface_subsurface_source,
    _comac_surface_subsurface_acquire_source_image,
    _comac_surface_subsurface_release_source_image,
    _comac_surface_subsurface_snapshot,

    NULL, /* copy_page */
    NULL, /* show_page */

    _comac_surface_subsurface_get_extents,
    _comac_surface_subsurface_get_font_options,

    _comac_surface_subsurface_flush,
    _comac_surface_subsurface_mark_dirty,

    _comac_surface_subsurface_paint,
    _comac_surface_subsurface_mask,
    _comac_surface_subsurface_stroke,
    _comac_surface_subsurface_fill,
    NULL, /* fill/stroke */
    _comac_surface_subsurface_glyphs,
};

/**
 * comac_surface_create_for_rectangle:
 * @target: an existing surface for which the sub-surface will point to
 * @x: the x-origin of the sub-surface from the top-left of the target surface (in device-space units)
 * @y: the y-origin of the sub-surface from the top-left of the target surface (in device-space units)
 * @width: width of the sub-surface (in device-space units)
 * @height: height of the sub-surface (in device-space units)
 *
 * Create a new surface that is a rectangle within the target surface.
 * All operations drawn to this surface are then clipped and translated
 * onto the target surface. Nothing drawn via this sub-surface outside of
 * its bounds is drawn onto the target surface, making this a useful method
 * for passing constrained child surfaces to library routines that draw
 * directly onto the parent surface, i.e. with no further backend allocations,
 * double buffering or copies.
 *
 * <note><para>The semantics of subsurfaces have not been finalized yet
 * unless the rectangle is in full device units, is contained within
 * the extents of the target surface, and the target or subsurface's
 * device transforms are not changed.</para></note>
 *
 * Return value: a pointer to the newly allocated surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs.
 *
 * Since: 1.10
 **/
comac_surface_t *
comac_surface_create_for_rectangle (
    comac_surface_t *target, double x, double y, double width, double height)
{
    comac_surface_subsurface_t *surface;

    if (unlikely (width < 0 || height < 0))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_INVALID_SIZE));

    if (unlikely (target->status))
	return _comac_surface_create_in_error (target->status);
    if (unlikely (target->finished))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    surface = _comac_malloc (sizeof (comac_surface_subsurface_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    x *= target->device_transform.xx;
    y *= target->device_transform.yy;

    width *= target->device_transform.xx;
    height *= target->device_transform.yy;

    x += target->device_transform.x0;
    y += target->device_transform.y0;

    _comac_surface_init (&surface->base,
			 &_comac_surface_subsurface_backend,
			 NULL, /* device */
			 target->content,
			 target->is_vector,
			 target->colorspace);

    /* XXX forced integer alignment */
    surface->extents.x = ceil (x);
    surface->extents.y = ceil (y);
    surface->extents.width = floor (x + width) - surface->extents.x;
    surface->extents.height = floor (y + height) - surface->extents.y;
    if ((surface->extents.width | surface->extents.height) < 0)
	surface->extents.width = surface->extents.height = 0;

    if (target->backend->type == COMAC_SURFACE_TYPE_SUBSURFACE) {
	/* Maintain subsurfaces as 1-depth */
	comac_surface_subsurface_t *sub = (comac_surface_subsurface_t *) target;
	surface->extents.x += sub->extents.x;
	surface->extents.y += sub->extents.y;
	target = sub->target;
    }

    surface->target = comac_surface_reference (target);
    surface->base.type = surface->target->type;

    surface->snapshot = NULL;

    comac_surface_set_device_scale (&surface->base,
				    target->device_transform.xx,
				    target->device_transform.yy);

    return &surface->base;
}

comac_surface_t *
_comac_surface_create_for_rectangle_int (comac_surface_t *target,
					 const comac_rectangle_int_t *extents)
{
    comac_surface_subsurface_t *surface;

    if (unlikely (target->status))
	return _comac_surface_create_in_error (target->status);
    if (unlikely (target->finished))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    assert (target->backend->type != COMAC_SURFACE_TYPE_SUBSURFACE);

    surface = _comac_malloc (sizeof (comac_surface_subsurface_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_surface_init (&surface->base,
			 &_comac_surface_subsurface_backend,
			 NULL, /* device */
			 target->content,
			 target->is_vector,
			 target->colorspace);

    surface->extents = *extents;
    surface->extents.x *= target->device_transform.xx;
    surface->extents.y *= target->device_transform.yy;
    surface->extents.width *= target->device_transform.xx;
    surface->extents.height *= target->device_transform.yy;
    surface->extents.x += target->device_transform.x0;
    surface->extents.y += target->device_transform.y0;

    surface->target = comac_surface_reference (target);
    surface->base.type = surface->target->type;

    surface->snapshot = NULL;

    comac_surface_set_device_scale (&surface->base,
				    target->device_transform.xx,
				    target->device_transform.yy);

    return &surface->base;
}
/* XXX observe mark-dirty */

static void
_comac_surface_subsurface_detach_snapshot (comac_surface_t *surface)
{
    comac_surface_subsurface_t *ss = (comac_surface_subsurface_t *) surface;

    TRACE ((stderr, "%s: target=%d\n", __FUNCTION__, ss->target->unique_id));

    comac_surface_destroy (ss->snapshot);
    ss->snapshot = NULL;
}

void
_comac_surface_subsurface_set_snapshot (comac_surface_t *surface,
					comac_surface_t *snapshot)
{
    comac_surface_subsurface_t *ss = (comac_surface_subsurface_t *) surface;

    TRACE ((stderr,
	    "%s: target=%d, snapshot=%d\n",
	    __FUNCTION__,
	    ss->target->unique_id,
	    snapshot->unique_id));

    /* FIXME: attaching the subsurface as a snapshot to its target creates
     * a reference cycle.  Let's make this call as a no-op until that bug
     * is fixed.
     */
    return;

    if (ss->snapshot)
	_comac_surface_detach_snapshot (ss->snapshot);

    ss->snapshot = comac_surface_reference (snapshot);

    _comac_surface_attach_snapshot (ss->target,
				    &ss->base,
				    _comac_surface_subsurface_detach_snapshot);
}
