/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2006, 2008 Red Hat, Inc
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
 *      Kristian Høgsberg <krh@redhat.com>
 *      Behdad Esfahbod <behdad@behdad.org>
 */

#include "comacint.h"
#include "comac-user-font-private.h"
#include "comac-recording-surface-private.h"
#include "comac-analysis-surface-private.h"
#include "comac-error-private.h"

/**
 * SECTION:comac-user-fonts
 * @Title:User Fonts
 * @Short_Description: Font support with font data provided by the user
 *
 * The user-font feature allows the comac user to provide drawings for glyphs
 * in a font.  This is most useful in implementing fonts in non-standard
 * formats, like SVG fonts and Flash fonts, but can also be used by games and
 * other application to draw "funky" fonts.
 **/

/**
 * COMAC_HAS_USER_FONT:
 *
 * Defined if the user font backend is available.
 * This macro can be used to conditionally compile backend-specific code.
 * The user font backend is always built in versions of comac that support
 * this feature (1.8 and later).
 *
 * Since: 1.8
 **/

typedef struct _comac_user_scaled_font_methods {
    comac_user_scaled_font_init_func_t			init;
    comac_user_scaled_font_render_glyph_func_t		render_color_glyph;
    comac_user_scaled_font_render_glyph_func_t		render_glyph;
    comac_user_scaled_font_unicode_to_glyph_func_t	unicode_to_glyph;
    comac_user_scaled_font_text_to_glyphs_func_t	text_to_glyphs;
} comac_user_scaled_font_methods_t;

typedef struct _comac_user_font_face {
    comac_font_face_t	             base;

    /* Set to true after first scaled font is created.  At that point,
     * the scaled_font_methods cannot change anymore. */
    comac_bool_t		     immutable;
    comac_bool_t                     has_color;
    comac_user_scaled_font_methods_t scaled_font_methods;
} comac_user_font_face_t;

typedef struct _comac_user_scaled_font {
    comac_scaled_font_t  base;

    comac_text_extents_t default_glyph_extents;

    /* space to compute extents in, and factors to convert back to user space */
    comac_matrix_t extent_scale;
    double extent_x_scale;
    double extent_y_scale;

    /* multiplier for metrics hinting */
    double snap_x_scale;
    double snap_y_scale;

} comac_user_scaled_font_t;

/* #comac_user_scaled_font_t */

static comac_surface_t *
_comac_user_scaled_font_create_recording_surface (const comac_user_scaled_font_t *scaled_font,
						  comac_bool_t                    color)
{
    comac_content_t content;

    if (color) {
	content = COMAC_CONTENT_COLOR_ALPHA;
    } else {
	content = scaled_font->base.options.antialias == COMAC_ANTIALIAS_SUBPIXEL ?
						         COMAC_CONTENT_COLOR_ALPHA :
						         COMAC_CONTENT_ALPHA;
    }

    return comac_recording_surface_create (content, NULL);
}


static comac_t *
_comac_user_scaled_font_create_recording_context (const comac_user_scaled_font_t *scaled_font,
						  comac_surface_t                *recording_surface,
						  comac_bool_t                    color)
{
    comac_t *cr;

    cr = comac_create (recording_surface);

    if (!_comac_matrix_is_scale_0 (&scaled_font->base.scale)) {
        comac_matrix_t scale;
	scale = scaled_font->base.scale;
	scale.x0 = scale.y0 = 0.;
	comac_set_matrix (cr, &scale);
    }

    comac_set_font_size (cr, 1.0);
    comac_set_font_options (cr, &scaled_font->base.options);
    if (!color)
	comac_set_source_rgb (cr, 1., 1., 1.);

    return cr;
}

