/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2000 Keith Packard
 * Copyright © 2005 Red Hat, Inc
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
 *      Graydon Hoare <graydon@redhat.com>
 *	Owen Taylor <otaylor@redhat.com>
 *      Keith Packard <keithp@keithp.com>
 *      Carl Worth <cworth@cworth.org>
 */

#define _DEFAULT_SOURCE /* for strdup() */
#include "comacint.h"

#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-ft-private.h"
#include "comac-pattern-private.h"
#include "comac-pixman-private.h"

#include <float.h>

#include "comac-fontconfig-private.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_IMAGE_H
#include FT_BITMAP_H
#include FT_TRUETYPE_TABLES_H
#include FT_XFREE86_H
#include FT_MULTIPLE_MASTERS_H
#if HAVE_FT_GLYPHSLOT_EMBOLDEN
#include FT_SYNTHESIS_H
#endif

#if HAVE_FT_LIBRARY_SETLCDFILTER
#include FT_LCD_FILTER_H
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#elif !defined(access)
#define access(p, m) 0
#endif

/* Fontconfig version older than 2.6 didn't have these options */
#ifndef FC_LCD_FILTER
#define FC_LCD_FILTER	"lcdfilter"
#endif
/* Some Ubuntu versions defined FC_LCD_FILTER without defining the following */
#ifndef FC_LCD_NONE
#define FC_LCD_NONE	0
#define FC_LCD_DEFAULT	1
#define FC_LCD_LIGHT	2
#define FC_LCD_LEGACY	3
#endif

/* FreeType version older than 2.3.5(?) didn't have these options */
#ifndef FT_LCD_FILTER_NONE
#define FT_LCD_FILTER_NONE	0
#define FT_LCD_FILTER_DEFAULT	1
#define FT_LCD_FILTER_LIGHT	2
#define FT_LCD_FILTER_LEGACY	16
#endif

/*  FreeType version older than 2.11 does not have the FT_RENDER_MODE_SDF enum value in FT_Render_Mode */
#if FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR >= 11)
#define HAVE_FT_RENDER_MODE_SDF 1
#endif

#define DOUBLE_FROM_26_6(t) ((double)(t) / 64.0)
#define DOUBLE_TO_16_16(d) ((FT_Fixed)((d) * 65536.0))
#define DOUBLE_FROM_16_16(t) ((double)(t) / 65536.0)

/* This is the max number of FT_face objects we keep open at once
 */
#define MAX_OPEN_FACES 10

/**
 * SECTION:comac-ft
 * @Title: FreeType Fonts
 * @Short_Description: Font support for FreeType
 * @See_Also: #comac_font_face_t
 *
 * The FreeType font backend is primarily used to render text on GNU/Linux
 * systems, but can be used on other platforms too.
 **/

/**
 * COMAC_HAS_FT_FONT:
 *
 * Defined if the FreeType font backend is available.
 * This macro can be used to conditionally compile backend-specific code.
 *
 * Since: 1.0
 **/

/**
 * COMAC_HAS_FC_FONT:
 *
 * Defined if the Fontconfig-specific functions of the FreeType font backend
 * are available.
 * This macro can be used to conditionally compile backend-specific code.
 *
 * Since: 1.10
 **/

/*
 * The simple 2x2 matrix is converted into separate scale and shape
 * factors so that hinting works right
 */

typedef struct _comac_ft_font_transform {
    double  x_scale, y_scale;
    double  shape[2][2];
} comac_ft_font_transform_t;

/*
 * We create an object that corresponds to a single font on the disk;
 * (identified by a filename/id pair) these are shared between all
 * fonts using that file.  For comac_ft_font_face_create_for_ft_face(), we
 * just create a one-off version with a permanent face value.
 */

typedef struct _comac_ft_font_face comac_ft_font_face_t;

struct _comac_ft_unscaled_font {
    comac_unscaled_font_t base;

    comac_bool_t from_face; /* was the FT_Face provided by user? */
    FT_Face face;	    /* provided or cached face */

    /* only set if from_face is false */
    char *filename;
    int id;

    /* We temporarily scale the unscaled font as needed */
    comac_bool_t have_scale;
    comac_matrix_t current_scale;
    double x_scale;		/* Extracted X scale factor */
    double y_scale;             /* Extracted Y scale factor */
    comac_bool_t have_shape;	/* true if the current scale has a non-scale component*/
    comac_matrix_t current_shape;
    FT_Matrix Current_Shape;

    unsigned int have_color_set  : 1;
    unsigned int have_color      : 1;  /* true if the font contains color glyphs */
    FT_Fixed *variations;              /* variation settings that FT_Face came */
    unsigned int num_palettes;

    comac_mutex_t mutex;
    int lock_count;

    comac_ft_font_face_t *faces;	/* Linked list of faces for this font */
};

static int
_comac_ft_unscaled_font_keys_equal (const void *key_a,
				    const void *key_b);

static void
_comac_ft_unscaled_font_fini (comac_ft_unscaled_font_t *unscaled);

typedef struct _comac_ft_options {
    comac_font_options_t base;
    unsigned int load_flags; /* flags for FT_Load_Glyph */
    unsigned int synth_flags;
} comac_ft_options_t;

static void
_comac_ft_options_init_copy (comac_ft_options_t       *options,
                             const comac_ft_options_t *other)
{
    _comac_font_options_init_copy (&options->base, &other->base);
    options->load_flags = other->load_flags;
    options->synth_flags = other->synth_flags;
}

static void
_comac_ft_options_fini (comac_ft_options_t *options)
{
    _comac_font_options_fini (&options->base);
}

struct _comac_ft_font_face {
    comac_font_face_t base;

    comac_ft_unscaled_font_t *unscaled;
    comac_ft_options_t ft_options;
    comac_ft_font_face_t *next;

#if COMAC_HAS_FC_FONT
    FcPattern *pattern; /* if pattern is set, the above fields will be NULL */
    comac_font_face_t *resolved_font_face;
    FcConfig *resolved_config;
#endif
};

static const comac_unscaled_font_backend_t comac_ft_unscaled_font_backend;

#if COMAC_HAS_FC_FONT
static comac_status_t
_comac_ft_font_options_substitute (const comac_font_options_t *options,
				   FcPattern                  *pattern);

static comac_font_face_t *
_comac_ft_resolve_pattern (FcPattern		      *pattern,
			   const comac_matrix_t       *font_matrix,
			   const comac_matrix_t       *ctm,
			   const comac_font_options_t *options);

#endif

static comac_status_t
_ft_to_comac_error (FT_Error error)
{
  /* Currently we don't get many (any?) useful statuses here.
   * Populate as needed. */
  switch (error)
  {
  case FT_Err_Out_Of_Memory:
      return COMAC_STATUS_NO_MEMORY;
  default:
      return COMAC_STATUS_FREETYPE_ERROR;
  }
}

/*
 * We maintain a hash table to map file/id => #comac_ft_unscaled_font_t.
 * The hash table itself isn't limited in size. However, we limit the
 * number of FT_Face objects we keep around; when we've exceeded that
 * limit and need to create a new FT_Face, we dump the FT_Face from a
 * random #comac_ft_unscaled_font_t which has an unlocked FT_Face, (if
 * there are any).
 */

typedef struct _comac_ft_unscaled_font_map {
    comac_hash_table_t *hash_table;
    FT_Library ft_library;
    int num_open_faces;
} comac_ft_unscaled_font_map_t;

static comac_ft_unscaled_font_map_t *comac_ft_unscaled_font_map = NULL;


static FT_Face
_comac_ft_unscaled_font_lock_face (comac_ft_unscaled_font_t *unscaled);

static void
_comac_ft_unscaled_font_unlock_face (comac_ft_unscaled_font_t *unscaled);

static comac_bool_t
_comac_ft_scaled_font_is_vertical (comac_scaled_font_t *scaled_font);


static void
_font_map_release_face_lock_held (comac_ft_unscaled_font_map_t *font_map,
				  comac_ft_unscaled_font_t *unscaled)
{
    if (unscaled->face) {
	FT_Done_Face (unscaled->face);
	unscaled->face = NULL;
	unscaled->have_scale = FALSE;

	font_map->num_open_faces--;
    }
}

