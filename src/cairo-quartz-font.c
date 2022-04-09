/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright � 2008 Mozilla Corporation
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
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 *
 * Contributor(s):
 *	Vladimir Vukicevic <vladimir@mozilla.com>
 */

#include "cairoint.h"

#include <dlfcn.h>

#include "cairo-image-surface-private.h"
#include "cairo-quartz.h"
#include "cairo-quartz-private.h"

#include "cairo-error-private.h"

/**
 * SECTION:cairo-quartz-fonts
 * @Title: Quartz (CGFont) Fonts
 * @Short_Description: Font support via Core Text on Apple operating systems.
 * @See_Also: #cairo_font_face_t
 *
 **/

/**
 * CAIRO_HAS_QUARTZ_FONT:
 *
 * Defined if the Quartz font backend is available.
 * This macro can be used to conditionally compile backend-specific code.
 *
 * Since: 1.6
 **/

/* These are private functions */
static bool (*CGContextGetAllowsFontSmoothingPtr) (CGContextRef) = NULL;
static ATSFontRef (*FMGetATSFontRefFromFontPtr) (FMFont iFont) = NULL;

static cairo_bool_t _cairo_quartz_font_symbol_lookup_done = FALSE;

/* Defined in 10.11 */
#define CGGLYPH_MAX ((CGGlyph) 0xFFFE) /* kCGFontIndexMax */
#define CGGLYPH_INVALID ((CGGlyph) 0xFFFF) /* kCGFontIndexInvalid */

static void
quartz_font_ensure_symbols(void)
{
    if (_cairo_quartz_font_symbol_lookup_done)
	return;

    CGContextGetAllowsFontSmoothingPtr =
	dlsym (RTLD_DEFAULT, "CGContextGetAllowsFontSmoothing");

    FMGetATSFontRefFromFontPtr = dlsym(RTLD_DEFAULT, "FMGetATSFontRefFromFont");

    _cairo_quartz_font_symbol_lookup_done = TRUE;
}

typedef struct _cairo_quartz_font_face cairo_quartz_font_face_t;
typedef struct _cairo_quartz_scaled_font cairo_quartz_scaled_font_t;

struct _cairo_quartz_scaled_font {
    cairo_scaled_font_t base;
};

struct _cairo_quartz_font_face {
    cairo_font_face_t base;

    CGFontRef cgFont;
    CTFontRef ctFont;
    double ctFont_scale;
};

/*
 * font face backend
 */