static comac_int_status_t
_comac_user_scaled_glyph_init_record_glyph (comac_user_scaled_font_t *scaled_font,
					    comac_scaled_glyph_t     *scaled_glyph)
{
    comac_user_font_face_t *face =
	(comac_user_font_face_t *) scaled_font->base.font_face;
    comac_text_extents_t extents = scaled_font->default_glyph_extents;
    comac_surface_t *recording_surface = NULL;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_t *cr;

    if (!face->scaled_font_methods.render_color_glyph && !face->scaled_font_methods.render_glyph)
	return COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED;

    /* special case for 0 rank matrix (as in _comac_scaled_font_init): empty surface */
    if (_comac_matrix_is_scale_0 (&scaled_font->base.scale)) {
	recording_surface = _comac_user_scaled_font_create_recording_surface (scaled_font, FALSE);
	_comac_scaled_glyph_set_recording_surface (scaled_glyph,
						   &scaled_font->base,
						   recording_surface);
    } else {
	status = COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED;

	if (face->scaled_font_methods.render_color_glyph) {
	    comac_pattern_t *pattern;

	    recording_surface = _comac_user_scaled_font_create_recording_surface (scaled_font, TRUE);

	    cr = _comac_user_scaled_font_create_recording_context (scaled_font, recording_surface, TRUE);
	    pattern = comac_pattern_create_rgb (0, 0, 0);
	    pattern->is_userfont_foreground = TRUE;
	    comac_set_source (cr, pattern);
	    comac_pattern_destroy (pattern);
	    status = face->scaled_font_methods.render_color_glyph ((comac_scaled_font_t *)scaled_font,
								   _comac_scaled_glyph_index(scaled_glyph),
								   cr, &extents);
	    if (status == COMAC_INT_STATUS_SUCCESS) {
		status = comac_status (cr);
		scaled_glyph->color_glyph = TRUE;
		scaled_glyph->color_glyph_set = TRUE;
	    }
	    comac_destroy (cr);
	}

	if (status == (comac_int_status_t)COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED &&
	    face->scaled_font_methods.render_glyph) {
	    if (recording_surface)
		comac_surface_destroy (recording_surface);
	    recording_surface = _comac_user_scaled_font_create_recording_surface (scaled_font, FALSE);
	    recording_surface->device_transform.x0 = .25 * _comac_scaled_glyph_xphase (scaled_glyph);
	    recording_surface->device_transform.y0 = .25 * _comac_scaled_glyph_yphase (scaled_glyph);

	    cr = _comac_user_scaled_font_create_recording_context (scaled_font, recording_surface, FALSE);

	    status = face->scaled_font_methods.render_glyph ((comac_scaled_font_t *)scaled_font,
							     _comac_scaled_glyph_index(scaled_glyph),
							     cr, &extents);
	    if (status == COMAC_INT_STATUS_SUCCESS) {
		status = comac_status (cr);
		scaled_glyph->color_glyph = FALSE;
		scaled_glyph->color_glyph_set = TRUE;
	    }

	    comac_destroy (cr);
	}

	if (status != COMAC_INT_STATUS_SUCCESS) {
	    if (recording_surface)
		comac_surface_destroy (recording_surface);
	    return status;
	}

	_comac_scaled_glyph_set_recording_surface (scaled_glyph,
						   &scaled_font->base,
						   recording_surface);
    }

    /* set metrics */

    if (extents.width == 0.) {
	comac_box_t bbox;
	double x1, y1, x2, y2;
	double x_scale, y_scale;

	/* Compute extents.x/y/width/height from recording_surface,
	 * in font space.
	 */
	status = _comac_recording_surface_get_bbox ((comac_recording_surface_t *) recording_surface,
						    &bbox,
						    &scaled_font->extent_scale);
	if (unlikely (status))
	    return status;

	_comac_box_to_doubles (&bbox, &x1, &y1, &x2, &y2);

	x_scale = scaled_font->extent_x_scale;
	y_scale = scaled_font->extent_y_scale;
	extents.x_bearing = x1 * x_scale;
	extents.y_bearing = y1 * y_scale;
	extents.width     = (x2 - x1) * x_scale;
	extents.height    = (y2 - y1) * y_scale;
    }

    if (scaled_font->base.options.hint_metrics != COMAC_HINT_METRICS_OFF) {
	extents.x_advance = _comac_lround (extents.x_advance / scaled_font->snap_x_scale) * scaled_font->snap_x_scale;
	extents.y_advance = _comac_lround (extents.y_advance / scaled_font->snap_y_scale) * scaled_font->snap_y_scale;
    }

    _comac_scaled_glyph_set_metrics (scaled_glyph,
				     &scaled_font->base,
				     &extents);

    return status;
}