static comac_status_t
_comac_ft_unscaled_font_map_create (void)
{
    comac_ft_unscaled_font_map_t *font_map;

    /* This function is only intended to be called from
     * _comac_ft_unscaled_font_map_lock. So we'll crash if we can
     * detect some other call path. */
    assert (comac_ft_unscaled_font_map == NULL);

    font_map = _comac_malloc (sizeof (comac_ft_unscaled_font_map_t));
    if (unlikely (font_map == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    font_map->hash_table =
	_comac_hash_table_create (_comac_ft_unscaled_font_keys_equal);

    if (unlikely (font_map->hash_table == NULL))
	goto FAIL;

    if (unlikely (FT_Init_FreeType (&font_map->ft_library)))
	goto FAIL;

    font_map->num_open_faces = 0;

    comac_ft_unscaled_font_map = font_map;
    return COMAC_STATUS_SUCCESS;

FAIL:
    if (font_map->hash_table)
	_comac_hash_table_destroy (font_map->hash_table);
    free (font_map);

    return _comac_error (COMAC_STATUS_NO_MEMORY);
}


static void
_comac_ft_unscaled_font_map_pluck_entry (void *entry, void *closure)
{
    comac_ft_unscaled_font_t *unscaled = entry;
    comac_ft_unscaled_font_map_t *font_map = closure;

    _comac_hash_table_remove (font_map->hash_table,
			      &unscaled->base.hash_entry);

    if (! unscaled->from_face)
	_font_map_release_face_lock_held (font_map, unscaled);

    _comac_ft_unscaled_font_fini (unscaled);
    free (unscaled);
}

static void
_comac_ft_unscaled_font_map_destroy (void)
{
    comac_ft_unscaled_font_map_t *font_map;

    COMAC_MUTEX_LOCK (_comac_ft_unscaled_font_map_mutex);
    font_map = comac_ft_unscaled_font_map;
    comac_ft_unscaled_font_map = NULL;
    COMAC_MUTEX_UNLOCK (_comac_ft_unscaled_font_map_mutex);

    if (font_map != NULL) {
	_comac_hash_table_foreach (font_map->hash_table,
				   _comac_ft_unscaled_font_map_pluck_entry,
				   font_map);
	assert (font_map->num_open_faces == 0);

	FT_Done_FreeType (font_map->ft_library);

	_comac_hash_table_destroy (font_map->hash_table);

	free (font_map);
    }
}

static comac_ft_unscaled_font_map_t *
_comac_ft_unscaled_font_map_lock (void)
{
    COMAC_MUTEX_INITIALIZE ();

    COMAC_MUTEX_LOCK (_comac_ft_unscaled_font_map_mutex);

    if (unlikely (comac_ft_unscaled_font_map == NULL)) {
	if (unlikely (_comac_ft_unscaled_font_map_create ())) {
	    COMAC_MUTEX_UNLOCK (_comac_ft_unscaled_font_map_mutex);
	    return NULL;
	}
    }

    return comac_ft_unscaled_font_map;
}

static void
_comac_ft_unscaled_font_map_unlock (void)
{
    COMAC_MUTEX_UNLOCK (_comac_ft_unscaled_font_map_mutex);
}

static void
_comac_ft_unscaled_font_init_key (comac_ft_unscaled_font_t *key,
				  comac_bool_t              from_face,
				  char			   *filename,
				  int			    id,
				  FT_Face		    face)
{
    uintptr_t hash;

    key->from_face = from_face;
    key->filename = filename;
    key->id = id;
    key->face = face;

    hash = _comac_hash_string (filename);
    /* the constants are just arbitrary primes */
    hash += ((uintptr_t) id) * 1607;
    hash += ((uintptr_t) face) * 2137;

    key->base.hash_entry.hash = hash;
}

/**
 * _comac_ft_unscaled_font_init:
 *
 * Initialize a #comac_ft_unscaled_font_t.
 *
 * There are two basic flavors of #comac_ft_unscaled_font_t, one
 * created from an FT_Face and the other created from a filename/id
 * pair. These two flavors are identified as from_face and !from_face.
 *
 * To initialize a from_face font, pass filename==%NULL, id=0 and the
 * desired face.
 *
 * To initialize a !from_face font, pass the filename/id as desired
 * and face==%NULL.
 *
 * Note that the code handles these two flavors in very distinct
 * ways. For example there is a hash_table mapping
 * filename/id->#comac_unscaled_font_t in the !from_face case, but no
 * parallel in the from_face case, (where the calling code would have
 * to do its own mapping to ensure similar sharing).
 **/
static comac_status_t
_comac_ft_unscaled_font_init (comac_ft_unscaled_font_t *unscaled,
			      comac_bool_t              from_face,
			      const char	       *filename,
			      int			id,
			      FT_Face			face)
{
    _comac_unscaled_font_init (&unscaled->base,
			       &comac_ft_unscaled_font_backend);

    unscaled->variations = NULL;

    if (from_face) {
	unscaled->from_face = TRUE;
	_comac_ft_unscaled_font_init_key (unscaled, TRUE, NULL, id, face);


        unscaled->have_color = FT_HAS_COLOR (face) != 0;
        unscaled->have_color_set = TRUE;

#ifdef HAVE_FT_GET_VAR_DESIGN_COORDINATES
	{
	    FT_MM_Var *ft_mm_var;
	    if (0 == FT_Get_MM_Var (face, &ft_mm_var))
	    {
		unscaled->variations = calloc (ft_mm_var->num_axis, sizeof (FT_Fixed));
		if (unscaled->variations)
		    FT_Get_Var_Design_Coordinates (face, ft_mm_var->num_axis, unscaled->variations);
#if HAVE_FT_DONE_MM_VAR
		FT_Done_MM_Var (face->glyph->library, ft_mm_var);
#else
		free (ft_mm_var);
#endif
	    }
	}
#endif
    } else {
	char *filename_copy;

	unscaled->from_face = FALSE;
	unscaled->face = NULL;

	filename_copy = strdup (filename);
	if (unlikely (filename_copy == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	_comac_ft_unscaled_font_init_key (unscaled, FALSE, filename_copy, id, NULL);

	unscaled->have_color_set = FALSE;
    }

    unscaled->have_scale = FALSE;
    COMAC_MUTEX_INIT (unscaled->mutex);
    unscaled->lock_count = 0;

    unscaled->faces = NULL;

    return COMAC_STATUS_SUCCESS;
}

/**
 * _comac_ft_unscaled_font_fini:
 *
 * Free all data associated with a #comac_ft_unscaled_font_t.
 *
 * CAUTION: The unscaled->face field must be %NULL before calling this
 * function. This is because the #comac_ft_unscaled_font_t_map keeps a
 * count of these faces (font_map->num_open_faces) so it maintains the
 * unscaled->face field while it has its lock held. See
 * _font_map_release_face_lock_held().
 **/
static void
_comac_ft_unscaled_font_fini (comac_ft_unscaled_font_t *unscaled)
{
    assert (unscaled->face == NULL);

    free (unscaled->filename);
    unscaled->filename = NULL;

    free (unscaled->variations);

    COMAC_MUTEX_FINI (unscaled->mutex);
}

static int
_comac_ft_unscaled_font_keys_equal (const void *key_a,
				    const void *key_b)
{
    const comac_ft_unscaled_font_t *unscaled_a = key_a;
    const comac_ft_unscaled_font_t *unscaled_b = key_b;

    if (unscaled_a->id == unscaled_b->id &&
	unscaled_a->from_face == unscaled_b->from_face)
     {
        if (unscaled_a->from_face)
	    return unscaled_a->face == unscaled_b->face;

	if (unscaled_a->filename == NULL && unscaled_b->filename == NULL)
	    return TRUE;
	else if (unscaled_a->filename == NULL || unscaled_b->filename == NULL)
	    return FALSE;
	else
	    return (strcmp (unscaled_a->filename, unscaled_b->filename) == 0);
    }

    return FALSE;
}

/* Finds or creates a #comac_ft_unscaled_font_t for the filename/id from
 * pattern.  Returns a new reference to the unscaled font.
 */
static comac_status_t
_comac_ft_unscaled_font_create_internal (comac_bool_t from_face,
					 char *filename,
					 int id,
					 FT_Face font_face,
					 comac_ft_unscaled_font_t **out)
{
    comac_ft_unscaled_font_t key, *unscaled;
    comac_ft_unscaled_font_map_t *font_map;
    comac_status_t status;

    font_map = _comac_ft_unscaled_font_map_lock ();
    if (unlikely (font_map == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    _comac_ft_unscaled_font_init_key (&key, from_face, filename, id, font_face);

    /* Return existing unscaled font if it exists in the hash table. */
    unscaled = _comac_hash_table_lookup (font_map->hash_table,
					 &key.base.hash_entry);
    if (unscaled != NULL) {
	_comac_unscaled_font_reference (&unscaled->base);
	goto DONE;
    }

    /* Otherwise create it and insert into hash table. */
    unscaled = _comac_malloc (sizeof (comac_ft_unscaled_font_t));
    if (unlikely (unscaled == NULL)) {
	status = _comac_error (COMAC_STATUS_NO_MEMORY);
	goto UNWIND_FONT_MAP_LOCK;
    }

    status = _comac_ft_unscaled_font_init (unscaled, from_face, filename, id, font_face);
    if (unlikely (status))
	goto UNWIND_UNSCALED_MALLOC;

    assert (unscaled->base.hash_entry.hash == key.base.hash_entry.hash);
    status = _comac_hash_table_insert (font_map->hash_table,
				       &unscaled->base.hash_entry);
    if (unlikely (status))
	goto UNWIND_UNSCALED_FONT_INIT;

DONE:
    _comac_ft_unscaled_font_map_unlock ();
    *out = unscaled;
    return COMAC_STATUS_SUCCESS;

UNWIND_UNSCALED_FONT_INIT:
    _comac_ft_unscaled_font_fini (unscaled);
UNWIND_UNSCALED_MALLOC:
    free (unscaled);
UNWIND_FONT_MAP_LOCK:
    _comac_ft_unscaled_font_map_unlock ();
    return status;
}


#if COMAC_HAS_FC_FONT
static comac_status_t
_comac_ft_unscaled_font_create_for_pattern (FcPattern *pattern,
					    comac_ft_unscaled_font_t **out)
{
    FT_Face font_face = NULL;
    char *filename = NULL;
    int id = 0;
    FcResult ret;

    ret = FcPatternGetFTFace (pattern, FC_FT_FACE, 0, &font_face);
    if (ret == FcResultMatch)
	goto DONE;
    if (ret == FcResultOutOfMemory)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    ret = FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **) &filename);
    if (ret == FcResultOutOfMemory)
	return _comac_error (COMAC_STATUS_NO_MEMORY);
    if (ret == FcResultMatch) {
	if (access (filename, R_OK) == 0) {
	    /* If FC_INDEX is not set, we just use 0 */
	    ret = FcPatternGetInteger (pattern, FC_INDEX, 0, &id);
	    if (ret == FcResultOutOfMemory)
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	    goto DONE;
	} else
	    return _comac_error (COMAC_STATUS_FILE_NOT_FOUND);
    }

    /* The pattern contains neither a face nor a filename, resolve it later. */
    *out = NULL;
    return COMAC_STATUS_SUCCESS;

DONE:
    return _comac_ft_unscaled_font_create_internal (font_face != NULL,
						    filename, id, font_face,
						    out);
}
#endif

static comac_status_t
_comac_ft_unscaled_font_create_from_face (FT_Face face,
					  comac_ft_unscaled_font_t **out)
{
    return _comac_ft_unscaled_font_create_internal (TRUE, NULL, face->face_index, face, out);
}

static comac_bool_t
_comac_ft_unscaled_font_destroy (void *abstract_font)
{
    comac_ft_unscaled_font_t *unscaled  = abstract_font;
    comac_ft_unscaled_font_map_t *font_map;

    font_map = _comac_ft_unscaled_font_map_lock ();
    /* All created objects must have been mapped in the font map. */
    assert (font_map != NULL);

    if (! _comac_reference_count_dec_and_test (&unscaled->base.ref_count)) {
	/* somebody recreated the font whilst we waited for the lock */
	_comac_ft_unscaled_font_map_unlock ();
	return FALSE;
    }

    _comac_hash_table_remove (font_map->hash_table,
			      &unscaled->base.hash_entry);

    if (unscaled->from_face) {
	/* See comments in _ft_font_face_destroy about the "zombie" state
	 * for a _ft_font_face.
	 */
	if (unscaled->faces && unscaled->faces->unscaled == NULL) {
	    assert (unscaled->faces->next == NULL);
	    comac_font_face_destroy (&unscaled->faces->base);
	}
    } else {
	_font_map_release_face_lock_held (font_map, unscaled);
    }
    unscaled->face = NULL;

    _comac_ft_unscaled_font_map_unlock ();

    _comac_ft_unscaled_font_fini (unscaled);
    return TRUE;
}

static comac_bool_t
_has_unlocked_face (const void *entry)
{
    const comac_ft_unscaled_font_t *unscaled = entry;

    return (!unscaled->from_face && unscaled->lock_count == 0 && unscaled->face);
}

/* Ensures that an unscaled font has a face object. If we exceed
 * MAX_OPEN_FACES, try to close some.
 *
 * This differs from _comac_ft_scaled_font_lock_face in that it doesn't
 * set the scale on the face, but just returns it at the last scale.
 */
static comac_warn FT_Face
_comac_ft_unscaled_font_lock_face (comac_ft_unscaled_font_t *unscaled)
{
    comac_ft_unscaled_font_map_t *font_map;
    FT_Face face = NULL;
    FT_Error error;

    COMAC_MUTEX_LOCK (unscaled->mutex);
    unscaled->lock_count++;

    if (unscaled->face)
	return unscaled->face;

    /* If this unscaled font was created from an FT_Face then we just
     * returned it above. */
    assert (!unscaled->from_face);

    font_map = _comac_ft_unscaled_font_map_lock ();
    {
	assert (font_map != NULL);

	while (font_map->num_open_faces >= MAX_OPEN_FACES)
	{
	    comac_ft_unscaled_font_t *entry;

	    entry = _comac_hash_table_random_entry (font_map->hash_table,
						    _has_unlocked_face);
	    if (entry == NULL)
		break;

	    _font_map_release_face_lock_held (font_map, entry);
	}
    }
    _comac_ft_unscaled_font_map_unlock ();

    error = FT_New_Face (font_map->ft_library,
			 unscaled->filename,
			 unscaled->id,
			 &face);
    if (error)
    {
	unscaled->lock_count--;
	COMAC_MUTEX_UNLOCK (unscaled->mutex);
	_comac_error_throw (_ft_to_comac_error (error));
	return NULL;
    }

    unscaled->face = face;

    unscaled->have_color = FT_HAS_COLOR (face) != 0;
    unscaled->have_color_set = TRUE;

    font_map->num_open_faces++;

    return face;
}


/* Unlock unscaled font locked with _comac_ft_unscaled_font_lock_face
 */
static void
_comac_ft_unscaled_font_unlock_face (comac_ft_unscaled_font_t *unscaled)
{
    assert (unscaled->lock_count > 0);

    unscaled->lock_count--;

    COMAC_MUTEX_UNLOCK (unscaled->mutex);
}


static comac_status_t
_compute_transform (comac_ft_font_transform_t *sf,
		    comac_matrix_t      *scale,
		    comac_ft_unscaled_font_t *unscaled)
{
    comac_status_t status;
    double x_scale, y_scale;
    comac_matrix_t normalized = *scale;

    /* The font matrix has x and y "scale" components which we extract and
     * use as character scale values. These influence the way freetype
     * chooses hints, as well as selecting different bitmaps in
     * hand-rendered fonts. We also copy the normalized matrix to
     * freetype's transformation.
     */

    status = _comac_matrix_compute_basis_scale_factors (scale,
						  &x_scale, &y_scale,
						  1);
    if (unlikely (status))
	return status;

    /* FreeType docs say this about x_scale and y_scale:
     * "A character width or height smaller than 1pt is set to 1pt;"
     * So, we cap them from below at 1.0 and let the FT transform
     * take care of sub-1.0 scaling. */
    if (x_scale < 1.0)
      x_scale = 1.0;
    if (y_scale < 1.0)
      y_scale = 1.0;

    if (unscaled && (unscaled->face->face_flags & FT_FACE_FLAG_SCALABLE) == 0) {
	double min_distance = DBL_MAX;
	comac_bool_t magnify = TRUE;
	int i;
	double best_x_size = 0;
	double best_y_size = 0;

	for (i = 0; i < unscaled->face->num_fixed_sizes; i++) {
	    double x_size = unscaled->face->available_sizes[i].x_ppem / 64.;
	    double y_size = unscaled->face->available_sizes[i].y_ppem / 64.;
	    double distance = y_size - y_scale;

	    /*
	     * distance is positive if current strike is larger than desired
	     * size, and negative if smaller.
	     *
	     * We like to prefer down-scaling to upscaling.
	     */

	    if ((magnify && distance >= 0) || fabs (distance) <= min_distance) {
		magnify = distance < 0;
		min_distance = fabs (distance);
		best_x_size = x_size;
		best_y_size = y_size;
	    }
	}

	x_scale = best_x_size;
	y_scale = best_y_size;
    }

    sf->x_scale = x_scale;
    sf->y_scale = y_scale;

    comac_matrix_scale (&normalized, 1.0 / x_scale, 1.0 / y_scale);

    _comac_matrix_get_affine (&normalized,
			      &sf->shape[0][0], &sf->shape[0][1],
			      &sf->shape[1][0], &sf->shape[1][1],
			      NULL, NULL);

    return COMAC_STATUS_SUCCESS;
}

/* Temporarily scales an unscaled font to the give scale. We catch
 * scaling to the same size, since changing a FT_Face is expensive.
 */
static comac_status_t
_comac_ft_unscaled_font_set_scale (comac_ft_unscaled_font_t *unscaled,
				   comac_matrix_t	      *scale)
{
    comac_status_t status;
    comac_ft_font_transform_t sf;
    FT_Matrix mat;
    FT_Error error;

    assert (unscaled->face != NULL);

    if (unscaled->have_scale &&
	scale->xx == unscaled->current_scale.xx &&
	scale->yx == unscaled->current_scale.yx &&
	scale->xy == unscaled->current_scale.xy &&
	scale->yy == unscaled->current_scale.yy)
	return COMAC_STATUS_SUCCESS;

    unscaled->have_scale = TRUE;
    unscaled->current_scale = *scale;

    status = _compute_transform (&sf, scale, unscaled);
    if (unlikely (status))
	return status;

    unscaled->x_scale = sf.x_scale;
    unscaled->y_scale = sf.y_scale;

    mat.xx = DOUBLE_TO_16_16(sf.shape[0][0]);
    mat.yx = - DOUBLE_TO_16_16(sf.shape[0][1]);
    mat.xy = - DOUBLE_TO_16_16(sf.shape[1][0]);
    mat.yy = DOUBLE_TO_16_16(sf.shape[1][1]);

    unscaled->have_shape = (mat.xx != 0x10000 ||
			    mat.yx != 0x00000 ||
			    mat.xy != 0x00000 ||
			    mat.yy != 0x10000);

    unscaled->Current_Shape = mat;
    comac_matrix_init (&unscaled->current_shape,
		       sf.shape[0][0], sf.shape[0][1],
		       sf.shape[1][0], sf.shape[1][1],
		       0.0, 0.0);

    FT_Set_Transform(unscaled->face, &mat, NULL);

    error = FT_Set_Char_Size (unscaled->face,
			      sf.x_scale * 64.0 + .5,
			      sf.y_scale * 64.0 + .5,
			      0, 0);
    if (error)
      return _comac_error (_ft_to_comac_error (error));

    return COMAC_STATUS_SUCCESS;
}

/* we sometimes need to convert the glyph bitmap in a FT_GlyphSlot
 * into a different format. For example, we want to convert a
 * FT_PIXEL_MODE_LCD or FT_PIXEL_MODE_LCD_V bitmap into a 32-bit
 * ARGB or ABGR bitmap.
 *
 * this function prepares a target descriptor for this operation.
 *
 * input :: target bitmap descriptor. The function will set its
 *          'width', 'rows' and 'pitch' fields, and only these
 *
 * slot  :: the glyph slot containing the source bitmap. this
 *          function assumes that slot->format == FT_GLYPH_FORMAT_BITMAP
 *
 * mode  :: the requested final rendering mode. supported values are
 *          MONO, NORMAL (i.e. gray), LCD and LCD_V
 *
 * the function returns the size in bytes of the corresponding buffer,
 * it's up to the caller to allocate the corresponding memory block
 * before calling _fill_xrender_bitmap
 *
 * it also returns -1 in case of error (e.g. incompatible arguments,
 * like trying to convert a gray bitmap into a monochrome one)
 */
static int
_compute_xrender_bitmap_size(FT_Bitmap      *target,
			     FT_GlyphSlot    slot,
			     FT_Render_Mode  mode)
{
    FT_Bitmap *ftbit;
    int width, height, pitch;

    if (slot->format != FT_GLYPH_FORMAT_BITMAP)
	return -1;

    /* compute the size of the final bitmap */
    ftbit = &slot->bitmap;

    width = ftbit->width;
    height = ftbit->rows;
    pitch = (width + 3) & ~3;

    switch (ftbit->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
	if (mode == FT_RENDER_MODE_MONO) {
	    pitch = (((width + 31) & ~31) >> 3);
	    break;
	}
	/* fall-through */

    case FT_PIXEL_MODE_GRAY:
	if (mode == FT_RENDER_MODE_LCD ||
	    mode == FT_RENDER_MODE_LCD_V)
	{
	    /* each pixel is replicated into a 32-bit ARGB value */
	    pitch = width * 4;
	}
	break;

    case FT_PIXEL_MODE_LCD:
	if (mode != FT_RENDER_MODE_LCD)
	    return -1;

	/* horz pixel triplets are packed into 32-bit ARGB values */
	width /= 3;
	pitch = width * 4;
	break;

    case FT_PIXEL_MODE_LCD_V:
	if (mode != FT_RENDER_MODE_LCD_V)
	    return -1;

	/* vert pixel triplets are packed into 32-bit ARGB values */
	height /= 3;
	pitch = width * 4;
	break;

#ifdef FT_LOAD_COLOR
    case FT_PIXEL_MODE_BGRA:
	/* each pixel is replicated into a 32-bit ARGB value */
	pitch = width * 4;
	break;
#endif

    default:  /* unsupported source format */
	return -1;
    }

    target->width = width;
    target->rows = height;
    target->pitch = pitch;
    target->buffer = NULL;

    return pitch * height;
}

/* this functions converts the glyph bitmap found in a FT_GlyphSlot
 * into a different format (see _compute_xrender_bitmap_size)
 *
 * you should call this function after _compute_xrender_bitmap_size
 *
 * target :: target bitmap descriptor. Note that its 'buffer' pointer
 *           must point to memory allocated by the caller
 *
 * slot   :: the glyph slot containing the source bitmap
 *
 * mode   :: the requested final rendering mode
 *
 * bgr    :: boolean, set if BGR or VBGR pixel ordering is needed
 */
static void
_fill_xrender_bitmap(FT_Bitmap      *target,
		     FT_GlyphSlot    slot,
		     FT_Render_Mode  mode,
		     int             bgr)
{
    FT_Bitmap *ftbit = &slot->bitmap;
    unsigned char *srcLine = ftbit->buffer;
    unsigned char *dstLine = target->buffer;
    int src_pitch = ftbit->pitch;
    int width = target->width;
    int height = target->rows;
    int pitch = target->pitch;
    int subpixel;
    int h;

    subpixel = (mode == FT_RENDER_MODE_LCD ||
		mode == FT_RENDER_MODE_LCD_V);

    if (src_pitch < 0)
	srcLine -= src_pitch * (ftbit->rows - 1);

    target->pixel_mode = ftbit->pixel_mode;

    switch (ftbit->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
	if (subpixel) {
	    /* convert mono to ARGB32 values */

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;

		for (x = 0; x < width; x++) {
		    if (srcLine[(x >> 3)] & (0x80 >> (x & 7)))
			((unsigned int *) dstLine)[x] = 0xffffffffU;
		}
	    }
	    target->pixel_mode = FT_PIXEL_MODE_LCD;

	} else if (mode == FT_RENDER_MODE_NORMAL) {
	    /* convert mono to 8-bit gray */

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;

		for (x = 0; x < width; x++) {
		    if (srcLine[(x >> 3)] & (0x80 >> (x & 7)))
			dstLine[x] = 0xff;
		}
	    }
	    target->pixel_mode = FT_PIXEL_MODE_GRAY;

	} else {
	    /* copy mono to mono */

	    int  bytes = (width + 7) >> 3;

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch)
		memcpy (dstLine, srcLine, bytes);
	}
	break;

    case FT_PIXEL_MODE_GRAY:
	if (subpixel) {
	    /* convert gray to ARGB32 values */

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++) {
		    unsigned int pix = srcLine[x];

		    pix |= (pix << 8);
		    pix |= (pix << 16);

		    dst[x] = pix;
		}
	    }
	    target->pixel_mode = FT_PIXEL_MODE_LCD;
        } else {
            /* copy gray into gray */

            for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch)
                memcpy (dstLine, srcLine, width);
        }
        break;

    case FT_PIXEL_MODE_LCD:
	if (!bgr) {
	    /* convert horizontal RGB into ARGB32 */

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;
		unsigned char *src = srcLine;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 3) {
		    unsigned int  pix;

		    pix = ((unsigned int)src[0] << 16) |
			  ((unsigned int)src[1] <<  8) |
			  ((unsigned int)src[2]      ) |
			  ((unsigned int)src[1] << 24) ;

		    dst[x] = pix;
		}
	    }
	} else {
	    /* convert horizontal BGR into ARGB32 */

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {

		int x;
		unsigned char *src = srcLine;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 3) {
		    unsigned int  pix;

		    pix = ((unsigned int)src[2] << 16) |
			  ((unsigned int)src[1] <<  8) |
			  ((unsigned int)src[0]      ) |
			  ((unsigned int)src[1] << 24) ;

		    dst[x] = pix;
		}
	    }
	}
	break;

    case FT_PIXEL_MODE_LCD_V:
	/* convert vertical RGB into ARGB32 */
	if (!bgr) {

	    for (h = height; h > 0; h--, srcLine += 3 * src_pitch, dstLine += pitch) {
		int x;
		unsigned char* src = srcLine;
		unsigned int*  dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 1) {
		    unsigned int pix;
		    pix = ((unsigned int)src[0]           << 16) |
			  ((unsigned int)src[src_pitch]   <<  8) |
			  ((unsigned int)src[src_pitch*2]      ) |
			  ((unsigned int)src[src_pitch]   << 24) ;
		    dst[x] = pix;
		}
	    }
	} else {

	    for (h = height; h > 0; h--, srcLine += 3*src_pitch, dstLine += pitch) {
		int x;
		unsigned char *src = srcLine;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 1) {
		    unsigned int  pix;

		    pix = ((unsigned int)src[src_pitch * 2] << 16) |
			  ((unsigned int)src[src_pitch]     <<  8) |
			  ((unsigned int)src[0]                  ) |
			  ((unsigned int)src[src_pitch]     << 24) ;

		    dst[x] = pix;
		}
	    }
	}
	break;

