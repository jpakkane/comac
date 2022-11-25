/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2003 University of Southern California
 * Copyright © 2009,2010,2011 Intel Corporation
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
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-boxes-private.h"
#include "comac-clip-private.h"
#include "comac-composite-rectangles-private.h"
#include "comac-compositor-private.h"
#include "comac-default-context-private.h"
#include "comac-error-private.h"
#include "comac-image-surface-inline.h"
#include "comac-paginated-private.h"
#include "comac-pattern-private.h"
#include "comac-pixman-private.h"
#include "comac-recording-surface-private.h"
#include "comac-region-private.h"
#include "comac-scaled-font-private.h"
#include "comac-surface-snapshot-inline.h"
#include "comac-surface-snapshot-private.h"
#include "comac-surface-subsurface-private.h"

/* Limit on the width / height of an image surface in pixels.  This is
 * mainly determined by coordinates of things sent to pixman at the
 * moment being in 16.16 format. */
#define MAX_IMAGE_SIZE 32767

/**
 * SECTION:comac-image
 * @Title: Image Surfaces
 * @Short_Description: Rendering to memory buffers
 * @See_Also: #comac_surface_t
 *
 * Image surfaces provide the ability to render to memory buffers
 * either allocated by comac or by the calling code.  The supported
 * image formats are those defined in #comac_format_t.
 **/

/**
 * COMAC_HAS_IMAGE_SURFACE:
 *
 * Defined if the image surface backend is available.
 * The image surface backend is always built in.
 * This macro was added for completeness in comac 1.8.
 *
 * Since: 1.8
 **/

static comac_bool_t
_comac_image_surface_is_size_valid (int width, int height)
{
    return 0 <= width  &&  width <= MAX_IMAGE_SIZE &&
	   0 <= height && height <= MAX_IMAGE_SIZE;
}

comac_format_t
_comac_format_from_pixman_format (pixman_format_code_t pixman_format)
{
    switch (pixman_format) {
    case PIXMAN_rgba_float:
	return COMAC_FORMAT_RGBA128F;
    case PIXMAN_rgb_float:
	return COMAC_FORMAT_RGB96F;
    case PIXMAN_a8r8g8b8:
	return COMAC_FORMAT_ARGB32;
    case PIXMAN_x2r10g10b10:
	return COMAC_FORMAT_RGB30;
    case PIXMAN_x8r8g8b8:
	return COMAC_FORMAT_RGB24;
    case PIXMAN_a8:
	return COMAC_FORMAT_A8;
    case PIXMAN_a1:
	return COMAC_FORMAT_A1;
    case PIXMAN_r5g6b5:
	return COMAC_FORMAT_RGB16_565;
#if PIXMAN_VERSION >= PIXMAN_VERSION_ENCODE(0,22,0)
    case PIXMAN_r8g8b8a8: case PIXMAN_r8g8b8x8:
#endif
#if PIXMAN_VERSION >= PIXMAN_VERSION_ENCODE(0,27,2)
    case PIXMAN_a8r8g8b8_sRGB:
#endif
    case PIXMAN_a8b8g8r8: case PIXMAN_x8b8g8r8: case PIXMAN_r8g8b8:
    case PIXMAN_b8g8r8:   case PIXMAN_b5g6r5:
    case PIXMAN_a1r5g5b5: case PIXMAN_x1r5g5b5: case PIXMAN_a1b5g5r5:
    case PIXMAN_x1b5g5r5: case PIXMAN_a4r4g4b4: case PIXMAN_x4r4g4b4:
    case PIXMAN_a4b4g4r4: case PIXMAN_x4b4g4r4: case PIXMAN_r3g3b2:
    case PIXMAN_b2g3r3:   case PIXMAN_a2r2g2b2: case PIXMAN_a2b2g2r2:
    case PIXMAN_c8:       case PIXMAN_g8:       case PIXMAN_x4a4:
    case PIXMAN_a4:       case PIXMAN_r1g2b1:   case PIXMAN_b1g2r1:
    case PIXMAN_a1r1g1b1: case PIXMAN_a1b1g1r1: case PIXMAN_c4:
    case PIXMAN_g4:       case PIXMAN_g1:
    case PIXMAN_yuy2:     case PIXMAN_yv12:
    case PIXMAN_b8g8r8x8:
    case PIXMAN_b8g8r8a8:
    case PIXMAN_a2b10g10r10:
    case PIXMAN_x2b10g10r10:
    case PIXMAN_a2r10g10b10:
#if PIXMAN_VERSION >= PIXMAN_VERSION_ENCODE(0,22,0)
    case PIXMAN_x14r6g6b6:
#endif
    default:
	return COMAC_FORMAT_INVALID;
    }

    return COMAC_FORMAT_INVALID;
}

comac_content_t
_comac_content_from_pixman_format (pixman_format_code_t pixman_format)
{
    comac_content_t content;

    content = 0;
    if (PIXMAN_FORMAT_RGB (pixman_format))
	content |= COMAC_CONTENT_COLOR;
    if (PIXMAN_FORMAT_A (pixman_format))
	content |= COMAC_CONTENT_ALPHA;

    return content;
}

