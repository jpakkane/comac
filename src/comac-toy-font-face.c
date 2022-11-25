/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005,2008 Red Hat Inc.
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
 *      Behdad Esfahbod <behdad@behdad.org>
 */

#define _DEFAULT_SOURCE /* for strdup() */
#include "comacint.h"
#include "comac-error-private.h"

static const comac_font_face_t _comac_font_face_null_pointer = {
    {0},			   /* hash_entry */
    COMAC_STATUS_NULL_POINTER,	   /* status */
    COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
    {0, 0, 0, NULL},		   /* user_data */
    NULL};

static const comac_font_face_t _comac_font_face_invalid_string = {
    {0},			   /* hash_entry */
    COMAC_STATUS_INVALID_STRING,   /* status */
    COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
    {0, 0, 0, NULL},		   /* user_data */
    NULL};

static const comac_font_face_t _comac_font_face_invalid_slant = {
    {0},			   /* hash_entry */
    COMAC_STATUS_INVALID_SLANT,	   /* status */
    COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
    {0, 0, 0, NULL},		   /* user_data */
    NULL};

static const comac_font_face_t _comac_font_face_invalid_weight = {
    {0},			   /* hash_entry */
    COMAC_STATUS_INVALID_WEIGHT,   /* status */
    COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
    {0, 0, 0, NULL},		   /* user_data */
    NULL};

static const comac_font_face_backend_t _comac_toy_font_face_backend;

static int
_comac_toy_font_face_keys_equal (const void *key_a, const void *key_b);

/* We maintain a hash table from family/weight/slant =>
 * #comac_font_face_t for #comac_toy_font_t. The primary purpose of
 * this mapping is to provide unique #comac_font_face_t values so that
 * our cache and mapping from #comac_font_face_t => #comac_scaled_font_t
 * works. Once the corresponding #comac_font_face_t objects fall out of
 * downstream caches, we don't need them in this hash table anymore.
 *
 * Modifications to this hash table are protected by
 * _comac_toy_font_face_mutex.
 */
static comac_hash_table_t *comac_toy_font_face_hash_table = NULL;

static comac_hash_table_t *
_comac_toy_font_face_hash_table_lock (void)
{
    COMAC_MUTEX_LOCK (_comac_toy_font_face_mutex);

    if (comac_toy_font_face_hash_table == NULL) {
	comac_toy_font_face_hash_table =
	    _comac_hash_table_create (_comac_toy_font_face_keys_equal);

	if (comac_toy_font_face_hash_table == NULL) {
	    COMAC_MUTEX_UNLOCK (_comac_toy_font_face_mutex);
	    return NULL;
	}
    }

    return comac_toy_font_face_hash_table;
}

static void
_comac_toy_font_face_hash_table_unlock (void)
{
    COMAC_MUTEX_UNLOCK (_comac_toy_font_face_mutex);
}

/**
 * _comac_toy_font_face_init_key:
 *
 * Initialize those portions of #comac_toy_font_face_t needed to use
 * it as a hash table key, including the hash code buried away in
 * font_face->base.hash_entry. No memory allocation is performed here
 * so that no fini call is needed. We do this to make it easier to use
 * an automatic #comac_toy_font_face_t variable as a key.
 **/
static void
_comac_toy_font_face_init_key (comac_toy_font_face_t *key,
			       const char *family,
			       comac_font_slant_t slant,
			       comac_font_weight_t weight)
{
    uintptr_t hash;

    key->family = family;
    key->owns_family = FALSE;

    key->slant = slant;
    key->weight = weight;

    /* 1607 and 1451 are just a couple of arbitrary primes. */
    hash = _comac_hash_string (family);
    hash += ((uintptr_t) slant) * 1607;
    hash += ((uintptr_t) weight) * 1451;

    key->base.hash_entry.hash = hash;
}