#ifdef FT_LOAD_COLOR
    case FT_PIXEL_MODE_BGRA:
	for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch)
	    memcpy (dstLine, srcLine, (size_t)width * 4);
	break;
#endif

    default:
	assert (0);
    }
}


/* Fills in val->image with an image surface created from @bitmap
 */
static comac_status_t
_get_bitmap_surface (FT_Bitmap		     *bitmap,
		     FT_Library		      library,
		     comac_bool_t	      own_buffer,
		     comac_font_options_t    *font_options,
		     comac_image_surface_t  **surface)
{
    unsigned int width, height;
    unsigned char *data;
    int format = COMAC_FORMAT_A8;
    int stride;
    comac_image_surface_t *image;
    comac_bool_t component_alpha = FALSE;

    width = bitmap->width;
    height = bitmap->rows;

    if (width == 0 || height == 0) {
	*surface = (comac_image_surface_t *)
	    comac_image_surface_create_for_data (NULL, format, 0, 0, 0);
	return (*surface)->base.status;
    }

    switch (bitmap->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
	stride = (((width + 31) & ~31) >> 3);
	if (own_buffer) {
	    data = bitmap->buffer;
	    assert (stride == bitmap->pitch);
	} else {
	    data = _comac_malloc_ab (height, stride);
	    if (!data)
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	    if (stride == bitmap->pitch) {
		memcpy (data, bitmap->buffer, (size_t)stride * height);
	    } else {
		int i;
		unsigned char *source, *dest;

		source = bitmap->buffer;
		dest = data;
		for (i = height; i; i--) {
		    memcpy (dest, source, bitmap->pitch);
		    memset (dest + bitmap->pitch, '\0', stride - bitmap->pitch);

		    source += bitmap->pitch;
		    dest += stride;
		}
	    }
	}

#ifndef WORDS_BIGENDIAN
	{
	    uint8_t *d = data;
	    int count = stride * height;

	    while (count--) {
		*d = COMAC_BITSWAP8 (*d);
		d++;
	    }
	}
#endif
	format = COMAC_FORMAT_A1;
	break;

    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
    case FT_PIXEL_MODE_GRAY:
	if (font_options->antialias != COMAC_ANTIALIAS_SUBPIXEL ||
	    bitmap->pixel_mode == FT_PIXEL_MODE_GRAY)
	{
	    stride = bitmap->pitch;

	    /* We don't support stride not multiple of 4. */
	    if (stride & 3)
	    {
		assert (!own_buffer);
		goto convert;
	    }

	    if (own_buffer) {
		data = bitmap->buffer;
	    } else {
		data = _comac_malloc_ab (height, stride);
		if (!data)
		    return _comac_error (COMAC_STATUS_NO_MEMORY);

		memcpy (data, bitmap->buffer, (size_t)stride * height);
	    }

	    format = COMAC_FORMAT_A8;
	} else {
	    data = bitmap->buffer;
	    stride = bitmap->pitch;
	    format = COMAC_FORMAT_ARGB32;
	    component_alpha = TRUE;
	}
	break;
#ifdef FT_LOAD_COLOR
    case FT_PIXEL_MODE_BGRA:
	stride = width * 4;
	if (own_buffer) {
	    data = bitmap->buffer;
	} else {
	    data = _comac_malloc_ab (height, stride);
	    if (!data)
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	    memcpy (data, bitmap->buffer, (size_t)stride * height);
	}

	if (!_comac_is_little_endian ())
	{
	    /* Byteswap. */
	    unsigned int i, count = height * width;
	    uint32_t *p = (uint32_t *) data;
	    for (i = 0; i < count; i++)
		p[i] = be32_to_cpu (p[i]);
	}
	format = COMAC_FORMAT_ARGB32;
	break;
#endif
    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
    convert:
	if (!own_buffer && library)
	{
	    /* This is pretty much the only case that we can get in here. */
	    /* Convert to 8bit grayscale. */

	    FT_Bitmap  tmp;
	    FT_Int     align;
	    FT_Error   error;

	    format = COMAC_FORMAT_A8;

	    align = comac_format_stride_for_width (format, bitmap->width);

	    FT_Bitmap_New( &tmp );

	    error = FT_Bitmap_Convert( library, bitmap, &tmp, align );
	    if (error)
		return _comac_error (_ft_to_comac_error (error));

	    FT_Bitmap_Done( library, bitmap );
	    *bitmap = tmp;

	    stride = bitmap->pitch;
	    data = _comac_malloc_ab (height, stride);
	    if (!data)
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	    if (bitmap->num_grays != 256)
	    {
	      unsigned int x, y;
	      unsigned int mul = 255 / (bitmap->num_grays - 1);
	      FT_Byte *p = bitmap->buffer;
	      for (y = 0; y < height; y++) {
	        for (x = 0; x < width; x++)
		  p[x] *= mul;
		p += bitmap->pitch;
	      }
	    }

	    memcpy (data, bitmap->buffer, (size_t)stride * height);
	    break;
	}
	/* fall through */
	/* These could be triggered by very rare types of TrueType fonts */
    default:
	if (own_buffer)
	    free (bitmap->buffer);
	return _comac_error (COMAC_STATUS_INVALID_FORMAT);
    }

    /* XXX */
    *surface = image = (comac_image_surface_t *)
	comac_image_surface_create_for_data (data,
					     format,
					     width, height, stride);
    if (image->base.status) {
	free (data);
	return (*surface)->base.status;
    }

    if (component_alpha)
	pixman_image_set_component_alpha (image->pixman_image, TRUE);

    _comac_image_surface_assume_ownership_of_data (image);

    _comac_debug_check_image_surface_is_defined (&image->base);

    return COMAC_STATUS_SUCCESS;
}

/* Converts an outline FT_GlyphSlot into an image
 *
 * This could go through _render_glyph_bitmap as well, letting
 * FreeType convert the outline to a bitmap, but doing it ourselves
 * has two minor advantages: first, we save a copy of the bitmap
 * buffer: we can directly use the buffer that FreeType renders
 * into.
 *
 * Second, it may help when we add support for subpixel
 * rendering: the Xft code does it this way. (Keith thinks that
 * it may also be possible to get the subpixel rendering with
 * FT_Render_Glyph: something worth looking into in more detail
 * when we add subpixel support. If so, we may want to eliminate
 * this version of the code path entirely.
 */
static comac_status_t
_render_glyph_outline (FT_Face                    face,
		       comac_font_options_t	 *font_options,
		       comac_image_surface_t	**surface)
{
    int rgba = FC_RGBA_UNKNOWN;
    int lcd_filter = FT_LCD_FILTER_DEFAULT;
    FT_GlyphSlot glyphslot = face->glyph;
    FT_Outline *outline = &glyphslot->outline;
    FT_Bitmap bitmap;
    FT_BBox cbox;
    unsigned int width, height;
    comac_status_t status;
    FT_Error error;
    FT_Library library = glyphslot->library;
    FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;

    switch (font_options->antialias) {
    case COMAC_ANTIALIAS_NONE:
	render_mode = FT_RENDER_MODE_MONO;
	break;

    case COMAC_ANTIALIAS_SUBPIXEL:
    case COMAC_ANTIALIAS_BEST:
	switch (font_options->subpixel_order) {
	    case COMAC_SUBPIXEL_ORDER_DEFAULT:
	    case COMAC_SUBPIXEL_ORDER_RGB:
	    case COMAC_SUBPIXEL_ORDER_BGR:
		render_mode = FT_RENDER_MODE_LCD;
		break;

	    case COMAC_SUBPIXEL_ORDER_VRGB:
	    case COMAC_SUBPIXEL_ORDER_VBGR:
		render_mode = FT_RENDER_MODE_LCD_V;
		break;
	}

	switch (font_options->lcd_filter) {
	case COMAC_LCD_FILTER_NONE:
	    lcd_filter = FT_LCD_FILTER_NONE;
	    break;
	case COMAC_LCD_FILTER_INTRA_PIXEL:
	    lcd_filter = FT_LCD_FILTER_LEGACY;
	    break;
	case COMAC_LCD_FILTER_FIR3:
	    lcd_filter = FT_LCD_FILTER_LIGHT;
	    break;
	case COMAC_LCD_FILTER_DEFAULT:
	case COMAC_LCD_FILTER_FIR5:
	    lcd_filter = FT_LCD_FILTER_DEFAULT;
	    break;
	}

	break;

    case COMAC_ANTIALIAS_DEFAULT:
    case COMAC_ANTIALIAS_GRAY:
    case COMAC_ANTIALIAS_GOOD:
    case COMAC_ANTIALIAS_FAST:
	render_mode = FT_RENDER_MODE_NORMAL;
    }

    FT_Outline_Get_CBox (outline, &cbox);

    cbox.xMin &= -64;
    cbox.yMin &= -64;
    cbox.xMax = (cbox.xMax + 63) & -64;
    cbox.yMax = (cbox.yMax + 63) & -64;

    width = (unsigned int) ((cbox.xMax - cbox.xMin) >> 6);
    height = (unsigned int) ((cbox.yMax - cbox.yMin) >> 6);

    if (width * height == 0) {
	comac_format_t format;
	/* Looks like fb handles zero-sized images just fine */
	switch (render_mode) {
	case FT_RENDER_MODE_MONO:
	    format = COMAC_FORMAT_A1;
	    break;
	case FT_RENDER_MODE_LCD:
	case FT_RENDER_MODE_LCD_V:
	    format= COMAC_FORMAT_ARGB32;
	    break;
	case FT_RENDER_MODE_LIGHT:
	case FT_RENDER_MODE_NORMAL:
	case FT_RENDER_MODE_MAX:
#if HAVE_FT_RENDER_MODE_SDF
	case FT_RENDER_MODE_SDF:
#endif
	default:
	    format = COMAC_FORMAT_A8;
	    break;
	}

	(*surface) = (comac_image_surface_t *)
	    comac_image_surface_create_for_data (NULL, format, 0, 0, 0);
	pixman_image_set_component_alpha ((*surface)->pixman_image, TRUE);
	if ((*surface)->base.status)
	    return (*surface)->base.status;
    } else {

	int bitmap_size;

	switch (render_mode) {
	case FT_RENDER_MODE_LCD:
	    if (font_options->subpixel_order == COMAC_SUBPIXEL_ORDER_BGR)
		rgba = FC_RGBA_BGR;
	    else
		rgba = FC_RGBA_RGB;
	    break;

	case FT_RENDER_MODE_LCD_V:
	    if (font_options->subpixel_order == COMAC_SUBPIXEL_ORDER_VBGR)
		rgba = FC_RGBA_VBGR;
	    else
		rgba = FC_RGBA_VRGB;
	    break;

	case FT_RENDER_MODE_MONO:
	case FT_RENDER_MODE_LIGHT:
	case FT_RENDER_MODE_NORMAL:
	case FT_RENDER_MODE_MAX:
#if HAVE_FT_RENDER_MODE_SDF
	case FT_RENDER_MODE_SDF:
#endif
	default:
	    break;
	}

#if HAVE_FT_LIBRARY_SETLCDFILTER
	FT_Library_SetLcdFilter (library, lcd_filter);
#endif

	error = FT_Render_Glyph (face->glyph, render_mode);

#if HAVE_FT_LIBRARY_SETLCDFILTER
	FT_Library_SetLcdFilter (library, FT_LCD_FILTER_NONE);
#endif

	if (error)
	    return _comac_error (_ft_to_comac_error (error));

	bitmap_size = _compute_xrender_bitmap_size (&bitmap,
						    face->glyph,
						    render_mode);
	if (bitmap_size < 0)
	    return _comac_error (COMAC_STATUS_INVALID_FORMAT);

	bitmap.buffer = calloc (1, bitmap_size);
	if (bitmap.buffer == NULL)
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	_fill_xrender_bitmap (&bitmap, face->glyph, render_mode,
			      (rgba == FC_RGBA_BGR || rgba == FC_RGBA_VBGR));

	/* Note:
	 * _get_bitmap_surface will free bitmap.buffer if there is an error
	 */
	status = _get_bitmap_surface (&bitmap, NULL, TRUE, font_options, surface);
	if (unlikely (status))
	    return status;

	/* Note: the font's coordinate system is upside down from ours, so the
	 * Y coordinate of the control box needs to be negated.  Moreover, device
	 * offsets are position of glyph origin relative to top left while xMin
	 * and yMax are offsets of top left relative to origin.  Another negation.
	 */
	comac_surface_set_device_offset (&(*surface)->base,
					 (double)-glyphslot->bitmap_left,
					 (double)+glyphslot->bitmap_top);
    }

    return COMAC_STATUS_SUCCESS;
}