void
_comac_image_surface_init (comac_image_surface_t *surface,
			   pixman_image_t	*pixman_image,
			   pixman_format_code_t	 pixman_format)
{
    surface->parent = NULL;
    surface->pixman_image = pixman_image;

    surface->pixman_format = pixman_format;
    surface->format = _comac_format_from_pixman_format (pixman_format);
    surface->data = (uint8_t *) pixman_image_get_data (pixman_image);
    surface->owns_data = FALSE;
    surface->transparency = COMAC_IMAGE_UNKNOWN;
    surface->color = COMAC_IMAGE_UNKNOWN_COLOR;

    surface->width = pixman_image_get_width (pixman_image);
    surface->height = pixman_image_get_height (pixman_image);
    surface->stride = pixman_image_get_stride (pixman_image);
    surface->depth = pixman_image_get_depth (pixman_image);

    surface->base.is_clear = surface->width == 0 || surface->height == 0;

    surface->compositor = _comac_image_spans_compositor_get ();
}

comac_surface_t *
_comac_image_surface_create_for_pixman_image (pixman_image_t		*pixman_image,
					      pixman_format_code_t	 pixman_format)
{
    comac_image_surface_t *surface;

    surface = _comac_malloc (sizeof (comac_image_surface_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_surface_init (&surface->base,
			 &_comac_image_surface_backend,
			 NULL, /* device */
			 _comac_content_from_pixman_format (pixman_format),
			 FALSE); /* is_vector */

    _comac_image_surface_init (surface, pixman_image, pixman_format);

    return &surface->base;
}

comac_bool_t
_pixman_format_from_masks (comac_format_masks_t *masks,
			   pixman_format_code_t *format_ret)
{
    pixman_format_code_t format;
    int format_type;
    int a, r, g, b;
    comac_format_masks_t format_masks;

    a = _comac_popcount (masks->alpha_mask);
    r = _comac_popcount (masks->red_mask);
    g = _comac_popcount (masks->green_mask);
    b = _comac_popcount (masks->blue_mask);

    if (masks->red_mask) {
	if (masks->red_mask > masks->blue_mask)
	    format_type = PIXMAN_TYPE_ARGB;
	else
	    format_type = PIXMAN_TYPE_ABGR;
    } else if (masks->alpha_mask) {
	format_type = PIXMAN_TYPE_A;
    } else {
	return FALSE;
    }

    format = PIXMAN_FORMAT (masks->bpp, format_type, a, r, g, b);

    if (! pixman_format_supported_destination (format))
	return FALSE;

    /* Sanity check that we got out of PIXMAN_FORMAT exactly what we
     * expected. This avoid any problems from something bizarre like
     * alpha in the least-significant bits, or insane channel order,
     * or whatever. */
     if (!_pixman_format_to_masks (format, &format_masks) ||
         masks->bpp        != format_masks.bpp            ||
	 masks->red_mask   != format_masks.red_mask       ||
	 masks->green_mask != format_masks.green_mask     ||
	 masks->blue_mask  != format_masks.blue_mask)
     {
	 return FALSE;
     }

    *format_ret = format;
    return TRUE;
}

/* A mask consisting of N bits set to 1. */
#define MASK(N) ((1UL << (N))-1)

comac_bool_t
_pixman_format_to_masks (pixman_format_code_t	 format,
			 comac_format_masks_t	*masks)
{
    int a, r, g, b;

    masks->bpp = PIXMAN_FORMAT_BPP (format);

    /* Number of bits in each channel */
    a = PIXMAN_FORMAT_A (format);
    r = PIXMAN_FORMAT_R (format);
    g = PIXMAN_FORMAT_G (format);
    b = PIXMAN_FORMAT_B (format);

    switch (PIXMAN_FORMAT_TYPE (format)) {
    case PIXMAN_TYPE_ARGB:
        masks->alpha_mask = MASK (a) << (r + g + b);
        masks->red_mask   = MASK (r) << (g + b);
        masks->green_mask = MASK (g) << (b);
        masks->blue_mask  = MASK (b);
        return TRUE;
    case PIXMAN_TYPE_ABGR:
        masks->alpha_mask = MASK (a) << (b + g + r);
        masks->blue_mask  = MASK (b) << (g + r);
        masks->green_mask = MASK (g) << (r);
        masks->red_mask   = MASK (r);
        return TRUE;
#ifdef PIXMAN_TYPE_BGRA
    case PIXMAN_TYPE_BGRA:
        masks->blue_mask  = MASK (b) << (masks->bpp - b);
        masks->green_mask = MASK (g) << (masks->bpp - b - g);
        masks->red_mask   = MASK (r) << (masks->bpp - b - g - r);
        masks->alpha_mask = MASK (a);
        return TRUE;
#endif
    case PIXMAN_TYPE_A:
        masks->alpha_mask = MASK (a);
        masks->red_mask   = 0;
        masks->green_mask = 0;
        masks->blue_mask  = 0;
        return TRUE;
    case PIXMAN_TYPE_OTHER:
    case PIXMAN_TYPE_COLOR:
    case PIXMAN_TYPE_GRAY:
    case PIXMAN_TYPE_YUY2:
    case PIXMAN_TYPE_YV12:
    default:
        masks->alpha_mask = 0;
        masks->red_mask   = 0;
        masks->green_mask = 0;
        masks->blue_mask  = 0;
        return FALSE;
    }
}

pixman_format_code_t
_comac_format_to_pixman_format_code (comac_format_t format)
{
    pixman_format_code_t ret;
    switch (format) {
    case COMAC_FORMAT_A1:
	ret = PIXMAN_a1;
	break;
    case COMAC_FORMAT_A8:
	ret = PIXMAN_a8;
	break;
    case COMAC_FORMAT_RGB24:
	ret = PIXMAN_x8r8g8b8;
	break;
    case COMAC_FORMAT_RGB30:
	ret = PIXMAN_x2r10g10b10;
	break;
    case COMAC_FORMAT_RGB16_565:
	ret = PIXMAN_r5g6b5;
	break;
    case COMAC_FORMAT_RGB96F:
	ret = PIXMAN_rgb_float;
	break;
    case COMAC_FORMAT_RGBA128F:
	ret = PIXMAN_rgba_float;
	break;
    case COMAC_FORMAT_ARGB32:
    case COMAC_FORMAT_INVALID:
    default:
	ret = PIXMAN_a8r8g8b8;
	break;
    }
    return ret;
}

comac_surface_t *
_comac_image_surface_create_with_pixman_format (unsigned char		*data,
						pixman_format_code_t	 pixman_format,
						int			 width,
						int			 height,
						int			 stride)
{
    comac_surface_t *surface;
    pixman_image_t *pixman_image;

    if (! _comac_image_surface_is_size_valid (width, height))
    {
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_SIZE));
    }

    pixman_image = pixman_image_create_bits (pixman_format, width, height,
					     (uint32_t *) data, stride);

    if (unlikely (pixman_image == NULL))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_NO_MEMORY));

    surface = _comac_image_surface_create_for_pixman_image (pixman_image,
							    pixman_format);
    if (unlikely (surface->status)) {
	pixman_image_unref (pixman_image);
	return surface;
    }

    /* we can not make any assumptions about the initial state of user data */
    surface->is_clear = data == NULL;
    return surface;
}