static comac_int_status_t
_comac_user_scaled_glyph_init_surface (comac_user_scaled_font_t  *scaled_font,
				       comac_scaled_glyph_t	 *scaled_glyph,
				       comac_scaled_glyph_info_t  info,
				       const comac_color_t       *foreground_color)
{
    comac_surface_t *surface;
    comac_format_t format;
    int width, height;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    /* TODO
     * extend the glyph cache to support argb glyphs.
     * need to figure out the semantics and interaction with subpixel
     * rendering first.
     */

    /* Only one info type at a time handled in this function */
    assert (info == COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE || info == COMAC_SCALED_GLYPH_INFO_SURFACE);

    width = _comac_fixed_integer_ceil (scaled_glyph->bbox.p2.x) -
	_comac_fixed_integer_floor (scaled_glyph->bbox.p1.x);
    height = _comac_fixed_integer_ceil (scaled_glyph->bbox.p2.y) -
	_comac_fixed_integer_floor (scaled_glyph->bbox.p1.y);

    if (info == COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) {
	format = COMAC_FORMAT_ARGB32;
    } else {
	switch (scaled_font->base.options.antialias) {
	    default:
	    case COMAC_ANTIALIAS_DEFAULT:
	    case COMAC_ANTIALIAS_FAST:
	    case COMAC_ANTIALIAS_GOOD:
	    case COMAC_ANTIALIAS_GRAY:
		format = COMAC_FORMAT_A8;
		break;
	    case COMAC_ANTIALIAS_NONE:
		format = COMAC_FORMAT_A1;
		break;
	    case COMAC_ANTIALIAS_BEST:
	    case COMAC_ANTIALIAS_SUBPIXEL:
		format = COMAC_FORMAT_ARGB32;
		break;
	}
    }
    surface = comac_image_surface_create (format, width, height);

    comac_surface_set_device_offset (surface,
				     - _comac_fixed_integer_floor (scaled_glyph->bbox.p1.x),
				     - _comac_fixed_integer_floor (scaled_glyph->bbox.p1.y));

    if (info == COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) {
	status = _comac_recording_surface_replay_with_foreground_color (scaled_glyph->recording_surface,
									surface,
									foreground_color);
    } else {
	status = _comac_recording_surface_replay (scaled_glyph->recording_surface, surface);
    }

    if (unlikely (status)) {
	comac_surface_destroy(surface);
	return status;
    }

    if (info == COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) {
	_comac_scaled_glyph_set_color_surface (scaled_glyph,
					       &scaled_font->base,
					       (comac_image_surface_t *)surface,
					       TRUE);
	surface = NULL;
    } else {
	_comac_scaled_glyph_set_surface (scaled_glyph,
					 &scaled_font->base,
					 (comac_image_surface_t *) surface);
	surface = NULL;
    }

    if (surface)
	comac_surface_destroy (surface);

    return status;
}