/* Converts a bitmap (or other) FT_GlyphSlot into an image */
static comac_status_t
_render_glyph_bitmap (FT_Face		      face,
		      comac_font_options_t   *font_options,
		      comac_image_surface_t **surface)
{
    FT_GlyphSlot glyphslot = face->glyph;
    comac_status_t status;
    FT_Error error;

    /* According to the FreeType docs, glyphslot->format could be
     * something other than FT_GLYPH_FORMAT_OUTLINE or
     * FT_GLYPH_FORMAT_BITMAP. Calling FT_Render_Glyph gives FreeType
     * the opportunity to convert such to
     * bitmap. FT_GLYPH_FORMAT_COMPOSITE will not be encountered since
     * we avoid the FT_LOAD_NO_RECURSE flag.
     */
    error = FT_Render_Glyph (glyphslot, FT_RENDER_MODE_NORMAL);
    /* XXX ignoring all other errors for now.  They are not fatal, typically
     * just a glyph-not-found. */
    if (error == FT_Err_Out_Of_Memory)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = _get_bitmap_surface (&glyphslot->bitmap,
				  glyphslot->library,
				  FALSE, font_options,
				  surface);
    if (unlikely (status))
	return status;

    /*
     * Note: the font's coordinate system is upside down from ours, so the
     * Y coordinate of the control box needs to be negated.  Moreover, device
     * offsets are position of glyph origin relative to top left while
     * bitmap_left and bitmap_top are offsets of top left relative to origin.
     * Another negation.
     */
    comac_surface_set_device_offset (&(*surface)->base,
				     -glyphslot->bitmap_left,
				     +glyphslot->bitmap_top);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_transform_glyph_bitmap (comac_matrix_t         * shape,
			 comac_image_surface_t ** surface)
{
    comac_matrix_t original_to_transformed;
    comac_matrix_t transformed_to_original;
    comac_image_surface_t *old_image;
    comac_surface_t *image;
    double x[4], y[4];
    double origin_x, origin_y;
    int orig_width, orig_height;
    int i;
    int x_min, y_min, x_max, y_max;
    int width, height;
    comac_status_t status;
    comac_surface_pattern_t pattern;

    /* We want to compute a transform that takes the origin
     * (device_x_offset, device_y_offset) to 0,0, then applies
     * the "shape" portion of the font transform
     */
    original_to_transformed = *shape;

    comac_surface_get_device_offset (&(*surface)->base, &origin_x, &origin_y);
    orig_width = (*surface)->width;
    orig_height = (*surface)->height;

    comac_matrix_translate (&original_to_transformed,
			    -origin_x, -origin_y);

    /* Find the bounding box of the original bitmap under that
     * transform
     */
    x[0] = 0;          y[0] = 0;
    x[1] = orig_width; y[1] = 0;
    x[2] = orig_width; y[2] = orig_height;
    x[3] = 0;          y[3] = orig_height;

    for (i = 0; i < 4; i++)
      comac_matrix_transform_point (&original_to_transformed,
				    &x[i], &y[i]);

    x_min = floor (x[0]);   y_min = floor (y[0]);
    x_max =  ceil (x[0]);   y_max =  ceil (y[0]);

    for (i = 1; i < 4; i++) {
	if (x[i] < x_min)
	    x_min = floor (x[i]);
	else if (x[i] > x_max)
	    x_max = ceil (x[i]);
	if (y[i] < y_min)
	    y_min = floor (y[i]);
	else if (y[i] > y_max)
	    y_max = ceil (y[i]);
    }

    /* Adjust the transform so that the bounding box starts at 0,0 ...
     * this gives our final transform from original bitmap to transformed
     * bitmap.
     */
    original_to_transformed.x0 -= x_min;
    original_to_transformed.y0 -= y_min;

    /* Create the transformed bitmap */
    width  = x_max - x_min;
    height = y_max - y_min;

    transformed_to_original = original_to_transformed;
    status = comac_matrix_invert (&transformed_to_original);
    if (unlikely (status))
	return status;

    if ((*surface)->format == COMAC_FORMAT_ARGB32 &&
        !pixman_image_get_component_alpha ((*surface)->pixman_image))
      image = comac_image_surface_create (COMAC_FORMAT_ARGB32, width, height);
    else
      image = comac_image_surface_create (COMAC_FORMAT_A8, width, height);
    if (unlikely (image->status))
	return image->status;

    /* Draw the original bitmap transformed into the new bitmap
     */
    _comac_pattern_init_for_surface (&pattern, &(*surface)->base);
    comac_pattern_set_matrix (&pattern.base, &transformed_to_original);

    status = _comac_surface_paint (image,
				   COMAC_OPERATOR_SOURCE,
				   &pattern.base,
				   NULL);

    _comac_pattern_fini (&pattern.base);

    if (unlikely (status)) {
	comac_surface_destroy (image);
	return status;
    }

    /* Now update the cache entry for the new bitmap, recomputing
     * the origin based on the final transform.
     */
    comac_matrix_transform_point (&original_to_transformed,
				  &origin_x, &origin_y);

    old_image = (*surface);
    (*surface) = (comac_image_surface_t *)image;

    /* Note: we converted subpixel-rendered RGBA images to grayscale,
     * so, no need to copy component alpha to new image. */

    comac_surface_destroy (&old_image->base);

    comac_surface_set_device_offset (&(*surface)->base,
				     _comac_lround (origin_x),
				     _comac_lround (origin_y));
    return COMAC_STATUS_SUCCESS;
}

static const comac_unscaled_font_backend_t comac_ft_unscaled_font_backend = {
    _comac_ft_unscaled_font_destroy,
#if 0
    _comac_ft_unscaled_font_create_glyph
#endif
};

/* #comac_ft_scaled_font_t */

typedef struct _comac_ft_scaled_font {
    comac_scaled_font_t base;
    comac_ft_unscaled_font_t *unscaled;
    comac_ft_options_t ft_options;
} comac_ft_scaled_font_t;

static const comac_scaled_font_backend_t _comac_ft_scaled_font_backend;

#if COMAC_HAS_FC_FONT
/* The load flags passed to FT_Load_Glyph control aspects like hinting and
 * antialiasing. Here we compute them from the fields of a FcPattern.
 */
static void
_get_pattern_ft_options (FcPattern *pattern, comac_ft_options_t *ret)
{
    FcBool antialias, vertical_layout, hinting, autohint, bitmap, embolden;
    comac_ft_options_t ft_options;
    int rgba;
#ifdef FC_HINT_STYLE
    int hintstyle;
#endif
    char *variations;

    _comac_font_options_init_default (&ft_options.base);
    ft_options.load_flags = FT_LOAD_DEFAULT;
    ft_options.synth_flags = 0;

#ifndef FC_EMBEDDED_BITMAP
#define FC_EMBEDDED_BITMAP "embeddedbitmap"
#endif

    /* Check whether to force use of embedded bitmaps */
    if (FcPatternGetBool (pattern,
			  FC_EMBEDDED_BITMAP, 0, &bitmap) != FcResultMatch)
	bitmap = FcFalse;

    /* disable antialiasing if requested */
    if (FcPatternGetBool (pattern,
			  FC_ANTIALIAS, 0, &antialias) != FcResultMatch)
	antialias = FcTrue;
    
    if (antialias) {
	comac_subpixel_order_t subpixel_order;
	int lcd_filter;

	/* disable hinting if requested */
	if (FcPatternGetBool (pattern,
			      FC_HINTING, 0, &hinting) != FcResultMatch)
	    hinting = FcTrue;

	if (FcPatternGetInteger (pattern,
				 FC_RGBA, 0, &rgba) != FcResultMatch)
	    rgba = FC_RGBA_UNKNOWN;

	switch (rgba) {
	case FC_RGBA_RGB:
	    subpixel_order = COMAC_SUBPIXEL_ORDER_RGB;
	    break;
	case FC_RGBA_BGR:
	    subpixel_order = COMAC_SUBPIXEL_ORDER_BGR;
	    break;
	case FC_RGBA_VRGB:
	    subpixel_order = COMAC_SUBPIXEL_ORDER_VRGB;
	    break;
	case FC_RGBA_VBGR:
	    subpixel_order = COMAC_SUBPIXEL_ORDER_VBGR;
	    break;
	case FC_RGBA_UNKNOWN:
	case FC_RGBA_NONE:
	default:
	    subpixel_order = COMAC_SUBPIXEL_ORDER_DEFAULT;
	    break;
	}

	if (subpixel_order != COMAC_SUBPIXEL_ORDER_DEFAULT) {
	    ft_options.base.subpixel_order = subpixel_order;
	    ft_options.base.antialias = COMAC_ANTIALIAS_SUBPIXEL;
	}

	if (FcPatternGetInteger (pattern,
				 FC_LCD_FILTER, 0, &lcd_filter) == FcResultMatch)
	{
	    switch (lcd_filter) {
	    case FC_LCD_NONE:
		ft_options.base.lcd_filter = COMAC_LCD_FILTER_NONE;
		break;
	    case FC_LCD_DEFAULT:
		ft_options.base.lcd_filter = COMAC_LCD_FILTER_FIR5;
		break;
	    case FC_LCD_LIGHT:
		ft_options.base.lcd_filter = COMAC_LCD_FILTER_FIR3;
		break;
	    case FC_LCD_LEGACY:
		ft_options.base.lcd_filter = COMAC_LCD_FILTER_INTRA_PIXEL;
		break;
	    }
	}

#ifdef FC_HINT_STYLE
	if (FcPatternGetInteger (pattern,
				 FC_HINT_STYLE, 0, &hintstyle) != FcResultMatch)
	    hintstyle = FC_HINT_FULL;

	if (!hinting)
	    hintstyle = FC_HINT_NONE;

	switch (hintstyle) {
	case FC_HINT_NONE:
	    ft_options.base.hint_style = COMAC_HINT_STYLE_NONE;
	    break;
	case FC_HINT_SLIGHT:
	    ft_options.base.hint_style = COMAC_HINT_STYLE_SLIGHT;
	    break;
	case FC_HINT_MEDIUM:
	default:
	    ft_options.base.hint_style = COMAC_HINT_STYLE_MEDIUM;
	    break;
	case FC_HINT_FULL:
	    ft_options.base.hint_style = COMAC_HINT_STYLE_FULL;
	    break;
	}
#else /* !FC_HINT_STYLE */
	if (!hinting) {
	    ft_options.base.hint_style = COMAC_HINT_STYLE_NONE;
	}
#endif /* FC_HINT_STYLE */

	/* Force embedded bitmaps off if no hinting requested */
	if (ft_options.base.hint_style == COMAC_HINT_STYLE_NONE)
	  bitmap = FcFalse;

	if (!bitmap)
	    ft_options.load_flags |= FT_LOAD_NO_BITMAP;

    } else {
	ft_options.base.antialias = COMAC_ANTIALIAS_NONE;
    }

    /* force autohinting if requested */
    if (FcPatternGetBool (pattern,
			  FC_AUTOHINT, 0, &autohint) != FcResultMatch)
	autohint = FcFalse;

    if (autohint)
	ft_options.load_flags |= FT_LOAD_FORCE_AUTOHINT;

    if (FcPatternGetBool (pattern,
			  FC_VERTICAL_LAYOUT, 0, &vertical_layout) != FcResultMatch)
	vertical_layout = FcFalse;

    if (vertical_layout)
	ft_options.load_flags |= FT_LOAD_VERTICAL_LAYOUT;

#ifndef FC_EMBOLDEN
#define FC_EMBOLDEN "embolden"
#endif
    if (FcPatternGetBool (pattern,
			  FC_EMBOLDEN, 0, &embolden) != FcResultMatch)
	embolden = FcFalse;

    if (embolden)
	ft_options.synth_flags |= COMAC_FT_SYNTHESIZE_BOLD;

#ifndef FC_FONT_VARIATIONS
#define FC_FONT_VARIATIONS "fontvariations"
#endif
    if (FcPatternGetString (pattern, FC_FONT_VARIATIONS, 0, (FcChar8 **) &variations) == FcResultMatch) {
      ft_options.base.variations = strdup (variations);
    }

    *ret = ft_options;
}
#endif

static void
_comac_ft_options_merge (comac_ft_options_t *options,
			 comac_ft_options_t *other)
{
    int load_flags = other->load_flags;
    int load_target = FT_LOAD_TARGET_NORMAL;

    /* clear load target mode */
    load_flags &= ~(FT_LOAD_TARGET_(FT_LOAD_TARGET_MODE(other->load_flags)));

    if (load_flags & FT_LOAD_NO_HINTING)
	other->base.hint_style = COMAC_HINT_STYLE_NONE;

    if (other->base.antialias == COMAC_ANTIALIAS_NONE ||
	options->base.antialias == COMAC_ANTIALIAS_NONE) {
	options->base.antialias = COMAC_ANTIALIAS_NONE;
	options->base.subpixel_order = COMAC_SUBPIXEL_ORDER_DEFAULT;
    }

    if (other->base.antialias == COMAC_ANTIALIAS_SUBPIXEL &&
	options->base.antialias == COMAC_ANTIALIAS_DEFAULT) {
	options->base.antialias = COMAC_ANTIALIAS_SUBPIXEL;
	options->base.subpixel_order = other->base.subpixel_order;
    }

    if (options->base.hint_style == COMAC_HINT_STYLE_DEFAULT)
	options->base.hint_style = other->base.hint_style;

    if (other->base.hint_style == COMAC_HINT_STYLE_NONE)
	options->base.hint_style = COMAC_HINT_STYLE_NONE;

    if (options->base.lcd_filter == COMAC_LCD_FILTER_DEFAULT)
	options->base.lcd_filter = other->base.lcd_filter;

    if (other->base.lcd_filter == COMAC_LCD_FILTER_NONE)
	options->base.lcd_filter = COMAC_LCD_FILTER_NONE;

    if (options->base.antialias == COMAC_ANTIALIAS_NONE) {
	if (options->base.hint_style == COMAC_HINT_STYLE_NONE)
	    load_flags |= FT_LOAD_NO_HINTING;
	else
	    load_target = FT_LOAD_TARGET_MONO;
	load_flags |= FT_LOAD_MONOCHROME;
    } else {
	switch (options->base.hint_style) {
	case COMAC_HINT_STYLE_NONE:
	    load_flags |= FT_LOAD_NO_HINTING;
	    break;
	case COMAC_HINT_STYLE_SLIGHT:
	    load_target = FT_LOAD_TARGET_LIGHT;
	    break;
	case COMAC_HINT_STYLE_MEDIUM:
	    break;
	case COMAC_HINT_STYLE_FULL:
	case COMAC_HINT_STYLE_DEFAULT:
	    if (options->base.antialias == COMAC_ANTIALIAS_SUBPIXEL) {
		switch (options->base.subpixel_order) {
		case COMAC_SUBPIXEL_ORDER_DEFAULT:
		case COMAC_SUBPIXEL_ORDER_RGB:
		case COMAC_SUBPIXEL_ORDER_BGR:
		    load_target = FT_LOAD_TARGET_LCD;
		    break;
		case COMAC_SUBPIXEL_ORDER_VRGB:
		case COMAC_SUBPIXEL_ORDER_VBGR:
		    load_target = FT_LOAD_TARGET_LCD_V;
		break;
		}
	    }
	    break;
	}
    }

    if (other->base.variations) {
      if (options->base.variations) {
        char *p;

        /* 'merge' variations by concatenating - later entries win */
        p = malloc (strlen (other->base.variations) + strlen (options->base.variations) + 2);
        p[0] = 0;
        strcat (p, other->base.variations);
        strcat (p, ",");
        strcat (p, options->base.variations);
        free (options->base.variations);
        options->base.variations = p;
      }
      else {
        options->base.variations = strdup (other->base.variations);
      }
    }

    options->load_flags = load_flags | load_target;
    options->synth_flags = other->synth_flags;
}

static comac_status_t
_comac_ft_font_face_scaled_font_create (void		    *abstract_font_face,
					const comac_matrix_t	 *font_matrix,
					const comac_matrix_t	 *ctm,
					const comac_font_options_t *options,
					comac_scaled_font_t       **font_out)
{
    comac_ft_font_face_t *font_face = abstract_font_face;
    comac_ft_scaled_font_t *scaled_font;
    FT_Face face;
    FT_Size_Metrics *metrics;
    comac_font_extents_t fs_metrics;
    comac_status_t status;
    comac_ft_unscaled_font_t *unscaled;

    assert (font_face->unscaled);

    face = _comac_ft_unscaled_font_lock_face (font_face->unscaled);
    if (unlikely (face == NULL)) /* backend error */
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    scaled_font = _comac_malloc (sizeof (comac_ft_scaled_font_t));
    if (unlikely (scaled_font == NULL)) {
	status = _comac_error (COMAC_STATUS_NO_MEMORY);
	goto FAIL;
    }

    scaled_font->unscaled = unscaled = font_face->unscaled;
    _comac_unscaled_font_reference (&unscaled->base);

    _comac_font_options_init_copy (&scaled_font->ft_options.base, options);
    _comac_ft_options_merge (&scaled_font->ft_options, &font_face->ft_options);

    status = _comac_scaled_font_init (&scaled_font->base,
			              &font_face->base,
				      font_matrix, ctm, options,
				      &_comac_ft_scaled_font_backend);
    if (unlikely (status))
	goto CLEANUP_SCALED_FONT;

    status = _comac_ft_unscaled_font_set_scale (unscaled,
				                &scaled_font->base.scale);
    if (unlikely (status)) {
	/* This can only fail if we encounter an error with the underlying
	 * font, so propagate the error back to the font-face. */
	_comac_ft_unscaled_font_unlock_face (unscaled);
	_comac_unscaled_font_destroy (&unscaled->base);
	free (scaled_font);
	return status;
    }


    metrics = &face->size->metrics;

    /*
     * Get to unscaled metrics so that the upper level can get back to
     * user space
     *
     * Also use this path for bitmap-only fonts.  The other branch uses
     * face members that are only relevant for scalable fonts.  This is
     * detected by simply checking for units_per_EM==0.
     */
    if (scaled_font->base.options.hint_metrics != COMAC_HINT_METRICS_OFF ||
	face->units_per_EM == 0) {
	double x_factor, y_factor;

	if (unscaled->x_scale == 0)
	    x_factor = 0;
	else
	    x_factor = 1 / unscaled->x_scale;

	if (unscaled->y_scale == 0)
	    y_factor = 0;
	else
	    y_factor = 1 / unscaled->y_scale;

	fs_metrics.ascent =        DOUBLE_FROM_26_6(metrics->ascender) * y_factor;
	fs_metrics.descent =       DOUBLE_FROM_26_6(- metrics->descender) * y_factor;
	fs_metrics.height =        DOUBLE_FROM_26_6(metrics->height) * y_factor;
	if (!_comac_ft_scaled_font_is_vertical (&scaled_font->base)) {
	    fs_metrics.max_x_advance = DOUBLE_FROM_26_6(metrics->max_advance) * x_factor;
	    fs_metrics.max_y_advance = 0;
	} else {
	    fs_metrics.max_x_advance = 0;
	    fs_metrics.max_y_advance = DOUBLE_FROM_26_6(metrics->max_advance) * y_factor;
	}
    } else {
	double scale = face->units_per_EM;

	fs_metrics.ascent =        face->ascender / scale;
	fs_metrics.descent =       - face->descender / scale;
	fs_metrics.height =        face->height / scale;
	if (!_comac_ft_scaled_font_is_vertical (&scaled_font->base)) {
	    fs_metrics.max_x_advance = face->max_advance_width / scale;
	    fs_metrics.max_y_advance = 0;
	} else {
	    fs_metrics.max_x_advance = 0;
	    fs_metrics.max_y_advance = face->max_advance_height / scale;
	}
    }

    status = _comac_scaled_font_set_metrics (&scaled_font->base, &fs_metrics);
    if (unlikely (status))
	goto CLEANUP_SCALED_FONT;

    _comac_ft_unscaled_font_unlock_face (unscaled);

    *font_out = &scaled_font->base;
    return COMAC_STATUS_SUCCESS;

  CLEANUP_SCALED_FONT:
    _comac_unscaled_font_destroy (&unscaled->base);
    free (scaled_font);
  FAIL:
    _comac_ft_unscaled_font_unlock_face (font_face->unscaled);
    *font_out = _comac_scaled_font_create_in_error (status);
    return COMAC_STATUS_SUCCESS; /* non-backend error */
}

comac_bool_t
_comac_scaled_font_is_ft (comac_scaled_font_t *scaled_font)
{
    return scaled_font->backend == &_comac_ft_scaled_font_backend;
}

static void
_comac_ft_scaled_font_fini (void *abstract_font)
{
    comac_ft_scaled_font_t *scaled_font = abstract_font;

    if (scaled_font == NULL)
        return;

    _comac_unscaled_font_destroy (&scaled_font->unscaled->base);
}

static int
_move_to (FT_Vector *to, void *closure)
{
    comac_path_fixed_t *path = closure;
    comac_fixed_t x, y;

    x = _comac_fixed_from_26_6 (to->x);
    y = _comac_fixed_from_26_6 (to->y);

    if (_comac_path_fixed_close_path (path) != COMAC_STATUS_SUCCESS)
	return 1;
    if (_comac_path_fixed_move_to (path, x, y) != COMAC_STATUS_SUCCESS)
	return 1;

    return 0;
}

static int
_line_to (FT_Vector *to, void *closure)
{
    comac_path_fixed_t *path = closure;
    comac_fixed_t x, y;

    x = _comac_fixed_from_26_6 (to->x);
    y = _comac_fixed_from_26_6 (to->y);

    if (_comac_path_fixed_line_to (path, x, y) != COMAC_STATUS_SUCCESS)
	return 1;

    return 0;
}

static int
_conic_to (FT_Vector *control, FT_Vector *to, void *closure)
{
    comac_path_fixed_t *path = closure;

    comac_fixed_t x0, y0;
    comac_fixed_t x1, y1;
    comac_fixed_t x2, y2;
    comac_fixed_t x3, y3;
    comac_point_t conic;

    if (! _comac_path_fixed_get_current_point (path, &x0, &y0))
	return 1;

    conic.x = _comac_fixed_from_26_6 (control->x);
    conic.y = _comac_fixed_from_26_6 (control->y);

    x3 = _comac_fixed_from_26_6 (to->x);
    y3 = _comac_fixed_from_26_6 (to->y);

    x1 = x0 + 2.0/3.0 * (conic.x - x0);
    y1 = y0 + 2.0/3.0 * (conic.y - y0);

    x2 = x3 + 2.0/3.0 * (conic.x - x3);
    y2 = y3 + 2.0/3.0 * (conic.y - y3);

    if (_comac_path_fixed_curve_to (path,
				    x1, y1,
				    x2, y2,
				    x3, y3) != COMAC_STATUS_SUCCESS)
	return 1;

    return 0;
}

static int
_cubic_to (FT_Vector *control1, FT_Vector *control2,
	   FT_Vector *to, void *closure)
{
    comac_path_fixed_t *path = closure;
    comac_fixed_t x0, y0;
    comac_fixed_t x1, y1;
    comac_fixed_t x2, y2;

    x0 = _comac_fixed_from_26_6 (control1->x);
    y0 = _comac_fixed_from_26_6 (control1->y);

    x1 = _comac_fixed_from_26_6 (control2->x);
    y1 = _comac_fixed_from_26_6 (control2->y);

    x2 = _comac_fixed_from_26_6 (to->x);
    y2 = _comac_fixed_from_26_6 (to->y);

    if (_comac_path_fixed_curve_to (path,
				    x0, y0,
				    x1, y1,
				    x2, y2) != COMAC_STATUS_SUCCESS)
	return 1;

    return 0;
}

static comac_status_t
_decompose_glyph_outline (FT_Face		  face,
			  comac_font_options_t	 *options,
			  comac_path_fixed_t	**pathp)
{
    static const FT_Outline_Funcs outline_funcs = {
	(FT_Outline_MoveToFunc)_move_to,
	(FT_Outline_LineToFunc)_line_to,
	(FT_Outline_ConicToFunc)_conic_to,
	(FT_Outline_CubicToFunc)_cubic_to,
	0, /* shift */
	0, /* delta */
    };
    static const FT_Matrix invert_y = {
	DOUBLE_TO_16_16 (1.0), 0,
	0, DOUBLE_TO_16_16 (-1.0),
    };

    FT_GlyphSlot glyph;
    comac_path_fixed_t *path;
    comac_status_t status;

    path = _comac_path_fixed_create ();
    if (!path)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    glyph = face->glyph;

    /* Font glyphs have an inverted Y axis compared to comac. */
    FT_Outline_Transform (&glyph->outline, &invert_y);
    if (FT_Outline_Decompose (&glyph->outline, &outline_funcs, path)) {
	_comac_path_fixed_destroy (path);
	return _comac_error (COMAC_STATUS_NO_MEMORY);
    }

    status = _comac_path_fixed_close_path (path);
    if (unlikely (status)) {
	_comac_path_fixed_destroy (path);
	return status;
    }

    *pathp = path;

    return COMAC_STATUS_SUCCESS;
}

/*
 * Translate glyph to match its metrics.
 */
static void
_comac_ft_scaled_glyph_vertical_layout_bearing_fix (void        *abstract_font,
						    FT_GlyphSlot glyph)
{
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    FT_Vector vector;

    vector.x = glyph->metrics.vertBearingX - glyph->metrics.horiBearingX;
    vector.y = -glyph->metrics.vertBearingY - glyph->metrics.horiBearingY;

    if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
	FT_Vector_Transform (&vector, &scaled_font->unscaled->Current_Shape);
	FT_Outline_Translate(&glyph->outline, vector.x, vector.y);
    } else if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
	glyph->bitmap_left += vector.x / 64;
	glyph->bitmap_top  += vector.y / 64;
    }
}