static comac_status_t
_comac_toy_font_face_create_impl_face (comac_toy_font_face_t *font_face,
				       comac_font_face_t **impl_font_face)
{
    const comac_font_face_backend_t *backend = COMAC_FONT_FACE_BACKEND_DEFAULT;
    comac_int_status_t status = COMAC_INT_STATUS_UNSUPPORTED;

    if (unlikely (font_face->base.status))
	return font_face->base.status;

    if (backend->create_for_toy != NULL &&
	0 != strncmp (font_face->family,
		      COMAC_USER_FONT_FAMILY_DEFAULT,
		      strlen (COMAC_USER_FONT_FAMILY_DEFAULT))) {
	status = backend->create_for_toy (font_face, impl_font_face);
    }

    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	backend = &_comac_user_font_face_backend;
	status = backend->create_for_toy (font_face, impl_font_face);
    }

    return status;
}

static comac_status_t
_comac_toy_font_face_init (comac_toy_font_face_t *font_face,
			   const char *family,
			   comac_font_slant_t slant,
			   comac_font_weight_t weight)
{
    char *family_copy;
    comac_status_t status;

    family_copy = strdup (family);
    if (unlikely (family_copy == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    _comac_toy_font_face_init_key (font_face, family_copy, slant, weight);
    font_face->owns_family = TRUE;

    _comac_font_face_init (&font_face->base, &_comac_toy_font_face_backend);

    status = _comac_toy_font_face_create_impl_face (font_face,
						    &font_face->impl_face);
    if (unlikely (status)) {
	free (family_copy);
	return status;
    }

    return COMAC_STATUS_SUCCESS;
}

static void
_comac_toy_font_face_fini (comac_toy_font_face_t *font_face)
{
    /* We assert here that we own font_face->family before casting
     * away the const qualifier. */
    assert (font_face->owns_family);
    free ((char *) font_face->family);

    if (font_face->impl_face)
	comac_font_face_destroy (font_face->impl_face);
}

static int
_comac_toy_font_face_keys_equal (const void *key_a, const void *key_b)
{
    const comac_toy_font_face_t *face_a = key_a;
    const comac_toy_font_face_t *face_b = key_b;

    return (strcmp (face_a->family, face_b->family) == 0 &&
	    face_a->slant == face_b->slant && face_a->weight == face_b->weight);
}

/**
 * comac_toy_font_face_create:
 * @family: a font family name, encoded in UTF-8
 * @slant: the slant for the font
 * @weight: the weight for the font
 *
 * Creates a font face from a triplet of family, slant, and weight.
 * These font faces are used in implementation of the the #comac_t "toy"
 * font API.
 *
 * If @family is the zero-length string "", the platform-specific default
 * family is assumed.  The default family then can be queried using
 * comac_toy_font_face_get_family().
 *
 * The comac_select_font_face() function uses this to create font faces.
 * See that function for limitations and other details of toy font faces.
 *
 * Return value: a newly created #comac_font_face_t. Free with
 *  comac_font_face_destroy() when you are done using it.
 *
 * Since: 1.8
 **/
comac_font_face_t *
comac_toy_font_face_create (const char *family,
			    comac_font_slant_t slant,
			    comac_font_weight_t weight)
{
    comac_status_t status;
    comac_toy_font_face_t key, *font_face;
    comac_hash_table_t *hash_table;

    if (family == NULL)
	return (comac_font_face_t *) &_comac_font_face_null_pointer;

    /* Make sure we've got valid UTF-8 for the family */
    status = _comac_utf8_to_ucs4 (family, -1, NULL, NULL);
    if (unlikely (status)) {
	if (status == COMAC_STATUS_INVALID_STRING)
	    return (comac_font_face_t *) &_comac_font_face_invalid_string;

	return (comac_font_face_t *) &_comac_font_face_nil;
    }

    switch (slant) {
    case COMAC_FONT_SLANT_NORMAL:
    case COMAC_FONT_SLANT_ITALIC:
    case COMAC_FONT_SLANT_OBLIQUE:
	break;
    default:
	return (comac_font_face_t *) &_comac_font_face_invalid_slant;
    }

    switch (weight) {
    case COMAC_FONT_WEIGHT_NORMAL:
    case COMAC_FONT_WEIGHT_BOLD:
	break;
    default:
	return (comac_font_face_t *) &_comac_font_face_invalid_weight;
    }

    if (*family == '\0')
	family = COMAC_FONT_FAMILY_DEFAULT;

    hash_table = _comac_toy_font_face_hash_table_lock ();
    if (unlikely (hash_table == NULL))
	goto UNWIND;

    _comac_toy_font_face_init_key (&key, family, slant, weight);

    /* Return existing font_face if it exists in the hash table. */
    font_face = _comac_hash_table_lookup (hash_table, &key.base.hash_entry);
    if (font_face != NULL) {
	if (font_face->base.status == COMAC_STATUS_SUCCESS) {
	    comac_font_face_reference (&font_face->base);
	    _comac_toy_font_face_hash_table_unlock ();
	    return &font_face->base;
	}

	/* remove the bad font from the hash table */
	_comac_hash_table_remove (hash_table, &font_face->base.hash_entry);
    }

    /* Otherwise create it and insert into hash table. */
    font_face = _comac_malloc (sizeof (comac_toy_font_face_t));
    if (unlikely (font_face == NULL)) {
	status = _comac_error (COMAC_STATUS_NO_MEMORY);
	goto UNWIND_HASH_TABLE_LOCK;
    }

    status = _comac_toy_font_face_init (font_face, family, slant, weight);
    if (unlikely (status))
	goto UNWIND_FONT_FACE_MALLOC;

    assert (font_face->base.hash_entry.hash == key.base.hash_entry.hash);
    status = _comac_hash_table_insert (hash_table, &font_face->base.hash_entry);
    if (unlikely (status))
	goto UNWIND_FONT_FACE_INIT;

    _comac_toy_font_face_hash_table_unlock ();

    return &font_face->base;

UNWIND_FONT_FACE_INIT:
    _comac_toy_font_face_fini (font_face);
UNWIND_FONT_FACE_MALLOC:
    free (font_face);
UNWIND_HASH_TABLE_LOCK:
    _comac_toy_font_face_hash_table_unlock ();
UNWIND:
    return (comac_font_face_t *) &_comac_font_face_nil;
}

static comac_bool_t
_comac_toy_font_face_destroy (void *abstract_face)
{
    comac_toy_font_face_t *font_face = abstract_face;
    comac_hash_table_t *hash_table;

    hash_table = _comac_toy_font_face_hash_table_lock ();
    /* All created objects must have been mapped in the hash table. */
    assert (hash_table != NULL);

    if (! _comac_reference_count_dec_and_test (&font_face->base.ref_count)) {
	/* somebody recreated the font whilst we waited for the lock */
	_comac_toy_font_face_hash_table_unlock ();
	return FALSE;
    }

    /* Font faces in SUCCESS status are guaranteed to be in the
     * hashtable. Font faces in an error status are removed from the
     * hashtable if they are found during a lookup, thus they should
     * only be removed if they are in the hashtable. */
    if (likely (font_face->base.status == COMAC_STATUS_SUCCESS) ||
	_comac_hash_table_lookup (hash_table, &font_face->base.hash_entry) ==
	    font_face)
	_comac_hash_table_remove (hash_table, &font_face->base.hash_entry);

    _comac_toy_font_face_hash_table_unlock ();

    _comac_toy_font_face_fini (font_face);
    return TRUE;
}

static comac_status_t
_comac_toy_font_face_scaled_font_create (void *abstract_font_face,
					 const comac_matrix_t *font_matrix,
					 const comac_matrix_t *ctm,
					 const comac_font_options_t *options,
					 comac_scaled_font_t **scaled_font)
{
    comac_toy_font_face_t *font_face =
	(comac_toy_font_face_t *) abstract_font_face;

    ASSERT_NOT_REACHED;

    return _comac_font_face_set_error (&font_face->base,
				       COMAC_STATUS_FONT_TYPE_MISMATCH);
}

static comac_font_face_t *
_comac_toy_font_face_get_implementation (void *abstract_font_face,
					 const comac_matrix_t *font_matrix,
					 const comac_matrix_t *ctm,
					 const comac_font_options_t *options)
{
    comac_toy_font_face_t *font_face = abstract_font_face;

    if (font_face->impl_face) {
	comac_font_face_t *impl = font_face->impl_face;

	if (impl->backend->get_implementation != NULL) {
	    return impl->backend->get_implementation (impl,
						      font_matrix,
						      ctm,
						      options);
	}

	return comac_font_face_reference (impl);
    }

    return abstract_font_face;
}

static comac_bool_t
_comac_font_face_is_toy (comac_font_face_t *font_face)
{
    return font_face->backend == &_comac_toy_font_face_backend;
}

/**
 * comac_toy_font_face_get_family:
 * @font_face: A toy font face
 *
 * Gets the family name of a toy font.
 *
 * Return value: The family name.  This string is owned by the font face
 * and remains valid as long as the font face is alive (referenced).
 *
 * Since: 1.8
 **/
const char *
comac_toy_font_face_get_family (comac_font_face_t *font_face)
{
    comac_toy_font_face_t *toy_font_face;

    if (font_face->status)
	return COMAC_FONT_FAMILY_DEFAULT;

    toy_font_face = (comac_toy_font_face_t *) font_face;
    if (! _comac_font_face_is_toy (font_face)) {
	if (_comac_font_face_set_error (font_face,
					COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return COMAC_FONT_FAMILY_DEFAULT;
    }
    assert (toy_font_face->owns_family);
    return toy_font_face->family;
}

/**
 * comac_toy_font_face_get_slant:
 * @font_face: A toy font face
 *
 * Gets the slant a toy font.
 *
 * Return value: The slant value
 *
 * Since: 1.8
 **/
comac_font_slant_t
comac_toy_font_face_get_slant (comac_font_face_t *font_face)
{
    comac_toy_font_face_t *toy_font_face;

    if (font_face->status)
	return COMAC_FONT_SLANT_DEFAULT;

    toy_font_face = (comac_toy_font_face_t *) font_face;
    if (! _comac_font_face_is_toy (font_face)) {
	if (_comac_font_face_set_error (font_face,
					COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return COMAC_FONT_SLANT_DEFAULT;
    }
    return toy_font_face->slant;
}

/**
 * comac_toy_font_face_get_weight:
 * @font_face: A toy font face
 *
 * Gets the weight a toy font.
 *
 * Return value: The weight value
 *
 * Since: 1.8
 **/
comac_font_weight_t
comac_toy_font_face_get_weight (comac_font_face_t *font_face)
{
    comac_toy_font_face_t *toy_font_face;

    if (font_face->status)
	return COMAC_FONT_WEIGHT_DEFAULT;

    toy_font_face = (comac_toy_font_face_t *) font_face;
    if (! _comac_font_face_is_toy (font_face)) {
	if (_comac_font_face_set_error (font_face,
					COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return COMAC_FONT_WEIGHT_DEFAULT;
    }
    return toy_font_face->weight;
}

static const comac_font_face_backend_t _comac_toy_font_face_backend = {
    COMAC_FONT_TYPE_TOY,
    NULL, /* create_for_toy */
    _comac_toy_font_face_destroy,
    _comac_toy_font_face_scaled_font_create,
    _comac_toy_font_face_get_implementation};

void
_comac_toy_font_face_reset_static_data (void)
{
    comac_hash_table_t *hash_table;

    /* We manually acquire the lock rather than calling
     * comac_toy_font_face_hash_table_lock simply to avoid
     * creating the table only to destroy it again. */
    COMAC_MUTEX_LOCK (_comac_toy_font_face_mutex);
    hash_table = comac_toy_font_face_hash_table;
    comac_toy_font_face_hash_table = NULL;
    COMAC_MUTEX_UNLOCK (_comac_toy_font_face_mutex);

    if (hash_table != NULL)
	_comac_hash_table_destroy (hash_table);
}
