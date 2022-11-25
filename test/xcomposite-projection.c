/*
 * Copyright 2009 Benjamin Otte
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
 * Author: Benjamin Otte <otte@gnome.org>
 */

#include "comac-test.h"

/*
 * Exercise a bug in the projection of a rotated trapezoid mask.
 * I used COMAC_ANTIALIAS_NONE and a single-color source in the test to get
 * rid of aliasing issues in the output images. This makes some issues
 * slightly less visible, but still fails for all of them. If you want to
 * get a clearer view:
 * #define ANTIALIAS COMAC_ANTIALIAS_DEFAULT
 */

#define ANTIALIAS COMAC_ANTIALIAS_NONE

static const char png_filename[] = "romedalen.png";

static comac_pattern_t *
get_source (const comac_test_context_t *ctx, int *width, int *height)
{
    comac_surface_t *surface;
    comac_pattern_t *pattern;

    if (ANTIALIAS == COMAC_ANTIALIAS_NONE) {
	comac_t *cr;

	surface = comac_image_surface_create (COMAC_FORMAT_RGB24, 256, 192);
	cr = comac_create (surface);

	comac_set_source_rgb (cr, 0.75, 0.25, 0.25);
	comac_paint (cr);

	pattern = comac_pattern_create_for_surface (comac_get_target (cr));
	comac_destroy (cr);
    } else {
	surface = comac_test_create_surface_from_png (ctx, png_filename);
	pattern = comac_pattern_create_for_surface (surface);
    }

    *width = comac_image_surface_get_width (surface);
    *height = comac_image_surface_get_height (surface);
    comac_surface_destroy (surface);

    return pattern;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *image;
    int img_width, img_height;

    image = get_source (comac_test_get_context (cr), &img_width, &img_height);

    /* we don't want to debug antialiasing artifacts */
    comac_set_antialias (cr, ANTIALIAS);

    /* dark grey background */
    comac_set_source_rgb (cr, 0.25, 0.25, 0.25);
    comac_paint (cr);

    /* magic transform */
    comac_translate (cr, 10, -40);
    comac_rotate (cr, -0.05);

    /* place the image on our surface */
    comac_set_source (cr, image);

    /* paint the image */
    comac_rectangle (cr, 0, 0, img_width, img_height);
    comac_fill (cr);

    comac_pattern_destroy (image);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (xcomposite_projection,
	    "Test a bug with XRenderComposite reference computation when "
	    "projecting the first trapezoid onto 16.16 space",
	    "xlib",	     /* keywords */
	    "target=raster", /* requirements */
	    300,
	    150,
	    NULL,
	    draw)
