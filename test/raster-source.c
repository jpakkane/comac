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

#include <stdio.h>
#include <errno.h>

/* Basic test to exercise the new mime-surface callback. */

#define WIDTH 200
#define HEIGHT 80

static char *png_filename = NULL;

/* Lazy way of determining PNG dimensions... */
static void
png_dimensions (const char *filename,
		comac_content_t *content, int *width, int *height)
{
    comac_surface_t *surface;

    surface = comac_image_surface_create_from_png (filename);
    *content = comac_surface_get_content (surface);
    *width = comac_image_surface_get_width (surface);
    *height = comac_image_surface_get_height (surface);
    comac_surface_destroy (surface);
}

static comac_surface_t *
png_acquire (comac_pattern_t *pattern, void *closure,
	     comac_surface_t *target,
	     const comac_rectangle_int_t *extents)
{
    return comac_image_surface_create_from_png (closure);
}

static comac_surface_t *
red_acquire (comac_pattern_t *pattern, void *closure,
	     comac_surface_t *target,
	     const comac_rectangle_int_t *extents)
{
    comac_surface_t *image;
    comac_t *cr;

    image = comac_surface_create_similar_image (target,
						COMAC_FORMAT_RGB24,
						extents->width,
						extents->height);
    comac_surface_set_device_offset (image, extents->x, extents->y);

    cr = comac_create (image);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);
    comac_destroy (cr);

    return image;
}

static void
release (comac_pattern_t *pattern, void *closure, comac_surface_t *image)
{
    comac_surface_destroy (image);
}

static void
free_filename(void)
{
    free (png_filename);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *png, *red;
    comac_content_t content;
    int png_width, png_height;
    int i, j;

    if (png_filename == NULL) {
      const comac_test_context_t *ctx = comac_test_get_context (cr);
      xasprintf (&png_filename, "%s/png.png", ctx->srcdir);
      atexit (free_filename);
    }

    png_dimensions (png_filename, &content, &png_width, &png_height);

    png = comac_pattern_create_raster_source ((void*)png_filename,
					      content, png_width, png_height);
    comac_raster_source_pattern_set_acquire (png, png_acquire, release);

    red = comac_pattern_create_raster_source (NULL,
					      COMAC_CONTENT_COLOR, WIDTH, HEIGHT);
    comac_raster_source_pattern_set_acquire (red, red_acquire, release);

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_paint (cr);

    comac_translate (cr, 0, (HEIGHT-png_height)/2);
    for (i = 0; i < 4; i++) {
	for (j = 0; j < 4; j++) {
	    comac_pattern_t *source;
	    if ((i ^ j) & 1)
		source = red;
	    else
		source = png;
	    comac_set_source (cr, source);
	    comac_rectangle (cr, i * WIDTH/4, j * png_height/4, WIDTH/4, png_height/4);
	    comac_fill (cr);
	}
    }

    comac_pattern_destroy (red);
    comac_pattern_destroy (png);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (raster_source,
	    "Check that the mime-surface embedding works",
	    "api", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