static void
comac_ft_apply_variations (FT_Face                 face,
			   comac_ft_scaled_font_t *scaled_font)
{
    FT_MM_Var *ft_mm_var;
    FT_Error ret;
    unsigned int instance_id = scaled_font->unscaled->id >> 16;

    ret = FT_Get_MM_Var (face, &ft_mm_var);
    if (ret == 0) {
        FT_Fixed *current_coords;
        FT_Fixed *coords;
        unsigned int i;
        const char *p;

        coords = malloc (sizeof (FT_Fixed) * ft_mm_var->num_axis);
	/* FIXME check coords. */

	if (scaled_font->unscaled->variations)
	{
	    memcpy (coords, scaled_font->unscaled->variations, ft_mm_var->num_axis * sizeof (*coords));
	}
	else if (instance_id && instance_id <= ft_mm_var->num_namedstyles)
	{
	    FT_Var_Named_Style *instance = &ft_mm_var->namedstyle[instance_id - 1];
	    memcpy (coords, instance->coords, ft_mm_var->num_axis * sizeof (*coords));
	}
	else
	    for (i = 0; i < ft_mm_var->num_axis; i++)
		coords[i] = ft_mm_var->axis[i].def;

        p = scaled_font->ft_options.base.variations;
        while (p && *p) {
            const char *start;
            const char *end, *end2;
            FT_ULong tag;
            double value;

            while (_comac_isspace (*p)) p++;

            start = p;
            end = strchr (p, ',');
            if (end && (end - p < 6))
                goto skip;

            tag = FT_MAKE_TAG(p[0], p[1], p[2], p[3]);

            p += 4;
            while (_comac_isspace (*p)) p++;
            if (*p == '=') p++;

            if (p - start < 5)
                goto skip;

            value = _comac_strtod (p, (char **) &end2);

            while (end2 && _comac_isspace (*end2)) end2++;

            if (end2 && (*end2 != ',' && *end2 != '\0'))
                goto skip;

            for (i = 0; i < ft_mm_var->num_axis; i++) {
                if (ft_mm_var->axis[i].tag == tag) {
                    coords[i] = (FT_Fixed)(value*65536);
                    break;
                }
            }

skip:
            p = end ? end + 1 : NULL;
        }

        current_coords = malloc (sizeof (FT_Fixed) * ft_mm_var->num_axis);
#ifdef HAVE_FT_GET_VAR_DESIGN_COORDINATES
        ret = FT_Get_Var_Design_Coordinates (face, ft_mm_var->num_axis, current_coords);
        if (ret == 0) {
            for (i = 0; i < ft_mm_var->num_axis; i++) {
              if (coords[i] != current_coords[i])
                break;
            }
            if (i == ft_mm_var->num_axis)
              goto done;
        }
#endif

        FT_Set_Var_Design_Coordinates (face, ft_mm_var->num_axis, coords);
done:
        free (coords);
        free (current_coords);
#if HAVE_FT_DONE_MM_VAR
        FT_Done_MM_Var (face->glyph->library, ft_mm_var);
#else
        free (ft_mm_var);
#endif
    }
}

static comac_int_status_t
_comac_ft_scaled_glyph_load_glyph (comac_ft_scaled_font_t *scaled_font,
				   comac_scaled_glyph_t   *scaled_glyph,
				   FT_Face                 face,
				   int                     load_flags,
				   comac_bool_t            use_em_size,
				   comac_bool_t            vertical_layout)
{
    FT_Error error;
    comac_status_t status;

    if (use_em_size) {
	comac_matrix_t em_size;
	comac_matrix_init_scale (&em_size, face->units_per_EM, face->units_per_EM);
	status = _comac_ft_unscaled_font_set_scale (scaled_font->unscaled, &em_size);
    } else {
	status = _comac_ft_unscaled_font_set_scale (scaled_font->unscaled,
						    &scaled_font->base.scale);
    }
    if (unlikely (status))
	return status;

    comac_ft_apply_variations (face, scaled_font);

    error = FT_Load_Glyph (face,
			   _comac_scaled_glyph_index(scaled_glyph),
			   load_flags);
    /* XXX ignoring all other errors for now.  They are not fatal, typically
     * just a glyph-not-found. */
    if (error == FT_Err_Out_Of_Memory)
	return  _comac_error (COMAC_STATUS_NO_MEMORY);

    /*
     * synthesize glyphs if requested
     */
#if HAVE_FT_GLYPHSLOT_EMBOLDEN
    if (scaled_font->ft_options.synth_flags & COMAC_FT_SYNTHESIZE_BOLD)
	FT_GlyphSlot_Embolden (face->glyph);
#endif

#if HAVE_FT_GLYPHSLOT_OBLIQUE
    if (scaled_font->ft_options.synth_flags & COMAC_FT_SYNTHESIZE_OBLIQUE)
	FT_GlyphSlot_Oblique (face->glyph);
#endif

    if (vertical_layout)
	_comac_ft_scaled_glyph_vertical_layout_bearing_fix (scaled_font, face->glyph);

    if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
        FT_Pos xshift, yshift;

        xshift = _comac_scaled_glyph_xphase (scaled_glyph) << 4;
        yshift = _comac_scaled_glyph_yphase (scaled_glyph) << 4;

        FT_Outline_Translate (&face->glyph->outline, xshift, -yshift);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_comac_ft_scaled_glyph_init_surface (comac_ft_scaled_font_t     *scaled_font,
				     comac_scaled_glyph_t	*scaled_glyph,
				     comac_scaled_glyph_info_t	 info,
				     FT_Face face,
				     const comac_color_t        *foreground_color,
				     comac_bool_t vertical_layout,
				     int load_flags)
{
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_GlyphSlot glyph;
    comac_status_t status;
    comac_image_surface_t	*surface;
    comac_bool_t uses_foreground_color = FALSE;

    /* Only one info type at a time handled in this function */
    assert (info == COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE || info == COMAC_SCALED_GLYPH_INFO_SURFACE);

    if (info == COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) {
	if (!unscaled->have_color) {
	    scaled_glyph->color_glyph = FALSE;
	    scaled_glyph->color_glyph_set = TRUE;
	    return COMAC_INT_STATUS_UNSUPPORTED;
	}

#ifdef HAVE_FT_PALETTE_SELECT
	FT_LayerIterator  iterator;
	FT_UInt layer_glyph_index;
	FT_UInt layer_color_index;
	FT_Color color;
	FT_Palette_Data palette_data;

	/* Check if there is a layer that uses the foreground color */
	iterator.p  = NULL;
	while (FT_Get_Color_Glyph_Layer(face,
					_comac_scaled_glyph_index(scaled_glyph),
					&layer_glyph_index,
					&layer_color_index,
					&iterator)) {
	    if (layer_color_index == 0xFFFF) {
		uses_foreground_color = TRUE;
		break;
	    }
	}

	if (uses_foreground_color) {
	    color.red = (FT_Byte)(foreground_color->red * 0xFF);
	    color.green = (FT_Byte)(foreground_color->green * 0xFF);
	    color.blue = (FT_Byte)(foreground_color->blue * 0xFF);
	    color.alpha = (FT_Byte)(foreground_color->alpha * 0xFF);
	    FT_Palette_Set_Foreground_Color (face, color);
	}

	if (FT_Palette_Data_Get(face, &palette_data) == 0 && palette_data.num_palettes > 0) {
	    FT_UShort palette_index = COMAC_COLOR_PALETTE_DEFAULT;
	    if (scaled_font->base.options.palette_index < palette_data.num_palettes)
		palette_index = scaled_font->base.options.palette_index;

	    FT_Palette_Select (face, palette_index, NULL);
	}
#endif

        load_flags &= ~FT_LOAD_MONOCHROME;
	/* clear load target mode */
	load_flags &= ~(FT_LOAD_TARGET_(FT_LOAD_TARGET_MODE(load_flags)));
	load_flags |= FT_LOAD_TARGET_NORMAL;
#ifdef FT_LOAD_COLOR
	load_flags |= FT_LOAD_COLOR;
#endif
    } else { /* info == COMAC_SCALED_GLYPH_INFO_SURFACE */
#ifdef FT_LOAD_COLOR
        load_flags &= ~FT_LOAD_COLOR;
#endif
    }

    status = _comac_ft_scaled_glyph_load_glyph (scaled_font,
						scaled_glyph,
						face,
						load_flags,
						FALSE,
						vertical_layout);
    if (unlikely (status))
	return status;

    glyph = face->glyph;

    if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
	status = _render_glyph_outline (face, &scaled_font->ft_options.base,
					    &surface);
    } else {
	status = _render_glyph_bitmap (face, &scaled_font->ft_options.base,
					   &surface);
	if (likely (status == COMAC_STATUS_SUCCESS) && unscaled->have_shape) {
	    status = _transform_glyph_bitmap (&unscaled->current_shape,
					      &surface);
	    if (unlikely (status))
		comac_surface_destroy (&surface->base);
	}
    }

    if (unlikely (status))
	return status;

    if (info == COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) {
	/* We tried loading a color glyph and can now check if we got
	 * a color glyph and set scaled_glyph->color_glyph
	 * accordingly */
	if (pixman_image_get_format (surface->pixman_image) == PIXMAN_a8r8g8b8 &&
	    !pixman_image_get_component_alpha (surface->pixman_image))
	{
	    _comac_scaled_glyph_set_color_surface (scaled_glyph,
						   &scaled_font->base,
						   surface,
						   uses_foreground_color);

	    scaled_glyph->color_glyph = TRUE;
	} else {
	    /* We didn't ask for a non-color surface, but store it
	     * anyway so we don't have to load it again. */
	    _comac_scaled_glyph_set_surface (scaled_glyph,
					     &scaled_font->base,
					     surface);
	    scaled_glyph->color_glyph = FALSE;
	    status = COMAC_INT_STATUS_UNSUPPORTED;
	}
	scaled_glyph->color_glyph_set = TRUE;
    } else { /* info == COMAC_SCALED_GLYPH_INFO_SURFACE */
	_comac_scaled_glyph_set_surface (scaled_glyph,
					 &scaled_font->base,
					 surface);
    }

    return status;
}

