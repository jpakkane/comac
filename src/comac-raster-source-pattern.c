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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"
#include "comac-error-private.h"
#include "comac-pattern-private.h"

/**
 * SECTION:comac-raster-source
 * @Title: Raster Sources
 * @Short_Description: Supplying arbitrary image data
 * @See_Also: #comac_pattern_t
 *
 * The raster source provides the ability to supply arbitrary pixel data
 * whilst rendering. The pixels are queried at the time of rasterisation
 * by means of user callback functions, allowing for the ultimate
 * flexibility. For example, in handling compressed image sources, you
 * may keep a MRU cache of decompressed images and decompress sources on the
 * fly and discard old ones to conserve memory.
 *
 * For the raster source to be effective, you must at least specify
 * the acquire and release callbacks which are used to retrieve the pixel
 * data for the region of interest and demark when it can be freed afterwards.
 * Other callbacks are provided for when the pattern is copied temporarily
 * during rasterisation, or more permanently as a snapshot in order to keep
 * the pixel data available for printing.
 **/

comac_surface_t *
_comac_raster_source_pattern_acquire (const comac_pattern_t *abstract_pattern,
				      comac_surface_t *target,
				      const comac_rectangle_int_t *extents)
{
    comac_raster_source_pattern_t *pattern =
	(comac_raster_source_pattern_t *) abstract_pattern;

    if (pattern->acquire == NULL)
	return NULL;

    if (extents == NULL)
	extents = &pattern->extents;

    return pattern->acquire (&pattern->base,
			     pattern->user_data,
			     target,
			     extents);
}

void
_comac_raster_source_pattern_release (const comac_pattern_t *abstract_pattern,
				      comac_surface_t *surface)
{
    comac_raster_source_pattern_t *pattern =
	(comac_raster_source_pattern_t *) abstract_pattern;

    if (pattern->release == NULL)
	return;

    pattern->release (&pattern->base, pattern->user_data, surface);
}

comac_status_t
_comac_raster_source_pattern_init_copy (comac_pattern_t *abstract_pattern,
					const comac_pattern_t *other)
{
    comac_raster_source_pattern_t *pattern =
	(comac_raster_source_pattern_t *) abstract_pattern;
    comac_status_t status;

    VG (VALGRIND_MAKE_MEM_UNDEFINED (pattern,
				     sizeof (comac_raster_source_pattern_t)));
    memcpy (pattern, other, sizeof (comac_raster_source_pattern_t));

    status = COMAC_STATUS_SUCCESS;
    if (pattern->copy)
	status = pattern->copy (&pattern->base, pattern->user_data, other);

    return status;
}

comac_status_t
_comac_raster_source_pattern_snapshot (comac_pattern_t *abstract_pattern)
{
    comac_raster_source_pattern_t *pattern =
	(comac_raster_source_pattern_t *) abstract_pattern;

    if (pattern->snapshot == NULL)
	return COMAC_STATUS_SUCCESS;

    return pattern->snapshot (&pattern->base, pattern->user_data);
}

void
_comac_raster_source_pattern_finish (comac_pattern_t *abstract_pattern)
{
    comac_raster_source_pattern_t *pattern =
	(comac_raster_source_pattern_t *) abstract_pattern;

    if (pattern->finish == NULL)
	return;

    pattern->finish (&pattern->base, pattern->user_data);
}

/* Public interface */

/**
 * comac_pattern_create_raster_source:
 * @user_data: the user data to be passed to all callbacks
 * @content: content type for the pixel data that will be returned. Knowing
 * the content type ahead of time is used for analysing the operation and
 * picking the appropriate rendering path.
 * @width: maximum size of the sample area
 * @height: maximum size of the sample area
 *
 * Creates a new user pattern for providing pixel data.
 *
 * Use the setter functions to associate callbacks with the returned
 * pattern.  The only mandatory callback is acquire.
 *
 * Return value: a newly created #comac_pattern_t. Free with
 *  comac_pattern_destroy() when you are done using it.
 *
 * Since: 1.12
 **/
comac_pattern_t *
comac_pattern_create_raster_source (void *user_data,
				    comac_content_t content,
				    int width,
				    int height)
{
    comac_raster_source_pattern_t *pattern;

    COMAC_MUTEX_INITIALIZE ();

    if (width < 0 || height < 0)
	return _comac_pattern_create_in_error (COMAC_STATUS_INVALID_SIZE);

    if (! COMAC_CONTENT_VALID (content))
	return _comac_pattern_create_in_error (COMAC_STATUS_INVALID_CONTENT);

    pattern = calloc (1, sizeof (*pattern));
    if (unlikely (pattern == NULL))
	return _comac_pattern_create_in_error (COMAC_STATUS_NO_MEMORY);

    _comac_pattern_init (&pattern->base, COMAC_PATTERN_TYPE_RASTER_SOURCE);
    COMAC_REFERENCE_COUNT_INIT (&pattern->base.ref_count, 1);

    pattern->content = content;

    pattern->extents.x = 0;
    pattern->extents.y = 0;
    pattern->extents.width = width;
    pattern->extents.height = height;

    pattern->user_data = user_data;

    return &pattern->base;
}

