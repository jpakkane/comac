/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
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
 */

#include "comacint.h"

#include "comac-array-private.h"
#include "comac-clip-inline.h"
#include "comac-clip-private.h"
#include "comac-damage-private.h"
#include "comac-device-private.h"
#include "comac-error-private.h"
#include "comac-list-inline.h"
#include "comac-image-surface-inline.h"
#include "comac-recording-surface-private.h"
#include "comac-region-private.h"
#include "comac-surface-inline.h"
#include "comac-tee-surface-private.h"

/**
 * SECTION:comac-surface
 * @Title: comac_surface_t
 * @Short_Description: Base class for surfaces
 * @See_Also: #comac_t, #comac_pattern_t
 *
 * #comac_surface_t is the abstract type representing all different drawing
 * targets that comac can render to.  The actual drawings are
 * performed using a comac <firstterm>context</firstterm>.
 *
 * A comac surface is created by using <firstterm>backend</firstterm>-specific
 * constructors, typically of the form
 * <function>comac_<emphasis>backend</emphasis>_surface_create(<!-- -->)</function>.
 *
 * Most surface types allow accessing the surface without using Comac
 * functions. If you do this, keep in mind that it is mandatory that you call
 * comac_surface_flush() before reading from or writing to the surface and that
 * you must use comac_surface_mark_dirty() after modifying it.
 * <example>
 * <title>Directly modifying an image surface</title>
 * <programlisting>
 * void
 * modify_image_surface (comac_surface_t *surface)
 * {
 *   unsigned char *data;
 *   int width, height, stride;
 *
 *   // flush to ensure all writing to the image was done
 *   comac_surface_flush (surface);
 *
 *   // modify the image
 *   data = comac_image_surface_get_data (surface);
 *   width = comac_image_surface_get_width (surface);
 *   height = comac_image_surface_get_height (surface);
 *   stride = comac_image_surface_get_stride (surface);
 *   modify_image_data (data, width, height, stride);
 *
 *   // mark the image dirty so Comac clears its caches.
 *   comac_surface_mark_dirty (surface);
 * }
 * </programlisting>
 * </example>
 * Note that for other surface types it might be necessary to acquire the
 * surface's device first. See comac_device_acquire() for a discussion of
 * devices.
 **/

#define DEFINE_NIL_SURFACE(status, name)                                       \
    const comac_surface_t name = {                                             \
	NULL,			       /* backend */                           \
	NULL,			       /* device */                            \
	COMAC_SURFACE_TYPE_IMAGE,      /* type */                              \
	COMAC_CONTENT_COLOR,	       /* content */                           \
	COMAC_REFERENCE_COUNT_INVALID, /* ref_count */                         \
	status,			       /* status */                            \
	0,			       /* unique id */                         \
	0,			       /* serial */                            \
	NULL,			       /* damage */                            \
	FALSE,			       /* _finishing */                        \
	FALSE,			       /* finished */                          \
	TRUE,			       /* is_clear */                          \
	FALSE,			       /* has_font_options */                  \
	FALSE,			       /* owns_device */                       \
	FALSE,			       /* is_vector */                         \
	{                                                                      \
	    0,                                                                 \
	    0,                                                                 \
	    0,                                                                 \
	    NULL,                                                              \
	}, /* user_data */                                                     \
	{                                                                      \
	    0,                                                                 \
	    0,                                                                 \
	    0,                                                                 \
	    NULL,                                                              \
	},				/* mime_data */                        \
	{1.0, 0.0, 0.0, 1.0, 0.0, 0.0}, /* device_transform */                 \
	{1.0, 0.0, 0.0, 1.0, 0.0, 0.0}, /* device_transform_inverse */         \
	{NULL, NULL},			/* device_transform_observers */       \
	0.0,				/* x_resolution */                     \
	0.0,				/* y_resolution */                     \
	0.0,				/* x_fallback_resolution */            \
	0.0,				/* y_fallback_resolution */            \
	NULL,				/* snapshot_of */                      \
	NULL,				/* snapshot_detach */                  \
	{NULL, NULL},			/* snapshots */                        \
	{NULL, NULL},			/* snapshot */                         \
	{                                                                      \
	    COMAC_ANTIALIAS_DEFAULT,	  /* antialias */                      \
	    COMAC_SUBPIXEL_ORDER_DEFAULT, /* subpixel_order */                 \
	    COMAC_LCD_FILTER_DEFAULT,	  /* lcd_filter */                     \
	    COMAC_HINT_STYLE_DEFAULT,	  /* hint_style */                     \
	    COMAC_HINT_METRICS_DEFAULT,	  /* hint_metrics */                   \
	    COMAC_ROUND_GLYPH_POS_DEFAULT /* round_glyph_positions */          \
	},				  /* font_options */                   \
	COMAC_COLORSPACE_RGB,                                                  \
	COMAC_RENDERING_INTENT_RELATIVE_COLORIMETRIC,                          \
	comac_default_color_convert_func,                                      \
	NULL,                                                                  \
    }

/* XXX error object! */

static DEFINE_NIL_SURFACE (COMAC_STATUS_NO_MEMORY, _comac_surface_nil);
static DEFINE_NIL_SURFACE (COMAC_STATUS_SURFACE_TYPE_MISMATCH,
			   _comac_surface_nil_surface_type_mismatch);
static DEFINE_NIL_SURFACE (COMAC_STATUS_INVALID_STATUS,
			   _comac_surface_nil_invalid_status);
static DEFINE_NIL_SURFACE (COMAC_STATUS_INVALID_CONTENT,
			   _comac_surface_nil_invalid_content);
static DEFINE_NIL_SURFACE (COMAC_STATUS_INVALID_FORMAT,
			   _comac_surface_nil_invalid_format);
static DEFINE_NIL_SURFACE (COMAC_STATUS_INVALID_VISUAL,
			   _comac_surface_nil_invalid_visual);
static DEFINE_NIL_SURFACE (COMAC_STATUS_FILE_NOT_FOUND,
			   _comac_surface_nil_file_not_found);
static DEFINE_NIL_SURFACE (COMAC_STATUS_TEMP_FILE_ERROR,
			   _comac_surface_nil_temp_file_error);
static DEFINE_NIL_SURFACE (COMAC_STATUS_READ_ERROR,
			   _comac_surface_nil_read_error);
static DEFINE_NIL_SURFACE (COMAC_STATUS_WRITE_ERROR,
			   _comac_surface_nil_write_error);
static DEFINE_NIL_SURFACE (COMAC_STATUS_INVALID_STRIDE,
			   _comac_surface_nil_invalid_stride);
static DEFINE_NIL_SURFACE (COMAC_STATUS_INVALID_SIZE,
			   _comac_surface_nil_invalid_size);
static DEFINE_NIL_SURFACE (COMAC_STATUS_DEVICE_TYPE_MISMATCH,
			   _comac_surface_nil_device_type_mismatch);
static DEFINE_NIL_SURFACE (COMAC_STATUS_DEVICE_ERROR,
			   _comac_surface_nil_device_error);

static DEFINE_NIL_SURFACE (COMAC_INT_STATUS_UNSUPPORTED,
			   _comac_surface_nil_unsupported);
static DEFINE_NIL_SURFACE (COMAC_INT_STATUS_NOTHING_TO_DO,
			   _comac_surface_nil_nothing_to_do);

static void
_comac_surface_finish_snapshots (comac_surface_t *surface);
static void
_comac_surface_finish (comac_surface_t *surface);

/**
 * _comac_surface_set_error:
 * @surface: a surface
 * @status: a status value indicating an error
 *
 * Atomically sets surface->status to @status and calls _comac_error;
 * Does nothing if status is %COMAC_STATUS_SUCCESS or any of the internal
 * status values.
 *
 * All assignments of an error status to surface->status should happen
 * through _comac_surface_set_error(). Note that due to the nature of
 * the atomic operation, it is not safe to call this function on the
 * nil objects.
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _comac_error() to generate a stack trace for when the
 * user causes comac to detect an error.
 *
 * Return value: the error status.
 **/
comac_int_status_t
_comac_surface_set_error (comac_surface_t *surface, comac_int_status_t status)
{
    /* NOTHING_TO_DO is magic. We use it to break out of the inner-most
     * surface function, but anything higher just sees "success".
     */
    if (status == COMAC_INT_STATUS_NOTHING_TO_DO)
	status = COMAC_INT_STATUS_SUCCESS;

    if (status == COMAC_INT_STATUS_SUCCESS ||
	status >= (int) COMAC_INT_STATUS_LAST_STATUS)
	return status;

    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. */
    _comac_status_set_error (&surface->status, (comac_status_t) status);

    return _comac_error (status);
}

/**
 * comac_surface_get_type:
 * @surface: a #comac_surface_t
 *
 * This function returns the type of the backend used to create
 * a surface. See #comac_surface_type_t for available types.
 *
 * Return value: The type of @surface.
 *
 * Since: 1.2
 **/
comac_surface_type_t
comac_surface_get_type (comac_surface_t *surface)
{
    /* We don't use surface->backend->type here so that some of the
     * special "wrapper" surfaces such as comac_paginated_surface_t
     * can override surface->type with the type of the "child"
     * surface. */
    return surface->type;
}

/**
 * comac_surface_get_content:
 * @surface: a #comac_surface_t
 *
 * This function returns the content type of @surface which indicates
 * whether the surface contains color and/or alpha information. See
 * #comac_content_t.
 *
 * Return value: The content type of @surface.
 *
 * Since: 1.2
 **/
comac_content_t
comac_surface_get_content (comac_surface_t *surface)
{
    return surface->content;
}

/**
 * comac_surface_status:
 * @surface: a #comac_surface_t
 *
 * Checks whether an error has previously occurred for this
 * surface.
 *
 * Return value: %COMAC_STATUS_SUCCESS, %COMAC_STATUS_NULL_POINTER,
 * %COMAC_STATUS_NO_MEMORY, %COMAC_STATUS_READ_ERROR,
 * %COMAC_STATUS_INVALID_CONTENT, %COMAC_STATUS_INVALID_FORMAT, or
 * %COMAC_STATUS_INVALID_VISUAL.
 *
 * Since: 1.0
 **/
comac_status_t
comac_surface_status (comac_surface_t *surface)
{
    return surface->status;
}

static unsigned int
_comac_surface_allocate_unique_id (void)
{
    static comac_atomic_int_t unique_id;

#if COMAC_NO_MUTEX
    if (++unique_id == 0)
	unique_id = 1;
    return unique_id;
#else
    comac_atomic_int_t old, id;

    do {
	old = _comac_atomic_uint_get (&unique_id);
	id = old + 1;
	if (id == 0)
	    id = 1;
    } while (! _comac_atomic_uint_cmpxchg (&unique_id, old, id));

    return id;
#endif
}

comac_public comac_colorspace_t
comac_surface_get_colorspace (comac_surface_t *surface)
{
    return surface->colorspace;
}

// FIXME, maybe this should only be settable on surface creation.
comac_public void
comac_surface_set_colorspace (comac_surface_t *surface, comac_colorspace_t cs)
{
    surface->colorspace = cs;
}

comac_public void
comac_surface_set_color_conversion_callback (comac_surface_t *surface,
					     comac_color_convert_cb callback,
					     void *ctx)
{
    surface->color_convert = callback;
    surface->color_convert_ctx = ctx;
}

/**
 * comac_surface_get_device:
 * @surface: a #comac_surface_t
 *
 * This function returns the device for a @surface.
 * See #comac_device_t.
 *
 * Return value: The device for @surface or %NULL if the surface does
 *               not have an associated device.
 *
 * Since: 1.10
 **/
comac_device_t *
comac_surface_get_device (comac_surface_t *surface)
{
    if (unlikely (surface->status))
	return _comac_device_create_in_error (surface->status);

    return surface->device;
}

