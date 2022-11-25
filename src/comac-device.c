/* Comac - a vector graphics library with display and print output
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
 * Contributors(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"
#include "comac-device-private.h"
#include "comac-error-private.h"

/**
 * SECTION:comac-device
 * @Title: comac_device_t
 * @Short_Description: interface to underlying rendering system
 * @See_Also: #comac_surface_t
 *
 * Devices are the abstraction Comac employs for the rendering system
 * used by a #comac_surface_t. You can get the device of a surface using
 * comac_surface_get_device().
 *
 * Devices are created using custom functions specific to the rendering
 * system you want to use. See the documentation for the surface types
 * for those functions.
 *
 * An important function that devices fulfill is sharing access to the
 * rendering system between Comac and your application. If you want to
 * access a device directly that you used to draw to with Comac, you must
 * first call comac_device_flush() to ensure that Comac finishes all
 * operations on the device and resets it to a clean state.
 *
 * Comac also provides the functions comac_device_acquire() and
 * comac_device_release() to synchronize access to the rendering system
 * in a multithreaded environment. This is done internally, but can also
 * be used by applications.
 *
 * Putting this all together, a function that works with devices should
 * look something like this:
 * <informalexample><programlisting>
 * void
 * my_device_modifying_function (comac_device_t *device)
 * {
 *   comac_status_t status;
 *
 *   // Ensure the device is properly reset
 *   comac_device_flush (device);
 *   // Try to acquire the device
 *   status = comac_device_acquire (device);
 *   if (status != COMAC_STATUS_SUCCESS) {
 *     printf ("Failed to acquire the device: %s\n", comac_status_to_string (status));
 *     return;
 *   }
 *
 *   // Do the custom operations on the device here.
 *   // But do not call any Comac functions that might acquire devices.
 *   
 *   // Release the device when done.
 *   comac_device_release (device);
 * }
 * </programlisting></informalexample>
 *
 * <note><para>Please refer to the documentation of each backend for
 * additional usage requirements, guarantees provided, and
 * interactions with existing surface API of the device functions for
 * surfaces of that type.
 * </para></note>
 **/

static const comac_device_t _nil_device = {
    COMAC_REFERENCE_COUNT_INVALID,
    COMAC_STATUS_NO_MEMORY,
};

static const comac_device_t _mismatch_device = {
    COMAC_REFERENCE_COUNT_INVALID,
    COMAC_STATUS_DEVICE_TYPE_MISMATCH,
};

static const comac_device_t _invalid_device = {
    COMAC_REFERENCE_COUNT_INVALID,
    COMAC_STATUS_DEVICE_ERROR,
};

comac_device_t *
_comac_device_create_in_error (comac_status_t status)
{
    switch (status) {
    case COMAC_STATUS_NO_MEMORY:
	return (comac_device_t *) &_nil_device;
    case COMAC_STATUS_DEVICE_ERROR:
	return (comac_device_t *) &_invalid_device;
    case COMAC_STATUS_DEVICE_TYPE_MISMATCH:
	return (comac_device_t *) &_mismatch_device;

    case COMAC_STATUS_SUCCESS:
    case COMAC_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	/* fall-through */
    case COMAC_STATUS_SURFACE_TYPE_MISMATCH:
    case COMAC_STATUS_INVALID_STATUS:
    case COMAC_STATUS_INVALID_FORMAT:
    case COMAC_STATUS_INVALID_VISUAL:
    case COMAC_STATUS_READ_ERROR:
    case COMAC_STATUS_WRITE_ERROR:
    case COMAC_STATUS_FILE_NOT_FOUND:
    case COMAC_STATUS_TEMP_FILE_ERROR:
    case COMAC_STATUS_INVALID_STRIDE:
    case COMAC_STATUS_INVALID_SIZE:
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
    case COMAC_STATUS_INVALID_CONTENT:
    case COMAC_STATUS_INVALID_MESH_CONSTRUCTION:
    case COMAC_STATUS_DEVICE_FINISHED:
    case COMAC_STATUS_JBIG2_GLOBAL_MISSING:
    case COMAC_STATUS_PNG_ERROR:
    case COMAC_STATUS_FREETYPE_ERROR:
    case COMAC_STATUS_WIN32_GDI_ERROR:
    case COMAC_STATUS_TAG_ERROR:
    case COMAC_STATUS_DWRITE_ERROR:
    default:
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_device_t *) &_nil_device;
    }
}