static comac_int_status_t
_comac_user_scaled_glyph_init (void			 *abstract_font,
			       comac_scaled_glyph_t	 *scaled_glyph,
			       comac_scaled_glyph_info_t  info,
			       const comac_color_t       *foreground_color)
{
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_user_scaled_font_t *scaled_font = abstract_font;

    if (!scaled_glyph->recording_surface) {
	status = _comac_user_scaled_glyph_init_record_glyph (scaled_font, scaled_glyph);
	if (status)
	    return status;
    }

    if (info & COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE) {
	if (!scaled_glyph->color_glyph )
	    return COMAC_INT_STATUS_UNSUPPORTED;

	status = _comac_user_scaled_glyph_init_surface (scaled_font,
							scaled_glyph,
							COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE,
							foreground_color);
	if (status)
	    return status;
    }

    if (info & COMAC_SCALED_GLYPH_INFO_SURFACE) {
	status = _comac_user_scaled_glyph_init_surface (scaled_font,
							scaled_glyph,
							COMAC_SCALED_GLYPH_INFO_SURFACE,
							NULL);
	if (status)
	    return status;
    }

    if (info & COMAC_SCALED_GLYPH_INFO_PATH) {
	comac_path_fixed_t *path = _comac_path_fixed_create ();
	if (!path)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	status = _comac_recording_surface_get_path (scaled_glyph->recording_surface, path);
	if (unlikely (status)) {
	    _comac_path_fixed_destroy (path);
	    return status;
	}

	_comac_scaled_glyph_set_path (scaled_glyph,
				      &scaled_font->base,
				      path);
    }

    return status;
}

static unsigned long
_comac_user_ucs4_to_index (void	    *abstract_font,
			   uint32_t  ucs4)
{
    comac_user_scaled_font_t *scaled_font = abstract_font;
    comac_user_font_face_t *face =
	(comac_user_font_face_t *) scaled_font->base.font_face;
    unsigned long glyph = 0;

    if (face->scaled_font_methods.unicode_to_glyph) {
	comac_status_t status;

	status = face->scaled_font_methods.unicode_to_glyph (&scaled_font->base,
							     ucs4, &glyph);

	if (status == COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED)
	    goto not_implemented;

	if (status != COMAC_STATUS_SUCCESS) {
	    status = _comac_scaled_font_set_error (&scaled_font->base, status);
	    glyph = 0;
	}

    } else {
not_implemented:
	glyph = ucs4;
    }

    return glyph;
}

static comac_bool_t
_comac_user_has_color_glyphs (void         *abstract_font)
{
    comac_user_scaled_font_t *scaled_font = abstract_font;
    comac_user_font_face_t *face =
	(comac_user_font_face_t *) scaled_font->base.font_face;

    return face->has_color;
}

static comac_int_status_t
_comac_user_text_to_glyphs (void		      *abstract_font,
			    double		       x,
			    double		       y,
			    const char		      *utf8,
			    int			       utf8_len,
			    comac_glyph_t	     **glyphs,
			    int			       *num_glyphs,
			    comac_text_cluster_t      **clusters,
			    int			       *num_clusters,
			    comac_text_cluster_flags_t *cluster_flags)
{
    comac_int_status_t status = COMAC_INT_STATUS_UNSUPPORTED;

    comac_user_scaled_font_t *scaled_font = abstract_font;
    comac_user_font_face_t *face =
	(comac_user_font_face_t *) scaled_font->base.font_face;

    if (face->scaled_font_methods.text_to_glyphs) {
	int i;
	comac_glyph_t *orig_glyphs = *glyphs;
	int orig_num_glyphs = *num_glyphs;

	status = face->scaled_font_methods.text_to_glyphs (&scaled_font->base,
							   utf8, utf8_len,
							   glyphs, num_glyphs,
							   clusters, num_clusters, cluster_flags);

	if (status != COMAC_INT_STATUS_SUCCESS &&
	    status != COMAC_INT_STATUS_USER_FONT_NOT_IMPLEMENTED)
	    return status;

	if (status == COMAC_INT_STATUS_USER_FONT_NOT_IMPLEMENTED ||
	    *num_glyphs < 0) {
	    if (orig_glyphs != *glyphs) {
		comac_glyph_free (*glyphs);
		*glyphs = orig_glyphs;
	    }
	    *num_glyphs = orig_num_glyphs;
	    return COMAC_INT_STATUS_UNSUPPORTED;
	}

	/* Convert from font space to user space and add x,y */
	for (i = 0; i < *num_glyphs; i++) {
	    double gx = (*glyphs)[i].x;
	    double gy = (*glyphs)[i].y;

	    comac_matrix_transform_point (&scaled_font->base.font_matrix,
					  &gx, &gy);

	    (*glyphs)[i].x = gx + x;
	    (*glyphs)[i].y = gy + y;
	}
    }

    return status;
}