static comac_int_status_t
_comac_ft_scaled_glyph_init (void			*abstract_font,
			     comac_scaled_glyph_t	*scaled_glyph,
			     comac_scaled_glyph_info_t	 info,
			     const comac_color_t        *foreground_color)
{
    comac_text_extents_t    fs_metrics;
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_GlyphSlot glyph;
    FT_Face face;
    int load_flags = scaled_font->ft_options.load_flags;
    FT_Glyph_Metrics *metrics;
    double x_factor, y_factor;
    comac_bool_t vertical_layout = FALSE;
    comac_status_t status = COMAC_STATUS_SUCCESS;
    comac_bool_t scaled_glyph_loaded = FALSE;

    face = _comac_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    /* Ignore global advance unconditionally */
    load_flags |= FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;

    if ((info & COMAC_SCALED_GLYPH_INFO_PATH) != 0 &&
	(info & (COMAC_SCALED_GLYPH_INFO_SURFACE |
                 COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE)) == 0) {
	load_flags |= FT_LOAD_NO_BITMAP;
    }

    /*
     * Don't pass FT_LOAD_VERTICAL_LAYOUT to FT_Load_Glyph here as
     * suggested by freetype people.
     */
    if (load_flags & FT_LOAD_VERTICAL_LAYOUT) {
	load_flags &= ~FT_LOAD_VERTICAL_LAYOUT;
	vertical_layout = TRUE;
    }

    if (info & COMAC_SCALED_GLYPH_INFO_METRICS) {

	comac_bool_t hint_metrics = scaled_font->base.options.hint_metrics != COMAC_HINT_METRICS_OFF;

	/* The font metrics for color glyphs should be the same as the
	 * outline glyphs. But just in case there aren't, request the
	 * color or outline metrics based on the font option and if
	 * the font has color.
	 */
	int color_flag = 0;
#ifdef FT_LOAD_COLOR
	if (unscaled->have_color && scaled_font->base.options.color_mode != COMAC_COLOR_MODE_NO_COLOR)
	    color_flag = FT_LOAD_COLOR;
#endif
	status = _comac_ft_scaled_glyph_load_glyph (scaled_font,
						    scaled_glyph,
						    face,
						    load_flags | color_flag,
						    !hint_metrics,
						    vertical_layout);
	if (unlikely (status))
	    goto FAIL;

	glyph = face->glyph;
	scaled_glyph_loaded = hint_metrics;

	/*
	 * Compute font-space metrics
	 */
	metrics = &glyph->metrics;

	if (unscaled->x_scale == 0)
	    x_factor = 0;
	else
	    x_factor = 1 / unscaled->x_scale;

	if (unscaled->y_scale == 0)
	    y_factor = 0;
	else
	    y_factor = 1 / unscaled->y_scale;

	/*
	 * Note: Y coordinates of the horizontal bearing need to be negated.
	 *
	 * Scale metrics back to glyph space from the scaled glyph space returned
	 * by FreeType
	 *
	 * If we want hinted metrics but aren't asking for hinted glyphs from
	 * FreeType, then we need to do the metric hinting ourselves.
	 */

	if (hint_metrics && (load_flags & FT_LOAD_NO_HINTING))
	{
	    FT_Pos x1, x2;
	    FT_Pos y1, y2;
	    FT_Pos advance;

	    if (!vertical_layout) {
		x1 = (metrics->horiBearingX) & -64;
		x2 = (metrics->horiBearingX + metrics->width + 63) & -64;
		y1 = (-metrics->horiBearingY) & -64;
		y2 = (-metrics->horiBearingY + metrics->height + 63) & -64;

		advance = ((metrics->horiAdvance + 32) & -64);

		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (x1) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (y1) * y_factor;

		fs_metrics.width  = DOUBLE_FROM_26_6 (x2 - x1) * x_factor;
		fs_metrics.height  = DOUBLE_FROM_26_6 (y2 - y1) * y_factor;

		fs_metrics.x_advance = DOUBLE_FROM_26_6 (advance) * x_factor;
		fs_metrics.y_advance = 0;
	    } else {
		x1 = (metrics->vertBearingX) & -64;
		x2 = (metrics->vertBearingX + metrics->width + 63) & -64;
		y1 = (metrics->vertBearingY) & -64;
		y2 = (metrics->vertBearingY + metrics->height + 63) & -64;

		advance = ((metrics->vertAdvance + 32) & -64);

		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (x1) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (y1) * y_factor;

		fs_metrics.width  = DOUBLE_FROM_26_6 (x2 - x1) * x_factor;
		fs_metrics.height  = DOUBLE_FROM_26_6 (y2 - y1) * y_factor;

		fs_metrics.x_advance = 0;
		fs_metrics.y_advance = DOUBLE_FROM_26_6 (advance) * y_factor;
	    }
	 } else {
	    fs_metrics.width  = DOUBLE_FROM_26_6 (metrics->width) * x_factor;
	    fs_metrics.height = DOUBLE_FROM_26_6 (metrics->height) * y_factor;

	    if (!vertical_layout) {
		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (metrics->horiBearingX) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (-metrics->horiBearingY) * y_factor;

		if (hint_metrics || glyph->format != FT_GLYPH_FORMAT_OUTLINE)
		    fs_metrics.x_advance = DOUBLE_FROM_26_6 (metrics->horiAdvance) * x_factor;
		else
		    fs_metrics.x_advance = DOUBLE_FROM_16_16 (glyph->linearHoriAdvance) * x_factor;
		fs_metrics.y_advance = 0 * y_factor;
	    } else {
		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (metrics->vertBearingX) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (metrics->vertBearingY) * y_factor;

		fs_metrics.x_advance = 0 * x_factor;
		if (hint_metrics || glyph->format != FT_GLYPH_FORMAT_OUTLINE)
		    fs_metrics.y_advance = DOUBLE_FROM_26_6 (metrics->vertAdvance) * y_factor;
		else
		    fs_metrics.y_advance = DOUBLE_FROM_16_16 (glyph->linearVertAdvance) * y_factor;
	    }
	 }

	_comac_scaled_glyph_set_metrics (scaled_glyph,
					 &scaled_font->base,
					 &fs_metrics);
    }

    if (info & COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) {
	status = _comac_ft_scaled_glyph_init_surface (scaled_font,
						      scaled_glyph,
						      COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE,
						      face,
						      foreground_color,
						      vertical_layout,
						      load_flags);
	if (unlikely (status))
	    goto FAIL;
    }

    if (info & COMAC_SCALED_GLYPH_INFO_SURFACE) {
	status = _comac_ft_scaled_glyph_init_surface (scaled_font,
						      scaled_glyph,
						      COMAC_SCALED_GLYPH_INFO_SURFACE,
						      face,
						      NULL, /* foreground color */
						      vertical_layout,
						      load_flags);
	if (unlikely (status))
	    goto FAIL;
    }

    if (info & COMAC_SCALED_GLYPH_INFO_PATH) {
	comac_path_fixed_t *path = NULL; /* hide compiler warning */

	/*
	 * A kludge -- the above code will trash the outline,
	 * so reload it. This will probably never occur though
	 */
	if ((info & (COMAC_SCALED_GLYPH_INFO_SURFACE | COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE)) != 0) {
	    scaled_glyph_loaded = FALSE;
	    load_flags |= FT_LOAD_NO_BITMAP;
	}

	if (!scaled_glyph_loaded) {
	    status = _comac_ft_scaled_glyph_load_glyph (scaled_font,
							scaled_glyph,
							face,
							load_flags,
							FALSE,
							vertical_layout);
	    if (unlikely (status))
		goto FAIL;

	    glyph = face->glyph;
	}

	if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	    status = _decompose_glyph_outline (face, &scaled_font->ft_options.base,
					       &path);
	else
	    status = COMAC_INT_STATUS_UNSUPPORTED;

	if (unlikely (status))
	    goto FAIL;

	_comac_scaled_glyph_set_path (scaled_glyph,
				      &scaled_font->base,
				      path);
    }
 FAIL:
    _comac_ft_unscaled_font_unlock_face (unscaled);

    return status;
}

static unsigned long
_comac_ft_ucs4_to_index (void	    *abstract_font,
			 uint32_t    ucs4)
{
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    FT_UInt index;

    face = _comac_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return 0;

#if COMAC_HAS_FC_FONT
    index = FcFreeTypeCharIndex (face, ucs4);
#else
    index = FT_Get_Char_Index (face, ucs4);
#endif

    _comac_ft_unscaled_font_unlock_face (unscaled);
    return index;
}

static comac_int_status_t
_comac_ft_load_truetype_table (void	       *abstract_font,
                              unsigned long     tag,
                              long              offset,
                              unsigned char    *buffer,
                              unsigned long    *length)
{
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    comac_status_t status = COMAC_INT_STATUS_UNSUPPORTED;

    /* We don't support the FreeType feature of loading a table
     * without specifying the size since this may overflow our
     * buffer. */
    assert (length != NULL);

    if (_comac_ft_scaled_font_is_vertical (&scaled_font->base))
        return COMAC_INT_STATUS_UNSUPPORTED;

#if HAVE_FT_LOAD_SFNT_TABLE
    face = _comac_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    if (FT_IS_SFNT (face)) {
	if (buffer == NULL)
	    *length = 0;

	if (FT_Load_Sfnt_Table (face, tag, offset, buffer, length) == 0)
	    status = COMAC_STATUS_SUCCESS;
    }

    _comac_ft_unscaled_font_unlock_face (unscaled);
#endif

    return status;
}

static comac_int_status_t
_comac_ft_index_to_ucs4(void	        *abstract_font,
			unsigned long    index,
			uint32_t	*ucs4)
{
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    FT_ULong  charcode;
    FT_UInt   gindex;

    face = _comac_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    *ucs4 = (uint32_t) -1;
    charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0) {
	if (gindex == index) {
	    *ucs4 = charcode;
	    break;
	}
	charcode = FT_Get_Next_Char (face, charcode, &gindex);
    }

    _comac_ft_unscaled_font_unlock_face (unscaled);

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_comac_ft_is_synthetic (void	        *abstract_font,
			comac_bool_t    *is_synthetic)
{
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    FT_Error error;

    if (scaled_font->ft_options.synth_flags != 0) {
	*is_synthetic = TRUE;
	return status;
    }

    *is_synthetic = FALSE;
    face = _comac_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    if (face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS) {
	FT_MM_Var *mm_var = NULL;
	FT_Fixed *coords = NULL;
	int num_axis;

	/* If this is an MM or variable font we can't assume the current outlines
	 * are the same as the font tables */
	*is_synthetic = TRUE;

	error = FT_Get_MM_Var (face, &mm_var);
	if (error) {
	    status = _comac_error (_ft_to_comac_error (error));
	    goto cleanup;
	}

	num_axis = mm_var->num_axis;
	coords = _comac_malloc_ab (num_axis, sizeof(FT_Fixed));
	if (!coords) {
	    status = _comac_error (COMAC_STATUS_NO_MEMORY);
	    goto cleanup;
	}

#if FREETYPE_MAJOR > 2 || ( FREETYPE_MAJOR == 2 &&  FREETYPE_MINOR >= 8)
	/* If FT_Get_Var_Blend_Coordinates() is available, we can check if the
	 * current design coordinates are the default coordinates. In this case
	 * the current outlines match the font tables.
	 */
	{
	    int i;

	    FT_Get_Var_Blend_Coordinates (face, num_axis, coords);
	    *is_synthetic = FALSE;
	    for (i = 0; i < num_axis; i++) {
		if (coords[i]) {
		    *is_synthetic = TRUE;
		    break;
		}
	    }
	}
#endif

      cleanup:
	free (coords);
#if HAVE_FT_DONE_MM_VAR
	FT_Done_MM_Var (face->glyph->library, mm_var);
#else
	free (mm_var);
#endif
    }

    _comac_ft_unscaled_font_unlock_face (unscaled);

    return status;
}

static comac_int_status_t
_comac_index_to_glyph_name (void	         *abstract_font,
			    char                **glyph_names,
			    int                   num_glyph_names,
			    unsigned long         glyph_index,
			    unsigned long        *glyph_array_index)
{
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    char buffer[256]; /* PLRM specifies max name length of 127 */
    FT_Error error;
    int i;

    face = _comac_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    error = FT_Get_Glyph_Name (face, glyph_index, buffer, sizeof buffer);

    _comac_ft_unscaled_font_unlock_face (unscaled);

    if (error != FT_Err_Ok) {
	/* propagate fatal errors from FreeType */
	if (error == FT_Err_Out_Of_Memory)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	return COMAC_INT_STATUS_UNSUPPORTED;
    }

    /* FT first numbers the glyphs in the order they are read from the
     * Type 1 font. Then if .notdef is not the first glyph, the first
     * glyph is swapped with .notdef to ensure that .notdef is at
     * glyph index 0.
     *
     * As all but two glyphs in glyph_names already have the same
     * index as the FT glyph index, we first check if
     * glyph_names[glyph_index] is the name we are looking for. If not
     * we fall back to searching the entire array.
     */

    if ((long)glyph_index < num_glyph_names &&
	strcmp (glyph_names[glyph_index], buffer) == 0)
    {
	*glyph_array_index = glyph_index;

	return COMAC_STATUS_SUCCESS;
    }

    for (i = 0; i < num_glyph_names; i++) {
	if (strcmp (glyph_names[i], buffer) == 0) {
	    *glyph_array_index = i;

	    return COMAC_STATUS_SUCCESS;
	}
    }

    return COMAC_INT_STATUS_UNSUPPORTED;
}

static comac_bool_t
_ft_is_type1 (FT_Face face)
{
#if HAVE_FT_GET_X11_FONT_FORMAT
    const char *font_format = FT_Get_X11_Font_Format (face);
    if (font_format &&
	(strcmp (font_format, "Type 1") == 0 ||
	 strcmp (font_format, "CFF") == 0))
    {
	return TRUE;
    }
#endif

    return FALSE;
}