static cairo_status_t
_cairo_quartz_font_face_create_for_toy (cairo_toy_font_face_t   *toy_face,
					cairo_font_face_t      **font_face)
{
    const char *family;
    char *full_name;
    CFStringRef FontName = NULL;
    CTFontRef ctFont = NULL;
    int loop;

    family = toy_face->family;
    full_name = _cairo_malloc (strlen (family) + 64); // give us a bit of room to tack on Bold, Oblique, etc.
    /* handle CSS-ish faces */
    if (!strcmp(family, "serif") || !strcmp(family, "Times Roman"))
	family = "Times";
    else if (!strcmp(family, "sans-serif") || !strcmp(family, "sans"))
	family = "Helvetica";
    else if (!strcmp(family, "cursive"))
	family = "Apple Chancery";
    else if (!strcmp(family, "fantasy"))
	family = "Papyrus";
    else if (!strcmp(family, "monospace") || !strcmp(family, "mono"))
	family = "Courier";

    /* Try to build up the full name, e.g. "Helvetica Bold Oblique" first,
     * then drop the bold, then drop the slant, then drop both.. finally
     * just use "Helvetica".  And if Helvetica doesn't exist, give up.
     */
    for (loop = 0; loop < 5; loop++) {
	if (loop == 4)
	    family = "Helvetica";

	strcpy (full_name, family);

	if (loop < 3 && (loop & 1) == 0) {
	    if (toy_face->weight == CAIRO_FONT_WEIGHT_BOLD)
		strcat (full_name, "-Bold");
	}

	if (loop < 3 && (loop & 2) == 0) {
	    if (toy_face->slant == CAIRO_FONT_SLANT_ITALIC)
		strcat (full_name, "-Italic");
	    else if (toy_face->slant == CAIRO_FONT_SLANT_OBLIQUE)
		strcat (full_name, "-Oblique");
	}

	FontName = CFStringCreateWithCString (NULL, full_name, kCFStringEncodingASCII);
	ctFont = CTFontCreateWithName (FontName, 1.0, NULL);
	CFRelease (FontName);

	if (ctFont)
	    break;
    }

    if (!ctFont) {
	/* Give up */
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    *font_face = _cairo_quartz_font_face_create_for_ctfont (ctFont);
    CFRelease (ctFont);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_bool_t
_cairo_quartz_font_face_destroy (void *abstract_face)
{
    cairo_quartz_font_face_t *font_face = (cairo_quartz_font_face_t*) abstract_face;

    CGFontRelease (font_face->cgFont);
    CFRelease (font_face->ctFont);
    return TRUE;
}

static const cairo_scaled_font_backend_t _cairo_quartz_scaled_font_backend;

static cairo_status_t
_cairo_quartz_font_face_scaled_font_create (void *abstract_face,
					    const cairo_matrix_t *font_matrix,
					    const cairo_matrix_t *ctm,
					    const cairo_font_options_t *options,
					    cairo_scaled_font_t **font_out)
{
    cairo_quartz_font_face_t *font_face = abstract_face;
    cairo_quartz_scaled_font_t *font = NULL;
    cairo_status_t status;
    cairo_font_extents_t fs_metrics;
    double ems;
    CGRect bbox;

    font = _cairo_malloc (sizeof(cairo_quartz_scaled_font_t));
    if (font == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    memset (font, 0, sizeof(cairo_quartz_scaled_font_t));

    status = _cairo_scaled_font_init (&font->base,
				      &font_face->base, font_matrix, ctm, options,
				      &_cairo_quartz_scaled_font_backend);
    if (status)
	goto FINISH;

    ems = CGFontGetUnitsPerEm (font_face->cgFont);

    /* initialize metrics */
    fs_metrics.ascent = (CGFontGetAscent (font_face->cgFont) / ems);
    fs_metrics.descent = - (CGFontGetDescent (font_face->cgFont) / ems);
    fs_metrics.height = fs_metrics.ascent + fs_metrics.descent +
	(CGFontGetLeading (font_face->cgFont) / ems);

    bbox = CGFontGetFontBBox (font_face->cgFont);
    fs_metrics.max_x_advance = CGRectGetMaxX(bbox) / ems;
    fs_metrics.max_y_advance = 0.0;

    status = _cairo_scaled_font_set_metrics (&font->base, &fs_metrics);

FINISH:
    if (status != CAIRO_STATUS_SUCCESS) {
	free (font);
    } else {
	*font_out = (cairo_scaled_font_t*) font;
    }

    return status;
}

const cairo_font_face_backend_t _cairo_quartz_font_face_backend = {
    CAIRO_FONT_TYPE_QUARTZ,
    _cairo_quartz_font_face_create_for_toy,
    _cairo_quartz_font_face_destroy,
    _cairo_quartz_font_face_scaled_font_create
};

static inline cairo_quartz_font_face_t*
_cairo_quartz_font_face_create ()
{
    cairo_quartz_font_face_t *font_face =
	_cairo_malloc (sizeof (cairo_quartz_font_face_t));

    if (!font_face) {
	cairo_status_t ignore_status;
	ignore_status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_quartz_font_face_t *)&_cairo_font_face_nil;
    }

    _cairo_font_face_init (&font_face->base, &_cairo_quartz_font_face_backend);

    return font_face;
}

/**
 * cairo_quartz_font_face_create_for_cgfont:
 * @font: a #CGFontRef obtained through a method external to cairo.
 *
 * Creates a new font for the Quartz font backend based on a
 * #CGFontRef.  This font can then be used with
 * cairo_set_font_face() or cairo_scaled_font_create().
 *
 * Return value: a newly created #cairo_font_face_t. Free with
 *  cairo_font_face_destroy() when you are done using it.
 *
 * Since: 1.6
 **/
cairo_font_face_t *
cairo_quartz_font_face_create_for_cgfont (CGFontRef font)
{
    cairo_quartz_font_face_t* font_face = _cairo_quartz_font_face_create ();
    double font_scale = 1.0; /* 1.0 produces glyphs of the right size to pass tests. */

    if (cairo_font_face_status (&font_face->base))
	return &font_face->base;

    font_face->cgFont = CGFontRetain (font);
    font_face->ctFont = CTFontCreateWithGraphicsFont (font, font_scale, NULL, NULL);
    font_face->ctFont_scale = CTFontGetUnitsPerEm(font_face->ctFont);

    return &font_face->base;
}

cairo_font_face_t *
_cairo_quartz_font_face_create_for_ctfont (CTFontRef font)
{
    cairo_quartz_font_face_t* font_face = _cairo_quartz_font_face_create ();

    if (cairo_font_face_status (&font_face->base))
	return &font_face->base;

    font_face->ctFont = CFRetain (font);
    font_face->cgFont = CTFontCopyGraphicsFont (font, NULL);
    font_face->ctFont_scale = CTFontGetUnitsPerEm(font_face->ctFont);

    return &font_face->base;
}

/*
 * scaled font backend
 */

static cairo_quartz_font_face_t *
_cairo_quartz_scaled_to_face (void *abstract_font)
{
    cairo_quartz_scaled_font_t *sfont = (cairo_quartz_scaled_font_t*) abstract_font;
    cairo_font_face_t *font_face = sfont->base.font_face;
    assert (font_face->backend->type == CAIRO_FONT_TYPE_QUARTZ);
    return (cairo_quartz_font_face_t*) font_face;
}

static void
_cairo_quartz_scaled_font_fini(void *abstract_font)
{
}

static inline CGGlyph
_cairo_quartz_scaled_glyph_index (cairo_scaled_glyph_t *scaled_glyph) {
    unsigned long index = _cairo_scaled_glyph_index (scaled_glyph);
    return index <= CGGLYPH_MAX ? index : CGGLYPH_INVALID;
}

static cairo_int_status_t
_cairo_quartz_init_glyph_metrics (cairo_quartz_scaled_font_t *font,
				  cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    cairo_quartz_font_face_t *font_face = _cairo_quartz_scaled_to_face(font);
    cairo_text_extents_t extents = {0, 0, 0, 0, 0, 0};
    CGGlyph glyph = _cairo_quartz_scaled_glyph_index (scaled_glyph);
    int advance;
    CGRect bbox;
    double emscale = CGFontGetUnitsPerEm (font_face->cgFont);
    double xmin, ymin, xmax, ymax;

    if (unlikely (glyph == CGGLYPH_INVALID))
	goto FAIL;

    if (!CGFontGetGlyphAdvances (font_face->cgFont, &glyph, 1, &advance) ||
	!CGFontGetGlyphBBoxes (font_face->cgFont, &glyph, 1, &bbox))
	goto FAIL;

    /* broken fonts like Al Bayan return incorrect bounds for some null characters,
       see https://bugzilla.mozilla.org/show_bug.cgi?id=534260 */
    if (unlikely (bbox.origin.x == -32767 &&
                  bbox.origin.y == -32767 &&
                  bbox.size.width == 65534 &&
                  bbox.size.height == 65534)) {
        bbox.origin.x = bbox.origin.y = 0;
        bbox.size.width = bbox.size.height = 0;
    }

    bbox = CGRectMake (bbox.origin.x / emscale,
		       bbox.origin.y / emscale,
		       bbox.size.width / emscale,
		       bbox.size.height / emscale);

    /* Should we want to always integer-align glyph extents, we can do so in this way */
#if 0
    {
	CGAffineTransform textMatrix;
	textMatrix = CGAffineTransformMake (font->base.scale.xx,
					    -font->base.scale.yx,
					    -font->base.scale.xy,
					    font->base.scale.yy,
					    0.0f, 0.0f);

	bbox = CGRectApplyAffineTransform (bbox, textMatrix);
	bbox = CGRectIntegral (bbox);
	bbox = CGRectApplyAffineTransform (bbox, CGAffineTransformInvert (textMatrix));
    }
#endif

#if 0
    fprintf (stderr, "[0x%04x] bbox: %f %f %f %f\n", glyph,
	     bbox.origin.x / emscale, bbox.origin.y / emscale,
	     bbox.size.width / emscale, bbox.size.height / emscale);
#endif

    xmin = CGRectGetMinX(bbox);
    ymin = CGRectGetMinY(bbox);
    xmax = CGRectGetMaxX(bbox);
    ymax = CGRectGetMaxY(bbox);

    extents.x_bearing = xmin;
    extents.y_bearing = - ymax;
    extents.width = xmax - xmin;
    extents.height = ymax - ymin;

    extents.x_advance = (double) advance / emscale;
    extents.y_advance = 0.0;

#if 0
    fprintf (stderr, "[0x%04x] extents: bearings: %f %f dim: %f %f adv: %f\n\n", glyph,
	     extents.x_bearing, extents.y_bearing, extents.width, extents.height, extents.x_advance);
#endif

  FAIL:
    _cairo_scaled_glyph_set_metrics (scaled_glyph,
				     &font->base,
				     &extents);

    return status;
}

static void
_cairo_quartz_path_apply_func (void *info, const CGPathElement *el)
{
    cairo_path_fixed_t *path = (cairo_path_fixed_t *) info;
    cairo_status_t status;

    switch (el->type) {
	case kCGPathElementMoveToPoint:
	    status = _cairo_path_fixed_move_to (path,
						_cairo_fixed_from_double(el->points[0].x),
						_cairo_fixed_from_double(el->points[0].y));
	    assert(!status);
	    break;
	case kCGPathElementAddLineToPoint:
	    status = _cairo_path_fixed_line_to (path,
						_cairo_fixed_from_double(el->points[0].x),
						_cairo_fixed_from_double(el->points[0].y));
	    assert(!status);
	    break;
	case kCGPathElementAddQuadCurveToPoint: {
	    cairo_fixed_t fx, fy;
	    double x, y;
	    if (!_cairo_path_fixed_get_current_point (path, &fx, &fy))
		fx = fy = 0;
	    x = _cairo_fixed_to_double (fx);
	    y = _cairo_fixed_to_double (fy);

	    status = _cairo_path_fixed_curve_to (path,
						 _cairo_fixed_from_double((x + el->points[0].x * 2.0) / 3.0),
						 _cairo_fixed_from_double((y + el->points[0].y * 2.0) / 3.0),
						 _cairo_fixed_from_double((el->points[0].x * 2.0 + el->points[1].x) / 3.0),
						 _cairo_fixed_from_double((el->points[0].y * 2.0 + el->points[1].y) / 3.0),
						 _cairo_fixed_from_double(el->points[1].x),
						 _cairo_fixed_from_double(el->points[1].y));
	}
	    assert(!status);
	    break;
	case kCGPathElementAddCurveToPoint:
	    status = _cairo_path_fixed_curve_to (path,
						 _cairo_fixed_from_double(el->points[0].x),
						 _cairo_fixed_from_double(el->points[0].y),
						 _cairo_fixed_from_double(el->points[1].x),
						 _cairo_fixed_from_double(el->points[1].y),
						 _cairo_fixed_from_double(el->points[2].x),
						 _cairo_fixed_from_double(el->points[2].y));
	    assert(!status);	    
	    break;
	case kCGPathElementCloseSubpath:
	    status = _cairo_path_fixed_close_path (path);
	    assert(!status);
	    break;
    }
}

static cairo_int_status_t
_cairo_quartz_init_glyph_path (cairo_quartz_scaled_font_t *font,
			       cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_quartz_font_face_t *font_face = _cairo_quartz_scaled_to_face(font);
    CGGlyph glyph = _cairo_quartz_scaled_glyph_index (scaled_glyph);
    CGAffineTransform textMatrix;
    CGPathRef glyphPath;
    cairo_path_fixed_t *path;

    if (unlikely (glyph == CGGLYPH_INVALID)) {
	_cairo_scaled_glyph_set_path (scaled_glyph, &font->base, _cairo_path_fixed_create());
	return CAIRO_STATUS_SUCCESS;
    }

    /* scale(1,-1) * font->base.scale */
    textMatrix = CGAffineTransformMake (font->base.scale.xx,
					font->base.scale.yx,
					-font->base.scale.xy,
					-font->base.scale.yy,
					0, 0);

    glyphPath = CTFontCreatePathForGlyph (font_face->ctFont, glyph, &textMatrix);

    if (!glyphPath)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    path = _cairo_path_fixed_create ();
    if (!path) {
	CGPathRelease (glyphPath);
	return _cairo_error(CAIRO_STATUS_NO_MEMORY);
    }

    CGPathApply (glyphPath, path, _cairo_quartz_path_apply_func);

    CGPathRelease (glyphPath);

    _cairo_scaled_glyph_set_path (scaled_glyph, &font->base, path);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_quartz_init_glyph_surface (cairo_quartz_scaled_font_t *font,
				  cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    cairo_quartz_font_face_t *font_face = _cairo_quartz_scaled_to_face(font);

    cairo_image_surface_t *surface = NULL;

    CGGlyph glyph = _cairo_quartz_scaled_glyph_index (scaled_glyph);

    int advance;
    CGRect bbox;
    double width, height;
    double emscale = CGFontGetUnitsPerEm (font_face->cgFont);

    CGContextRef cgContext = NULL;
    CGAffineTransform textMatrix;
    CGRect glyphRect, glyphRectInt;
    CGPoint glyphOrigin;

    //fprintf (stderr, "scaled_glyph: %p surface: %p\n", scaled_glyph, scaled_glyph->surface);

    /* Create blank 2x2 image if we don't have this character.
     * Maybe we should draw a better missing-glyph slug or something,
     * but this is ok for now.
     */
    if (unlikely (glyph == CGGLYPH_INVALID)) {
	surface = (cairo_image_surface_t*) cairo_image_surface_create (CAIRO_FORMAT_A8, 2, 2);
	status = cairo_surface_status ((cairo_surface_t *) surface);
	if (status)
	    return status;

	_cairo_scaled_glyph_set_surface (scaled_glyph,
					 &font->base,
					 surface);
	return CAIRO_STATUS_SUCCESS;
    }

    CTFontGetBoundingRectsForGlyphs (font_face->ctFont, FONT_ORIENTATION_HORIZONTAL, &glyph, &bbox, 1);

    /* scale(1,-1) * font->base.scale * scale(1,-1) */
    textMatrix = CGAffineTransformMake (font->base.scale.xx,
					-font->base.scale.yx,
					-font->base.scale.xy,
					font->base.scale.yy,
					0, -0);
    glyphRect = CGRectMake (bbox.origin.x / emscale,
			    bbox.origin.y / emscale,
			    bbox.size.width / emscale,
			    bbox.size.height / emscale);

    glyphRect = CGRectApplyAffineTransform (glyphRect, textMatrix);

    /* Round the rectangle outwards, so that we don't have to deal
     * with non-integer-pixel origins or dimensions.
     */
    glyphRectInt = CGRectIntegral (glyphRect);

#if 0
    fprintf (stderr, "glyphRect[o]: %f %f %f %f\n",
	     glyphRect.origin.x, glyphRect.origin.y, glyphRect.size.width, glyphRect.size.height);
    fprintf (stderr, "glyphRectInt: %f %f %f %f\n",
	     glyphRectInt.origin.x, glyphRectInt.origin.y, glyphRectInt.size.width, glyphRectInt.size.height);
#endif

    glyphOrigin = glyphRectInt.origin;

    //textMatrix = CGAffineTransformConcat (textMatrix, CGAffineTransformInvert (ctm));

    width = glyphRectInt.size.width;
    height = glyphRectInt.size.height;

    //fprintf (stderr, "glyphRect[n]: %f %f %f %f\n", glyphRect.origin.x, glyphRect.origin.y, glyphRect.size.width, glyphRect.size.height);

    surface = (cairo_image_surface_t*) cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    if (surface->base.status)
	return surface->base.status;

    if (surface->width != 0 && surface->height != 0) {
	cgContext = CGBitmapContextCreate (surface->data,
					   surface->width,
					   surface->height,
					   8,
					   surface->stride,
					   NULL,
					   kCGImageAlphaOnly);

	if (cgContext == NULL) {
	    cairo_surface_destroy (&surface->base);
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

	CGContextSetFont (cgContext, font_face->cgFont);
	CGContextSetFontSize (cgContext, 1.0);
	CGContextSetTextMatrix (cgContext, textMatrix);

	switch (font->base.options.antialias) {
	case CAIRO_ANTIALIAS_SUBPIXEL:
	case CAIRO_ANTIALIAS_BEST:
	    CGContextSetShouldAntialias (cgContext, TRUE);
	    CGContextSetShouldSmoothFonts (cgContext, TRUE);
	    quartz_font_ensure_symbols ();
	    if (CGContextGetAllowsFontSmoothingPtr &&
		!CGContextGetAllowsFontSmoothingPtr (cgContext))
		CGContextSetAllowsFontSmoothing (cgContext, TRUE);
	    break;
	case CAIRO_ANTIALIAS_NONE:
	    CGContextSetShouldAntialias (cgContext, FALSE);
	    break;
	case CAIRO_ANTIALIAS_GRAY:
	case CAIRO_ANTIALIAS_GOOD:
	case CAIRO_ANTIALIAS_FAST:
	    CGContextSetShouldAntialias (cgContext, TRUE);
	    CGContextSetShouldSmoothFonts (cgContext, FALSE);
	    break;
	case CAIRO_ANTIALIAS_DEFAULT:
	default:
	    /* Don't do anything */
	    break;
	}

	CGContextSetAlpha (cgContext, 1.0);
	CGContextShowGlyphsAtPoint (cgContext, - glyphOrigin.x, - glyphOrigin.y, &glyph, 1);

	CGContextRelease (cgContext);
    }

    cairo_surface_set_device_offset (&surface->base,
				     - glyphOrigin.x,
				     height + glyphOrigin.y);

    _cairo_scaled_glyph_set_surface (scaled_glyph, &font->base, surface);

    return status;
}

static cairo_int_status_t
_cairo_quartz_scaled_glyph_init (void *abstract_font,
				 cairo_scaled_glyph_t *scaled_glyph,
				 cairo_scaled_glyph_info_t info,
				 const cairo_color_t *foreground_color)
{
    cairo_quartz_scaled_font_t *font = (cairo_quartz_scaled_font_t *) abstract_font;
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    if (!status && (info & CAIRO_SCALED_GLYPH_INFO_METRICS))
	status = _cairo_quartz_init_glyph_metrics (font, scaled_glyph);

    if (!status && (info & CAIRO_SCALED_GLYPH_INFO_PATH))
	status = _cairo_quartz_init_glyph_path (font, scaled_glyph);

    if (!status && (info & CAIRO_SCALED_GLYPH_INFO_SURFACE))
	status = _cairo_quartz_init_glyph_surface (font, scaled_glyph);

    return status;
}

static unsigned long
_cairo_quartz_ucs4_to_index (void *abstract_font,
			     uint32_t ucs4)
{
    cairo_quartz_scaled_font_t *font = (cairo_quartz_scaled_font_t*) abstract_font;
    cairo_quartz_font_face_t *ffont = _cairo_quartz_scaled_to_face(font);
    CGGlyph glyph[2];
    UniChar utf16[2];

    int len = _cairo_ucs4_to_utf16 (ucs4, utf16);
    CTFontGetGlyphsForCharacters (ffont->ctFont, utf16, glyph, len);
    return glyph[0];
}

static cairo_int_status_t
_cairo_quartz_load_truetype_table (void	            *abstract_font,
				   unsigned long     tag,
				   long              offset,
				   unsigned char    *buffer,
				   unsigned long    *length)
{
    cairo_quartz_font_face_t *font_face = _cairo_quartz_scaled_to_face (abstract_font);
    CFDataRef data = CGFontCopyTableForTag (font_face->cgFont, tag);

    if (!data)
        return CAIRO_INT_STATUS_UNSUPPORTED;

    if (buffer == NULL) {
	*length = CFDataGetLength (data);
	CFRelease (data);
	return CAIRO_STATUS_SUCCESS;
    }

    if (CFDataGetLength (data) < offset + (long) *length) {
	CFRelease (data);
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    CFDataGetBytes (data, CFRangeMake (offset, *length), buffer);
    CFRelease (data);

    return CAIRO_STATUS_SUCCESS;
}

static const cairo_scaled_font_backend_t _cairo_quartz_scaled_font_backend = {
    CAIRO_FONT_TYPE_QUARTZ,
    _cairo_quartz_scaled_font_fini,
    _cairo_quartz_scaled_glyph_init,
    NULL, /* text_to_glyphs */
    _cairo_quartz_ucs4_to_index,
    _cairo_quartz_load_truetype_table,
    NULL, /*index_to_ucs4*/
}; /* is_synthetic, index_to_glyph_name, load_type1_data, has_color_glyphs */

/*
 * private methods that the quartz surface uses
 */

CGFontRef
_cairo_quartz_scaled_font_get_cg_font_ref (cairo_scaled_font_t *abstract_font)
{
    cairo_quartz_font_face_t *ffont = _cairo_quartz_scaled_to_face(abstract_font);

    return ffont->cgFont;
}

/*
 * compat with old ATSUI backend
 */

/**
 * cairo_quartz_font_face_create_for_atsu_font_id:
 * @font_id: an ATSUFontID for the font.
 *
 * Creates a new font for the Quartz font backend based on an
 * #ATSUFontID. This font can then be used with
 * cairo_set_font_face() or cairo_scaled_font_create().
 *
 * Return value: a newly created #cairo_font_face_t. Free with
 *  cairo_font_face_destroy() when you are done using it.
 *
 * Since: 1.6
 **/
cairo_font_face_t *
cairo_quartz_font_face_create_for_atsu_font_id (ATSUFontID font_id)
{
    quartz_font_ensure_symbols();

    if (FMGetATSFontRefFromFontPtr != NULL) {
	ATSFontRef atsFont = FMGetATSFontRefFromFontPtr (font_id);
	CGFontRef cgFont = CGFontCreateWithPlatformFont (&atsFont);
	cairo_font_face_t *ff;

	ff = cairo_quartz_font_face_create_for_cgfont (cgFont);

	CGFontRelease (cgFont);

	return ff;
    } else {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_font_face_t *)&_cairo_font_face_nil;
    }
}

/* This is the old name for the above function, exported for compat purposes */
cairo_font_face_t *cairo_atsui_font_face_create_for_atsu_font_id (ATSUFontID font_id);

cairo_font_face_t *
cairo_atsui_font_face_create_for_atsu_font_id (ATSUFontID font_id)
{
    return cairo_quartz_font_face_create_for_atsu_font_id (font_id);
}