static comac_bool_t
_comac_surface_has_snapshots (comac_surface_t *surface)
{
    return ! comac_list_is_empty (&surface->snapshots);
}

static comac_bool_t
_comac_surface_has_mime_data (comac_surface_t *surface)
{
    return surface->mime_data.num_elements != 0;
}

static void
_comac_surface_detach_mime_data (comac_surface_t *surface)
{
    if (! _comac_surface_has_mime_data (surface))
	return;

    _comac_user_data_array_fini (&surface->mime_data);
    _comac_user_data_array_init (&surface->mime_data);
}

static void
_comac_surface_detach_snapshots (comac_surface_t *surface)
{
    while (_comac_surface_has_snapshots (surface)) {
	_comac_surface_detach_snapshot (
	    comac_list_first_entry (&surface->snapshots,
				    comac_surface_t,
				    snapshot));
    }
}

void
_comac_surface_detach_snapshot (comac_surface_t *snapshot)
{
    assert (snapshot->snapshot_of != NULL);

    snapshot->snapshot_of = NULL;
    comac_list_del (&snapshot->snapshot);

    if (snapshot->snapshot_detach != NULL)
	snapshot->snapshot_detach (snapshot);

    comac_surface_destroy (snapshot);
}

void
_comac_surface_attach_snapshot (comac_surface_t *surface,
				comac_surface_t *snapshot,
				comac_surface_func_t detach_func)
{
    assert (surface != snapshot);
    assert (snapshot->snapshot_of != surface);

    comac_surface_reference (snapshot);

    if (snapshot->snapshot_of != NULL)
	_comac_surface_detach_snapshot (snapshot);

    snapshot->snapshot_of = surface;
    snapshot->snapshot_detach = detach_func;

    comac_list_add (&snapshot->snapshot, &surface->snapshots);

    assert (_comac_surface_has_snapshot (surface, snapshot->backend) ==
	    snapshot);
}

comac_surface_t *
_comac_surface_has_snapshot (comac_surface_t *surface,
			     const comac_surface_backend_t *backend)
{
    comac_surface_t *snapshot;

    comac_list_foreach_entry (snapshot,
			      comac_surface_t,
			      &surface->snapshots,
			      snapshot)
    {
	if (snapshot->backend == backend)
	    return snapshot;
    }

    return NULL;
}

comac_status_t
_comac_surface_begin_modification (comac_surface_t *surface)
{
    assert (surface->status == COMAC_STATUS_SUCCESS);
    assert (! surface->finished);

    return _comac_surface_flush (surface, 1);
}

void
_comac_surface_init (comac_surface_t *surface,
		     const comac_surface_backend_t *backend,
		     comac_device_t *device,
		     comac_content_t content,
		     comac_bool_t is_vector,
		     comac_colorspace_t colorspace,
		     comac_rendering_intent_t intent,
		     comac_color_convert_cb color_convert,
		     void *color_convert_ctx)
{
    COMAC_MUTEX_INITIALIZE ();

    surface->backend = backend;
    surface->device = comac_device_reference (device);
    surface->content = content;
    surface->type = backend->type;
    surface->is_vector = is_vector;
    surface->colorspace = colorspace;
    surface->color_convert = NULL;
    surface->color_convert_ctx = NULL;
    surface->intent = intent;
    surface->color_convert = color_convert;
    surface->color_convert_ctx = color_convert_ctx;

    COMAC_REFERENCE_COUNT_INIT (&surface->ref_count, 1);
    surface->status = COMAC_STATUS_SUCCESS;
    surface->unique_id = _comac_surface_allocate_unique_id ();
    surface->finished = FALSE;
    surface->_finishing = FALSE;
    surface->is_clear = FALSE;
    surface->serial = 0;
    surface->damage = NULL;
    surface->owns_device = (device != NULL);

    _comac_user_data_array_init (&surface->user_data);
    _comac_user_data_array_init (&surface->mime_data);

    comac_matrix_init_identity (&surface->device_transform);
    comac_matrix_init_identity (&surface->device_transform_inverse);
    comac_list_init (&surface->device_transform_observers);

    surface->x_resolution = COMAC_SURFACE_RESOLUTION_DEFAULT;
    surface->y_resolution = COMAC_SURFACE_RESOLUTION_DEFAULT;

    surface->x_fallback_resolution = COMAC_SURFACE_FALLBACK_RESOLUTION_DEFAULT;
    surface->y_fallback_resolution = COMAC_SURFACE_FALLBACK_RESOLUTION_DEFAULT;

    comac_list_init (&surface->snapshots);
    surface->snapshot_of = NULL;

    surface->has_font_options = FALSE;
}

static void
_comac_surface_copy_similar_properties (comac_surface_t *surface,
					comac_surface_t *other)
{
    if (other->has_font_options || other->backend != surface->backend) {
	comac_font_options_t options;

	comac_surface_get_font_options (other, &options);
	_comac_surface_set_font_options (surface, &options);
    }

    comac_surface_set_fallback_resolution (surface,
					   other->x_fallback_resolution,
					   other->y_fallback_resolution);
}

/**
 * comac_surface_create_similar:
 * @other: an existing surface used to select the backend of the new surface
 * @content: the content for the new surface
 * @width: width of the new surface, (in device-space units)
 * @height: height of the new surface (in device-space units)
 *
 * Create a new surface that is as compatible as possible with an
 * existing surface. For example the new surface will have the same
 * device scale, fallback resolution and font options as
 * @other. Generally, the new surface will also use the same backend
 * as @other, unless that is not possible for some reason. The type of
 * the returned surface may be examined with
 * comac_surface_get_type().
 *
 * Initially the surface contents are all 0 (transparent if contents
 * have transparency, black otherwise.)
 *
 * Use comac_surface_create_similar_image() if you need an image surface
 * which can be painted quickly to the target surface.
 *
 * Return value: a pointer to the newly allocated surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs.
 *
 * Since: 1.0
 **/
comac_surface_t *
comac_surface_create_similar (comac_surface_t *other,
			      comac_content_t content,
			      int width,
			      int height)
{
    comac_surface_t *surface;
    comac_status_t status;
    comac_solid_pattern_t pattern;

    if (unlikely (other->status))
	return _comac_surface_create_in_error (other->status);
    if (unlikely (other->finished))
	return _comac_surface_create_in_error (COMAC_STATUS_SURFACE_FINISHED);
    if (unlikely (width < 0 || height < 0))
	return _comac_surface_create_in_error (COMAC_STATUS_INVALID_SIZE);
    if (unlikely (! COMAC_CONTENT_VALID (content)))
	return _comac_surface_create_in_error (COMAC_STATUS_INVALID_CONTENT);

    /* We inherit the device scale, so create a larger surface */
    width = width * other->device_transform.xx;
    height = height * other->device_transform.yy;

    surface = NULL;
    if (other->backend->create_similar)
	surface =
	    other->backend->create_similar (other, content, width, height);
    if (surface == NULL)
	surface = comac_surface_create_similar_image (
	    other,
	    _comac_format_from_content (content),
	    width,
	    height);

    if (unlikely (surface->status))
	return surface;

    _comac_surface_copy_similar_properties (surface, other);
    comac_surface_set_device_scale (surface,
				    other->device_transform.xx,
				    other->device_transform.yy);

    if (unlikely (surface->status))
	return surface;

    _comac_pattern_init_solid (&pattern, COMAC_COLOR_TRANSPARENT);
    status = _comac_surface_paint (surface,
				   COMAC_OPERATOR_CLEAR,
				   &pattern.base,
				   NULL);
    if (unlikely (status)) {
	comac_surface_destroy (surface);
	surface = _comac_surface_create_in_error (status);
    }

    assert (surface->is_clear);

    return surface;
}

/**
 * comac_surface_create_similar_image:
 * @other: an existing surface used to select the preference of the new surface
 * @format: the format for the new surface
 * @width: width of the new surface, (in pixels)
 * @height: height of the new surface (in pixels)
 *
 * Create a new image surface that is as compatible as possible for uploading
 * to and the use in conjunction with an existing surface. However, this surface
 * can still be used like any normal image surface. Unlike
 * comac_surface_create_similar() the new image surface won't inherit
 * the device scale from @other.
 *
 * Initially the surface contents are all 0 (transparent if contents
 * have transparency, black otherwise.)
 *
 * Use comac_surface_create_similar() if you don't need an image surface.
 *
 * Return value: a pointer to the newly allocated image surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs.
 *
 * Since: 1.12
 **/
comac_surface_t *
comac_surface_create_similar_image (comac_surface_t *other,
				    comac_format_t format,
				    int width,
				    int height)
{
    comac_surface_t *image;

    if (unlikely (other->status))
	return _comac_surface_create_in_error (other->status);
    if (unlikely (other->finished))
	return _comac_surface_create_in_error (COMAC_STATUS_SURFACE_FINISHED);

    if (unlikely (width < 0 || height < 0))
	return _comac_surface_create_in_error (COMAC_STATUS_INVALID_SIZE);
    if (unlikely (! COMAC_FORMAT_VALID (format)))
	return _comac_surface_create_in_error (COMAC_STATUS_INVALID_FORMAT);

    image = NULL;
    if (other->backend->create_similar_image)
	image =
	    other->backend->create_similar_image (other, format, width, height);
    if (image == NULL)
	image = comac_image_surface_create (format, width, height);

    assert (image->is_clear);

    return image;
}

/**
 * _comac_surface_map_to_image:
 * @surface: an existing surface used to extract the image from
 * @extents: limit the extraction to an rectangular region
 *
 * Returns an image surface that is the most efficient mechanism for
 * modifying the backing store of the target surface. The region
 * retrieved is limited to @extents.
 *
 * Note, the use of the original surface as a target or source whilst
 * it is mapped is undefined. The result of mapping the surface
 * multiple times is undefined. Calling comac_surface_destroy() or
 * comac_surface_finish() on the resulting image surface results in
 * undefined behavior. Changing the device transform of the image
 * surface or of @surface before the image surface is unmapped results
 * in undefined behavior.
 *
 * Assumes that @surface is valid (COMAC_STATUS_SUCCESS,
 * non-finished).
 *
 * Return value: a pointer to the newly allocated image surface. The
 * caller must use _comac_surface_unmap_image() to destroy this image
 * surface.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs.
 *
 * The returned image might have a %COMAC_FORMAT_INVALID format.
 **/
comac_image_surface_t *
_comac_surface_map_to_image (comac_surface_t *surface,
			     const comac_rectangle_int_t *extents)
{
    comac_image_surface_t *image = NULL;

    assert (extents != NULL);

    /* TODO: require map_to_image != NULL */
    if (surface->backend->map_to_image)
	image = surface->backend->map_to_image (surface, extents);

    if (image == NULL)
	image = _comac_image_surface_clone_subimage (surface, extents);

    return image;
}

/**
 * _comac_surface_unmap_image:
 * @surface: the surface passed to _comac_surface_map_to_image().
 * @image: the currently mapped image
 *
 * Unmaps the image surface as returned from
 * _comac_surface_map_to_image().
 *
 * The content of the image will be uploaded to the target surface.
 * Afterwards, the image is destroyed.
 *
 * Using an image surface which wasn't returned by
 * _comac_surface_map_to_image() results in undefined behavior.
 *
 * An image surface in error status can be passed to
 * _comac_surface_unmap_image().
 *
 * Return value: the unmap status.
 *
 * Even if the unmap status is not successful, @image is destroyed.
 **/