/**
 * comac_image_surface_create:
 * @format: format of pixels in the surface to create
 * @width: width of the surface, in pixels
 * @height: height of the surface, in pixels
 *
 * Creates an image surface of the specified format and
 * dimensions. Initially the surface contents are set to 0.
 * (Specifically, within each pixel, each color or alpha channel
 * belonging to format will be 0. The contents of bits within a pixel,
 * but not belonging to the given format are undefined).
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if an error such as out of memory
 * occurs. You can use comac_surface_status() to check for this.
 *
 * Since: 1.0
 **/
comac_surface_t *
comac_image_surface_create (comac_format_t	format,
			    int			width,
			    int			height)
{
    pixman_format_code_t pixman_format;

    if (! COMAC_FORMAT_VALID (format))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_FORMAT));

    pixman_format = _comac_format_to_pixman_format_code (format);

    return _comac_image_surface_create_with_pixman_format (NULL, pixman_format,
							   width, height, -1);
}

    comac_surface_t *
_comac_image_surface_create_with_content (comac_content_t	content,
					  int			width,
					  int			height)
{
    return comac_image_surface_create (_comac_format_from_content (content),
				       width, height);
}

/**
 * comac_format_stride_for_width:
 * @format: A #comac_format_t value
 * @width: The desired width of an image surface to be created.
 *
 * This function provides a stride value that will respect all
 * alignment requirements of the accelerated image-rendering code
 * within comac. Typical usage will be of the form:
 *
 * <informalexample><programlisting>
 * int stride;
 * unsigned char *data;
 * comac_surface_t *surface;
 *
 * stride = comac_format_stride_for_width (format, width);
 * data = malloc (stride * height);
 * surface = comac_image_surface_create_for_data (data, format,
 *						  width, height,
 *						  stride);
 * </programlisting></informalexample>
 *
 * Return value: the appropriate stride to use given the desired
 * format and width, or -1 if either the format is invalid or the width
 * too large.
 *
 * Since: 1.6
 **/
    int
comac_format_stride_for_width (comac_format_t	format,
			       int		width)
{
    int bpp;

    if (! COMAC_FORMAT_VALID (format)) {
	_comac_error_throw (COMAC_STATUS_INVALID_FORMAT);
	return -1;
    }

    bpp = _comac_format_bits_per_pixel (format);
    if ((unsigned) (width) >= (INT32_MAX - 7) / (unsigned) (bpp))
	return -1;

    return COMAC_STRIDE_FOR_WIDTH_BPP (width, bpp);
}