/**
 * comac_raster_source_pattern_set_callback_data:
 * @pattern: the pattern to update
 * @data: the user data to be passed to all callbacks
 *
 * Updates the user data that is provided to all callbacks.
 *
 * Since: 1.12
 **/
void
comac_raster_source_pattern_set_callback_data (
    comac_pattern_t *abstract_pattern, void *data)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    pattern->user_data = data;
}

/**
 * comac_raster_source_pattern_get_callback_data:
 * @pattern: the pattern to update
 *
 * Queries the current user data.
 *
 * Return value: the current user-data passed to each callback
 *
 * Since: 1.12
 **/
void *
comac_raster_source_pattern_get_callback_data (
    comac_pattern_t *abstract_pattern)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return NULL;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    return pattern->user_data;
}

/**
 * comac_raster_source_pattern_set_acquire:
 * @pattern: the pattern to update
 * @acquire: acquire callback
 * @release: release callback
 *
 * Specifies the callbacks used to generate the image surface for a rendering
 * operation (acquire) and the function used to cleanup that surface afterwards.
 *
 * The @acquire callback should create a surface (preferably an image
 * surface created to match the target using
 * comac_surface_create_similar_image()) that defines at least the region
 * of interest specified by extents. The surface is allowed to be the entire
 * sample area, but if it does contain a subsection of the sample area,
 * the surface extents should be provided by setting the device offset (along
 * with its width and height) using comac_surface_set_device_offset().
 *
 * Since: 1.12
 **/
void
comac_raster_source_pattern_set_acquire (
    comac_pattern_t *abstract_pattern,
    comac_raster_source_acquire_func_t acquire,
    comac_raster_source_release_func_t release)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    pattern->acquire = acquire;
    pattern->release = release;
}

/**
 * comac_raster_source_pattern_get_acquire:
 * @pattern: the pattern to query
 * @acquire: return value for the current acquire callback
 * @release: return value for the current release callback
 *
 * Queries the current acquire and release callbacks.
 *
 * Since: 1.12
 **/
void
comac_raster_source_pattern_get_acquire (
    comac_pattern_t *abstract_pattern,
    comac_raster_source_acquire_func_t *acquire,
    comac_raster_source_release_func_t *release)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    if (acquire)
	*acquire = pattern->acquire;
    if (release)
	*release = pattern->release;
}

/**
 * comac_raster_source_pattern_set_snapshot:
 * @pattern: the pattern to update
 * @snapshot: snapshot callback
 *
 * Sets the callback that will be used whenever a snapshot is taken of the
 * pattern, that is whenever the current contents of the pattern should be
 * preserved for later use. This is typically invoked whilst printing.
 *
 * Since: 1.12
 **/
void
comac_raster_source_pattern_set_snapshot (
    comac_pattern_t *abstract_pattern,
    comac_raster_source_snapshot_func_t snapshot)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    pattern->snapshot = snapshot;
}

/**
 * comac_raster_source_pattern_get_snapshot:
 * @pattern: the pattern to query
 *
 * Queries the current snapshot callback.
 *
 * Return value: the current snapshot callback
 *
 * Since: 1.12
 **/
comac_raster_source_snapshot_func_t
comac_raster_source_pattern_get_snapshot (comac_pattern_t *abstract_pattern)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return NULL;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    return pattern->snapshot;
}

/**
 * comac_raster_source_pattern_set_copy:
 * @pattern: the pattern to update
 * @copy: the copy callback
 *
 * Updates the copy callback which is used whenever a temporary copy of the
 * pattern is taken.
 *
 * Since: 1.12
 **/
void
comac_raster_source_pattern_set_copy (comac_pattern_t *abstract_pattern,
				      comac_raster_source_copy_func_t copy)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    pattern->copy = copy;
}

/**
 * comac_raster_source_pattern_get_copy:
 * @pattern: the pattern to query
 *
 * Queries the current copy callback.
 *
 * Return value: the current copy callback
 *
 * Since: 1.12
 **/
comac_raster_source_copy_func_t
comac_raster_source_pattern_get_copy (comac_pattern_t *abstract_pattern)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return NULL;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    return pattern->copy;
}

/**
 * comac_raster_source_pattern_set_finish:
 * @pattern: the pattern to update
 * @finish: the finish callback
 *
 * Updates the finish callback which is used whenever a pattern (or a copy
 * thereof) will no longer be used.
 *
 * Since: 1.12
 **/
void
comac_raster_source_pattern_set_finish (
    comac_pattern_t *abstract_pattern, comac_raster_source_finish_func_t finish)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    pattern->finish = finish;
}

/**
 * comac_raster_source_pattern_get_finish:
 * @pattern: the pattern to query
 *
 * Queries the current finish callback.
 *
 * Return value: the current finish callback
 *
 * Since: 1.12
 **/
comac_raster_source_finish_func_t
comac_raster_source_pattern_get_finish (comac_pattern_t *abstract_pattern)
{
    comac_raster_source_pattern_t *pattern;

    if (abstract_pattern->type != COMAC_PATTERN_TYPE_RASTER_SOURCE)
	return NULL;

    pattern = (comac_raster_source_pattern_t *) abstract_pattern;
    return pattern->finish;
}
