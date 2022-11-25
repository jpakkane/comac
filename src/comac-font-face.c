/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat Inc.
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
 *      Graydon Hoare <graydon@redhat.com>
 *      Owen Taylor <otaylor@redhat.com>
 */

#include "comacint.h"
#include "comac-error-private.h"

/**
 * SECTION:comac-font-face
 * @Title: comac_font_face_t
 * @Short_Description: Base class for font faces
 * @See_Also: #comac_scaled_font_t
 *
 * #comac_font_face_t represents a particular font at a particular weight,
 * slant, and other characteristic but no size, transformation, or size.
 *
 * Font faces are created using <firstterm>font-backend</firstterm>-specific
 * constructors, typically of the form
 * <function>comac_<emphasis>backend</emphasis>_font_face_create(<!-- -->)</function>,
 * or implicitly using the <firstterm>toy</firstterm> text API by way of
 * comac_select_font_face().  The resulting face can be accessed using
 * comac_get_font_face().
 **/

/* #comac_font_face_t */

const comac_font_face_t _comac_font_face_nil = {
    { 0 },				/* hash_entry */
    COMAC_STATUS_NO_MEMORY,		/* status */
    COMAC_REFERENCE_COUNT_INVALID,	/* ref_count */
    { 0, 0, 0, NULL },			/* user_data */
    NULL
};
const comac_font_face_t _comac_font_face_nil_file_not_found = {
    { 0 },				/* hash_entry */
    COMAC_STATUS_FILE_NOT_FOUND,	/* status */
    COMAC_REFERENCE_COUNT_INVALID,	/* ref_count */
    { 0, 0, 0, NULL },			/* user_data */
    NULL
};

comac_status_t
_comac_font_face_set_error (comac_font_face_t *font_face,
	                    comac_status_t     status)
{
    if (status == COMAC_STATUS_SUCCESS)
	return status;

    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. */
    _comac_status_set_error (&font_face->status, status);

    return _comac_error (status);
}

void
_comac_font_face_init (comac_font_face_t               *font_face,
		       const comac_font_face_backend_t *backend)
{
    COMAC_MUTEX_INITIALIZE ();

    font_face->status = COMAC_STATUS_SUCCESS;
    COMAC_REFERENCE_COUNT_INIT (&font_face->ref_count, 1);
    font_face->backend = backend;

    _comac_user_data_array_init (&font_face->user_data);
}

/**
 * comac_font_face_reference:
 * @font_face: a #comac_font_face_t, (may be %NULL in which case this
 * function does nothing).
 *
 * Increases the reference count on @font_face by one. This prevents
 * @font_face from being destroyed until a matching call to
 * comac_font_face_destroy() is made.
 *
 * Use comac_font_face_get_reference_count() to get the number of
 * references to a #comac_font_face_t.
 *
 * Return value: the referenced #comac_font_face_t.
 *
 * Since: 1.0
 **/
comac_font_face_t *
comac_font_face_reference (comac_font_face_t *font_face)
{
    if (font_face == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return font_face;

    /* We would normally assert that we have a reference here but we
     * can't get away with that due to the zombie case as documented
     * in _comac_ft_font_face_destroy. */

    _comac_reference_count_inc (&font_face->ref_count);

    return font_face;
}

static inline comac_bool_t
__put(comac_reference_count_t *v)
{
    int c, old;

    c = COMAC_REFERENCE_COUNT_GET_VALUE(v);
    while (c != 1 && (old = _comac_atomic_int_cmpxchg_return_old(&v->ref_count, c, c - 1)) != c)
	c = old;

    return c != 1;
}

comac_bool_t
_comac_font_face_destroy (void *abstract_face)
{
#if 0 /* Nothing needs to be done, we can just drop the last reference */
    comac_font_face_t *font_face = abstract_face;
    return _comac_reference_count_dec_and_test (&font_face->ref_count);
#endif
    return TRUE;
}

/**
 * comac_font_face_destroy:
 * @font_face: a #comac_font_face_t
 *
 * Decreases the reference count on @font_face by one. If the result
 * is zero, then @font_face and all associated resources are freed.
 * See comac_font_face_reference().
 *
 * Since: 1.0
 **/
void
comac_font_face_destroy (comac_font_face_t *font_face)
{
    if (font_face == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&font_face->ref_count));

    /* We allow resurrection to deal with some memory management for the
     * FreeType backend where comac_ft_font_face_t and comac_ft_unscaled_font_t
     * need to effectively mutually reference each other
     */
    if (__put (&font_face->ref_count))
	return;

    if (! font_face->backend->destroy (font_face))
	return;

    _comac_user_data_array_fini (&font_face->user_data);

    free (font_face);
}