/**
 * comac_image_surface_create_for_data:
 * @data: a pointer to a buffer supplied by the application in which
 *     to write contents. This pointer must be suitably aligned for any
 *     kind of variable, (for example, a pointer returned by malloc).
 * @format: the format of pixels in the buffer
 * @width: the width of the image to be stored in the buffer
 * @height: the height of the image to be stored in the buffer
 * @stride: the number of bytes between the start of rows in the
 *     buffer as allocated. This value should always be computed by
 *     comac_format_stride_for_width() before allocating the data
 *     buffer.
 *
 * Creates an image surface for the provided pixel data. The output
 * buffer must be kept around until the #comac_surface_t is destroyed
 * or comac_surface_finish() is called on the surface.  The initial
 * contents of @data will be used as the initial image contents; you
 * must explicitly clear the buffer, using, for example,
 * comac_rectangle() and comac_fill() if you want it cleared.
 *
 * Note that the stride may be larger than
 * width*bytes_per_pixel to provide proper alignment for each pixel
 * and row. This alignment is required to allow high-performance rendering
 * within comac. The correct way to obtain a legal stride value is to
 * call comac_format_stride_for_width() with the desired format and
 * maximum image width value, and then use the resulting stride value
 * to allocate the data and to create the image surface. See
 * comac_format_stride_for_width() for example code.
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface in the case of an error such as out of
 * memory or an invalid stride value. In case of invalid stride value
 * the error status of the returned surface will be
 * %COMAC_STATUS_INVALID_STRIDE.  You can use
 * comac_surface_status() to check for this.
 *
 * See comac_surface_set_user_data() for a means of attaching a
 * destroy-notification fallback to the surface if necessary.
 *
 * Since: 1.0
 **/
    comac_surface_t *
comac_image_surface_create_for_data (unsigned char     *data,
				     comac_format_t	format,
				     int		width,
				     int		height,
				     int		stride)
{
    pixman_format_code_t pixman_format;
    int minstride;

    if (! COMAC_FORMAT_VALID (format))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_FORMAT));

    if ((stride & (COMAC_STRIDE_ALIGNMENT-1)) != 0)
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_STRIDE));

    if (! _comac_image_surface_is_size_valid (width, height))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_SIZE));

    minstride = comac_format_stride_for_width (format, width);
    if (stride < 0) {
	if (stride > -minstride) {
	    return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_STRIDE));
	}
    } else {
	if (stride < minstride) {
	    return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_STRIDE));
	}
    }

    pixman_format = _comac_format_to_pixman_format_code (format);
    return _comac_image_surface_create_with_pixman_format (data,
							   pixman_format,
							   width, height,
							   stride);
}

/**
 * comac_image_surface_get_data:
 * @surface: a #comac_image_surface_t
 *
 * Get a pointer to the data of the image surface, for direct
 * inspection or modification.
 *
 * A call to comac_surface_flush() is required before accessing the
 * pixel data to ensure that all pending drawing operations are
 * finished. A call to comac_surface_mark_dirty() is required after
 * the data is modified.
 *
 * Return value: a pointer to the image data of this surface or %NULL
 * if @surface is not an image surface, or if comac_surface_finish()
 * has been called.
 *
 * Since: 1.2
 **/
unsigned char *
comac_image_surface_get_data (comac_surface_t *surface)
{
    comac_image_surface_t *image_surface = (comac_image_surface_t *) surface;

    if (! _comac_surface_is_image (surface)) {
	_comac_error_throw (COMAC_STATUS_SURFACE_TYPE_MISMATCH);
	return NULL;
    }

    return image_surface->data;
}

/**
 * comac_image_surface_get_format:
 * @surface: a #comac_image_surface_t
 *
 * Get the format of the surface.
 *
 * Return value: the format of the surface
 *
 * Since: 1.2
 **/
comac_format_t
comac_image_surface_get_format (comac_surface_t *surface)
{
    comac_image_surface_t *image_surface = (comac_image_surface_t *) surface;

    if (! _comac_surface_is_image (surface)) {
	_comac_error_throw (COMAC_STATUS_SURFACE_TYPE_MISMATCH);
	return COMAC_FORMAT_INVALID;
    }

    return image_surface->format;
}

/**
 * comac_image_surface_get_width:
 * @surface: a #comac_image_surface_t
 *
 * Get the width of the image surface in pixels.
 *
 * Return value: the width of the surface in pixels.
 *
 * Since: 1.0
 **/
int
comac_image_surface_get_width (comac_surface_t *surface)
{
    comac_image_surface_t *image_surface = (comac_image_surface_t *) surface;

    if (! _comac_surface_is_image (surface)) {
	_comac_error_throw (COMAC_STATUS_SURFACE_TYPE_MISMATCH);
	return 0;
    }

    return image_surface->width;
}

/**
 * comac_image_surface_get_height:
 * @surface: a #comac_image_surface_t
 *
 * Get the height of the image surface in pixels.
 *
 * Return value: the height of the surface in pixels.
 *
 * Since: 1.0
 **/
int
comac_image_surface_get_height (comac_surface_t *surface)
{
    comac_image_surface_t *image_surface = (comac_image_surface_t *) surface;

    if (! _comac_surface_is_image (surface)) {
	_comac_error_throw (COMAC_STATUS_SURFACE_TYPE_MISMATCH);
	return 0;
    }

    return image_surface->height;
}

/**
 * comac_image_surface_get_stride:
 * @surface: a #comac_image_surface_t
 *
 * Get the stride of the image surface in bytes
 *
 * Return value: the stride of the image surface in bytes (or 0 if
 * @surface is not an image surface). The stride is the distance in
 * bytes from the beginning of one row of the image data to the
 * beginning of the next row.
 *
 * Since: 1.2
 **/
int
comac_image_surface_get_stride (comac_surface_t *surface)
{

    comac_image_surface_t *image_surface = (comac_image_surface_t *) surface;

    if (! _comac_surface_is_image (surface)) {
	_comac_error_throw (COMAC_STATUS_SURFACE_TYPE_MISMATCH);
	return 0;
    }

    return image_surface->stride;
}

    comac_format_t