static comac_status_t
_comac_user_font_face_scaled_font_create (void                        *abstract_face,
					  const comac_matrix_t        *font_matrix,
					  const comac_matrix_t        *ctm,
					  const comac_font_options_t  *options,
					  comac_scaled_font_t        **scaled_font);

static comac_status_t
_comac_user_font_face_create_for_toy (comac_toy_font_face_t   *toy_face,
				      comac_font_face_t      **font_face)
{
    return _comac_font_face_twin_create_for_toy (toy_face, font_face);
}

static const comac_scaled_font_backend_t _comac_user_scaled_font_backend = {
    COMAC_FONT_TYPE_USER,
    NULL,	/* scaled_font_fini */
    _comac_user_scaled_glyph_init,
    _comac_user_text_to_glyphs,
    _comac_user_ucs4_to_index,
    NULL,	/* load_truetype_table */
    NULL,	/* index_to_ucs4 */
    NULL,       /* is_synthetic */
    NULL,       /* index_to_glyph_name */
    NULL,       /* load_type1_data */
    _comac_user_has_color_glyphs,
};

/* #comac_user_font_face_t */

static comac_status_t
_comac_user_font_face_scaled_font_create (void                        *abstract_face,
					  const comac_matrix_t        *font_matrix,
					  const comac_matrix_t        *ctm,
					  const comac_font_options_t  *options,
					  comac_scaled_font_t        **scaled_font)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    comac_user_font_face_t *font_face = abstract_face;
    comac_user_scaled_font_t *user_scaled_font = NULL;
    comac_font_extents_t font_extents = {1., 0., 1., 1., 0.};

    font_face->immutable = TRUE;

    user_scaled_font = _comac_malloc (sizeof (comac_user_scaled_font_t));
    if (unlikely (user_scaled_font == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = _comac_scaled_font_init (&user_scaled_font->base,
				      &font_face->base,
				      font_matrix, ctm, options,
				      &_comac_user_scaled_font_backend);

    if (unlikely (status)) {
	free (user_scaled_font);
	return status;
    }

    /* XXX metrics hinting? */

    /* compute a normalized version of font scale matrix to compute
     * extents in.  This is to minimize error caused by the comac_fixed_t
     * representation. */
    {
	double fixed_scale, x_scale, y_scale;

	user_scaled_font->extent_scale = user_scaled_font->base.scale_inverse;
	status = _comac_matrix_compute_basis_scale_factors (&user_scaled_font->extent_scale,
						      &x_scale, &y_scale,
						      1);
	if (status == COMAC_STATUS_SUCCESS) {

	    if (x_scale == 0) x_scale = 1.;
	    if (y_scale == 0) y_scale = 1.;

	    user_scaled_font->snap_x_scale = x_scale;
	    user_scaled_font->snap_y_scale = y_scale;

	    /* since glyphs are pretty much 1.0x1.0, we can reduce error by
	     * scaling to a larger square.  say, 1024.x1024. */
	    fixed_scale = 1024.;
	    x_scale /= fixed_scale;
	    y_scale /= fixed_scale;

	    comac_matrix_scale (&user_scaled_font->extent_scale, 1. / x_scale, 1. / y_scale);

	    user_scaled_font->extent_x_scale = x_scale;
	    user_scaled_font->extent_y_scale = y_scale;
	}
    }

    if (status == COMAC_STATUS_SUCCESS &&
	font_face->scaled_font_methods.init != NULL)
    {
	/* Lock the scaled_font mutex such that user doesn't accidentally try
         * to use it just yet. */
	COMAC_MUTEX_LOCK (user_scaled_font->base.mutex);

	/* Give away fontmap lock such that user-font can use other fonts */
	status = _comac_scaled_font_register_placeholder_and_unlock_font_map (&user_scaled_font->base);
	if (status == COMAC_STATUS_SUCCESS) {
	    comac_surface_t *recording_surface;
	    comac_t *cr;

	    recording_surface = _comac_user_scaled_font_create_recording_surface (user_scaled_font, FALSE);
	    cr = _comac_user_scaled_font_create_recording_context (user_scaled_font, recording_surface, FALSE);
	    comac_surface_destroy (recording_surface);

	    status = font_face->scaled_font_methods.init (&user_scaled_font->base,
							  cr,
							  &font_extents);

	    if (status == COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED)
		status = COMAC_STATUS_SUCCESS;

	    if (status == COMAC_STATUS_SUCCESS)
		status = comac_status (cr);

	    comac_destroy (cr);

	    _comac_scaled_font_unregister_placeholder_and_lock_font_map (&user_scaled_font->base);
	}

	COMAC_MUTEX_UNLOCK (user_scaled_font->base.mutex);
    }

    if (status == COMAC_STATUS_SUCCESS)
	status = _comac_scaled_font_set_metrics (&user_scaled_font->base, &font_extents);

    if (status != COMAC_STATUS_SUCCESS) {
        _comac_scaled_font_fini (&user_scaled_font->base);
	free (user_scaled_font);
    } else {
        user_scaled_font->default_glyph_extents.x_bearing = 0.;
        user_scaled_font->default_glyph_extents.y_bearing = -font_extents.ascent;
        user_scaled_font->default_glyph_extents.width = 0.;
        user_scaled_font->default_glyph_extents.height = font_extents.ascent + font_extents.descent;
        user_scaled_font->default_glyph_extents.x_advance = font_extents.max_x_advance;
        user_scaled_font->default_glyph_extents.y_advance = 0.;

	*scaled_font = &user_scaled_font->base;
    }

    return status;
}

const comac_font_face_backend_t _comac_user_font_face_backend = {
    COMAC_FONT_TYPE_USER,
    _comac_user_font_face_create_for_toy,
    _comac_font_face_destroy,
    _comac_user_font_face_scaled_font_create
};


comac_bool_t
_comac_font_face_is_user (comac_font_face_t *font_face)
{
    return font_face->backend == &_comac_user_font_face_backend;
}

/* Implement the public interface */

/**
 * comac_user_font_face_create:
 *
 * Creates a new user font-face.
 *
 * Use the setter functions to associate callbacks with the returned
 * user font.  The only mandatory callback is render_glyph.
 *
 * After the font-face is created, the user can attach arbitrary data
 * (the actual font data) to it using comac_font_face_set_user_data()
 * and access it from the user-font callbacks by using
 * comac_scaled_font_get_font_face() followed by
 * comac_font_face_get_user_data().
 *
 * Return value: a newly created #comac_font_face_t. Free with
 *  comac_font_face_destroy() when you are done using it.
 *
 * Since: 1.8
 **/
comac_font_face_t *
comac_user_font_face_create (void)
{
    comac_user_font_face_t *font_face;

    font_face = _comac_malloc (sizeof (comac_user_font_face_t));
    if (!font_face) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_font_face_t *)&_comac_font_face_nil;
    }

    _comac_font_face_init (&font_face->base, &_comac_user_font_face_backend);

    font_face->immutable = FALSE;
    font_face->has_color = FALSE;
    memset (&font_face->scaled_font_methods, 0, sizeof (font_face->scaled_font_methods));

    return &font_face->base;
}

