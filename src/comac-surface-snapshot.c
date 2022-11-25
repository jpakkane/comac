/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@cworth.org>
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-surface-snapshot-inline.h"

static comac_status_t
_comac_surface_snapshot_finish (void *abstract_surface)
{
    comac_surface_snapshot_t *surface = abstract_surface;
    comac_status_t status = COMAC_STATUS_SUCCESS;

    TRACE ((stderr, "%s\n", __FUNCTION__));

    if (surface->clone != NULL) {
	comac_surface_finish (surface->clone);
	status = surface->clone->status;

	comac_surface_destroy (surface->clone);
    }

    COMAC_MUTEX_FINI (surface->mutex);

    return status;
}

static comac_status_t
_comac_surface_snapshot_flush (void *abstract_surface, unsigned flags)
{
    comac_surface_snapshot_t *surface = abstract_surface;
    comac_surface_t *target;
    comac_status_t status;

    target = _comac_surface_snapshot_get_target (&surface->base);
    status = target->status;
    if (status == COMAC_STATUS_SUCCESS)
	status = _comac_surface_flush (target, flags);
    comac_surface_destroy (target);

    return status;
}

static comac_surface_t *
_comac_surface_snapshot_source (void                    *abstract_surface,
				comac_rectangle_int_t *extents)
{
    comac_surface_snapshot_t *surface = abstract_surface;
    return _comac_surface_get_source (surface->target, extents); /* XXX racy */
}

struct snapshot_extra {
    comac_surface_t *target;
    void *extra;
};

static comac_status_t
_comac_surface_snapshot_acquire_source_image (void                    *abstract_surface,
					      comac_image_surface_t  **image_out,
					      void                   **extra_out)
{
    comac_surface_snapshot_t *surface = abstract_surface;
    struct snapshot_extra *extra;
    comac_status_t status;

    extra = _comac_malloc (sizeof (*extra));
    if (unlikely (extra == NULL)) {
	*extra_out = NULL;
	return _comac_error (COMAC_STATUS_NO_MEMORY);
    }

    extra->target = _comac_surface_snapshot_get_target (&surface->base);
    status =  _comac_surface_acquire_source_image (extra->target, image_out, &extra->extra);
    if (unlikely (status)) {
	comac_surface_destroy (extra->target);
	free (extra);
	extra = NULL;
    }

    *extra_out = extra;
    return status;
}

static void
_comac_surface_snapshot_release_source_image (void                   *abstract_surface,
					      comac_image_surface_t  *image,
					      void                   *_extra)
{
    struct snapshot_extra *extra = _extra;

    _comac_surface_release_source_image (extra->target, image, extra->extra);
    comac_surface_destroy (extra->target);
    free (extra);
}

static comac_bool_t
_comac_surface_snapshot_get_extents (void                  *abstract_surface,
				     comac_rectangle_int_t *extents)
{
    comac_surface_snapshot_t *surface = abstract_surface;
    comac_surface_t *target;
    comac_bool_t bounded;

    target = _comac_surface_snapshot_get_target (&surface->base);
    bounded = _comac_surface_get_extents (target, extents);
    comac_surface_destroy (target);

    return bounded;
}

static const comac_surface_backend_t _comac_surface_snapshot_backend = {
    COMAC_INTERNAL_SURFACE_TYPE_SNAPSHOT,
    _comac_surface_snapshot_finish,
    NULL,

    NULL, /* create similar */
    NULL, /* create similar image  */
    NULL, /* map to image */
    NULL, /* unmap image  */

    _comac_surface_snapshot_source,
    _comac_surface_snapshot_acquire_source_image,
    _comac_surface_snapshot_release_source_image,
    NULL, /* snapshot */

    NULL, /* copy_page */
    NULL, /* show_page */

    _comac_surface_snapshot_get_extents,
    NULL, /* get-font-options */

    _comac_surface_snapshot_flush,
};