_comac_format_from_content (comac_content_t content)
{
    switch (content) {
    case COMAC_CONTENT_COLOR:
	return COMAC_FORMAT_RGB24;
    case COMAC_CONTENT_ALPHA:
	return COMAC_FORMAT_A8;
    case COMAC_CONTENT_COLOR_ALPHA:
	return COMAC_FORMAT_ARGB32;
    }

    ASSERT_NOT_REACHED;
    return COMAC_FORMAT_INVALID;
}

    comac_content_t
_comac_content_from_format (comac_format_t format)
{
    switch (format) {
    case COMAC_FORMAT_RGBA128F:
    case COMAC_FORMAT_ARGB32:
	return COMAC_CONTENT_COLOR_ALPHA;
    case COMAC_FORMAT_RGB96F:
    case COMAC_FORMAT_RGB30:
	return COMAC_CONTENT_COLOR;
    case COMAC_FORMAT_RGB24:
	return COMAC_CONTENT_COLOR;
    case COMAC_FORMAT_RGB16_565:
	return COMAC_CONTENT_COLOR;
    case COMAC_FORMAT_A8:
    case COMAC_FORMAT_A1:
	return COMAC_CONTENT_ALPHA;
    case COMAC_FORMAT_INVALID:
	break;
    }

    ASSERT_NOT_REACHED;
    return COMAC_CONTENT_COLOR_ALPHA;
}

    int
_comac_format_bits_per_pixel (comac_format_t format)
{
    switch (format) {
    case COMAC_FORMAT_RGBA128F:
	return 128;
    case COMAC_FORMAT_RGB96F:
	return 96;
    case COMAC_FORMAT_ARGB32:
    case COMAC_FORMAT_RGB30:
    case COMAC_FORMAT_RGB24:
	return 32;
    case COMAC_FORMAT_RGB16_565:
	return 16;
    case COMAC_FORMAT_A8:
	return 8;
    case COMAC_FORMAT_A1:
	return 1;
    case COMAC_FORMAT_INVALID:
    default:
	ASSERT_NOT_REACHED;
	return 0;
    }
}

comac_surface_t *
_comac_image_surface_create_similar (void	       *abstract_other,
				     comac_content_t	content,
				     int		width,
				     int		height)
{
    comac_image_surface_t *other = abstract_other;

    TRACE ((stderr, "%s (other=%u)\n", __FUNCTION__, other->base.unique_id));

    if (! _comac_image_surface_is_size_valid (width, height))
	return _comac_surface_create_in_error (_comac_error (COMAC_STATUS_INVALID_SIZE));

    if (content == other->base.content) {
	return _comac_image_surface_create_with_pixman_format (NULL,
							       other->pixman_format,
							       width, height,
							       0);
    }

    return _comac_image_surface_create_with_content (content,
						     width, height);
}

comac_surface_t *
_comac_image_surface_snapshot (void *abstract_surface)
{
    comac_image_surface_t *image = abstract_surface;
    comac_image_surface_t *clone;

    /* If we own the image, we can simply steal the memory for the snapshot */
    if (image->owns_data && image->base._finishing) {
	clone = (comac_image_surface_t *)
	    _comac_image_surface_create_for_pixman_image (image->pixman_image,
							  image->pixman_format);
	if (unlikely (clone->base.status))
	    return &clone->base;

	image->pixman_image = NULL;
	image->owns_data = FALSE;

	clone->transparency = image->transparency;
	clone->color = image->color;

	clone->owns_data = TRUE;
	return &clone->base;
    }

    clone = (comac_image_surface_t *)
	_comac_image_surface_create_with_pixman_format (NULL,
							image->pixman_format,
							image->width,
							image->height,
							0);
    if (unlikely (clone->base.status))
	return &clone->base;

    if (clone->stride == image->stride) {
	memcpy (clone->data, image->data, clone->stride * clone->height);
    } else {
	pixman_image_composite32 (PIXMAN_OP_SRC,
				  image->pixman_image, NULL, clone->pixman_image,
				  0, 0,
				  0, 0,
				  0, 0,
				  image->width, image->height);
    }
    clone->base.is_clear = FALSE;
    return &clone->base;
}

comac_image_surface_t *
_comac_image_surface_map_to_image (void *abstract_other,
				   const comac_rectangle_int_t *extents)
{
    comac_image_surface_t *other = abstract_other;
    comac_surface_t *surface;
    uint8_t *data;

    data = other->data;
    data += extents->y * other->stride;
    data += extents->x * PIXMAN_FORMAT_BPP (other->pixman_format)/ 8;

    surface =
	_comac_image_surface_create_with_pixman_format (data,
							other->pixman_format,
							extents->width,
							extents->height,
							other->stride);

    comac_surface_set_device_offset (surface, -extents->x, -extents->y);
    return (comac_image_surface_t *) surface;
}

comac_int_status_t
_comac_image_surface_unmap_image (void *abstract_surface,
				  comac_image_surface_t *image)
{
    comac_surface_finish (&image->base);
    comac_surface_destroy (&image->base);

    return COMAC_INT_STATUS_SUCCESS;
}