comac_int_status_t
_comac_surface_unmap_image (comac_surface_t *surface,
			    comac_image_surface_t *image)
{
    comac_surface_pattern_t pattern;
    comac_rectangle_int_t extents;
    comac_clip_t *clip;
    comac_int_status_t status;

    /* map_to_image can return error surfaces */
    if (unlikely (image->base.status)) {
	status = image->base.status;
	goto destroy;
    }

    /* If the image is untouched just skip the update */
    if (image->base.serial == 0) {
	status = COMAC_STATUS_SUCCESS;
	goto destroy;
    }

    /* TODO: require unmap_image != NULL */
    if (surface->backend->unmap_image &&
	! _comac_image_surface_is_clone (image)) {
	status = surface->backend->unmap_image (surface, image);
	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    _comac_pattern_init_for_surface (&pattern, &image->base);
    pattern.base.filter = COMAC_FILTER_NEAREST;

    /* We have to apply the translate from map_to_image's extents.x and .y */
    comac_matrix_init_translate (&pattern.base.matrix,
				 image->base.device_transform.x0,
				 image->base.device_transform.y0);

    /* And we also have to clip the operation to the image's extents */
    extents.x = image->base.device_transform_inverse.x0;
    extents.y = image->base.device_transform_inverse.y0;
    extents.width = image->width;
    extents.height = image->height;
    clip = _comac_clip_intersect_rectangle (NULL, &extents);

    status = _comac_surface_paint (surface,
				   COMAC_OPERATOR_SOURCE,
				   &pattern.base,
				   clip);

    _comac_pattern_fini (&pattern.base);
    _comac_clip_destroy (clip);

destroy:
    comac_surface_finish (&image->base);
    comac_surface_destroy (&image->base);

    return status;
}

/**
 * comac_surface_map_to_image:
 * @surface: an existing surface used to extract the image from
 * @extents: limit the extraction to an rectangular region
 *
 * Returns an image surface that is the most efficient mechanism for
 * modifying the backing store of the target surface. The region retrieved
 * may be limited to the @extents or %NULL for the whole surface
 *
 * Note, the use of the original surface as a target or source whilst
 * it is mapped is undefined. The result of mapping the surface
 * multiple times is undefined. Calling comac_surface_destroy() or
 * comac_surface_finish() on the resulting image surface results in
 * undefined behavior. Changing the device transform of the image
 * surface or of @surface before the image surface is unmapped results
 * in undefined behavior.
 *
 * Return value: a pointer to the newly allocated image surface. The caller
 * must use comac_surface_unmap_image() to destroy this image surface.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs. If the returned pointer does not have an
 * error status, it is guaranteed to be an image surface whose format
 * is not %COMAC_FORMAT_INVALID.
 *
 * Since: 1.12
 **/
comac_surface_t *
comac_surface_map_to_image (comac_surface_t *surface,
			    const comac_rectangle_int_t *extents)
{
    comac_rectangle_int_t rect;
    comac_image_surface_t *image;
    comac_status_t status;

    if (unlikely (surface->status))
	return _comac_surface_create_in_error (surface->status);
    if (unlikely (surface->finished))
	return _comac_surface_create_in_error (COMAC_STATUS_SURFACE_FINISHED);

    if (extents == NULL) {
	if (unlikely (! surface->backend->get_extents (surface, &rect)))
	    return _comac_surface_create_in_error (COMAC_STATUS_INVALID_SIZE);

	extents = &rect;
    } else {
	comac_rectangle_int_t surface_extents;

	/* If this surface is bounded, we can't map parts
	 * that are outside of it. */
	if (likely (
		surface->backend->get_extents (surface, &surface_extents))) {
	    if (unlikely (
		    ! _comac_rectangle_contains_rectangle (&surface_extents,
							   extents)))
		return _comac_surface_create_in_error (
		    COMAC_STATUS_INVALID_SIZE);
	}
    }

    image = _comac_surface_map_to_image (surface, extents);

    status = image->base.status;
    if (unlikely (status)) {
	comac_surface_destroy (&image->base);
	return _comac_surface_create_in_error (status);
    }

    if (image->format == COMAC_FORMAT_INVALID) {
	comac_surface_destroy (&image->base);
	image = _comac_image_surface_clone_subimage (surface, extents);
    }

    return &image->base;
}

/**
 * comac_surface_unmap_image:
 * @surface: the surface passed to comac_surface_map_to_image().
 * @image: the currently mapped image
 *
 * Unmaps the image surface as returned from #comac_surface_map_to_image().
 *
 * The content of the image will be uploaded to the target surface.
 * Afterwards, the image is destroyed.
 *
 * Using an image surface which wasn't returned by comac_surface_map_to_image()
 * results in undefined behavior.
 *
 * Since: 1.12
 **/
void
comac_surface_unmap_image (comac_surface_t *surface, comac_surface_t *image)
{
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    if (unlikely (surface->status)) {
	status = surface->status;
	goto error;
    }
    if (unlikely (surface->finished)) {
	status = _comac_error (COMAC_STATUS_SURFACE_FINISHED);
	goto error;
    }
    if (unlikely (image->status)) {
	status = image->status;
	goto error;
    }
    if (unlikely (image->finished)) {
	status = _comac_error (COMAC_STATUS_SURFACE_FINISHED);
	goto error;
    }
    if (unlikely (! _comac_surface_is_image (image))) {
	status = _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);
	goto error;
    }

    status =
	_comac_surface_unmap_image (surface, (comac_image_surface_t *) image);
    if (unlikely (status))
	_comac_surface_set_error (surface, status);

    return;

error:
    _comac_surface_set_error (surface, status);
    comac_surface_finish (image);
    comac_surface_destroy (image);
}

comac_surface_t *
_comac_surface_create_scratch (comac_surface_t *other,
			       comac_content_t content,
			       int width,
			       int height,
			       const comac_color_t *color)
{
    comac_surface_t *surface;
    comac_status_t status;
    comac_solid_pattern_t pattern;

    if (unlikely (other->status))
	return _comac_surface_create_in_error (other->status);

    surface = NULL;
    if (other->backend->create_similar)
	surface =
	    other->backend->create_similar (other, content, width, height);
    if (surface == NULL)
	surface = comac_surface_create_similar_image (
	    other,
	    _comac_format_from_content (content),
	    width,
	    height);

    if (unlikely (surface->status))
	return surface;

    _comac_surface_copy_similar_properties (surface, other);

    if (unlikely (surface->status))
	return surface;

    if (color) {
	_comac_pattern_init_solid (&pattern, color);
	status = _comac_surface_paint (surface,
				       color == COMAC_COLOR_TRANSPARENT
					   ? COMAC_OPERATOR_CLEAR
					   : COMAC_OPERATOR_SOURCE,
				       &pattern.base,
				       NULL);
	if (unlikely (status)) {
	    comac_surface_destroy (surface);
	    surface = _comac_surface_create_in_error (status);
	}
    }

    return surface;
}

/**
 * comac_surface_reference:
 * @surface: a #comac_surface_t
 *
 * Increases the reference count on @surface by one. This prevents
 * @surface from being destroyed until a matching call to
 * comac_surface_destroy() is made.
 *
 * Use comac_surface_get_reference_count() to get the number of
 * references to a #comac_surface_t.
 *
 * Return value: the referenced #comac_surface_t.
 *
 * Since: 1.0
 **/
comac_surface_t *
comac_surface_reference (comac_surface_t *surface)
{
    if (surface == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return surface;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    _comac_reference_count_inc (&surface->ref_count);

    return surface;
}

/**
 * comac_surface_destroy:
 * @surface: a #comac_surface_t
 *
 * Decreases the reference count on @surface by one. If the result is
 * zero, then @surface and all associated resources are freed.  See
 * comac_surface_reference().
 *
 * Since: 1.0
 **/
void
comac_surface_destroy (comac_surface_t *surface)
{
    if (surface == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    if (! _comac_reference_count_dec_and_test (&surface->ref_count))
	return;

    assert (surface->snapshot_of == NULL);

    if (! surface->finished) {
	_comac_surface_finish_snapshots (surface);
	/* We may have been referenced by a snapshot prior to have
	 * detaching it with the copy-on-write.
	 */
	if (COMAC_REFERENCE_COUNT_GET_VALUE (&surface->ref_count))
	    return;

	_comac_surface_finish (surface);
    }

    if (surface->damage)
	_comac_damage_destroy (surface->damage);

    _comac_user_data_array_fini (&surface->user_data);
    _comac_user_data_array_fini (&surface->mime_data);

    if (surface->owns_device)
	comac_device_destroy (surface->device);

    assert (surface->snapshot_of == NULL);
    assert (! _comac_surface_has_snapshots (surface));
    /* paranoid check that nobody took a reference whilst finishing */
    assert (! COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count));

    free (surface);
}

/**
 * comac_surface_get_reference_count:
 * @surface: a #comac_surface_t
 *
 * Returns the current reference count of @surface.
 *
 * Return value: the current reference count of @surface.  If the
 * object is a nil object, 0 will be returned.
 *
 * Since: 1.4
 **/
unsigned int
comac_surface_get_reference_count (comac_surface_t *surface)
{
    if (surface == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return 0;

    return COMAC_REFERENCE_COUNT_GET_VALUE (&surface->ref_count);
}

static void
_comac_surface_finish_snapshots (comac_surface_t *surface)
{
    comac_status_t status;

    /* update the snapshots *before* we declare the surface as finished */
    surface->_finishing = TRUE;
    status = _comac_surface_flush (surface, 0);
    (void) status;
}

static void
_comac_surface_finish (comac_surface_t *surface)
{
    comac_status_t status;

    /* call finish even if in error mode */
    if (surface->backend->finish) {
	status = surface->backend->finish (surface);
	if (unlikely (status))
	    _comac_surface_set_error (surface, status);
    }

    surface->finished = TRUE;

    assert (surface->snapshot_of == NULL);
    assert (! _comac_surface_has_snapshots (surface));
}

/**
 * comac_surface_finish:
 * @surface: the #comac_surface_t to finish
 *
 * This function finishes the surface and drops all references to
 * external resources.  For example, for the Xlib backend it means
 * that comac will no longer access the drawable, which can be freed.
 * After calling comac_surface_finish() the only valid operations on a
 * surface are checking status, getting and setting user, referencing
 * and destroying, and flushing and finishing it.
 * Further drawing to the surface will not affect the
 * surface but will instead trigger a %COMAC_STATUS_SURFACE_FINISHED
 * error.
 *
 * When the last call to comac_surface_destroy() decreases the
 * reference count to zero, comac will call comac_surface_finish() if
 * it hasn't been called already, before freeing the resources
 * associated with the surface.
 *
 * Since: 1.0
 **/
void
comac_surface_finish (comac_surface_t *surface)
{
    if (surface == NULL)
	return;

    if (COMAC_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return;

    if (surface->finished)
	return;

    /* We have to be careful when decoupling potential reference cycles */
    comac_surface_reference (surface);

    _comac_surface_finish_snapshots (surface);
    /* XXX need to block and wait for snapshot references */
    _comac_surface_finish (surface);

    comac_surface_destroy (surface);
}

/**
 * _comac_surface_release_device_reference:
 * @surface: a #comac_surface_t
 *
 * This function makes @surface release the reference to its device. The
 * function is intended to be used for avoiding cycling references for
 * surfaces that are owned by their device, for example cache surfaces.
 * Note that the @surface will still assume that the device is available.
 * So it is the caller's responsibility to ensure the device stays around
 * until the @surface is destroyed. Just calling comac_surface_finish() is
 * not enough.
 **/
void
_comac_surface_release_device_reference (comac_surface_t *surface)
{
    assert (surface->owns_device);

    comac_device_destroy (surface->device);
    surface->owns_device = FALSE;
}

/**
 * comac_surface_get_user_data:
 * @surface: a #comac_surface_t
 * @key: the address of the #comac_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @surface using the specified
 * key.  If no user data has been attached with the given key this
 * function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 *
 * Since: 1.0
 **/
void *
comac_surface_get_user_data (comac_surface_t *surface,
			     const comac_user_data_key_t *key)
{
    /* Prevent reads of the array during teardown */
    if (! COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count))
	return NULL;

    return _comac_user_data_array_get_data (&surface->user_data, key);
}

/**
 * comac_surface_set_user_data:
 * @surface: a #comac_surface_t
 * @key: the address of a #comac_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the surface
 * @destroy: a #comac_destroy_func_t which will be called when the
 * surface is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @surface.  To remove user data from a surface,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 *
 * Since: 1.0
 **/
comac_status_t
comac_surface_set_user_data (comac_surface_t *surface,
			     const comac_user_data_key_t *key,
			     void *user_data,
			     comac_destroy_func_t destroy)
{
    if (COMAC_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return surface->status;

    if (! COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count))
	return _comac_error (COMAC_STATUS_SURFACE_FINISHED);

    return _comac_user_data_array_set_data (&surface->user_data,
					    key,
					    user_data,
					    destroy);
}

/**
 * comac_surface_get_mime_data:
 * @surface: a #comac_surface_t
 * @mime_type: the mime type of the image data
 * @data: the image data to attached to the surface
 * @length: the length of the image data
 *
 * Return mime data previously attached to @surface using the
 * specified mime type.  If no data has been attached with the given
 * mime type, @data is set %NULL.
 *
 * Since: 1.10
 **/
void
comac_surface_get_mime_data (comac_surface_t *surface,
			     const char *mime_type,
			     const unsigned char **data,
			     unsigned long *length)
{
    comac_user_data_slot_t *slots;
    int i, num_slots;

    *data = NULL;
    *length = 0;

    /* Prevent reads of the array during teardown */
    if (! COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count))
	return;

    /* The number of mime-types attached to a surface is usually small,
     * typically zero. Therefore it is quicker to do a strcmp() against
     * each key than it is to intern the string (i.e. compute a hash,
     * search the hash table, and do a final strcmp).
     */
    num_slots = surface->mime_data.num_elements;
    slots = _comac_array_index (&surface->mime_data, 0);
    for (i = 0; i < num_slots; i++) {
	if (slots[i].key != NULL &&
	    strcmp ((char *) slots[i].key, mime_type) == 0) {
	    comac_mime_data_t *mime_data = slots[i].user_data;

	    *data = mime_data->data;
	    *length = mime_data->length;
	    return;
	}
    }
}

static void
_comac_mime_data_destroy (void *ptr)
{
    comac_mime_data_t *mime_data = ptr;

    if (! _comac_reference_count_dec_and_test (&mime_data->ref_count))
	return;

    if (mime_data->destroy && mime_data->closure)
	mime_data->destroy (mime_data->closure);

    free (mime_data);
}

static const char *_comac_surface_image_mime_types[] = {
    COMAC_MIME_TYPE_JPEG,
    COMAC_MIME_TYPE_PNG,
    COMAC_MIME_TYPE_JP2,
    COMAC_MIME_TYPE_JBIG2,
    COMAC_MIME_TYPE_CCITT_FAX,
};

comac_bool_t
_comac_surface_has_mime_image (comac_surface_t *surface)
{
    comac_user_data_slot_t *slots;
    int i, j, num_slots;

    /* Prevent reads of the array during teardown */
    if (! COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count))
	return FALSE;

    /* The number of mime-types attached to a surface is usually small,
     * typically zero. Therefore it is quicker to do a strcmp() against
     * each key than it is to intern the string (i.e. compute a hash,
     * search the hash table, and do a final strcmp).
     */
    num_slots = surface->mime_data.num_elements;
    slots = _comac_array_index (&surface->mime_data, 0);
    for (i = 0; i < num_slots; i++) {
	if (slots[i].key != NULL) {
	    for (j = 0; j < ARRAY_LENGTH (_comac_surface_image_mime_types);
		 j++) {
		if (strcmp ((char *) slots[i].key,
			    _comac_surface_image_mime_types[j]) == 0)
		    return TRUE;
	    }
	}
    }

    return FALSE;
}