/**
 * comac_font_face_get_type:
 * @font_face: a font face
 *
 * This function returns the type of the backend used to create
 * a font face. See #comac_font_type_t for available types.
 *
 * Return value: The type of @font_face.
 *
 * Since: 1.2
 **/
comac_font_type_t
comac_font_face_get_type (comac_font_face_t *font_face)
{
    if (COMAC_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return COMAC_FONT_TYPE_TOY;

    return font_face->backend->type;
}

/**
 * comac_font_face_get_reference_count:
 * @font_face: a #comac_font_face_t
 *
 * Returns the current reference count of @font_face.
 *
 * Return value: the current reference count of @font_face.  If the
 * object is a nil object, 0 will be returned.
 *
 * Since: 1.4
 **/
unsigned int
comac_font_face_get_reference_count (comac_font_face_t *font_face)
{
    if (font_face == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return 0;

    return COMAC_REFERENCE_COUNT_GET_VALUE (&font_face->ref_count);
}

/**
 * comac_font_face_status:
 * @font_face: a #comac_font_face_t
 *
 * Checks whether an error has previously occurred for this
 * font face
 *
 * Return value: %COMAC_STATUS_SUCCESS or another error such as
 *   %COMAC_STATUS_NO_MEMORY.
 *
 * Since: 1.0
 **/
comac_status_t
comac_font_face_status (comac_font_face_t *font_face)
{
    return font_face->status;
}

/**
 * comac_font_face_get_user_data:
 * @font_face: a #comac_font_face_t
 * @key: the address of the #comac_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @font_face using the specified
 * key.  If no user data has been attached with the given key this
 * function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 *
 * Since: 1.0
 **/
void *
comac_font_face_get_user_data (comac_font_face_t	   *font_face,
			       const comac_user_data_key_t *key)
{
    return _comac_user_data_array_get_data (&font_face->user_data,
					    key);
}

/**
 * comac_font_face_set_user_data:
 * @font_face: a #comac_font_face_t
 * @key: the address of a #comac_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the font face
 * @destroy: a #comac_destroy_func_t which will be called when the
 * font face is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @font_face.  To remove user data from a font face,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 *
 * Since: 1.0
 **/
comac_status_t
comac_font_face_set_user_data (comac_font_face_t	   *font_face,
			       const comac_user_data_key_t *key,
			       void			   *user_data,
			       comac_destroy_func_t	    destroy)
{
    if (COMAC_REFERENCE_COUNT_IS_INVALID (&font_face->ref_count))
	return font_face->status;

    return _comac_user_data_array_set_data (&font_face->user_data,
					    key, user_data, destroy);
}

void
_comac_unscaled_font_init (comac_unscaled_font_t               *unscaled_font,
			   const comac_unscaled_font_backend_t *backend)
{
    COMAC_REFERENCE_COUNT_INIT (&unscaled_font->ref_count, 1);
    unscaled_font->backend = backend;
}

comac_unscaled_font_t *
_comac_unscaled_font_reference (comac_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return NULL;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&unscaled_font->ref_count));

    _comac_reference_count_inc (&unscaled_font->ref_count);

    return unscaled_font;
}

void
_comac_unscaled_font_destroy (comac_unscaled_font_t *unscaled_font)
{
    if (unscaled_font == NULL)
	return;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&unscaled_font->ref_count));

    if (__put (&unscaled_font->ref_count))
	return;

    if (! unscaled_font->backend->destroy (unscaled_font))
	return;

    free (unscaled_font);
}