comac_status_t
_comac_image_surface_finish (void *abstract_surface)
{
    comac_image_surface_t *surface = abstract_surface;

    if (surface->pixman_image) {
	pixman_image_unref (surface->pixman_image);
	surface->pixman_image = NULL;
    }

    if (surface->owns_data) {
	free (surface->data);
	surface->data = NULL;
    }

    if (surface->parent) {
	comac_surface_t *parent = surface->parent;
	surface->parent = NULL;
	comac_surface_destroy (parent);
    }

    return COMAC_STATUS_SUCCESS;
}

void
_comac_image_surface_assume_ownership_of_data (comac_image_surface_t *surface)
{
    surface->owns_data = TRUE;
}

comac_surface_t *
_comac_image_surface_source (void			*abstract_surface,
			     comac_rectangle_int_t	*extents)
{
    comac_image_surface_t *surface = abstract_surface;

    if (extents) {
	extents->x = extents->y = 0;
	extents->width = surface->width;
	extents->height = surface->height;
    }

    return &surface->base;
}

comac_status_t
_comac_image_surface_acquire_source_image (void                    *abstract_surface,
					   comac_image_surface_t  **image_out,
					   void                   **image_extra)
{
    *image_out = abstract_surface;
    *image_extra = NULL;

    return COMAC_STATUS_SUCCESS;
}

void
_comac_image_surface_release_source_image (void                   *abstract_surface,
					   comac_image_surface_t  *image,
					   void                   *image_extra)
{
}

/* high level image interface */
comac_bool_t
_comac_image_surface_get_extents (void			  *abstract_surface,
				  comac_rectangle_int_t   *rectangle)
{
    comac_image_surface_t *surface = abstract_surface;

    rectangle->x = 0;
    rectangle->y = 0;
    rectangle->width  = surface->width;
    rectangle->height = surface->height;

    return TRUE;
}

comac_int_status_t
_comac_image_surface_paint (void			*abstract_surface,
			    comac_operator_t		 op,
			    const comac_pattern_t	*source,
			    const comac_clip_t		*clip)
{
    comac_image_surface_t *surface = abstract_surface;

    TRACE ((stderr, "%s (surface=%d)\n",
	    __FUNCTION__, surface->base.unique_id));

    return _comac_compositor_paint (surface->compositor,
				    &surface->base, op, source, clip);
}

comac_int_status_t
_comac_image_surface_mask (void				*abstract_surface,
			   comac_operator_t		 op,
			   const comac_pattern_t	*source,
			   const comac_pattern_t	*mask,
			   const comac_clip_t		*clip)
{
    comac_image_surface_t *surface = abstract_surface;

    TRACE ((stderr, "%s (surface=%d)\n",
	    __FUNCTION__, surface->base.unique_id));

    return _comac_compositor_mask (surface->compositor,
				   &surface->base, op, source, mask, clip);
}

comac_int_status_t
_comac_image_surface_stroke (void			*abstract_surface,
			     comac_operator_t		 op,
			     const comac_pattern_t	*source,
			     const comac_path_fixed_t	*path,
			     const comac_stroke_style_t	*style,
			     const comac_matrix_t	*ctm,
			     const comac_matrix_t	*ctm_inverse,
			     double			 tolerance,
			     comac_antialias_t		 antialias,
			     const comac_clip_t		*clip)
{
    comac_image_surface_t *surface = abstract_surface;

    TRACE ((stderr, "%s (surface=%d)\n",
	    __FUNCTION__, surface->base.unique_id));

    return _comac_compositor_stroke (surface->compositor, &surface->base,
				     op, source, path,
				     style, ctm, ctm_inverse,
				     tolerance, antialias, clip);
}

comac_int_status_t
_comac_image_surface_fill (void				*abstract_surface,
			   comac_operator_t		 op,
			   const comac_pattern_t	*source,
			   const comac_path_fixed_t	*path,
			   comac_fill_rule_t		 fill_rule,
			   double			 tolerance,
			   comac_antialias_t		 antialias,
			   const comac_clip_t		*clip)
{
    comac_image_surface_t *surface = abstract_surface;

    TRACE ((stderr, "%s (surface=%d)\n",
	    __FUNCTION__, surface->base.unique_id));

    return _comac_compositor_fill (surface->compositor, &surface->base,
				   op, source, path,
				   fill_rule, tolerance, antialias,
				   clip);
}

comac_int_status_t
_comac_image_surface_glyphs (void			*abstract_surface,
			     comac_operator_t		 op,
			     const comac_pattern_t	*source,
			     comac_glyph_t		*glyphs,
			     int			 num_glyphs,
			     comac_scaled_font_t	*scaled_font,
			     const comac_clip_t		*clip)
{
    comac_image_surface_t *surface = abstract_surface;

    TRACE ((stderr, "%s (surface=%d)\n",
	    __FUNCTION__, surface->base.unique_id));

    return _comac_compositor_glyphs (surface->compositor, &surface->base,
				     op, source,
				     glyphs, num_glyphs, scaled_font,
				     clip);
}

void
_comac_image_surface_get_font_options (void                  *abstract_surface,
				       comac_font_options_t  *options)
{
    _comac_font_options_init_default (options);

    comac_font_options_set_hint_metrics (options, COMAC_HINT_METRICS_ON);
    _comac_font_options_set_round_glyph_positions (options, COMAC_ROUND_GLYPH_POS_ON);
}