/**
 * COMAC_MIME_TYPE_CCITT_FAX:
 *
 * Group 3 or Group 4 CCITT facsimile encoding (International
 * Telecommunication Union, Recommendations T.4 and T.6.)
 *
 * Since: 1.16
 **/

/**
 * COMAC_MIME_TYPE_CCITT_FAX_PARAMS:
 *
 * Decode parameters for Group 3 or Group 4 CCITT facsimile encoding.
 * See [CCITT Fax Images][ccitt].
 *
 * Since: 1.16
 **/

/**
 * COMAC_MIME_TYPE_EPS:
 *
 * Encapsulated PostScript file.
 * [Encapsulated PostScript File Format Specification](http://wwwimages.adobe.com/content/dam/Adobe/endevnet/postscript/pdfs/5002.EPSF_Spec.pdf)
 *
 * Since: 1.16
 **/

/**
 * COMAC_MIME_TYPE_EPS_PARAMS:
 *
 * Embedding parameters Encapsulated PostScript data.
 * See [Embedding EPS files][eps].
 *
 * Since: 1.16
 **/

/**
 * COMAC_MIME_TYPE_JBIG2:
 *
 * Joint Bi-level Image Experts Group image coding standard (ISO/IEC 11544).
 *
 * Since: 1.14
 **/

/**
 * COMAC_MIME_TYPE_JBIG2_GLOBAL:
 *
 * Joint Bi-level Image Experts Group image coding standard (ISO/IEC 11544) global segment.
 *
 * Since: 1.14
 **/

/**
 * COMAC_MIME_TYPE_JBIG2_GLOBAL_ID:
 *
 * An unique identifier shared by a JBIG2 global segment and all JBIG2 images
 * that depend on the global segment.
 *
 * Since: 1.14
 **/

/**
 * COMAC_MIME_TYPE_JP2:
 *
 * The Joint Photographic Experts Group (JPEG) 2000 image coding standard (ISO/IEC 15444-1).
 *
 * Since: 1.10
 **/

/**
 * COMAC_MIME_TYPE_JPEG:
 *
 * The Joint Photographic Experts Group (JPEG) image coding standard (ISO/IEC 10918-1).
 *
 * Since: 1.10
 **/

/**
 * COMAC_MIME_TYPE_PNG:
 *
 * The Portable Network Graphics image file format (ISO/IEC 15948).
 *
 * Since: 1.10
 **/

/**
 * COMAC_MIME_TYPE_URI:
 *
 * URI for an image file (unofficial MIME type).
 *
 * Since: 1.10
 **/

/**
 * COMAC_MIME_TYPE_UNIQUE_ID:
 *
 * Unique identifier for a surface (comac specific MIME type). All surfaces with
 * the same unique identifier will only be embedded once.
 *
 * Since: 1.12
 **/

/**
 * comac_surface_set_mime_data:
 * @surface: a #comac_surface_t
 * @mime_type: the MIME type of the image data
 * @data: the image data to attach to the surface
 * @length: the length of the image data
 * @destroy: a #comac_destroy_func_t which will be called when the
 * surface is destroyed or when new image data is attached using the
 * same mime type.
 * @closure: the data to be passed to the @destroy notifier
 *
 * Attach an image in the format @mime_type to @surface. To remove
 * the data from a surface, call this function with same mime type
 * and %NULL for @data.
 *
 * The attached image (or filename) data can later be used by backends
 * which support it (currently: PDF, PS, SVG and Win32 Printing
 * surfaces) to emit this data instead of making a snapshot of the
 * @surface.  This approach tends to be faster and requires less
 * memory and disk space.
 *
 * The recognized MIME types are the following: %COMAC_MIME_TYPE_JPEG,
 * %COMAC_MIME_TYPE_PNG, %COMAC_MIME_TYPE_JP2, %COMAC_MIME_TYPE_URI,
 * %COMAC_MIME_TYPE_UNIQUE_ID, %COMAC_MIME_TYPE_JBIG2,
 * %COMAC_MIME_TYPE_JBIG2_GLOBAL, %COMAC_MIME_TYPE_JBIG2_GLOBAL_ID,
 * %COMAC_MIME_TYPE_CCITT_FAX, %COMAC_MIME_TYPE_CCITT_FAX_PARAMS.
 *
 * See corresponding backend surface docs for details about which MIME
 * types it can handle. Caution: the associated MIME data will be
 * discarded if you draw on the surface afterwards. Use this function
 * with care.
 *
 * Even if a backend supports a MIME type, that does not mean comac
 * will always be able to use the attached MIME data. For example, if
 * the backend does not natively support the compositing operation used
 * to apply the MIME data to the backend. In that case, the MIME data
 * will be ignored. Therefore, to apply an image in all cases, it is best
 * to create an image surface which contains the decoded image data and
 * then attach the MIME data to that. This ensures the image will always
 * be used while still allowing the MIME data to be used whenever
 * possible.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 *
 * Since: 1.10
 **/