void
_comac_device_init (comac_device_t *device,
		    const comac_device_backend_t *backend)
{
    COMAC_REFERENCE_COUNT_INIT (&device->ref_count, 1);
    device->status = COMAC_STATUS_SUCCESS;

    device->backend = backend;

    COMAC_RECURSIVE_MUTEX_INIT (device->mutex);
    device->mutex_depth = 0;

    device->finished = FALSE;

    _comac_user_data_array_init (&device->user_data);
}

/**
 * comac_device_reference:
 * @device: a #comac_device_t
 *
 * Increases the reference count on @device by one. This prevents
 * @device from being destroyed until a matching call to
 * comac_device_destroy() is made.
 *
 * Use comac_device_get_reference_count() to get the number of references
 * to a #comac_device_t.
 *
 * Return value: the referenced #comac_device_t.
 *
 * Since: 1.10
 **/
comac_device_t *
comac_device_reference (comac_device_t *device)
{
    if (device == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return device;
    }

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&device->ref_count));
    _comac_reference_count_inc (&device->ref_count);

    return device;
}

/**
 * comac_device_status:
 * @device: a #comac_device_t
 *
 * Checks whether an error has previously occurred for this
 * device.
 *
 * Return value: %COMAC_STATUS_SUCCESS on success or an error code if
 *               the device is in an error state.
 *
 * Since: 1.10
 **/
comac_status_t
comac_device_status (comac_device_t *device)
{
    if (device == NULL)
	return COMAC_STATUS_NULL_POINTER;

    return device->status;
}

/**
 * comac_device_flush:
 * @device: a #comac_device_t
 *
 * Finish any pending operations for the device and also restore any
 * temporary modifications comac has made to the device's state.
 * This function must be called before switching from using the 
 * device with Comac to operating on it directly with native APIs.
 * If the device doesn't support direct access, then this function
 * does nothing.
 *
 * This function may acquire devices.
 *
 * Since: 1.10
 **/
void
comac_device_flush (comac_device_t *device)
{
    comac_status_t status;

    if (device == NULL || device->status)
	return;

    if (device->finished)
	return;

    if (device->backend->flush != NULL) {
	status = device->backend->flush (device);
	if (unlikely (status))
	    status = _comac_device_set_error (device, status);
    }
}

/**
 * comac_device_finish:
 * @device: the #comac_device_t to finish
 *
 * This function finishes the device and drops all references to
 * external resources. All surfaces, fonts and other objects created
 * for this @device will be finished, too.
 * Further operations on the @device will not affect the @device but
 * will instead trigger a %COMAC_STATUS_DEVICE_FINISHED error.
 *
 * When the last call to comac_device_destroy() decreases the
 * reference count to zero, comac will call comac_device_finish() if
 * it hasn't been called already, before freeing the resources
 * associated with the device.
 *
 * This function may acquire devices.
 *
 * Since: 1.10
 **/
void
comac_device_finish (comac_device_t *device)
{
    if (device == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return;
    }

    if (device->finished)
	return;

    comac_device_flush (device);

    if (device->backend->finish != NULL)
	device->backend->finish (device);

    /* We only finish the device after the backend's callback returns because
     * the device might still be needed during the callback
     * (e.g. for comac_device_acquire ()).
     */
    device->finished = TRUE;
}

/**
 * comac_device_destroy:
 * @device: a #comac_device_t
 *
 * Decreases the reference count on @device by one. If the result is
 * zero, then @device and all associated resources are freed.  See
 * comac_device_reference().
 *
 * This function may acquire devices if the last reference was dropped.
 *
 * Since: 1.10
 **/
void
comac_device_destroy (comac_device_t *device)
{
    comac_user_data_array_t user_data;

    if (device == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return;
    }

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&device->ref_count));
    if (! _comac_reference_count_dec_and_test (&device->ref_count))
	return;

    comac_device_finish (device);

    assert (device->mutex_depth == 0);
    COMAC_MUTEX_FINI (device->mutex);

    user_data = device->user_data;

    device->backend->destroy (device);

    _comac_user_data_array_fini (&user_data);

}

/**
 * comac_device_get_type:
 * @device: a #comac_device_t
 *
 * This function returns the type of the device. See #comac_device_type_t
 * for available types.
 *
 * Return value: The type of @device.
 *
 * Since: 1.10
 **/