/* User-font method setters */


/**
 * comac_user_font_face_set_init_func:
 * @font_face: A user font face
 * @init_func: The init callback, or %NULL
 *
 * Sets the scaled-font initialization function of a user-font.
 * See #comac_user_scaled_font_init_func_t for details of how the callback
 * works.
 *
 * The font-face should not be immutable or a %COMAC_STATUS_USER_FONT_IMMUTABLE
 * error will occur.  A user font-face is immutable as soon as a scaled-font
 * is created from it.
 *
 * Since: 1.8
 **/
void
comac_user_font_face_set_init_func (comac_font_face_t                  *font_face,
				    comac_user_scaled_font_init_func_t  init_func)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.init = init_func;
}

/**
 * comac_user_font_face_set_render_color_glyph_func:
 * @font_face: A user font face
 * @render_glyph_func: The render_glyph callback, or %NULL
 *
 * Sets the color glyph rendering function of a user-font.
 * See #comac_user_scaled_font_render_glyph_func_t for details of how the callback
 * works.
 *
 * The font-face should not be immutable or a %COMAC_STATUS_USER_FONT_IMMUTABLE
 * error will occur.  A user font-face is immutable as soon as a scaled-font
 * is created from it.
 *
 * The render_glyph callback is the only mandatory callback of a
 * user-font. At least one of
 * comac_user_font_face_set_render_color_glyph_func() or
 * comac_user_font_face_set_render_glyph_func() must be called to set
 * a render callback. If both callbacks are set, the color glyph
 * render callback is invoked first. If the color glyph render
 * callback returns %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED, the
 * non-color version of the callback is invoked.
 *
 * If the callback is %NULL and a glyph is tried to be rendered using
 * @font_face, a %COMAC_STATUS_USER_FONT_ERROR will occur.
 *
 * Since: 1.18
 **/