const comac_surface_backend_t _comac_image_surface_backend = {
    COMAC_SURFACE_TYPE_IMAGE,
    _comac_image_surface_finish,

    _comac_default_context_create,

    _comac_image_surface_create_similar,
    NULL, /* create similar image */
    _comac_image_surface_map_to_image,
    _comac_image_surface_unmap_image,

    _comac_image_surface_source,
    _comac_image_surface_acquire_source_image,
    _comac_image_surface_release_source_image,
    _comac_image_surface_snapshot,

    NULL, /* copy_page */
    NULL, /* show_page */

    _comac_image_surface_get_extents,
    _comac_image_surface_get_font_options,

    NULL, /* flush */
    NULL,

    _comac_image_surface_paint,
    _comac_image_surface_mask,
    _comac_image_surface_stroke,
    _comac_image_surface_fill,
    NULL, /* fill-stroke */
    _comac_image_surface_glyphs,
};

/* A convenience function for when one needs to coerce an image
 * surface to an alternate format. */
comac_image_surface_t *
_comac_image_surface_coerce (comac_image_surface_t *surface)
{
    return _comac_image_surface_coerce_to_format (surface,
		                                  _comac_format_from_content (surface->base.content));
}

/* A convenience function for when one needs to coerce an image
 * surface to an alternate format. */
comac_image_surface_t *
_comac_image_surface_coerce_to_format (comac_image_surface_t *surface,
			               comac_format_t	      format)
{
    comac_image_surface_t *clone;
    comac_status_t status;

    status = surface->base.status;
    if (unlikely (status))
	return (comac_image_surface_t *)_comac_surface_create_in_error (status);

    if (surface->format == format)
	return (comac_image_surface_t *)comac_surface_reference(&surface->base);

    clone = (comac_image_surface_t *)
	comac_image_surface_create (format, surface->width, surface->height);
    if (unlikely (clone->base.status))
	return clone;

    pixman_image_composite32 (PIXMAN_OP_SRC,
                              surface->pixman_image, NULL, clone->pixman_image,
                              0, 0,
                              0, 0,
                              0, 0,
                              surface->width, surface->height);
    clone->base.is_clear = FALSE;

    clone->base.device_transform =
	surface->base.device_transform;
    clone->base.device_transform_inverse =
	surface->base.device_transform_inverse;

    return clone;
}

comac_image_surface_t *
_comac_image_surface_create_from_image (comac_image_surface_t *other,
					pixman_format_code_t format,
					int x, int y,
					int width, int height, int stride)
{
    comac_image_surface_t *surface;
    comac_status_t status;
    pixman_image_t *image;
    void *mem = NULL;

    status = other->base.status;
    if (unlikely (status))
	goto cleanup;

    if (stride) {
	mem = _comac_malloc_ab (height, stride);
	if (unlikely (mem == NULL)) {
	    status = _comac_error (COMAC_STATUS_NO_MEMORY);
	    goto cleanup;
	}
    }

    image = pixman_image_create_bits (format, width, height, mem, stride);
    if (unlikely (image == NULL)) {
	status = _comac_error (COMAC_STATUS_NO_MEMORY);
	goto cleanup_mem;
    }

    surface = (comac_image_surface_t *)
	_comac_image_surface_create_for_pixman_image (image, format);
    if (unlikely (surface->base.status)) {
	status = surface->base.status;
	goto cleanup_image;
    }

    pixman_image_composite32 (PIXMAN_OP_SRC,
                              other->pixman_image, NULL, image,
                              x, y,
                              0, 0,
                              0, 0,
                              width, height);
    surface->base.is_clear = FALSE;
    surface->owns_data = mem != NULL;

    return surface;

cleanup_image:
    pixman_image_unref (image);
cleanup_mem:
    free (mem);
cleanup:
    return (comac_image_surface_t *) _comac_surface_create_in_error (status);
}

static comac_image_transparency_t
_comac_image_compute_transparency (comac_image_surface_t *image)
{
    int x, y;
    comac_image_transparency_t transparency;

    if ((image->base.content & COMAC_CONTENT_ALPHA) == 0)
	return COMAC_IMAGE_IS_OPAQUE;

    if (image->base.is_clear)
	return COMAC_IMAGE_HAS_BILEVEL_ALPHA;

    if ((image->base.content & COMAC_CONTENT_COLOR) == 0) {
	if (image->format == COMAC_FORMAT_A1) {
	    return COMAC_IMAGE_HAS_BILEVEL_ALPHA;
	} else if (image->format == COMAC_FORMAT_A8) {
	    for (y = 0; y < image->height; y++) {
		uint8_t *alpha = (uint8_t *) (image->data + y * image->stride);

		for (x = 0; x < image->width; x++, alpha++) {
		    if (*alpha > 0 && *alpha < 255)
			return COMAC_IMAGE_HAS_ALPHA;
		}
	    }
	    return COMAC_IMAGE_HAS_BILEVEL_ALPHA;
	} else {
	    return COMAC_IMAGE_HAS_ALPHA;
	}
    }

    if (image->format == COMAC_FORMAT_RGB16_565) {
	return COMAC_IMAGE_IS_OPAQUE;
    }

    if (image->format != COMAC_FORMAT_ARGB32)
	return COMAC_IMAGE_HAS_ALPHA;

    transparency = COMAC_IMAGE_IS_OPAQUE;
    for (y = 0; y < image->height; y++) {
	uint32_t *pixel = (uint32_t *) (image->data + y * image->stride);

	for (x = 0; x < image->width; x++, pixel++) {
	    int a = (*pixel & 0xff000000) >> 24;
	    if (a > 0 && a < 255) {
		return COMAC_IMAGE_HAS_ALPHA;
	    } else if (a == 0) {
		transparency = COMAC_IMAGE_HAS_BILEVEL_ALPHA;
	    }
	}
    }

    return transparency;
}

