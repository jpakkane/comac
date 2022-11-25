/*
 * Copyright © 2008 Chris Wilson
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

/*
 * We wish to check the optimization away of non-fractional translations
 * for NEAREST surface patterns under a few transformations.
 */

static const char png_filename[] = "romedalen.png";

/* A single, black pixel */
static const uint32_t black_pixel = 0xff000000;

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    unsigned int i, j, k;
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    const comac_matrix_t transform[] = {
	{  1, 0, 0,  1,  0, 0 },
	{ -1, 0, 0,  1,  8, 0 },
	{  1, 0, 0, -1,  0, 8 },
	{ -1, 0, 0, -1,  8, 8 },
    };
    const comac_matrix_t ctx_transform[] = {
	{  1, 0, 0,  1,   0,  0 },
	{ -1, 0, 0,  1,  14,  0 },
	{  1, 0, 0, -1,   0, 14 },
	{ -1, 0, 0, -1,  14, 14 },
    };
    const double colour[][3] = {
	{0, 0, 0},
	{1, 0, 0},
	{0, 1, 0},
	{0, 0, 1},
    };
    comac_matrix_t m;

    surface = comac_image_surface_create_for_data ((uint8_t *) &black_pixel,
						   COMAC_FORMAT_ARGB32,
						   1, 1, 4);
    pattern = comac_pattern_create_for_surface (surface);
    comac_surface_destroy (surface);

    comac_pattern_set_filter (pattern, COMAC_FILTER_NEAREST);

    surface = comac_test_create_surface_from_png (ctx, png_filename);

    /* Fill background white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    for (k = 0; k < ARRAY_LENGTH (transform); k++) {
	/* draw a "large" section from an image */
	comac_save (cr); {
	    comac_set_matrix(cr, &ctx_transform[k]);
	    comac_rectangle (cr, 0, 0, 7, 7);
	    comac_clip (cr);

	    comac_set_source_surface (cr, surface,
				      -comac_image_surface_get_width (surface)/2.,
				      -comac_image_surface_get_height (surface)/2.);
	    comac_pattern_set_filter (comac_get_source (cr), COMAC_FILTER_NEAREST);
	    comac_paint (cr);
	} comac_restore (cr);

	comac_set_source_rgb (cr, colour[k][0], colour[k][1], colour[k][2]);
	for (j = 4; j <= 6; j++) {
	    for (i = 4; i <= 6; i++) {
		comac_matrix_init_translate (&m,
					     -(2*(i-4) + .1*i),
					     -(2*(j-4) + .1*j));
		comac_matrix_multiply (&m, &m, &transform[k]);
		comac_pattern_set_matrix (pattern, &m);
		comac_mask (cr, pattern);
	    }
	}
    }

    comac_pattern_destroy (pattern);
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (filter_nearest_transformed,
	    "Test sample position when drawing transformed images with FILTER_NEAREST",
	    "filter, nearest", /* keywords */
	    NULL,
	    14, 14,
	    NULL, draw)