void
comac_user_font_face_set_render_color_glyph_func (comac_font_face_t                          *font_face,
                                                  comac_user_scaled_font_render_glyph_func_t  render_glyph_func)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.render_color_glyph = render_glyph_func;
    user_font_face->has_color = render_glyph_func ? TRUE : FALSE;
}

/**
 * comac_user_font_face_set_render_glyph_func:
 * @font_face: A user font face
 * @render_glyph_func: The render_glyph callback, or %NULL
 *
 * Sets the glyph rendering function of a user-font.
 * See #comac_user_scaled_font_render_glyph_func_t for details of how the callback
 * works.
 *
 * The font-face should not be immutable or a %COMAC_STATUS_USER_FONT_IMMUTABLE
 * error will occur.  A user font-face is immutable as soon as a scaled-font
 * is created from it.
 *
 * The render_glyph callback is the only mandatory callback of a
 * user-font. At least one of
 * comac_user_font_face_set_render_color_glyph_func() or
 * comac_user_font_face_set_render_glyph_func() must be called to set
 * a render callback. If both callbacks are set, the color glyph
 * render callback is invoked first. If the color glyph render
 * callback returns %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED, the
 * non-color version of the callback is invoked.
 *
 * If the callback is %NULL and a glyph is tried to be rendered using
 * @font_face, a %COMAC_STATUS_USER_FONT_ERROR will occur.
 *
 * Since: 1.8
 **/
void
comac_user_font_face_set_render_glyph_func (comac_font_face_t                          *font_face,
					    comac_user_scaled_font_render_glyph_func_t  render_glyph_func)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.render_glyph = render_glyph_func;
}

/**
 * comac_user_font_face_set_text_to_glyphs_func:
 * @font_face: A user font face
 * @text_to_glyphs_func: The text_to_glyphs callback, or %NULL
 *
 * Sets th text-to-glyphs conversion function of a user-font.
 * See #comac_user_scaled_font_text_to_glyphs_func_t for details of how the callback
 * works.
 *
 * The font-face should not be immutable or a %COMAC_STATUS_USER_FONT_IMMUTABLE
 * error will occur.  A user font-face is immutable as soon as a scaled-font
 * is created from it.
 *
 * Since: 1.8
 **/
void
comac_user_font_face_set_text_to_glyphs_func (comac_font_face_t                            *font_face,
					      comac_user_scaled_font_text_to_glyphs_func_t  text_to_glyphs_func)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.text_to_glyphs = text_to_glyphs_func;
}

