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

/* Test the fidelity of the rasterisation, because Comac is my favourite
 * driver test suite.
 */

#define SIZE 256
#define WIDTH 2
#define HEIGHT 10

static comac_test_status_t
rectangles (comac_t *cr, int width, int height)
{
    int i;

    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
    comac_paint (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);

    for (i = 1; i <= SIZE; i++) {
	int x, y;

	comac_save (cr);
	comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
	comac_clip (cr);

	comac_scale (cr, 1. / SIZE, 1. / SIZE);
	for (x = -i; x < SIZE * WIDTH; x += 2 * i) {
	    for (y = -i; y < SIZE * HEIGHT; y += 2 * i) {
		/* Add a little tile composed of two non-overlapping squares
		 *   +--+
		 *   |  |
		 *   |__|__
		 *      |  |
		 *      |  |
		 *      +--+
		 */
		comac_rectangle (cr, x, y, i, i);
		comac_rectangle (cr, x + i, y + i, i, i);
	    }
	}
	comac_fill (cr);
	comac_restore (cr);

	comac_translate (cr, WIDTH, 0);
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
triangles (comac_t *cr, int width, int height)
{
    int i;

    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
    comac_paint (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);

    for (i = 1; i <= SIZE; i++) {
	int x, y;

	comac_save (cr);
	comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
	comac_clip (cr);

	comac_scale (cr, 1. / SIZE, 1. / SIZE);
	for (x = -i; x < SIZE * WIDTH; x += 2 * i) {
	    for (y = -i; y < SIZE * HEIGHT; y += 2 * i) {
		/* Add a tile composed of four non-overlapping
		 * triangles.  The plus and minus signs inside the
		 * triangles denote the orientation of the triangle's
		 * edges: + for clockwise and - for anticlockwise.
		 *
		 *   +-----+
		 *    \-|+/
		 *     \|/
		 *     /|\
		 *    /-|-\
		 *   +-----+
		 */

		/* top left triangle */
		comac_move_to (cr, x, y);
		comac_line_to (cr, x + i, y + i);
		comac_line_to (cr, x + i, y);
		comac_close_path (cr);

		/* top right triangle */
		comac_move_to (cr, x + i, y);
		comac_line_to (cr, x + 2 * i, y);
		comac_line_to (cr, x + i, y + i);
		comac_close_path (cr);

		/* bottom left triangle */
		comac_move_to (cr, x + i, y + i);
		comac_line_to (cr, x, y + 2 * i);
		comac_line_to (cr, x + i, y + 2 * i);
		comac_close_path (cr);

		/* bottom right triangle */
		comac_move_to (cr, x + i, y + i);
		comac_line_to (cr, x + i, y + 2 * i);
		comac_line_to (cr, x + 2 * i, y + 2 * i);
		comac_close_path (cr);
	    }
	}
	comac_fill (cr);
	comac_restore (cr);

	comac_translate (cr, WIDTH, 0);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (half_coverage_rectangles,
	    "Check the fidelity of the rasterisation.",
	    NULL,		  /* keywords */
	    "target=raster slow", /* requirements */
	    WIDTH *SIZE,
	    HEIGHT,
	    NULL,
	    rectangles)

COMAC_TEST (half_coverage_triangles,
	    "Check the fidelity of the rasterisation.",
	    NULL,		  /* keywords */
	    "target=raster slow", /* requirements */
	    WIDTH *SIZE,
	    HEIGHT,
	    NULL,
	    triangles)