static comac_int_status_t
_comac_ft_load_type1_data (void	            *abstract_font,
			   long              offset,
			   unsigned char    *buffer,
			   unsigned long    *length)
{
    comac_ft_scaled_font_t *scaled_font = abstract_font;
    comac_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    comac_status_t status = COMAC_STATUS_SUCCESS;
    unsigned long available_length;
    unsigned long ret;

    assert (length != NULL);

    if (_comac_ft_scaled_font_is_vertical (&scaled_font->base))
        return COMAC_INT_STATUS_UNSUPPORTED;

    face = _comac_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

#if HAVE_FT_LOAD_SFNT_TABLE
    if (FT_IS_SFNT (face)) {
	status = COMAC_INT_STATUS_UNSUPPORTED;
	goto unlock;
    }
#endif

    if (! _ft_is_type1 (face)) {
        status = COMAC_INT_STATUS_UNSUPPORTED;
	goto unlock;
    }

    available_length = MAX (face->stream->size - offset, 0);
    if (!buffer) {
	*length = available_length;
    } else {
	if (*length > available_length) {
	    status = COMAC_INT_STATUS_UNSUPPORTED;
	} else if (face->stream->read != NULL) {
	    /* Note that read() may be implemented as a macro, thanks POSIX!, so we
	     * need to wrap the following usage in parentheses in order to
	     * disambiguate it for the pre-processor - using the verbose function
	     * pointer dereference for clarity.
	     */
	    ret = (* face->stream->read) (face->stream,
					  offset,
					  buffer,
					  *length);
	    if (ret != *length)
		status = _comac_error (COMAC_STATUS_READ_ERROR);
	} else {
	    memcpy (buffer, face->stream->base + offset, *length);
	}
    }

  unlock:
    _comac_ft_unscaled_font_unlock_face (unscaled);

    return status;
}

static comac_bool_t
_comac_ft_has_color_glyphs (void *scaled)
{
    comac_ft_unscaled_font_t *unscaled = ((comac_ft_scaled_font_t *)scaled)->unscaled;

    if (!unscaled->have_color_set) {
	FT_Face face;
	face = _comac_ft_unscaled_font_lock_face (unscaled);
	if (unlikely (face == NULL))
	    return FALSE;
	_comac_ft_unscaled_font_unlock_face (unscaled);
    }

    return unscaled->have_color;
}

static const comac_scaled_font_backend_t _comac_ft_scaled_font_backend = {
    COMAC_FONT_TYPE_FT,
    _comac_ft_scaled_font_fini,
    _comac_ft_scaled_glyph_init,
    NULL,			/* text_to_glyphs */
    _comac_ft_ucs4_to_index,
    _comac_ft_load_truetype_table,
    _comac_ft_index_to_ucs4,
    _comac_ft_is_synthetic,
    _comac_index_to_glyph_name,
    _comac_ft_load_type1_data,
    _comac_ft_has_color_glyphs
};

/* #comac_ft_font_face_t */

#if COMAC_HAS_FC_FONT
static comac_font_face_t *
_comac_ft_font_face_create_for_pattern (FcPattern *pattern);

static comac_status_t
_comac_ft_font_face_create_for_toy (comac_toy_font_face_t *toy_face,
				    comac_font_face_t **font_face_out)
{
    comac_font_face_t *font_face = (comac_font_face_t *) &_comac_font_face_nil;
    FcPattern *pattern;
    int fcslant;
    int fcweight;

    pattern = FcPatternCreate ();
    if (!pattern) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return font_face->status;
    }

    if (!FcPatternAddString (pattern,
		             FC_FAMILY, (unsigned char *) toy_face->family))
    {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	goto FREE_PATTERN;
    }

    switch (toy_face->slant)
    {
    case COMAC_FONT_SLANT_ITALIC:
        fcslant = FC_SLANT_ITALIC;
        break;
    case COMAC_FONT_SLANT_OBLIQUE:
	fcslant = FC_SLANT_OBLIQUE;
        break;
    case COMAC_FONT_SLANT_NORMAL:
    default:
        fcslant = FC_SLANT_ROMAN;
        break;
    }

    if (!FcPatternAddInteger (pattern, FC_SLANT, fcslant)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	goto FREE_PATTERN;
    }

    switch (toy_face->weight)
    {
    case COMAC_FONT_WEIGHT_BOLD:
        fcweight = FC_WEIGHT_BOLD;
        break;
    case COMAC_FONT_WEIGHT_NORMAL:
    default:
        fcweight = FC_WEIGHT_MEDIUM;
        break;
    }

    if (!FcPatternAddInteger (pattern, FC_WEIGHT, fcweight)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	goto FREE_PATTERN;
    }

    font_face = _comac_ft_font_face_create_for_pattern (pattern);

 FREE_PATTERN:
    FcPatternDestroy (pattern);

    *font_face_out = font_face;
    return font_face->status;
}
#endif

static comac_bool_t
_comac_ft_font_face_destroy (void *abstract_face)
{
    comac_ft_font_face_t *font_face = abstract_face;

    /* When destroying a face created by comac_ft_font_face_create_for_ft_face,
     * we have a special "zombie" state for the face when the unscaled font
     * is still alive but there are no other references to a font face with
     * the same FT_Face.
     *
     * We go from:
     *
     *   font_face ------> unscaled
     *        <-....weak....../
     *
     * To:
     *
     *    font_face <------- unscaled
     */

    if (font_face->unscaled &&
	font_face->unscaled->from_face &&
	font_face->next == NULL &&
	font_face->unscaled->faces == font_face &&
	COMAC_REFERENCE_COUNT_GET_VALUE (&font_face->unscaled->base.ref_count) > 1)
    {
	_comac_unscaled_font_destroy (&font_face->unscaled->base);
	font_face->unscaled = NULL;

	return FALSE;
    }

    if (font_face->unscaled) {
	comac_ft_font_face_t *tmp_face = NULL;
	comac_ft_font_face_t *last_face = NULL;

	/* Remove face from linked list */
	for (tmp_face = font_face->unscaled->faces;
	     tmp_face;
	     tmp_face = tmp_face->next)
	{
	    if (tmp_face == font_face) {
		if (last_face)
		    last_face->next = tmp_face->next;
		else
		    font_face->unscaled->faces = tmp_face->next;
	    }

	    last_face = tmp_face;
	}

	_comac_unscaled_font_destroy (&font_face->unscaled->base);
	font_face->unscaled = NULL;
    }

    _comac_ft_options_fini (&font_face->ft_options);

#if COMAC_HAS_FC_FONT
    if (font_face->pattern) {
	FcPatternDestroy (font_face->pattern);
	comac_font_face_destroy (font_face->resolved_font_face);
    }
#endif

    return TRUE;
}

static comac_font_face_t *
_comac_ft_font_face_get_implementation (void                     *abstract_face,
					const comac_matrix_t       *font_matrix,
					const comac_matrix_t       *ctm,
					const comac_font_options_t *options)
{
    /* The handling of font options is different depending on how the
     * font face was created. When the user creates a font face with
     * comac_ft_font_face_create_for_ft_face(), then the load flags
     * passed in augment the load flags for the options.  But for
     * comac_ft_font_face_create_for_pattern(), the load flags are
     * derived from a pattern where the user has called
     * comac_ft_font_options_substitute(), so *just* use those load
     * flags and ignore the options.
     */

#if COMAC_HAS_FC_FONT
    comac_ft_font_face_t      *font_face = abstract_face;

    /* If we have an unresolved pattern, resolve it and create
     * unscaled font.  Otherwise, use the ones stored in font_face.
     */
    if (font_face->pattern) {
	comac_font_face_t *resolved;

	/* Cache the resolved font whilst the FcConfig remains consistent. */
	resolved = font_face->resolved_font_face;
	if (resolved != NULL) {
	    if (! FcInitBringUptoDate ()) {
		_comac_error_throw (COMAC_STATUS_NO_MEMORY);
		return (comac_font_face_t *) &_comac_font_face_nil;
	    }

	    if (font_face->resolved_config == FcConfigGetCurrent ())
		return comac_font_face_reference (resolved);

	    comac_font_face_destroy (resolved);
	    font_face->resolved_font_face = NULL;
	}

	resolved = _comac_ft_resolve_pattern (font_face->pattern,
					      font_matrix,
					      ctm,
					      options);
	if (unlikely (resolved->status))
	    return resolved;

	font_face->resolved_font_face = comac_font_face_reference (resolved);
	font_face->resolved_config = FcConfigGetCurrent ();

	return resolved;
    }
#endif

    return abstract_face;
}

const comac_font_face_backend_t _comac_ft_font_face_backend = {
    COMAC_FONT_TYPE_FT,
#if COMAC_HAS_FC_FONT
    _comac_ft_font_face_create_for_toy,
#else
    NULL,
#endif
    _comac_ft_font_face_destroy,
    _comac_ft_font_face_scaled_font_create,
    _comac_ft_font_face_get_implementation
};

#if COMAC_HAS_FC_FONT
static comac_font_face_t *
_comac_ft_font_face_create_for_pattern (FcPattern *pattern)
{
    comac_ft_font_face_t *font_face;

    font_face = _comac_malloc (sizeof (comac_ft_font_face_t));
    if (unlikely (font_face == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_font_face_t *) &_comac_font_face_nil;
    }

    font_face->unscaled = NULL;

    _get_pattern_ft_options (pattern, &font_face->ft_options);

    font_face->next = NULL;

    font_face->pattern = FcPatternDuplicate (pattern);
    if (unlikely (font_face->pattern == NULL)) {
	free (font_face);
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_font_face_t *) &_comac_font_face_nil;
    }

    font_face->resolved_font_face = NULL;
    font_face->resolved_config = NULL;

    _comac_font_face_init (&font_face->base, &_comac_ft_font_face_backend);

    return &font_face->base;
}
#endif

static comac_font_face_t *
_comac_ft_font_face_create (comac_ft_unscaled_font_t *unscaled,
			    comac_ft_options_t	     *ft_options)
{
    comac_ft_font_face_t *font_face, **prev_font_face;

    /* Looked for an existing matching font face */
    for (font_face = unscaled->faces, prev_font_face = &unscaled->faces;
	 font_face;
	 prev_font_face = &font_face->next, font_face = font_face->next)
    {
	if (font_face->ft_options.load_flags == ft_options->load_flags &&
	    font_face->ft_options.synth_flags == ft_options->synth_flags &&
	    comac_font_options_equal (&font_face->ft_options.base, &ft_options->base))
	{
	    if (font_face->base.status) {
		/* The font_face has been left in an error state, abandon it. */
		*prev_font_face = font_face->next;
		break;
	    }

	    if (font_face->unscaled == NULL) {
		/* Resurrect this "zombie" font_face (from
		 * _comac_ft_font_face_destroy), switching its unscaled_font
		 * from owner to ownee. */
		font_face->unscaled = unscaled;
		_comac_unscaled_font_reference (&unscaled->base);
		return &font_face->base;
	    } else
		return comac_font_face_reference (&font_face->base);
	}
    }

    /* No match found, create a new one */
    font_face = _comac_malloc (sizeof (comac_ft_font_face_t));
    if (unlikely (!font_face)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_font_face_t *)&_comac_font_face_nil;
    }

    font_face->unscaled = unscaled;
    _comac_unscaled_font_reference (&unscaled->base);

    _comac_ft_options_init_copy (&font_face->ft_options, ft_options);

    if (unscaled->faces && unscaled->faces->unscaled == NULL) {
	/* This "zombie" font_face (from _comac_ft_font_face_destroy)
	 * is no longer needed. */
	assert (unscaled->from_face && unscaled->faces->next == NULL);
	comac_font_face_destroy (&unscaled->faces->base);
	unscaled->faces = NULL;
    }

    font_face->next = unscaled->faces;
    unscaled->faces = font_face;

#if COMAC_HAS_FC_FONT
    font_face->pattern = NULL;
#endif

    _comac_font_face_init (&font_face->base, &_comac_ft_font_face_backend);

    return &font_face->base;
}

/* implement the platform-specific interface */

#if COMAC_HAS_FC_FONT
static comac_status_t
_comac_ft_font_options_substitute (const comac_font_options_t *options,
				   FcPattern                  *pattern)
{
    FcValue v;

    if (options->antialias != COMAC_ANTIALIAS_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_ANTIALIAS, 0, &v) == FcResultNoMatch)
	{
	    if (! FcPatternAddBool (pattern,
			            FC_ANTIALIAS,
				    options->antialias != COMAC_ANTIALIAS_NONE))
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	    if (options->antialias != COMAC_ANTIALIAS_SUBPIXEL) {
		FcPatternDel (pattern, FC_RGBA);
		if (! FcPatternAddInteger (pattern, FC_RGBA, FC_RGBA_NONE))
		    return _comac_error (COMAC_STATUS_NO_MEMORY);
	    }
	}
    }

    if (options->antialias != COMAC_ANTIALIAS_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_RGBA, 0, &v) == FcResultNoMatch)
	{
	    int rgba;

	    if (options->antialias == COMAC_ANTIALIAS_SUBPIXEL) {
		switch (options->subpixel_order) {
		case COMAC_SUBPIXEL_ORDER_DEFAULT:
		case COMAC_SUBPIXEL_ORDER_RGB:
		default:
		    rgba = FC_RGBA_RGB;
		    break;
		case COMAC_SUBPIXEL_ORDER_BGR:
		    rgba = FC_RGBA_BGR;
		    break;
		case COMAC_SUBPIXEL_ORDER_VRGB:
		    rgba = FC_RGBA_VRGB;
		    break;
		case COMAC_SUBPIXEL_ORDER_VBGR:
		    rgba = FC_RGBA_VBGR;
		    break;
		}
	    } else {
		rgba = FC_RGBA_NONE;
	    }

	    if (! FcPatternAddInteger (pattern, FC_RGBA, rgba))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	}
    }

    if (options->lcd_filter != COMAC_LCD_FILTER_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_LCD_FILTER, 0, &v) == FcResultNoMatch)
	{
	    int lcd_filter;

	    switch (options->lcd_filter) {
	    case COMAC_LCD_FILTER_NONE:
		lcd_filter = FT_LCD_FILTER_NONE;
		break;
	    case COMAC_LCD_FILTER_INTRA_PIXEL:
		lcd_filter = FT_LCD_FILTER_LEGACY;
		break;
	    case COMAC_LCD_FILTER_FIR3:
		lcd_filter = FT_LCD_FILTER_LIGHT;
		break;
	    default:
	    case COMAC_LCD_FILTER_DEFAULT:
	    case COMAC_LCD_FILTER_FIR5:
		lcd_filter = FT_LCD_FILTER_DEFAULT;
		break;
	    }

	    if (! FcPatternAddInteger (pattern, FC_LCD_FILTER, lcd_filter))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	}
    }

    if (options->hint_style != COMAC_HINT_STYLE_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_HINTING, 0, &v) == FcResultNoMatch)
	{
	    if (! FcPatternAddBool (pattern,
			            FC_HINTING,
				    options->hint_style != COMAC_HINT_STYLE_NONE))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	}

#ifdef FC_HINT_STYLE
	if (FcPatternGet (pattern, FC_HINT_STYLE, 0, &v) == FcResultNoMatch)
	{
	    int hint_style;

	    switch (options->hint_style) {
	    case COMAC_HINT_STYLE_NONE:
		hint_style = FC_HINT_NONE;
		break;
	    case COMAC_HINT_STYLE_SLIGHT:
		hint_style = FC_HINT_SLIGHT;
		break;
	    case COMAC_HINT_STYLE_MEDIUM:
		hint_style = FC_HINT_MEDIUM;
		break;
	    case COMAC_HINT_STYLE_FULL:
	    case COMAC_HINT_STYLE_DEFAULT:
	    default:
		hint_style = FC_HINT_FULL;
		break;
	    }

	    if (! FcPatternAddInteger (pattern, FC_HINT_STYLE, hint_style))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	}
#endif
    }

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_ft_font_options_substitute:
 * @options: a #comac_font_options_t object
 * @pattern: an existing #FcPattern
 *
 * Add options to a #FcPattern based on a #comac_font_options_t font
 * options object. Options that are already in the pattern, are not overridden,
 * so you should call this function after calling FcConfigSubstitute() (the
 * user's settings should override options based on the surface type), but
 * before calling FcDefaultSubstitute().
 *
 * Since: 1.0
 **/
void
comac_ft_font_options_substitute (const comac_font_options_t *options,
				  FcPattern                  *pattern)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return;

    _comac_ft_font_options_substitute (options, pattern);
}