comac_status_t
comac_surface_set_mime_data (comac_surface_t *surface,
			     const char *mime_type,
			     const unsigned char *data,
			     unsigned long length,
			     comac_destroy_func_t destroy,
			     void *closure)
{
    comac_status_t status;
    comac_mime_data_t *mime_data;

    if (COMAC_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	return surface->status;

    if (! COMAC_REFERENCE_COUNT_HAS_REFERENCE (&surface->ref_count))
	return _comac_error (COMAC_STATUS_SURFACE_FINISHED);

    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    status = _comac_intern_string (&mime_type, -1);
    if (unlikely (status))
	return _comac_surface_set_error (surface, status);

    if (data != NULL) {
	mime_data = _comac_malloc (sizeof (comac_mime_data_t));
	if (unlikely (mime_data == NULL))
	    return _comac_surface_set_error (
		surface,
		_comac_error (COMAC_STATUS_NO_MEMORY));

	COMAC_REFERENCE_COUNT_INIT (&mime_data->ref_count, 1);

	mime_data->data = (unsigned char *) data;
	mime_data->length = length;
	mime_data->destroy = destroy;
	mime_data->closure = closure;
    } else
	mime_data = NULL;

    status =
	_comac_user_data_array_set_data (&surface->mime_data,
					 (comac_user_data_key_t *) mime_type,
					 mime_data,
					 _comac_mime_data_destroy);
    if (unlikely (status)) {
	free (mime_data);

	return _comac_surface_set_error (surface, status);
    }

    surface->is_clear = FALSE;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_surface_supports_mime_type:
 * @surface: a #comac_surface_t
 * @mime_type: the mime type
 *
 * Return whether @surface supports @mime_type.
 *
 * Return value: %TRUE if @surface supports
 *               @mime_type, %FALSE otherwise
 *
 * Since: 1.12
 **/
comac_bool_t
comac_surface_supports_mime_type (comac_surface_t *surface,
				  const char *mime_type)
{
    const char **types;

    if (unlikely (surface->status))
	return FALSE;
    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return FALSE;
    }

    if (surface->backend->get_supported_mime_types) {
	types = surface->backend->get_supported_mime_types (surface);
	if (types) {
	    while (*types) {
		if (strcmp (*types, mime_type) == 0)
		    return TRUE;
		types++;
	    }
	}
    }

    return FALSE;
}

static void
_comac_mime_data_reference (const void *key, void *elt, void *closure)
{
    comac_mime_data_t *mime_data = elt;

    _comac_reference_count_inc (&mime_data->ref_count);
}

comac_status_t
_comac_surface_copy_mime_data (comac_surface_t *dst, comac_surface_t *src)
{
    comac_status_t status;

    if (dst->status)
	return dst->status;

    if (src->status)
	return _comac_surface_set_error (dst, src->status);

    /* first copy the mime-data, discarding any already set on dst */
    status = _comac_user_data_array_copy (&dst->mime_data, &src->mime_data);
    if (unlikely (status))
	return _comac_surface_set_error (dst, status);

    /* now increment the reference counters for the copies */
    _comac_user_data_array_foreach (&dst->mime_data,
				    _comac_mime_data_reference,
				    NULL);

    dst->is_clear = FALSE;

    return COMAC_STATUS_SUCCESS;
}

/**
 * _comac_surface_set_font_options:
 * @surface: a #comac_surface_t
 * @options: a #comac_font_options_t object that contains the
 *   options to use for this surface instead of backend's default
 *   font options.
 *
 * Sets the default font rendering options for the surface.
 * This is useful to correctly propagate default font options when
 * falling back to an image surface in a backend implementation.
 * This affects the options returned in comac_surface_get_font_options().
 *
 * If @options is %NULL the surface options are reset to those of
 * the backend default.
 **/
void
_comac_surface_set_font_options (comac_surface_t *surface,
				 comac_font_options_t *options)
{
    if (surface->status)
	return;

    assert (surface->snapshot_of == NULL);

    if (surface->finished) {
	_comac_surface_set_error (surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    if (options) {
	surface->has_font_options = TRUE;
	_comac_font_options_init_copy (&surface->font_options, options);
    } else {
	surface->has_font_options = FALSE;
    }
}

/**
 * comac_surface_get_font_options:
 * @surface: a #comac_surface_t
 * @options: a #comac_font_options_t object into which to store
 *   the retrieved options. All existing values are overwritten
 *
 * Retrieves the default font rendering options for the surface.
 * This allows display surfaces to report the correct subpixel order
 * for rendering on them, print surfaces to disable hinting of
 * metrics and so forth. The result can then be used with
 * comac_scaled_font_create().
 *
 * Since: 1.0
 **/
void
comac_surface_get_font_options (comac_surface_t *surface,
				comac_font_options_t *options)
{
    if (comac_font_options_status (options))
	return;

    if (surface->status) {
	_comac_font_options_init_default (options);
	return;
    }

    if (! surface->has_font_options) {
	surface->has_font_options = TRUE;

	_comac_font_options_init_default (&surface->font_options);

	if (! surface->finished && surface->backend->get_font_options) {
	    surface->backend->get_font_options (surface,
						&surface->font_options);
	}
    }

    _comac_font_options_init_copy (options, &surface->font_options);
}

comac_status_t
_comac_surface_flush (comac_surface_t *surface, unsigned flags)
{
    /* update the current snapshots *before* the user updates the surface */
    _comac_surface_detach_snapshots (surface);
    if (surface->snapshot_of != NULL)
	_comac_surface_detach_snapshot (surface);
    _comac_surface_detach_mime_data (surface);

    return __comac_surface_flush (surface, flags);
}

/**
 * comac_surface_flush:
 * @surface: a #comac_surface_t
 *
 * Do any pending drawing for the surface and also restore any temporary
 * modifications comac has made to the surface's state. This function
 * must be called before switching from drawing on the surface with
 * comac to drawing on it directly with native APIs, or accessing its
 * memory outside of Comac. If the surface doesn't support direct
 * access, then this function does nothing.
 *
 * Since: 1.0
 **/
void
comac_surface_flush (comac_surface_t *surface)
{
    comac_status_t status;

    if (surface->status)
	return;

    if (surface->finished)
	return;

    status = _comac_surface_flush (surface, 0);
    if (unlikely (status))
	_comac_surface_set_error (surface, status);
}

/**
 * comac_surface_mark_dirty:
 * @surface: a #comac_surface_t
 *
 * Tells comac that drawing has been done to surface using means other
 * than comac, and that comac should reread any cached areas. Note
 * that you must call comac_surface_flush() before doing such drawing.
 *
 * Since: 1.0
 **/
void
comac_surface_mark_dirty (comac_surface_t *surface)
{
    comac_rectangle_int_t extents;

    if (unlikely (surface->status))
	return;
    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    _comac_surface_get_extents (surface, &extents);
    comac_surface_mark_dirty_rectangle (surface,
					extents.x,
					extents.y,
					extents.width,
					extents.height);
}

/**
 * comac_surface_mark_dirty_rectangle:
 * @surface: a #comac_surface_t
 * @x: X coordinate of dirty rectangle
 * @y: Y coordinate of dirty rectangle
 * @width: width of dirty rectangle
 * @height: height of dirty rectangle
 *
 * Like comac_surface_mark_dirty(), but drawing has been done only to
 * the specified rectangle, so that comac can retain cached contents
 * for other parts of the surface.
 *
 * Any cached clip set on the surface will be reset by this function,
 * to make sure that future comac calls have the clip set that they
 * expect.
 *
 * Since: 1.0
 **/
void
comac_surface_mark_dirty_rectangle (
    comac_surface_t *surface, int x, int y, int width, int height)
{
    comac_status_t status;

    if (unlikely (surface->status))
	return;

    assert (surface->snapshot_of == NULL);

    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    /* The application *should* have called comac_surface_flush() before
     * modifying the surface independently of comac (and thus having to
     * call mark_dirty()). */
    assert (! _comac_surface_has_snapshots (surface));
    assert (! _comac_surface_has_mime_data (surface));

    surface->is_clear = FALSE;
    surface->serial++;

    if (surface->damage) {
	comac_box_t box;

	box.p1.x = x;
	box.p1.y = y;
	box.p2.x = x + width;
	box.p2.y = y + height;

	surface->damage = _comac_damage_add_box (surface->damage, &box);
    }

    if (surface->backend->mark_dirty_rectangle != NULL) {
	/* XXX: FRAGILE: We're ignoring the scaling component of
	 * device_transform here. I don't know what the right thing to
	 * do would actually be if there were some scaling here, but
	 * we avoid this since device_transfom scaling is not exported
	 * publicly and mark_dirty is not used internally. */
	status = surface->backend->mark_dirty_rectangle (
	    surface,
	    x + surface->device_transform.x0,
	    y + surface->device_transform.y0,
	    width,
	    height);

	if (unlikely (status))
	    _comac_surface_set_error (surface, status);
    }
}

/**
 * comac_surface_set_device_scale:
 * @surface: a #comac_surface_t
 * @x_scale: a scale factor in the X direction
 * @y_scale: a scale factor in the Y direction
 *
 * Sets a scale that is multiplied to the device coordinates determined
 * by the CTM when drawing to @surface. One common use for this is to
 * render to very high resolution display devices at a scale factor, so
 * that code that assumes 1 pixel will be a certain size will still work.
 * Setting a transformation via comac_translate() isn't
 * sufficient to do this, since functions like
 * comac_device_to_user() will expose the hidden scale.
 *
 * Note that the scale affects drawing to the surface as well as
 * using the surface in a source pattern.
 *
 * Since: 1.14
 **/
void
comac_surface_set_device_scale (comac_surface_t *surface,
				double x_scale,
				double y_scale)
{
    comac_status_t status;

    if (unlikely (surface->status))
	return;

    assert (surface->snapshot_of == NULL);

    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status)) {
	_comac_surface_set_error (surface, status);
	return;
    }

    surface->device_transform.xx = x_scale;
    surface->device_transform.yy = y_scale;
    surface->device_transform.xy = 0.0;
    surface->device_transform.yx = 0.0;

    surface->device_transform_inverse = surface->device_transform;
    status = comac_matrix_invert (&surface->device_transform_inverse);
    /* should always be invertible unless given pathological input */
    assert (status == COMAC_STATUS_SUCCESS);

    _comac_observers_notify (&surface->device_transform_observers, surface);
}

/**
 * comac_surface_get_device_scale:
 * @surface: a #comac_surface_t
 * @x_scale: the scale in the X direction, in device units
 * @y_scale: the scale in the Y direction, in device units
 *
 * This function returns the previous device offset set by
 * comac_surface_set_device_scale().
 *
 * Since: 1.14
 **/
void
comac_surface_get_device_scale (comac_surface_t *surface,
				double *x_scale,
				double *y_scale)
{
    if (x_scale)
	*x_scale = surface->device_transform.xx;
    if (y_scale)
	*y_scale = surface->device_transform.yy;
}

/**
 * comac_surface_set_device_offset:
 * @surface: a #comac_surface_t
 * @x_offset: the offset in the X direction, in device units
 * @y_offset: the offset in the Y direction, in device units
 *
 * Sets an offset that is added to the device coordinates determined
 * by the CTM when drawing to @surface. One use case for this function
 * is when we want to create a #comac_surface_t that redirects drawing
 * for a portion of an onscreen surface to an offscreen surface in a
 * way that is completely invisible to the user of the comac
 * API. Setting a transformation via comac_translate() isn't
 * sufficient to do this, since functions like
 * comac_device_to_user() will expose the hidden offset.
 *
 * Note that the offset affects drawing to the surface as well as
 * using the surface in a source pattern.
 *
 * Since: 1.0
 **/
void
comac_surface_set_device_offset (comac_surface_t *surface,
				 double x_offset,
				 double y_offset)
{
    comac_status_t status;

    if (unlikely (surface->status))
	return;

    assert (surface->snapshot_of == NULL);

    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status)) {
	_comac_surface_set_error (surface, status);
	return;
    }

    surface->device_transform.x0 = x_offset;
    surface->device_transform.y0 = y_offset;

    surface->device_transform_inverse = surface->device_transform;
    status = comac_matrix_invert (&surface->device_transform_inverse);
    /* should always be invertible unless given pathological input */
    assert (status == COMAC_STATUS_SUCCESS);

    _comac_observers_notify (&surface->device_transform_observers, surface);
}

/**
 * comac_surface_get_device_offset:
 * @surface: a #comac_surface_t
 * @x_offset: the offset in the X direction, in device units
 * @y_offset: the offset in the Y direction, in device units
 *
 * This function returns the previous device offset set by
 * comac_surface_set_device_offset().
 *
 * Since: 1.2
 **/
void
comac_surface_get_device_offset (comac_surface_t *surface,
				 double *x_offset,
				 double *y_offset)
{
    if (x_offset)
	*x_offset = surface->device_transform.x0;
    if (y_offset)
	*y_offset = surface->device_transform.y0;
}

/**
 * comac_surface_set_fallback_resolution:
 * @surface: a #comac_surface_t
 * @x_pixels_per_inch: horizontal setting for pixels per inch
 * @y_pixels_per_inch: vertical setting for pixels per inch
 *
 * Set the horizontal and vertical resolution for image fallbacks.
 *
 * When certain operations aren't supported natively by a backend,
 * comac will fallback by rendering operations to an image and then
 * overlaying that image onto the output. For backends that are
 * natively vector-oriented, this function can be used to set the
 * resolution used for these image fallbacks, (larger values will
 * result in more detailed images, but also larger file sizes).
 *
 * Some examples of natively vector-oriented backends are the ps, pdf,
 * and svg backends.
 *
 * For backends that are natively raster-oriented, image fallbacks are
 * still possible, but they are always performed at the native
 * device resolution. So this function has no effect on those
 * backends.
 *
 * Note: The fallback resolution only takes effect at the time of
 * completing a page (with comac_show_page() or comac_copy_page()) so
 * there is currently no way to have more than one fallback resolution
 * in effect on a single page.
 *
 * The default fallback resolution is 300 pixels per inch in both
 * dimensions.
 *
 * Since: 1.2
 **/
void
comac_surface_set_fallback_resolution (comac_surface_t *surface,
				       double x_pixels_per_inch,
				       double y_pixels_per_inch)
{
    comac_status_t status;

    if (unlikely (surface->status))
	return;

    assert (surface->snapshot_of == NULL);

    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    if (x_pixels_per_inch <= 0 || y_pixels_per_inch <= 0) {
	/* XXX Could delay raising the error until we fallback, but throwing
	 * the error here means that we can catch the real culprit.
	 */
	_comac_surface_set_error (surface, COMAC_STATUS_INVALID_MATRIX);
	return;
    }

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status)) {
	_comac_surface_set_error (surface, status);
	return;
    }

    surface->x_fallback_resolution = x_pixels_per_inch;
    surface->y_fallback_resolution = y_pixels_per_inch;
}

