/*
 * Copyright 2010 Intel Corporation
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
 * Test the fidelity of the rasterisation, paying careful attention to rounding.
 */

#include "../src/comac-fixed-type-private.h"
#define PRECISION (int)(1 << COMAC_FIXED_FRAC_BITS)

#define WIDTH ((PRECISION/2+1)*3)
#define HEIGHT ((PRECISION/2+1)*3)

#define SUBPIXEL(v) ((v)/(double)(PRECISION/2))

static comac_test_status_t
rectangles (comac_t *cr, int width, int height)
{
    int x, y;

    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    for (x = 0; x < WIDTH; x += 3) {
	for (y = 0; y < HEIGHT; y += 3) {
	    comac_rectangle (cr, x + SUBPIXEL (y/3) - .5, y + SUBPIXEL (x/3) - .5, .5, .5);
	}
    }
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
triangles (comac_t *cr, int width, int height)
{
    int x, y;

    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    for (x = 0; x < WIDTH; x += 3) {
	for (y = 0; y < HEIGHT; y += 3) {
	    /* a rectangle with a diagonal to force tessellation */
	    comac_move_to (cr, x + SUBPIXEL (y/3) - .5, y + SUBPIXEL (x/3) - .5);
	    comac_rel_line_to (cr, .5, .5);
	    comac_rel_line_to (cr, 0, -.5);
	    comac_rel_line_to (cr, -.5, 0);
	    comac_rel_line_to (cr, 0, .5);
	    comac_rel_line_to (cr, .5, 0);
	}
    }
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (a1_rasterisation_rectangles,
	    "Check the fidelity of the rasterisation.",
	    "rasterisation", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, rectangles)

COMAC_TEST (a1_rasterisation_triangles,
	    "Check the fidelity of the rasterisation.",
	    "rasterisation", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, triangles)