comac_image_transparency_t
_comac_image_analyze_transparency (comac_image_surface_t *image)
{
    if (_comac_surface_is_snapshot (&image->base)) {
	if (image->transparency == COMAC_IMAGE_UNKNOWN)
	    image->transparency = _comac_image_compute_transparency (image);

	return image->transparency;
    }

    return _comac_image_compute_transparency (image);
}

static comac_image_color_t
_comac_image_compute_color (comac_image_surface_t      *image)
{
    int x, y;
    comac_image_color_t color;

    if (image->width == 0 || image->height == 0)
	return COMAC_IMAGE_IS_MONOCHROME;

    if (image->format == COMAC_FORMAT_A1)
	return COMAC_IMAGE_IS_MONOCHROME;

    if (image->format == COMAC_FORMAT_A8)
	return COMAC_IMAGE_IS_GRAYSCALE;

    if (image->format == COMAC_FORMAT_ARGB32) {
	color = COMAC_IMAGE_IS_MONOCHROME;
	for (y = 0; y < image->height; y++) {
	    uint32_t *pixel = (uint32_t *) (image->data + y * image->stride);

	    for (x = 0; x < image->width; x++, pixel++) {
		int a = (*pixel & 0xff000000) >> 24;
		int r = (*pixel & 0x00ff0000) >> 16;
		int g = (*pixel & 0x0000ff00) >> 8;
		int b = (*pixel & 0x000000ff);
		if (a == 0) {
		    r = g = b = 0;
		} else {
		    r = (r * 255 + a / 2) / a;
		    g = (g * 255 + a / 2) / a;
		    b = (b * 255 + a / 2) / a;
		}
		if (!(r == g && g == b))
		    return COMAC_IMAGE_IS_COLOR;
		else if (r > 0 && r < 255)
		    color = COMAC_IMAGE_IS_GRAYSCALE;
	    }
	}
	return color;
    }

    if (image->format == COMAC_FORMAT_RGB24) {
	color = COMAC_IMAGE_IS_MONOCHROME;
	for (y = 0; y < image->height; y++) {
	    uint32_t *pixel = (uint32_t *) (image->data + y * image->stride);

	    for (x = 0; x < image->width; x++, pixel++) {
		int r = (*pixel & 0x00ff0000) >> 16;
		int g = (*pixel & 0x0000ff00) >>  8;
		int b = (*pixel & 0x000000ff);
		if (!(r == g && g == b))
		    return COMAC_IMAGE_IS_COLOR;
		else if (r > 0 && r < 255)
		    color = COMAC_IMAGE_IS_GRAYSCALE;
	    }
	}
	return color;
    }

    return COMAC_IMAGE_IS_COLOR;
}

comac_image_color_t
_comac_image_analyze_color (comac_image_surface_t      *image)
{
    if (_comac_surface_is_snapshot (&image->base)) {
	if (image->color == COMAC_IMAGE_UNKNOWN_COLOR)
	    image->color = _comac_image_compute_color (image);

	return image->color;
    }

    return _comac_image_compute_color (image);
}

comac_image_surface_t *
_comac_image_surface_clone_subimage (comac_surface_t             *surface,
				     const comac_rectangle_int_t *extents)
{
    comac_surface_t *image;
    comac_surface_pattern_t pattern;
    comac_status_t status;

    image = comac_surface_create_similar_image (surface,
						_comac_format_from_content (surface->content),
						extents->width,
						extents->height);
    if (image->status)
	return to_image_surface (image);

    /* TODO: check me with non-identity device_transform. Should we
     * clone the scaling, too? */
    comac_surface_set_device_offset (image,
				     -extents->x,
				     -extents->y);

    _comac_pattern_init_for_surface (&pattern, surface);
    pattern.base.filter = COMAC_FILTER_NEAREST;

    status = _comac_surface_paint (image,
				   COMAC_OPERATOR_SOURCE,
				   &pattern.base,
				   NULL);

    _comac_pattern_fini (&pattern.base);

    if (unlikely (status))
	goto error;

    /* We use the parent as a flag during map-to-image/umap-image that the
     * resultant image came from a fallback rather than as direct call
     * to the backend's map_to_image(). Whilst we use it as a simple flag,
     * we need to make sure the parent surface obeys the reference counting
     * semantics and is consistent for all callers.
     */
    _comac_image_surface_set_parent (to_image_surface (image),
				     comac_surface_reference (surface));

    return to_image_surface (image);

error:
    comac_surface_destroy (image);
    return to_image_surface (_comac_surface_create_in_error (status));
}