/**
 * comac_surface_get_fallback_resolution:
 * @surface: a #comac_surface_t
 * @x_pixels_per_inch: horizontal pixels per inch
 * @y_pixels_per_inch: vertical pixels per inch
 *
 * This function returns the previous fallback resolution set by
 * comac_surface_set_fallback_resolution(), or default fallback
 * resolution if never set.
 *
 * Since: 1.8
 **/
void
comac_surface_get_fallback_resolution (comac_surface_t *surface,
				       double *x_pixels_per_inch,
				       double *y_pixels_per_inch)
{
    if (x_pixels_per_inch)
	*x_pixels_per_inch = surface->x_fallback_resolution;
    if (y_pixels_per_inch)
	*y_pixels_per_inch = surface->y_fallback_resolution;
}

comac_bool_t
_comac_surface_has_device_transform (comac_surface_t *surface)
{
    return ! _comac_matrix_is_identity (&surface->device_transform);
}

/**
 * _comac_surface_acquire_source_image:
 * @surface: a #comac_surface_t
 * @image_out: location to store a pointer to an image surface that
 *    has identical contents to @surface. This surface could be @surface
 *    itself, a surface held internal to @surface, or it could be a new
 *    surface with a copy of the relevant portion of @surface.
 * @image_extra: location to store image specific backend data
 *
 * Gets an image surface to use when drawing as a fallback when drawing with
 * @surface as a source. _comac_surface_release_source_image() must be called
 * when finished.
 *
 * Return value: %COMAC_STATUS_SUCCESS if an image was stored in @image_out.
 * %COMAC_INT_STATUS_UNSUPPORTED if an image cannot be retrieved for the specified
 * surface. Or %COMAC_STATUS_NO_MEMORY.
 **/
comac_status_t
_comac_surface_acquire_source_image (comac_surface_t *surface,
				     comac_image_surface_t **image_out,
				     void **image_extra)
{
    comac_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    assert (! surface->finished);

    if (surface->backend->acquire_source_image == NULL)
	return COMAC_INT_STATUS_UNSUPPORTED;

    status = surface->backend->acquire_source_image (surface,
						     image_out,
						     image_extra);
    if (unlikely (status))
	return _comac_surface_set_error (surface, status);

    _comac_debug_check_image_surface_is_defined (&(*image_out)->base);

    return COMAC_STATUS_SUCCESS;
}

comac_status_t
_comac_surface_default_acquire_source_image (void *_surface,
					     comac_image_surface_t **image_out,
					     void **image_extra)
{
    comac_surface_t *surface = _surface;
    comac_rectangle_int_t extents;

    if (unlikely (! surface->backend->get_extents (surface, &extents)))
	return _comac_error (COMAC_STATUS_INVALID_SIZE);

    *image_out = _comac_surface_map_to_image (surface, &extents);
    *image_extra = NULL;
    return (*image_out)->base.status;
}

/**
 * _comac_surface_release_source_image:
 * @surface: a #comac_surface_t
 * @image_extra: same as return from the matching _comac_surface_acquire_source_image()
 *
 * Releases any resources obtained with _comac_surface_acquire_source_image()
 **/
void
_comac_surface_release_source_image (comac_surface_t *surface,
				     comac_image_surface_t *image,
				     void *image_extra)
{
    assert (! surface->finished);

    if (surface->backend->release_source_image)
	surface->backend->release_source_image (surface, image, image_extra);
}

void
_comac_surface_default_release_source_image (void *surface,
					     comac_image_surface_t *image,
					     void *image_extra)
{
    comac_status_t ignored;

    ignored = _comac_surface_unmap_image (surface, image);
    (void) ignored;
}

comac_surface_t *
_comac_surface_get_source (comac_surface_t *surface,
			   comac_rectangle_int_t *extents)
{
    assert (surface->backend->source);
    return surface->backend->source (surface, extents);
}

comac_surface_t *
_comac_surface_default_source (void *surface, comac_rectangle_int_t *extents)
{
    if (extents)
	_comac_surface_get_extents (surface, extents);
    return surface;
}

static comac_status_t
_pattern_has_error (const comac_pattern_t *pattern)
{
    const comac_surface_pattern_t *spattern;

    if (unlikely (pattern->status))
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_SURFACE)
	return COMAC_STATUS_SUCCESS;

    spattern = (const comac_surface_pattern_t *) pattern;
    if (unlikely (spattern->surface->status))
	return spattern->surface->status;

    if (unlikely (spattern->surface->finished))
	return _comac_error (COMAC_STATUS_SURFACE_FINISHED);

    return COMAC_STATUS_SUCCESS;
}

static comac_bool_t
nothing_to_do (comac_surface_t *surface,
	       comac_operator_t op,
	       const comac_pattern_t *source)
{
    if (_comac_pattern_is_clear (source)) {
	if (op == COMAC_OPERATOR_OVER || op == COMAC_OPERATOR_ADD)
	    return TRUE;

	if (op == COMAC_OPERATOR_SOURCE)
	    op = COMAC_OPERATOR_CLEAR;
    }

    if (op == COMAC_OPERATOR_CLEAR && surface->is_clear)
	return TRUE;

    if (op == COMAC_OPERATOR_ATOP &&
	(surface->content & COMAC_CONTENT_COLOR) == 0)
	return TRUE;

    return FALSE;
}

comac_status_t
_comac_surface_paint (comac_surface_t *surface,
		      comac_operator_t op,
		      const comac_pattern_t *source,
		      const comac_clip_t *clip)
{
    comac_int_status_t status;
    comac_bool_t is_clear;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    if (nothing_to_do (surface, op, source))
	return COMAC_STATUS_SUCCESS;

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status))
	return status;

    status = surface->backend->paint (surface, op, source, clip);
    is_clear = op == COMAC_OPERATOR_CLEAR && clip == NULL;
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO || is_clear) {
	surface->is_clear = is_clear;
	surface->serial++;
    }

    return _comac_surface_set_error (surface, status);
}

comac_status_t
_comac_surface_mask (comac_surface_t *surface,
		     comac_operator_t op,
		     const comac_pattern_t *source,
		     const comac_pattern_t *mask,
		     const comac_clip_t *clip)
{
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    /* If the mask is blank, this is just an expensive no-op */
    if (_comac_pattern_is_clear (mask) &&
	_comac_operator_bounded_by_mask (op)) {
	return COMAC_STATUS_SUCCESS;
    }

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    status = _pattern_has_error (mask);
    if (unlikely (status))
	return status;

    if (nothing_to_do (surface, op, source))
	return COMAC_STATUS_SUCCESS;

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status))
	return status;

    status = surface->backend->mask (surface, op, source, mask, clip);
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO) {
	surface->is_clear = FALSE;
	surface->serial++;
    }

    return _comac_surface_set_error (surface, status);
}

comac_status_t
_comac_surface_fill_stroke (comac_surface_t *surface,
			    comac_operator_t fill_op,
			    const comac_pattern_t *fill_source,
			    comac_fill_rule_t fill_rule,
			    double fill_tolerance,
			    comac_antialias_t fill_antialias,
			    comac_path_fixed_t *path,
			    comac_operator_t stroke_op,
			    const comac_pattern_t *stroke_source,
			    const comac_stroke_style_t *stroke_style,
			    const comac_matrix_t *stroke_ctm,
			    const comac_matrix_t *stroke_ctm_inverse,
			    double stroke_tolerance,
			    comac_antialias_t stroke_antialias,
			    const comac_clip_t *clip)
{
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    if (surface->is_clear && fill_op == COMAC_OPERATOR_CLEAR &&
	stroke_op == COMAC_OPERATOR_CLEAR) {
	return COMAC_STATUS_SUCCESS;
    }

    status = _pattern_has_error (fill_source);
    if (unlikely (status))
	return status;

    status = _pattern_has_error (stroke_source);
    if (unlikely (status))
	return status;

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status))
	return status;

    if (surface->backend->fill_stroke) {
	comac_matrix_t dev_ctm = *stroke_ctm;
	comac_matrix_t dev_ctm_inverse = *stroke_ctm_inverse;

	status = surface->backend->fill_stroke (surface,
						fill_op,
						fill_source,
						fill_rule,
						fill_tolerance,
						fill_antialias,
						path,
						stroke_op,
						stroke_source,
						stroke_style,
						&dev_ctm,
						&dev_ctm_inverse,
						stroke_tolerance,
						stroke_antialias,
						clip);

	if (status != COMAC_INT_STATUS_UNSUPPORTED)
	    goto FINISH;
    }

    status = _comac_surface_fill (surface,
				  fill_op,
				  fill_source,
				  path,
				  fill_rule,
				  fill_tolerance,
				  fill_antialias,
				  clip);
    if (unlikely (status))
	goto FINISH;

    status = _comac_surface_stroke (surface,
				    stroke_op,
				    stroke_source,
				    path,
				    stroke_style,
				    stroke_ctm,
				    stroke_ctm_inverse,
				    stroke_tolerance,
				    stroke_antialias,
				    clip);
    if (unlikely (status))
	goto FINISH;

FINISH:
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO) {
	surface->is_clear = FALSE;
	surface->serial++;
    }

    return _comac_surface_set_error (surface, status);
}

comac_status_t
_comac_surface_stroke (comac_surface_t *surface,
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
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    if (nothing_to_do (surface, op, source))
	return COMAC_STATUS_SUCCESS;

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status))
	return status;

    status = surface->backend->stroke (surface,
				       op,
				       source,
				       path,
				       stroke_style,
				       ctm,
				       ctm_inverse,
				       tolerance,
				       antialias,
				       clip);
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO) {
	surface->is_clear = FALSE;
	surface->serial++;
    }

    return _comac_surface_set_error (surface, status);
}

comac_status_t
_comac_surface_fill (comac_surface_t *surface,
		     comac_operator_t op,
		     const comac_pattern_t *source,
		     const comac_path_fixed_t *path,
		     comac_fill_rule_t fill_rule,
		     double tolerance,
		     comac_antialias_t antialias,
		     const comac_clip_t *clip)
{
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    if (nothing_to_do (surface, op, source))
	return COMAC_STATUS_SUCCESS;

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status))
	return status;

    status = surface->backend->fill (surface,
				     op,
				     source,
				     path,
				     fill_rule,
				     tolerance,
				     antialias,
				     clip);
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO) {
	surface->is_clear = FALSE;
	surface->serial++;
    }

    return _comac_surface_set_error (surface, status);
}

/**
 * comac_surface_copy_page:
 * @surface: a #comac_surface_t
 *
 * Emits the current page for backends that support multiple pages,
 * but doesn't clear it, so that the contents of the current page will
 * be retained for the next page.  Use comac_surface_show_page() if you
 * want to get an empty page after the emission.
 *
 * There is a convenience function for this that takes a #comac_t,
 * namely comac_copy_page().
 *
 * Since: 1.6
 **/
void
comac_surface_copy_page (comac_surface_t *surface)
{
    if (unlikely (surface->status))
	return;

    assert (surface->snapshot_of == NULL);

    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface, COMAC_STATUS_SURFACE_FINISHED);
	return;
    }

    /* It's fine if some backends don't implement copy_page */
    if (surface->backend->copy_page == NULL)
	return;

    _comac_surface_set_error (surface, surface->backend->copy_page (surface));
}

