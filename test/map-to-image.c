/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define WIDTH	3
#define HEIGHT	3

/* A single, black pixel */
static const uint32_t black_pixel_argb = 0xff000000;
static const uint32_t black_pixel      = 0x00000000;

static comac_bool_t
set_pixel_black(uint8_t *data, int stride,
	  comac_format_t format, int x, int y)
{
    switch (format) {
    case COMAC_FORMAT_ARGB32:
    case COMAC_FORMAT_RGB24:
	*(uint32_t *)(data + y * stride + 4*x) = black_pixel_argb;
	break;
    case COMAC_FORMAT_RGB16_565:
	*(uint16_t *)(data + y * stride + 2*x) = black_pixel;
	break;
    case COMAC_FORMAT_RGBA128F:
    case COMAC_FORMAT_RGB96F:
    case COMAC_FORMAT_RGB30:
    case COMAC_FORMAT_A8:
    case COMAC_FORMAT_A1:
    case COMAC_FORMAT_INVALID:
    default:
	return FALSE;
    }
    return TRUE;
}

static comac_test_status_t
all (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    uint8_t *data;
    int stride;
    comac_format_t format;
    int i, j;

    /* Fill background white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    surface = comac_surface_map_to_image (comac_get_target (cr), NULL);
    comac_surface_flush (surface);
    format = comac_image_surface_get_format (surface);
    stride = comac_image_surface_get_stride (surface);
    data = comac_image_surface_get_data (surface);
    if (data) {
	for (j = 0; j < HEIGHT; j++)
	    for (i = 0; i < WIDTH; i++)
		if (! set_pixel_black (data, stride, format, i, j))
		    return COMAC_TEST_FAILURE;
    }
    comac_surface_mark_dirty (surface);
    comac_surface_unmap_image (comac_get_target (cr), surface);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
bit (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    comac_rectangle_int_t extents;
    comac_format_t format;
    uint8_t *data;

    extents.x = extents.y = extents.width = extents.height = 1;

    /* Fill background white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    surface = comac_surface_map_to_image (comac_get_target (cr), &extents);
    comac_surface_flush (surface);
    data = comac_image_surface_get_data (surface);
    format = comac_image_surface_get_format (surface);
    if (data) {
	if (! set_pixel_black (data, 0, format, 0, 0))
	    return COMAC_TEST_FAILURE;
    }
    comac_surface_mark_dirty (surface);
    comac_surface_unmap_image (comac_get_target (cr), surface);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
fill (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    comac_rectangle_int_t extents;
    comac_t *cr2;

    extents.x = extents.y = extents.width = extents.height = 1;

    /* Fill background white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    surface = comac_surface_map_to_image (comac_get_target (cr), &extents);
    cr2 = comac_create (surface);
    comac_set_source_rgb (cr2, 1, 0, 0);
    comac_paint (cr2);
    comac_destroy (cr2);
    comac_surface_unmap_image (comac_get_target (cr), surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (map_all_to_image,
	    "Test mapping a surface to an image and modifying it externally",
	    "image", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, all)
COMAC_TEST (map_bit_to_image,
	    "Test mapping a surface to an image and modifying it externally",
	    "image", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, bit)
COMAC_TEST (map_to_image_fill,
	    "Test mapping a surface to an image and modifying it externally",
	    "image", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, fill)
