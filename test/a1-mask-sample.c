/*
 * Copyright © 2008 Red Hat, Inc.
 * Copyright © 2010 Intel Corporation
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
 * Author: Carl D. Worth <cworth@cworth.org>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define POINTS 10
#define STEP (1.0 / POINTS)
#define PAD 1
#define WIDTH (PAD + POINTS * 2 + PAD)
#define HEIGHT (WIDTH)

/* A single, opaque pixel */
static const uint32_t black_pixel = 0xffffffff;

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    comac_pattern_t *mask;
    int i, j;

    surface =
	comac_image_surface_create_for_data ((unsigned char *) &black_pixel,
					     COMAC_FORMAT_A8,
					     1,
					     1,
					     4);
    mask = comac_pattern_create_for_surface (surface);
    comac_pattern_set_filter (mask, COMAC_FILTER_NEAREST);
    comac_surface_destroy (surface);

    /* Fill background white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_translate (cr, PAD, PAD);

    comac_set_source_rgb (cr, 0, 0, 0);
    for (i = 0; i < POINTS; i++) {
	for (j = 0; j < POINTS; j++) {
	    comac_matrix_t m;

	    comac_matrix_init_translate (&m,
					 -(2 * i + i * STEP),
					 -(2 * j + j * STEP));
	    comac_pattern_set_matrix (mask, &m);
	    comac_mask (cr, mask);
	}
    }

    comac_pattern_destroy (mask);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (a1_mask_sample,
	    "Test sample position when masking with FILTER_NEAREST",
	    "image, alpha",  /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