/**
 * comac_surface_show_page:
 * @surface: a #comac_Surface_t
 *
 * Emits and clears the current page for backends that support multiple
 * pages.  Use comac_surface_copy_page() if you don't want to clear the page.
 *
 * There is a convenience function for this that takes a #comac_t,
 * namely comac_show_page().
 *
 * Since: 1.6
 **/
void
comac_surface_show_page (comac_surface_t *surface)
{
    comac_status_t status;

    if (unlikely (surface->status))
	return;

    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface, COMAC_STATUS_SURFACE_FINISHED);
	return;
    }

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status)) {
	_comac_surface_set_error (surface, status);
	return;
    }

    /* It's fine if some backends don't implement show_page */
    if (surface->backend->show_page == NULL)
	return;

    _comac_surface_set_error (surface, surface->backend->show_page (surface));
}

/**
 * _comac_surface_get_extents:
 * @surface: the #comac_surface_t to fetch extents for
 *
 * This function returns a bounding box for the surface.  The surface
 * bounds are defined as a region beyond which no rendering will
 * possibly be recorded, in other words, it is the maximum extent of
 * potentially usable coordinates.
 *
 * For vector surfaces, (PDF, PS, SVG and recording-surfaces), the surface
 * might be conceived as unbounded, but we force the user to provide a
 * maximum size at the time of surface_create. So get_extents uses
 * that size.
 *
 * Note: The coordinates returned are in "backend" space rather than
 * "surface" space. That is, they are relative to the true (0,0)
 * origin rather than the device_transform origin. This might seem a
 * bit inconsistent with other #comac_surface_t interfaces, but all
 * current callers are within the surface layer where backend space is
 * desired.
 *
 * This behavior would have to be changed is we ever exported a public
 * variant of this function.
 **/
comac_bool_t
_comac_surface_get_extents (comac_surface_t *surface,
			    comac_rectangle_int_t *extents)
{
    comac_bool_t bounded;

    if (unlikely (surface->status))
	goto zero_extents;
    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface, COMAC_STATUS_SURFACE_FINISHED);
	goto zero_extents;
    }

    bounded = FALSE;
    if (surface->backend->get_extents != NULL)
	bounded = surface->backend->get_extents (surface, extents);

    if (! bounded)
	_comac_unbounded_rectangle_init (extents);

    return bounded;

zero_extents:
    extents->x = extents->y = 0;
    extents->width = extents->height = 0;
    return TRUE;
}

/**
 * comac_surface_has_show_text_glyphs:
 * @surface: a #comac_surface_t
 *
 * Returns whether the surface supports
 * sophisticated comac_show_text_glyphs() operations.  That is,
 * whether it actually uses the provided text and cluster data
 * to a comac_show_text_glyphs() call.
 *
 * Note: Even if this function returns %FALSE, a
 * comac_show_text_glyphs() operation targeted at @surface will
 * still succeed.  It just will
 * act like a comac_show_glyphs() operation.  Users can use this
 * function to avoid computing UTF-8 text and cluster mapping if the
 * target surface does not use it.
 *
 * Return value: %TRUE if @surface supports
 *               comac_show_text_glyphs(), %FALSE otherwise
 *
 * Since: 1.8
 **/
comac_bool_t
comac_surface_has_show_text_glyphs (comac_surface_t *surface)
{
    if (unlikely (surface->status))
	return FALSE;

    if (unlikely (surface->finished)) {
	_comac_surface_set_error (surface, COMAC_STATUS_SURFACE_FINISHED);
	return FALSE;
    }

    if (surface->backend->has_show_text_glyphs)
	return surface->backend->has_show_text_glyphs (surface);
    else
	return surface->backend->show_text_glyphs != NULL;
}

#define GLYPH_CACHE_SIZE 64

static inline comac_int_status_t
ensure_scaled_glyph (comac_scaled_font_t *scaled_font,
		     comac_color_t *foreground_color,
		     comac_scaled_glyph_t **glyph_cache,
		     comac_glyph_t *glyph,
		     comac_scaled_glyph_t **scaled_glyph)
{
    int cache_index;
    comac_int_status_t status = COMAC_INT_STATUS_SUCCESS;

    cache_index = glyph->index % GLYPH_CACHE_SIZE;
    *scaled_glyph = glyph_cache[cache_index];
    if (*scaled_glyph == NULL ||
	_comac_scaled_glyph_index (*scaled_glyph) != glyph->index) {
	status =
	    _comac_scaled_glyph_lookup (scaled_font,
					glyph->index,
					COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE,
					foreground_color,
					scaled_glyph);
	if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	    /* If the color surface not available, ensure scaled_glyph is not NULL. */
	    status =
		_comac_scaled_glyph_lookup (scaled_font,
					    glyph->index,
					    COMAC_SCALED_GLYPH_INFO_SURFACE,
					    NULL, /* foreground color */
					    scaled_glyph);
	}
	if (unlikely (status))
	    status = _comac_scaled_font_set_error (scaled_font, status);

	glyph_cache[cache_index] = *scaled_glyph;
    }

    return status;
}

static inline comac_int_status_t
composite_one_color_glyph (comac_surface_t *surface,
			   comac_operator_t op,
			   const comac_pattern_t *source,
			   const comac_clip_t *clip,
			   comac_glyph_t *glyph,
			   comac_scaled_glyph_t *scaled_glyph,
			   double x_scale,
			   double y_scale)
{
    comac_int_status_t status;
    comac_image_surface_t *glyph_surface;
    comac_pattern_t *pattern;
    comac_matrix_t matrix;
    int has_color;

    status = COMAC_INT_STATUS_SUCCESS;

    has_color = scaled_glyph->has_info & COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE;
    if (has_color)
	glyph_surface = scaled_glyph->color_surface;
    else
	glyph_surface = scaled_glyph->surface;

    if (glyph_surface->width && glyph_surface->height) {
	int x, y;
	/* round glyph locations to the nearest pixels */
	/* XXX: FRAGILE: We're ignoring device_transform scaling here. A bug? */
	x = _comac_lround (glyph->x * x_scale -
			   glyph_surface->base.device_transform.x0);
	y = _comac_lround (glyph->y * y_scale -
			   glyph_surface->base.device_transform.y0);

	pattern = comac_pattern_create_for_surface (
	    (comac_surface_t *) glyph_surface);
	comac_matrix_init_translate (&matrix, -x, -y);
	comac_matrix_scale (&matrix, x_scale, y_scale);
	comac_pattern_set_matrix (pattern, &matrix);
	if (op == COMAC_OPERATOR_SOURCE || op == COMAC_OPERATOR_CLEAR ||
	    ! has_color)
	    status = _comac_surface_mask (surface, op, pattern, pattern, clip);
	else
	    status = _comac_surface_paint (surface, op, pattern, clip);
	comac_pattern_destroy (pattern);
    }

    return status;
}

static comac_int_status_t
composite_color_glyphs (comac_surface_t *surface,
			comac_operator_t op,
			const comac_pattern_t *source,
			char *utf8,
			int *utf8_len,
			comac_glyph_t *glyphs,
			int *num_glyphs,
			comac_text_cluster_t *clusters,
			int *num_clusters,
			comac_text_cluster_flags_t cluster_flags,
			comac_scaled_font_t *scaled_font,
			const comac_clip_t *clip)
{
    comac_int_status_t status;
    int i, j;
    comac_scaled_glyph_t *scaled_glyph;
    int remaining_clusters = 0;
    int remaining_glyphs = 0;
    int remaining_bytes = 0;
    int glyph_pos = 0;
    int byte_pos = 0;
    int gp;
    comac_scaled_glyph_t *glyph_cache[GLYPH_CACHE_SIZE];
    comac_color_t *foreground_color = NULL;
    double x_scale = 1.0;
    double y_scale = 1.0;

    if (surface->is_vector) {
	comac_font_face_t *font_face;
	comac_matrix_t font_matrix;
	comac_matrix_t ctm;
	comac_font_options_t font_options;

	x_scale = surface->x_fallback_resolution / surface->x_resolution;
	y_scale = surface->y_fallback_resolution / surface->y_resolution;
	font_face = comac_scaled_font_get_font_face (scaled_font);
	comac_scaled_font_get_font_matrix (scaled_font, &font_matrix);
	comac_scaled_font_get_ctm (scaled_font, &ctm);
	comac_scaled_font_get_font_options (scaled_font, &font_options);
	comac_matrix_scale (&ctm, x_scale, y_scale);
	scaled_font = comac_scaled_font_create (font_face,
						&font_matrix,
						&ctm,
						&font_options);
    }

    if (source->type == COMAC_PATTERN_TYPE_SOLID)
	foreground_color = &((comac_solid_pattern_t *) source)->color;

    memset (glyph_cache, 0, sizeof (glyph_cache));

    status = COMAC_INT_STATUS_SUCCESS;

    _comac_scaled_font_freeze_cache (scaled_font);

    if (clusters) {

	if (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD)
	    glyph_pos = *num_glyphs - 1;

	for (i = 0; i < *num_clusters; i++) {
	    comac_bool_t skip_cluster = TRUE;

	    for (j = 0; j < clusters[i].num_glyphs; j++) {
		if (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD)
		    gp = glyph_pos - j;
		else
		    gp = glyph_pos + j;

		status = ensure_scaled_glyph (scaled_font,
					      foreground_color,
					      glyph_cache,
					      &glyphs[gp],
					      &scaled_glyph);
		if (unlikely (status))
		    goto UNLOCK;

		if ((scaled_glyph->has_info &
		     COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) != 0) {
		    skip_cluster = FALSE;
		    break;
		}
	    }

	    if (skip_cluster) {
		memmove (utf8 + remaining_bytes,
			 utf8 + byte_pos,
			 clusters[i].num_bytes);
		remaining_bytes += clusters[i].num_bytes;
		byte_pos += clusters[i].num_bytes;
		for (j = 0; j < clusters[i].num_glyphs;
		     j++, remaining_glyphs++) {
		    if (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD)
			glyphs[*num_glyphs - 1 - remaining_glyphs] =
			    glyphs[glyph_pos--];
		    else
			glyphs[remaining_glyphs] = glyphs[glyph_pos++];
		}
		clusters[remaining_clusters++] = clusters[i];
		continue;
	    }

	    for (j = 0; j < clusters[i].num_glyphs; j++) {
		if (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD)
		    gp = glyph_pos - j;
		else
		    gp = glyph_pos + j;

		status = ensure_scaled_glyph (scaled_font,
					      foreground_color,
					      glyph_cache,
					      &glyphs[gp],
					      &scaled_glyph);
		if (unlikely (status))
		    goto UNLOCK;

		status = composite_one_color_glyph (surface,
						    op,
						    source,
						    clip,
						    &glyphs[gp],
						    scaled_glyph,
						    x_scale,
						    y_scale);
		if (unlikely (status &&
			      status != COMAC_INT_STATUS_NOTHING_TO_DO))
		    goto UNLOCK;
	    }

	    if (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD)
		glyph_pos -= clusters[i].num_glyphs;
	    else
		glyph_pos += clusters[i].num_glyphs;

	    byte_pos += clusters[i].num_bytes;
	}

	if (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD) {
	    memmove (utf8, utf8 + *utf8_len - remaining_bytes, remaining_bytes);
	    memmove (glyphs,
		     glyphs + (*num_glyphs - remaining_glyphs),
		     sizeof (comac_glyph_t) * remaining_glyphs);
	}

	*utf8_len = remaining_bytes;
	*num_glyphs = remaining_glyphs;
	*num_clusters = remaining_clusters;

    } else {

	for (glyph_pos = 0; glyph_pos < *num_glyphs; glyph_pos++) {
	    status = ensure_scaled_glyph (scaled_font,
					  foreground_color,
					  glyph_cache,
					  &glyphs[glyph_pos],
					  &scaled_glyph);
	    if (unlikely (status))
		goto UNLOCK;

	    if ((scaled_glyph->has_info &
		 COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) == 0) {
		glyphs[remaining_glyphs++] = glyphs[glyph_pos];
		continue;
	    }

	    status = composite_one_color_glyph (surface,
						op,
						source,
						clip,
						&glyphs[glyph_pos],
						scaled_glyph,
						x_scale,
						y_scale);
	    if (unlikely (status && status != COMAC_INT_STATUS_NOTHING_TO_DO))
		goto UNLOCK;
	}

	*num_glyphs = remaining_glyphs;
    }

UNLOCK:
    _comac_scaled_font_thaw_cache (scaled_font);

    if (surface->is_vector)
	comac_scaled_font_destroy (scaled_font);

    return status;
}

