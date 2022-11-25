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

#ifndef COMAC_SCALED_FONT_PRIVATE_H
#define COMAC_SCALED_FONT_PRIVATE_H

#include "comac.h"

#include "comac-types-private.h"
#include "comac-list-private.h"
#include "comac-mutex-type-private.h"
#include "comac-reference-count-private.h"

COMAC_BEGIN_DECLS

typedef struct _comac_scaled_glyph_page comac_scaled_glyph_page_t;

struct _comac_scaled_font {
    /* For most comac objects, the rule for multiple threads is that
     * the user is responsible for any locking if the same object is
     * manipulated from multiple threads simultaneously.
     *
     * However, with the caching that comac does for scaled fonts, a
     * user can easily end up with the same comac_scaled_font object
     * being manipulated from multiple threads without the user ever
     * being aware of this, (and in fact, unable to control it).
     *
     * So, as a special exception, the comac implementation takes care
     * of all locking needed for comac_scaled_font_t. Most of what is
     * in the scaled font is immutable, (which is what allows for the
     * sharing in the first place). The things that are modified and
     * the locks protecting them are as follows:
     *
     * 1. The reference count (scaled_font->ref_count)
     *
     *    Modifications to the reference count are protected by the
     *    _comac_scaled_font_map_mutex. This is because the reference
     *    count of a scaled font is intimately related with the font
     *    map itself, (and the magic holdovers array).
     *
     * 2. The cache of glyphs (scaled_font->glyphs)
     * 3. The backend private data (scaled_font->surface_backend,
     *				    scaled_font->surface_private)
     *
     *    Modifications to these fields are protected with locks on
     *    scaled_font->mutex in the generic scaled_font code.
     */

    comac_hash_entry_t hash_entry;

    /* useful bits for _comac_scaled_font_nil */
    comac_status_t status;
    comac_reference_count_t ref_count;
    comac_user_data_array_t user_data;

    comac_font_face_t *original_font_face; /* may be NULL */

    /* hash key members */
    comac_font_face_t *font_face; /* may be NULL */
    comac_matrix_t font_matrix;	  /* font space => user space */
    comac_matrix_t ctm;		  /* user space => device space */
    comac_font_options_t options;

    unsigned int placeholder : 1; /*  protected by fontmap mutex */
    unsigned int holdover : 1;
    unsigned int finished : 1;

    /* "live" scaled_font members */
    comac_matrix_t scale;	     /* font space => device space */
    comac_matrix_t scale_inverse;    /* device space => font space */
    double max_scale;		     /* maximum x/y expansion of scale */
    comac_font_extents_t extents;    /* user space */
    comac_font_extents_t fs_extents; /* font space */

    /* The mutex protects modification to all subsequent fields. */
    comac_recursive_mutex_t mutex;

    comac_hash_table_t *glyphs;
    comac_list_t glyph_pages;
    comac_bool_t cache_frozen;
    comac_bool_t global_cache_frozen;
    comac_array_t recording_surfaces_to_free; /* array of comac_surface_t* */

    comac_list_t dev_privates;

    /* font backend managing this scaled font */
    const comac_scaled_font_backend_t *backend;
    comac_list_t link;
};

struct _comac_scaled_font_private {
    comac_list_t link;
    const void *key;
    void (*destroy) (comac_scaled_font_private_t *, comac_scaled_font_t *);
};

struct _comac_scaled_glyph {
    comac_hash_entry_t hash_entry;

    comac_text_extents_t metrics;    /* user-space metrics */
    comac_text_extents_t fs_metrics; /* font-space metrics */
    comac_box_t bbox;		     /* device-space bounds */
    int16_t x_advance;		     /* device-space rounded X advance */
    int16_t y_advance;		     /* device-space rounded Y advance */

    unsigned int has_info;
    comac_image_surface_t *surface;	  /* device-space image */
    comac_path_fixed_t *path;		  /* device-space outline */
    comac_surface_t *recording_surface;	  /* device-space recording-surface */
    comac_image_surface_t *color_surface; /* device-space color image */

    const void *dev_private_key;
    void *dev_private;
    comac_list_t dev_privates;

    comac_color_t foreground_color; /* only used for color glyphs */
    /* TRUE if the color_surface required the foreground_color to render. */
    unsigned uses_foreground_color : 1;

    /* TRUE if color_glyph specifies if glyph is color or non color, FALSE if glyph color type unknown. */
    unsigned color_glyph_set : 1;

    unsigned color_glyph : 1;
};

struct _comac_scaled_glyph_private {
    comac_list_t link;
    const void *key;
    void (*destroy) (comac_scaled_glyph_private_t *,
		     comac_scaled_glyph_t *,
		     comac_scaled_font_t *);
};

comac_private comac_scaled_font_private_t *
_comac_scaled_font_find_private (comac_scaled_font_t *scaled_font,
				 const void *key);

comac_private void
_comac_scaled_font_attach_private (
    comac_scaled_font_t *scaled_font,
    comac_scaled_font_private_t *priv,
    const void *key,
    void (*destroy) (comac_scaled_font_private_t *, comac_scaled_font_t *));

comac_private comac_scaled_glyph_private_t *
_comac_scaled_glyph_find_private (comac_scaled_glyph_t *scaled_glyph,
				  const void *key);

comac_private void
_comac_scaled_glyph_attach_private (
    comac_scaled_glyph_t *scaled_glyph,
    comac_scaled_glyph_private_t *priv,
    const void *key,
    void (*destroy) (comac_scaled_glyph_private_t *,
		     comac_scaled_glyph_t *,
		     comac_scaled_font_t *));
comac_private comac_bool_t
_comac_scaled_font_has_color_glyphs (comac_scaled_font_t *scaled_font);

COMAC_END_DECLS

#endif /* COMAC_SCALED_FONT_PRIVATE_H */