static comac_font_face_t *
_comac_ft_resolve_pattern (FcPattern		      *pattern,
			   const comac_matrix_t       *font_matrix,
			   const comac_matrix_t       *ctm,
			   const comac_font_options_t *font_options)
{
    comac_status_t status;

    comac_matrix_t scale;
    FcPattern *resolved;
    comac_ft_font_transform_t sf;
    FcResult result;
    comac_ft_unscaled_font_t *unscaled;
    comac_ft_options_t ft_options;
    comac_font_face_t *font_face;

    scale = *ctm;
    scale.x0 = scale.y0 = 0;
    comac_matrix_multiply (&scale,
                           font_matrix,
                           &scale);

    status = _compute_transform (&sf, &scale, NULL);
    if (unlikely (status))
	return (comac_font_face_t *)&_comac_font_face_nil;

    pattern = FcPatternDuplicate (pattern);
    if (pattern == NULL)
	return (comac_font_face_t *)&_comac_font_face_nil;

    if (! FcPatternAddDouble (pattern, FC_PIXEL_SIZE, sf.y_scale)) {
	font_face = (comac_font_face_t *)&_comac_font_face_nil;
	goto FREE_PATTERN;
    }

    if (! FcConfigSubstitute (NULL, pattern, FcMatchPattern)) {
	font_face = (comac_font_face_t *)&_comac_font_face_nil;
	goto FREE_PATTERN;
    }

    status = _comac_ft_font_options_substitute (font_options, pattern);
    if (status) {
	font_face = (comac_font_face_t *)&_comac_font_face_nil;
	goto FREE_PATTERN;
    }

    FcDefaultSubstitute (pattern);

    status = _comac_ft_unscaled_font_create_for_pattern (pattern, &unscaled);
    if (unlikely (status)) {
	font_face = (comac_font_face_t *)&_comac_font_face_nil;
	goto FREE_PATTERN;
    }

    if (unscaled == NULL) {
	resolved = FcFontMatch (NULL, pattern, &result);
	if (!resolved) {
	    /* We failed to find any font. Substitute twin so that the user can
	     * see something (and hopefully recognise that the font is missing)
	     * and not just receive a NO_MEMORY error during rendering.
	     */
	    font_face = _comac_font_face_twin_create_fallback ();
	    goto FREE_PATTERN;
	}

	status = _comac_ft_unscaled_font_create_for_pattern (resolved, &unscaled);
	if (unlikely (status || unscaled == NULL)) {
	    font_face = (comac_font_face_t *)&_comac_font_face_nil;
	    goto FREE_RESOLVED;
	}
    } else
	resolved = pattern;

    _get_pattern_ft_options (resolved, &ft_options);
    font_face = _comac_ft_font_face_create (unscaled, &ft_options);
     _comac_ft_options_fini (&ft_options);
    _comac_unscaled_font_destroy (&unscaled->base);

FREE_RESOLVED:
    if (resolved != pattern)
	FcPatternDestroy (resolved);

FREE_PATTERN:
    FcPatternDestroy (pattern);

    return font_face;
}

/**
 * comac_ft_font_face_create_for_pattern:
 * @pattern: A fontconfig pattern.  Comac makes a copy of the pattern
 * if it needs to.  You are free to modify or free @pattern after this call.
 *
 * Creates a new font face for the FreeType font backend based on a
 * fontconfig pattern. This font can then be used with
 * comac_set_font_face() or comac_scaled_font_create(). The
 * #comac_scaled_font_t returned from comac_scaled_font_create() is
 * also for the FreeType backend and can be used with functions such
 * as comac_ft_scaled_font_lock_face().
 *
 * Font rendering options are represented both here and when you
 * call comac_scaled_font_create(). Font options that have a representation
 * in a #FcPattern must be passed in here; to modify #FcPattern
 * appropriately to reflect the options in a #comac_font_options_t, call
 * comac_ft_font_options_substitute().
 *
 * The pattern's FC_FT_FACE element is inspected first and if that is set,
 * that will be the FreeType font face associated with the returned comac
 * font face.  Otherwise the FC_FILE element is checked.  If it's set,
 * that and the value of the FC_INDEX element (defaults to zero) of @pattern
 * are used to load a font face from file.
 *
 * If both steps from the previous paragraph fails, @pattern will be passed
 * to FcConfigSubstitute, FcDefaultSubstitute, and finally FcFontMatch,
 * and the resulting font pattern is used.
 *
 * If the FC_FT_FACE element of @pattern is set, the user is responsible
 * for making sure that the referenced FT_Face remains valid for the life
 * time of the returned #comac_font_face_t.  See
 * comac_ft_font_face_create_for_ft_face() for an example of how to couple
 * the life time of the FT_Face to that of the comac font-face.
 *
 * Return value: a newly created #comac_font_face_t. Free with
 *  comac_font_face_destroy() when you are done using it.
 *
 * Since: 1.0
 **/
comac_font_face_t *
comac_ft_font_face_create_for_pattern (FcPattern *pattern)
{
    comac_ft_unscaled_font_t *unscaled;
    comac_font_face_t *font_face;
    comac_ft_options_t ft_options;
    comac_status_t status;

    status = _comac_ft_unscaled_font_create_for_pattern (pattern, &unscaled);
    if (unlikely (status)) {
      if (status == COMAC_STATUS_FILE_NOT_FOUND)
	return (comac_font_face_t *) &_comac_font_face_nil_file_not_found;
      else
	return (comac_font_face_t *) &_comac_font_face_nil;
    }
    if (unlikely (unscaled == NULL)) {
	/* Store the pattern.  We will resolve it and create unscaled
	 * font when creating scaled fonts */
	return _comac_ft_font_face_create_for_pattern (pattern);
    }

    _get_pattern_ft_options (pattern, &ft_options);
    font_face = _comac_ft_font_face_create (unscaled, &ft_options);
    _comac_ft_options_fini (&ft_options);
    _comac_unscaled_font_destroy (&unscaled->base);

    return font_face;
}
#endif

/**
 * comac_ft_font_face_create_for_ft_face:
 * @face: A FreeType face object, already opened. This must
 *   be kept around until the face's ref_count drops to
 *   zero and it is freed. Since the face may be referenced
 *   internally to Comac, the best way to determine when it
 *   is safe to free the face is to pass a
 *   #comac_destroy_func_t to comac_font_face_set_user_data()
 * @load_flags: flags to pass to FT_Load_Glyph when loading
 *   glyphs from the font. These flags are OR'ed together with
 *   the flags derived from the #comac_font_options_t passed
 *   to comac_scaled_font_create(), so only a few values such
 *   as %FT_LOAD_VERTICAL_LAYOUT, and %FT_LOAD_FORCE_AUTOHINT
 *   are useful. You should not pass any of the flags affecting
 *   the load target, such as %FT_LOAD_TARGET_LIGHT.
 *
 * Creates a new font face for the FreeType font backend from a
 * pre-opened FreeType face. This font can then be used with
 * comac_set_font_face() or comac_scaled_font_create(). The
 * #comac_scaled_font_t returned from comac_scaled_font_create() is
 * also for the FreeType backend and can be used with functions such
 * as comac_ft_scaled_font_lock_face(). Note that Comac may keep a reference
 * to the FT_Face alive in a font-cache and the exact lifetime of the reference
 * depends highly upon the exact usage pattern and is subject to external
 * factors. You must not call FT_Done_Face() before the last reference to the
 * #comac_font_face_t has been dropped.
 *
 * As an example, below is how one might correctly couple the lifetime of
 * the FreeType face object to the #comac_font_face_t.
 *
 * <informalexample><programlisting>
 * static const comac_user_data_key_t key;
 *
 * font_face = comac_ft_font_face_create_for_ft_face (ft_face, 0);
 * status = comac_font_face_set_user_data (font_face, &key,
 *                                ft_face, (comac_destroy_func_t) FT_Done_Face);
 * if (status) {
 *    comac_font_face_destroy (font_face);
 *    FT_Done_Face (ft_face);
 *    return ERROR;
 * }
 * </programlisting></informalexample>
 *
 * Return value: a newly created #comac_font_face_t. Free with
 *  comac_font_face_destroy() when you are done using it.
 *
 * Since: 1.0
 **/
comac_font_face_t *
comac_ft_font_face_create_for_ft_face (FT_Face         face,
				       int             load_flags)
{
    comac_ft_unscaled_font_t *unscaled;
    comac_font_face_t *font_face;
    comac_ft_options_t ft_options;
    comac_status_t status;

    status = _comac_ft_unscaled_font_create_from_face (face, &unscaled);
    if (unlikely (status))
	return (comac_font_face_t *)&_comac_font_face_nil;

    ft_options.load_flags = load_flags;
    ft_options.synth_flags = 0;
    _comac_font_options_init_default (&ft_options.base);

    font_face = _comac_ft_font_face_create (unscaled, &ft_options);
    _comac_unscaled_font_destroy (&unscaled->base);

    return font_face;
}

/**
 * comac_ft_font_face_set_synthesize:
 * @font_face: The #comac_ft_font_face_t object to modify
 * @synth_flags: the set of synthesis options to enable
 *
 * FreeType provides the ability to synthesize different glyphs from a base
 * font, which is useful if you lack those glyphs from a true bold or oblique
 * font. See also #comac_ft_synthesize_t.
 *
 * Since: 1.12
 **/
void
comac_ft_font_face_set_synthesize (comac_font_face_t *font_face,
				   unsigned int synth_flags)
{
    comac_ft_font_face_t *ft;

    if (font_face->backend->type != COMAC_FONT_TYPE_FT)
	return;

    ft = (comac_ft_font_face_t *) font_face;
    ft->ft_options.synth_flags |= synth_flags;
}

/**
 * comac_ft_font_face_unset_synthesize:
 * @font_face: The #comac_ft_font_face_t object to modify
 * @synth_flags: the set of synthesis options to disable
 *
 * See comac_ft_font_face_set_synthesize().
 *
 * Since: 1.12
 **/
void
comac_ft_font_face_unset_synthesize (comac_font_face_t *font_face,
				     unsigned int synth_flags)
{
    comac_ft_font_face_t *ft;

    if (font_face->backend->type != COMAC_FONT_TYPE_FT)
	return;

    ft = (comac_ft_font_face_t *) font_face;
    ft->ft_options.synth_flags &= ~synth_flags;
}

/**
 * comac_ft_font_face_get_synthesize:
 * @font_face: The #comac_ft_font_face_t object to query
 *
 * See #comac_ft_synthesize_t.
 *
 * Returns: the current set of synthesis options.
 *
 * Since: 1.12
 **/
unsigned int
comac_ft_font_face_get_synthesize (comac_font_face_t *font_face)
{
    comac_ft_font_face_t *ft;

    if (font_face->backend->type != COMAC_FONT_TYPE_FT)
	return 0;

    ft = (comac_ft_font_face_t *) font_face;
    return ft->ft_options.synth_flags;
}

/**
 * comac_ft_scaled_font_lock_face:
 * @scaled_font: A #comac_scaled_font_t from the FreeType font backend. Such an
 *   object can be created by calling comac_scaled_font_create() on a
 *   FreeType backend font face (see comac_ft_font_face_create_for_pattern(),
 *   comac_ft_font_face_create_for_ft_face()).
 *
 * comac_ft_scaled_font_lock_face() gets the #FT_Face object from a FreeType
 * backend font and scales it appropriately for the font and applies OpenType
 * font variations if applicable. You must
 * release the face with comac_ft_scaled_font_unlock_face()
 * when you are done using it.  Since the #FT_Face object can be
 * shared between multiple #comac_scaled_font_t objects, you must not
 * lock any other font objects until you unlock this one. A count is
 * kept of the number of times comac_ft_scaled_font_lock_face() is
 * called. comac_ft_scaled_font_unlock_face() must be called the same number
 * of times.
 *
 * You must be careful when using this function in a library or in a
 * threaded application, because freetype's design makes it unsafe to
 * call freetype functions simultaneously from multiple threads, (even
 * if using distinct FT_Face objects). Because of this, application
 * code that acquires an FT_Face object with this call must add its
 * own locking to protect any use of that object, (and which also must
 * protect any other calls into comac as almost any comac function
 * might result in a call into the freetype library).
 *
 * Return value: The #FT_Face object for @font, scaled appropriately,
 * or %NULL if @scaled_font is in an error state (see
 * comac_scaled_font_status()) or there is insufficient memory.
 *
 * Since: 1.0
 **/
FT_Face
comac_ft_scaled_font_lock_face (comac_scaled_font_t *abstract_font)
{
    comac_ft_scaled_font_t *scaled_font = (comac_ft_scaled_font_t *) abstract_font;
    FT_Face face;
    comac_status_t status;

    if (! _comac_scaled_font_is_ft (abstract_font)) {
	_comac_error_throw (COMAC_STATUS_FONT_TYPE_MISMATCH);
	return NULL;
    }

    if (scaled_font->base.status)
	return NULL;

    face = _comac_ft_unscaled_font_lock_face (scaled_font->unscaled);
    if (unlikely (face == NULL)) {
	status = _comac_scaled_font_set_error (&scaled_font->base, COMAC_STATUS_NO_MEMORY);
	return NULL;
    }

    status = _comac_ft_unscaled_font_set_scale (scaled_font->unscaled,
				                &scaled_font->base.scale);
    if (unlikely (status)) {
	_comac_ft_unscaled_font_unlock_face (scaled_font->unscaled);
	status = _comac_scaled_font_set_error (&scaled_font->base, status);
	return NULL;
    }

    comac_ft_apply_variations (face, scaled_font);

    /* Note: We deliberately release the unscaled font's mutex here,
     * so that we are not holding a lock across two separate calls to
     * comac function, (which would give the application some
     * opportunity for creating deadlock. This is obviously unsafe,
     * but as documented, the user must add manual locking when using
     * this function. */
     COMAC_MUTEX_UNLOCK (scaled_font->unscaled->mutex);

    return face;
}

/**
 * comac_ft_scaled_font_unlock_face:
 * @scaled_font: A #comac_scaled_font_t from the FreeType font backend. Such an
 *   object can be created by calling comac_scaled_font_create() on a
 *   FreeType backend font face (see comac_ft_font_face_create_for_pattern(),
 *   comac_ft_font_face_create_for_ft_face()).
 *
 * Releases a face obtained with comac_ft_scaled_font_lock_face().
 *
 * Since: 1.0
 **/
void
comac_ft_scaled_font_unlock_face (comac_scaled_font_t *abstract_font)
{
    comac_ft_scaled_font_t *scaled_font = (comac_ft_scaled_font_t *) abstract_font;

    if (! _comac_scaled_font_is_ft (abstract_font)) {
	_comac_error_throw (COMAC_STATUS_FONT_TYPE_MISMATCH);
	return;
    }

    if (scaled_font->base.status)
	return;

    /* Note: We released the unscaled font's mutex at the end of
     * comac_ft_scaled_font_lock_face, so we have to acquire it again
     * as _comac_ft_unscaled_font_unlock_face expects it to be held
     * when we call into it. */
    COMAC_MUTEX_LOCK (scaled_font->unscaled->mutex);

    _comac_ft_unscaled_font_unlock_face (scaled_font->unscaled);
}

static comac_bool_t
_comac_ft_scaled_font_is_vertical (comac_scaled_font_t *scaled_font)
{
    comac_ft_scaled_font_t *ft_scaled_font;

    if (!_comac_scaled_font_is_ft (scaled_font))
	return FALSE;

    ft_scaled_font = (comac_ft_scaled_font_t *) scaled_font;
    if (ft_scaled_font->ft_options.load_flags & FT_LOAD_VERTICAL_LAYOUT)
	return TRUE;
    return FALSE;
}

unsigned int
_comac_ft_scaled_font_get_load_flags (comac_scaled_font_t *scaled_font)
{
    comac_ft_scaled_font_t *ft_scaled_font;

    if (! _comac_scaled_font_is_ft (scaled_font))
	return 0;

    ft_scaled_font = (comac_ft_scaled_font_t *) scaled_font;
    return ft_scaled_font->ft_options.load_flags;
}

void
_comac_ft_font_reset_static_data (void)
{
    _comac_ft_unscaled_font_map_destroy ();
}