/* Note: the backends may modify the contents of the glyph array as long as
 * they do not return %COMAC_INT_STATUS_UNSUPPORTED. This makes it possible to
 * avoid copying the array again and again, and edit it in-place.
 * Backends are in fact free to use the array as a generic buffer as they
 * see fit.
 *
 * For show_glyphs backend method, and NOT for show_text_glyphs method,
 * when they do return UNSUPPORTED, they may adjust remaining_glyphs to notify
 * that they have successfully rendered some of the glyphs (from the beginning
 * of the array), but not all.  If they don't touch remaining_glyphs, it
 * defaults to all glyphs.
 *
 * See commits 5a9642c5746fd677aed35ce620ce90b1029b1a0c and
 * 1781e6018c17909311295a9cc74b70500c6b4d0a for the rationale.
 */
comac_status_t
_comac_surface_show_text_glyphs (comac_surface_t *surface,
				 comac_operator_t op,
				 const comac_pattern_t *source,
				 const char *utf8,
				 int utf8_len,
				 comac_glyph_t *glyphs,
				 int num_glyphs,
				 const comac_text_cluster_t *clusters,
				 int num_clusters,
				 comac_text_cluster_flags_t cluster_flags,
				 comac_scaled_font_t *scaled_font,
				 const comac_clip_t *clip)
{
    comac_int_status_t status;
    char *utf8_copy = NULL;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (num_glyphs == 0 && utf8_len == 0)
	return COMAC_STATUS_SUCCESS;

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    status = _pattern_has_error (source);
    if (unlikely (status))
	return status;

    status = comac_scaled_font_status (scaled_font);
    if (unlikely (status))
	return status;

    if (nothing_to_do (surface, op, source))
	return COMAC_STATUS_SUCCESS;

    status = _comac_surface_begin_modification (surface);
    if (unlikely (status))
	return status;

    if (_comac_scaled_font_has_color_glyphs (scaled_font) &&
	scaled_font->options.color_mode != COMAC_COLOR_MODE_NO_COLOR) {
	utf8_copy = malloc (sizeof (char) * utf8_len);
	memcpy (utf8_copy, utf8, sizeof (char) * utf8_len);
	utf8 = utf8_copy;

	status = composite_color_glyphs (surface,
					 op,
					 source,
					 (char *) utf8,
					 &utf8_len,
					 glyphs,
					 &num_glyphs,
					 (comac_text_cluster_t *) clusters,
					 &num_clusters,
					 cluster_flags,
					 scaled_font,
					 clip);

	if (unlikely (status && status != COMAC_INT_STATUS_NOTHING_TO_DO))
	    goto DONE;

	if (num_glyphs == 0)
	    goto DONE;
    } else
	utf8_copy = NULL;

    /* The logic here is duplicated in _comac_analysis_surface show_glyphs and
     * show_text_glyphs.  Keep in synch. */
    if (clusters) {
	status = COMAC_INT_STATUS_UNSUPPORTED;
	/* A real show_text_glyphs call.  Try show_text_glyphs backend
	 * method first */
	if (surface->backend->show_text_glyphs != NULL) {
	    status = surface->backend->show_text_glyphs (surface,
							 op,
							 source,
							 utf8,
							 utf8_len,
							 glyphs,
							 num_glyphs,
							 clusters,
							 num_clusters,
							 cluster_flags,
							 scaled_font,
							 clip);
	}
	if (status == COMAC_INT_STATUS_UNSUPPORTED &&
	    surface->backend->show_glyphs) {
	    status = surface->backend->show_glyphs (surface,
						    op,
						    source,
						    glyphs,
						    num_glyphs,
						    scaled_font,
						    clip);
	}
    } else {
	/* A mere show_glyphs call.  Try show_glyphs backend method first */
	if (surface->backend->show_glyphs != NULL) {
	    status = surface->backend->show_glyphs (surface,
						    op,
						    source,
						    glyphs,
						    num_glyphs,
						    scaled_font,
						    clip);
	} else if (surface->backend->show_text_glyphs != NULL) {
	    /* Intentionally only try show_text_glyphs method for show_glyphs
	     * calls if backend does not have show_glyphs.  If backend has
	     * both methods implemented, we don't fallback from show_glyphs to
	     * show_text_glyphs, and hence the backend can assume in its
	     * show_text_glyphs call that clusters is not NULL (which also
	     * implies that UTF-8 is not NULL, unless the text is
	     * zero-length).
	     */
	    status = surface->backend->show_text_glyphs (surface,
							 op,
							 source,
							 utf8,
							 utf8_len,
							 glyphs,
							 num_glyphs,
							 clusters,
							 num_clusters,
							 cluster_flags,
							 scaled_font,
							 clip);
	}
    }

DONE:
    if (status != COMAC_INT_STATUS_NOTHING_TO_DO) {
	surface->is_clear = FALSE;
	surface->serial++;
    }

    if (utf8_copy)
	free (utf8_copy);

    return _comac_surface_set_error (surface, status);
}

comac_status_t
_comac_surface_tag (comac_surface_t *surface,
		    comac_bool_t begin,
		    const char *tag_name,
		    const char *attributes)
{
    comac_int_status_t status;

    TRACE ((stderr, "%s\n", __FUNCTION__));
    if (unlikely (surface->status))
	return surface->status;
    if (unlikely (surface->finished))
	return _comac_surface_set_error (
	    surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (surface->backend->tag == NULL)
	return COMAC_STATUS_SUCCESS;

    status = surface->backend->tag (surface, begin, tag_name, attributes);
    surface->is_clear = FALSE;

    return _comac_surface_set_error (surface, status);
}

/**
 * _comac_surface_set_resolution:
 * @surface: the surface
 * @x_res: x resolution, in dpi
 * @y_res: y resolution, in dpi
 *
 * Set the actual surface resolution of @surface to the given x and y DPI.
 * Mainly used for correctly computing the scale factor when fallback
 * rendering needs to take place in the paginated surface.
 **/
void
_comac_surface_set_resolution (comac_surface_t *surface,
			       double x_res,
			       double y_res)
{
    if (surface->status)
	return;

    surface->x_resolution = x_res;
    surface->y_resolution = y_res;
}

/**
 * _comac_surface_create_in_error:
 * @status: the error status
 *
 * Return an appropriate static error surface for the error status.
 * On error, surface creation functions should always return a surface
 * created with _comac_surface_create_in_error() instead of a new surface
 * in an error state. This simplifies internal code as no refcounting has
 * to be done.
 **/
comac_surface_t *
_comac_surface_create_in_error (comac_status_t status)
{
    assert (status < COMAC_STATUS_LAST_STATUS);
    switch (status) {
    case COMAC_STATUS_NO_MEMORY:
	return (comac_surface_t *) &_comac_surface_nil;
    case COMAC_STATUS_SURFACE_TYPE_MISMATCH:
	return (comac_surface_t *) &_comac_surface_nil_surface_type_mismatch;
    case COMAC_STATUS_INVALID_STATUS:
	return (comac_surface_t *) &_comac_surface_nil_invalid_status;
    case COMAC_STATUS_INVALID_CONTENT:
	return (comac_surface_t *) &_comac_surface_nil_invalid_content;
    case COMAC_STATUS_INVALID_FORMAT:
	return (comac_surface_t *) &_comac_surface_nil_invalid_format;
    case COMAC_STATUS_INVALID_VISUAL:
	return (comac_surface_t *) &_comac_surface_nil_invalid_visual;
    case COMAC_STATUS_READ_ERROR:
	return (comac_surface_t *) &_comac_surface_nil_read_error;
    case COMAC_STATUS_WRITE_ERROR:
	return (comac_surface_t *) &_comac_surface_nil_write_error;
    case COMAC_STATUS_FILE_NOT_FOUND:
	return (comac_surface_t *) &_comac_surface_nil_file_not_found;
    case COMAC_STATUS_TEMP_FILE_ERROR:
	return (comac_surface_t *) &_comac_surface_nil_temp_file_error;
    case COMAC_STATUS_INVALID_STRIDE:
	return (comac_surface_t *) &_comac_surface_nil_invalid_stride;
    case COMAC_STATUS_INVALID_SIZE:
	return (comac_surface_t *) &_comac_surface_nil_invalid_size;
    case COMAC_STATUS_DEVICE_TYPE_MISMATCH:
	return (comac_surface_t *) &_comac_surface_nil_device_type_mismatch;
    case COMAC_STATUS_DEVICE_ERROR:
	return (comac_surface_t *) &_comac_surface_nil_device_error;
    case COMAC_STATUS_SUCCESS:
    case COMAC_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	/* fall-through */
    case COMAC_STATUS_INVALID_RESTORE:
    case COMAC_STATUS_INVALID_POP_GROUP:
    case COMAC_STATUS_NO_CURRENT_POINT:
    case COMAC_STATUS_INVALID_MATRIX:
    case COMAC_STATUS_NULL_POINTER:
    case COMAC_STATUS_INVALID_STRING:
    case COMAC_STATUS_INVALID_PATH_DATA:
    case COMAC_STATUS_SURFACE_FINISHED:
    case COMAC_STATUS_PATTERN_TYPE_MISMATCH:
    case COMAC_STATUS_INVALID_DASH:
    case COMAC_STATUS_INVALID_DSC_COMMENT:
    case COMAC_STATUS_INVALID_INDEX:
    case COMAC_STATUS_CLIP_NOT_REPRESENTABLE:
    case COMAC_STATUS_FONT_TYPE_MISMATCH:
    case COMAC_STATUS_USER_FONT_IMMUTABLE:
    case COMAC_STATUS_USER_FONT_ERROR:
    case COMAC_STATUS_NEGATIVE_COUNT:
    case COMAC_STATUS_INVALID_CLUSTERS:
    case COMAC_STATUS_INVALID_SLANT:
    case COMAC_STATUS_INVALID_WEIGHT:
    case COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED:
    case COMAC_STATUS_INVALID_MESH_CONSTRUCTION:
    case COMAC_STATUS_DEVICE_FINISHED:
    case COMAC_STATUS_JBIG2_GLOBAL_MISSING:
    case COMAC_STATUS_PNG_ERROR:
    case COMAC_STATUS_FREETYPE_ERROR:
    case COMAC_STATUS_WIN32_GDI_ERROR:
    case COMAC_INT_STATUS_DWRITE_ERROR:
    case COMAC_STATUS_TAG_ERROR:
    default:
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_surface_t *) &_comac_surface_nil;
    }
}

comac_surface_t *
_comac_int_surface_create_in_error (comac_int_status_t status)
{
    if (status < COMAC_INT_STATUS_LAST_STATUS)
	return _comac_surface_create_in_error (status);

    switch ((int) status) {
    case COMAC_INT_STATUS_UNSUPPORTED:
	return (comac_surface_t *) &_comac_surface_nil_unsupported;
    case COMAC_INT_STATUS_NOTHING_TO_DO:
	return (comac_surface_t *) &_comac_surface_nil_nothing_to_do;
    default:
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_surface_t *) &_comac_surface_nil;
    }
}

/*  LocalWords:  rasterized
 */