static void
_comac_surface_snapshot_copy_on_write (comac_surface_t *surface)
{
    comac_surface_snapshot_t *snapshot = (comac_surface_snapshot_t *) surface;
    comac_image_surface_t *image;
    comac_surface_t *clone;
    void *extra;
    comac_status_t status;

    TRACE ((stderr, "%s: target=%d\n",
	    __FUNCTION__, snapshot->target->unique_id));

    /* We need to make an image copy of the original surface since the
     * snapshot may exceed the lifetime of the original device, i.e.
     * when we later need to use the snapshot the data may have already
     * been lost.
     */

    COMAC_MUTEX_LOCK (snapshot->mutex);

    if (snapshot->target->backend->snapshot != NULL) {
	clone = snapshot->target->backend->snapshot (snapshot->target);
	if (clone != NULL) {
	    assert (clone->status || ! _comac_surface_is_snapshot (clone));
	    goto done;
	}
    }

    /* XXX copy to a similar surface, leave acquisition till later?
     * We should probably leave such decisions to the backend in case we
     * rely upon devices/connections like Xlib.
    */
    status = _comac_surface_acquire_source_image (snapshot->target, &image, &extra);
    if (unlikely (status)) {
	snapshot->target = _comac_surface_create_in_error (status);
	status = _comac_surface_set_error (surface, status);
	goto unlock;
    }
    clone = image->base.backend->snapshot (&image->base);
    _comac_surface_release_source_image (snapshot->target, image, extra);

done:
    status = _comac_surface_set_error (surface, clone->status);
    snapshot->target = snapshot->clone = clone;
    snapshot->base.type = clone->type;
unlock:
    COMAC_MUTEX_UNLOCK (snapshot->mutex);
}

/**
 * _comac_surface_snapshot:
 * @surface: a #comac_surface_t
 *
 * Make an immutable reference to @surface. It is an error to call a
 * surface-modifying function on the result of this function. The
 * resulting 'snapshot' is a lazily copied-on-write surface i.e. it
 * remains a reference to the original surface until that surface is
 * written to again, at which time a copy is made of the original surface
 * and the snapshot then points to that instead. Multiple snapshots of the
 * same unmodified surface point to the same copy.
 *
 * The caller owns the return value and should call
 * comac_surface_destroy() when finished with it. This function will not
 * return %NULL, but will return a nil surface instead.
 *
 * Return value: The snapshot surface. Note that the return surface
 * may not necessarily be of the same type as @surface.
 **/
comac_surface_t *
_comac_surface_snapshot (comac_surface_t *surface)
{
    comac_surface_snapshot_t *snapshot;
    comac_status_t status;

    TRACE ((stderr, "%s: target=%d\n", __FUNCTION__, surface->unique_id));

    if (unlikely (surface->status))
	return _comac_surface_create_in_error (surface->status);

    if (unlikely (surface->finished))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (surface->snapshot_of != NULL)
	return comac_surface_reference (surface);

    if (_comac_surface_is_snapshot (surface))
	return comac_surface_reference (surface);

    snapshot = (comac_surface_snapshot_t *)
	_comac_surface_has_snapshot (surface, &_comac_surface_snapshot_backend);
    if (snapshot != NULL)
	return comac_surface_reference (&snapshot->base);

    snapshot = _comac_malloc (sizeof (comac_surface_snapshot_t));
    if (unlikely (snapshot == NULL))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_SURFACE_FINISHED));

    _comac_surface_init (&snapshot->base,
			 &_comac_surface_snapshot_backend,
			 NULL, /* device */
			 surface->content,
			 surface->is_vector);
    snapshot->base.type = surface->type;

    COMAC_MUTEX_INIT (snapshot->mutex);
    snapshot->target = surface;
    snapshot->clone = NULL;

    status = _comac_surface_copy_mime_data (&snapshot->base, surface);
    if (unlikely (status)) {
	comac_surface_destroy (&snapshot->base);
	return _comac_surface_create_in_error (status);
    }

    snapshot->base.device_transform = surface->device_transform;
    snapshot->base.device_transform_inverse = surface->device_transform_inverse;

    _comac_surface_attach_snapshot (surface,
				    &snapshot->base,
				    _comac_surface_snapshot_copy_on_write);

    return &snapshot->base;
}