/**
 * comac_user_font_face_set_unicode_to_glyph_func:
 * @font_face: A user font face
 * @unicode_to_glyph_func: The unicode_to_glyph callback, or %NULL
 *
 * Sets the unicode-to-glyph conversion function of a user-font.
 * See #comac_user_scaled_font_unicode_to_glyph_func_t for details of how the callback
 * works.
 *
 * The font-face should not be immutable or a %COMAC_STATUS_USER_FONT_IMMUTABLE
 * error will occur.  A user font-face is immutable as soon as a scaled-font
 * is created from it.
 *
 * Since: 1.8
 **/
void
comac_user_font_face_set_unicode_to_glyph_func (comac_font_face_t                              *font_face,
						comac_user_scaled_font_unicode_to_glyph_func_t  unicode_to_glyph_func)
{
    comac_user_font_face_t *user_font_face;
    if (font_face->status)
	return;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    if (user_font_face->immutable) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_USER_FONT_IMMUTABLE))
	    return;
    }
    user_font_face->scaled_font_methods.unicode_to_glyph = unicode_to_glyph_func;
}

/* User-font method getters */

/**
 * comac_user_font_face_get_init_func:
 * @font_face: A user font face
 *
 * Gets the scaled-font initialization function of a user-font.
 *
 * Return value: The init callback of @font_face
 * or %NULL if none set or an error has occurred.
 *
 * Since: 1.8
 **/
comac_user_scaled_font_init_func_t
comac_user_font_face_get_init_func (comac_font_face_t *font_face)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.init;
}

/**
 * comac_user_font_face_get_render_color_glyph_func:
 * @font_face: A user font face
 *
 * Gets the color glyph rendering function of a user-font.
 *
 * Return value: The render_glyph callback of @font_face
 * or %NULL if none set or an error has occurred.
 *
 * Since: 1.18
 **/
comac_user_scaled_font_render_glyph_func_t
comac_user_font_face_get_render_color_glyph_func (comac_font_face_t *font_face)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.render_color_glyph;
}

/**
 * comac_user_font_face_get_render_glyph_func:
 * @font_face: A user font face
 *
 * Gets the glyph rendering function of a user-font.
 *
 * Return value: The render_glyph callback of @font_face
 * or %NULL if none set or an error has occurred.
 *
 * Since: 1.8
 **/
comac_user_scaled_font_render_glyph_func_t
comac_user_font_face_get_render_glyph_func (comac_font_face_t *font_face)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.render_glyph;
}

/**
 * comac_user_font_face_get_text_to_glyphs_func:
 * @font_face: A user font face
 *
 * Gets the text-to-glyphs conversion function of a user-font.
 *
 * Return value: The text_to_glyphs callback of @font_face
 * or %NULL if none set or an error occurred.
 *
 * Since: 1.8
 **/
comac_user_scaled_font_text_to_glyphs_func_t
comac_user_font_face_get_text_to_glyphs_func (comac_font_face_t *font_face)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.text_to_glyphs;
}

/**
 * comac_user_font_face_get_unicode_to_glyph_func:
 * @font_face: A user font face
 *
 * Gets the unicode-to-glyph conversion function of a user-font.
 *
 * Return value: The unicode_to_glyph callback of @font_face
 * or %NULL if none set or an error occurred.
 *
 * Since: 1.8
 **/
comac_user_scaled_font_unicode_to_glyph_func_t
comac_user_font_face_get_unicode_to_glyph_func (comac_font_face_t *font_face)
{
    comac_user_font_face_t *user_font_face;

    if (font_face->status)
	return NULL;

    if (! _comac_font_face_is_user (font_face)) {
	if (_comac_font_face_set_error (font_face, COMAC_STATUS_FONT_TYPE_MISMATCH))
	    return NULL;
    }

    user_font_face = (comac_user_font_face_t *) font_face;
    return user_font_face->scaled_font_methods.unicode_to_glyph;
}