comac_device_type_t
comac_device_get_type (comac_device_t *device)
{
    if (device == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
    {
	return COMAC_DEVICE_TYPE_INVALID;
    }

    return device->backend->type;
}

/**
 * comac_device_acquire:
 * @device: a #comac_device_t
 *
 * Acquires the @device for the current thread. This function will block
 * until no other thread has acquired the device.
 *
 * If the return value is %COMAC_STATUS_SUCCESS, you successfully acquired the
 * device. From now on your thread owns the device and no other thread will be
 * able to acquire it until a matching call to comac_device_release(). It is
 * allowed to recursively acquire the device multiple times from the same
 * thread.
 *
 * <note><para>You must never acquire two different devices at the same time
 * unless this is explicitly allowed. Otherwise the possibility of deadlocks
 * exist.
 *
 * As various Comac functions can acquire devices when called, these functions
 * may also cause deadlocks when you call them with an acquired device. So you
 * must not have a device acquired when calling them. These functions are
 * marked in the documentation.
 * </para></note>
 *
 * Return value: %COMAC_STATUS_SUCCESS on success or an error code if
 *               the device is in an error state and could not be
 *               acquired. After a successful call to comac_device_acquire(),
 *               a matching call to comac_device_release() is required.
 *
 * Since: 1.10
 **/
comac_status_t
comac_device_acquire (comac_device_t *device)
{
    if (device == NULL)
	return COMAC_STATUS_SUCCESS;

    if (unlikely (device->status))
	return device->status;

    if (unlikely (device->finished))
	return _comac_device_set_error (device, COMAC_STATUS_DEVICE_FINISHED);

    COMAC_MUTEX_LOCK (device->mutex);
    if (device->mutex_depth++ == 0) {
	if (device->backend->lock != NULL)
	    device->backend->lock (device);
    }

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_device_release:
 * @device: a #comac_device_t
 *
 * Releases a @device previously acquired using comac_device_acquire(). See
 * that function for details.
 *
 * Since: 1.10
 **/
void
comac_device_release (comac_device_t *device)
{
    if (device == NULL)
	return;

    assert (device->mutex_depth > 0);

    if (--device->mutex_depth == 0) {
	if (device->backend->unlock != NULL)
	    device->backend->unlock (device);
    }

    COMAC_MUTEX_UNLOCK (device->mutex);
}

comac_status_t
_comac_device_set_error (comac_device_t *device,
			 comac_status_t  status)
{
    if (status == COMAC_STATUS_SUCCESS)
        return COMAC_STATUS_SUCCESS;

    _comac_status_set_error (&device->status, status);

    return _comac_error (status);
}

/**
 * comac_device_get_reference_count:
 * @device: a #comac_device_t
 *
 * Returns the current reference count of @device.
 *
 * Return value: the current reference count of @device.  If the
 * object is a nil object, 0 will be returned.
 *
 * Since: 1.10
 **/
unsigned int
comac_device_get_reference_count (comac_device_t *device)
{
    if (device == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
	return 0;

    return COMAC_REFERENCE_COUNT_GET_VALUE (&device->ref_count);
}

/**
 * comac_device_get_user_data:
 * @device: a #comac_device_t
 * @key: the address of the #comac_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @device using the
 * specified key.  If no user data has been attached with the given
 * key this function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 *
 * Since: 1.10
 **/
void *
comac_device_get_user_data (comac_device_t		 *device,
			    const comac_user_data_key_t *key)
{
    return _comac_user_data_array_get_data (&device->user_data,
					    key);
}

/**
 * comac_device_set_user_data:
 * @device: a #comac_device_t
 * @key: the address of a #comac_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the #comac_device_t
 * @destroy: a #comac_destroy_func_t which will be called when the
 * #comac_t is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @device.  To remove user data from a surface,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 *
 * Since: 1.10
 **/
comac_status_t
comac_device_set_user_data (comac_device_t		 *device,
			    const comac_user_data_key_t *key,
			    void			 *user_data,
			    comac_destroy_func_t	  destroy)
{
    if (COMAC_REFERENCE_COUNT_IS_INVALID (&device->ref_count))
	return device->status;

    return _comac_user_data_array_set_data (&device->user_data,
					    key, user_data, destroy);
}
