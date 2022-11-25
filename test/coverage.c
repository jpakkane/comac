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

#define GENERATE_REFERENCE 0

#define WIDTH 256
#define HEIGHT 40

#include "../src/comac-fixed-type-private.h"
#define PRECISION (1 << COMAC_FIXED_FRAC_BITS)

/* XXX beware multithreading! */
static uint32_t state;

static uint32_t
hars_petruska_f54_1_random (void)
{
#define rol(x, k) ((x << k) | (x >> (32 - k)))
    return state = (state ^ rol (state, 5) ^ rol (state, 24)) + 0x37798849;
#undef rol
}

static double
random_offset (int range, int precise)
{
    double x =
	hars_petruska_f54_1_random () / (double) UINT32_MAX * range / WIDTH;
    if (precise)
	x = floor (x * PRECISION) / PRECISION;
    return x;
}

static comac_test_status_t
rectangles (comac_t *cr, int width, int height)
{
    int x, y, channel;

    state = 0x12345678;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

#if GENERATE_REFERENCE
    for (x = 0; x < WIDTH; x++) {
	comac_set_source_rgba (cr, 1, 1, 1, x * x * 1.0 / (WIDTH * WIDTH));
	comac_rectangle (cr, x, 0, 1, HEIGHT);
	comac_fill (cr);
    }
#else
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    for (channel = 0; channel < 3; channel++) {
	switch (channel) {
	default:
	case 0:
	    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
	    break;
	case 1:
	    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
	    break;
	case 2:
	    comac_set_source_rgb (cr, 0.0, 0.0, 1.0);
	    break;
	}

	for (x = 0; x < WIDTH; x++) {
	    for (y = 0; y < HEIGHT; y++) {
		double dx = random_offset (WIDTH - x, TRUE);
		double dy = random_offset (WIDTH - x, TRUE);
		comac_rectangle (cr,
				 x + dx,
				 y + dy,
				 x / (double) WIDTH,
				 x / (double) WIDTH);
	    }
	}
	comac_fill (cr);
    }
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
rhombus (comac_t *cr, int width, int height)
{
    int x, y;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

#if GENERATE_REFERENCE
    for (y = 0; y < WIDTH; y++) {
	for (x = 0; x < WIDTH; x++) {
	    comac_set_source_rgba (cr, 1, 1, 1, x * y / (2. * WIDTH * WIDTH));
	    comac_rectangle (cr, 2 * x, 2 * y, 2, 2);
	    comac_fill (cr);
	}
    }
#else
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_rgb (cr, 1, 1, 1);

    for (y = 0; y < WIDTH; y++) {
	double yf = y / (double) WIDTH;
	for (x = 0; x < WIDTH; x++) {
	    double xf = x / (double) WIDTH;

	    comac_move_to (cr, 2 * x + 1 - xf, 2 * y + 1);
	    comac_line_to (cr, 2 * x + 1, 2 * y + 1 - yf);
	    comac_line_to (cr, 2 * x + 1 + xf, 2 * y + 1);
	    comac_line_to (cr, 2 * x + 1, 2 * y + 1 + yf);
	    comac_close_path (cr);
	}
    }

    comac_fill (cr);
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
intersecting_quads (comac_t *cr, int width, int height)
{
    int x, y, channel;

    state = 0x12345678;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

#if GENERATE_REFERENCE
    for (x = 0; x < WIDTH; x++) {
	comac_set_source_rgba (cr, 1, 1, 1, x * x * 0.5 / (WIDTH * WIDTH));
	comac_rectangle (cr, x, 0, 1, HEIGHT);
	comac_fill (cr);
    }
#else
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    for (channel = 0; channel < 3; channel++) {
	switch (channel) {
	default:
	case 0:
	    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
	    break;
	case 1:
	    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
	    break;
	case 2:
	    comac_set_source_rgb (cr, 0.0, 0.0, 1.0);
	    break;
	}

	for (x = 0; x < WIDTH; x++) {
	    double step = x / (double) WIDTH;
	    for (y = 0; y < HEIGHT; y++) {
		double dx = random_offset (WIDTH - x, TRUE);
		double dy = random_offset (WIDTH - x, TRUE);
		comac_move_to (cr, x + dx, y + dy);
		comac_rel_line_to (cr, step, step);
		comac_rel_line_to (cr, 0, -step);
		comac_rel_line_to (cr, -step, step);
		comac_close_path (cr);
	    }
	}
	comac_fill (cr);
    }
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
intersecting_triangles (comac_t *cr, int width, int height)
{
    int x, y, channel;

    state = 0x12345678;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

#if GENERATE_REFERENCE
    for (x = 0; x < WIDTH; x++) {
	comac_set_source_rgba (cr, 1, 1, 1, x * x * 0.75 / (WIDTH * WIDTH));
	comac_rectangle (cr, x, 0, 1, HEIGHT);
	comac_fill (cr);
    }
#else
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    for (channel = 0; channel < 3; channel++) {
	switch (channel) {
	default:
	case 0:
	    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
	    break;
	case 1:
	    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
	    break;
	case 2:
	    comac_set_source_rgb (cr, 0.0, 0.0, 1.0);
	    break;
	}

	for (x = 0; x < WIDTH; x++) {
	    double step = x / (double) WIDTH;
	    for (y = 0; y < HEIGHT; y++) {
		double dx = random_offset (WIDTH - x, TRUE);
		double dy = random_offset (WIDTH - x, TRUE);

		/* left */
		comac_move_to (cr, x + dx, y + dy);
		comac_rel_line_to (cr, 0, step);
		comac_rel_line_to (cr, step, 0);
		comac_close_path (cr);

		/* right, mirrored */
		comac_move_to (cr, x + dx + step, y + dy + step);
		comac_rel_line_to (cr, 0, -step);
		comac_rel_line_to (cr, -step, step);
		comac_close_path (cr);
	    }
	}
	comac_fill (cr);
    }
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
triangles (comac_t *cr, int width, int height)
{
    int x, y, channel;

    state = 0x12345678;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

#if GENERATE_REFERENCE
    for (x = 0; x < WIDTH; x++) {
	comac_set_source_rgba (cr, 1, 1, 1, x * x * 0.5 / (WIDTH * WIDTH));
	comac_rectangle (cr, x, 0, 1, HEIGHT);
	comac_fill (cr);
    }
#else
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    for (channel = 0; channel < 3; channel++) {
	switch (channel) {
	default:
	case 0:
	    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
	    break;
	case 1:
	    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
	    break;
	case 2:
	    comac_set_source_rgb (cr, 0.0, 0.0, 1.0);
	    break;
	}

	for (x = 0; x < WIDTH; x++) {
	    for (y = 0; y < HEIGHT; y++) {
		double dx = random_offset (WIDTH - x, TRUE);
		double dy = random_offset (WIDTH - x, TRUE);
		comac_move_to (cr, x + dx, y + dy);
		comac_rel_line_to (cr, x / (double) WIDTH, 0);
		comac_rel_line_to (cr, 0, x / (double) WIDTH);
		comac_close_path (cr);
	    }
	}
	comac_fill (cr);
    }
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
abutting (comac_t *cr, int width, int height)
{
    int x, y;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

    comac_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.75);

#if GENERATE_REFERENCE
    comac_paint (cr);
#else
    comac_set_operator (cr, COMAC_OPERATOR_ADD);

    for (y = 0; y < 16; y++) {
	for (x = 0; x < 16; x++) {
	    double theta = (y * 16 + x) * M_PI / 512;
	    double cx = 16 * cos (theta) + x * 16;
	    double cy = 16 * sin (theta) + y * 16;

	    comac_move_to (cr, x * 16, y * 16);
	    comac_line_to (cr, cx, cy);
	    comac_line_to (cr, (x + 1) * 16, y * 16);
	    comac_fill (cr);

	    comac_move_to (cr, (x + 1) * 16, y * 16);
	    comac_line_to (cr, cx, cy);
	    comac_line_to (cr, (x + 1) * 16, (y + 1) * 16);
	    comac_fill (cr);

	    comac_move_to (cr, (x + 1) * 16, (y + 1) * 16);
	    comac_line_to (cr, cx, cy);
	    comac_line_to (cr, x * 16, (y + 1) * 16);
	    comac_fill (cr);

	    comac_move_to (cr, x * 16, (y + 1) * 16);
	    comac_line_to (cr, cx, cy);
	    comac_line_to (cr, x * 16, y * 16);
	    comac_fill (cr);
	}
    }
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
column_triangles (comac_t *cr, int width, int height)
{
    int x, y, i, channel;

    state = 0x12345678;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

#if GENERATE_REFERENCE
    for (x = 0; x < WIDTH; x++) {
	comac_set_source_rgba (cr, 1, 1, 1, x * 0.5 / WIDTH);
	comac_rectangle (cr, x, 0, 1, HEIGHT);
	comac_fill (cr);
    }
#else
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    for (channel = 0; channel < 3; channel++) {
	switch (channel) {
	default:
	case 0:
	    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
	    break;
	case 1:
	    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
	    break;
	case 2:
	    comac_set_source_rgb (cr, 0.0, 0.0, 1.0);
	    break;
	}

	for (x = 0; x < WIDTH; x++) {
	    double step = x / (double) (2 * WIDTH);
	    for (y = 0; y < HEIGHT; y++) {
		for (i = 0; i < PRECISION; i++) {
		    double dy = random_offset (WIDTH - x, FALSE);

		    /*
		     * We want to test some sharing of edges to further
		     * stress the rasterisers, so instead of using one
		     * tall triangle, it is split into two, with vertical
		     * edges on either side that may co-align with their
		     * neighbours:
		     *
		     *  s ---  .      ---
		     *  t  |   |\      |
		     *  e  |   | \     |
		     *  p ---  ....    |  2 * step = x / WIDTH
		     *          \ |    |
		     *           \|    |
		     *            .   ---
		     *        |---|
		     *     1 / PRECISION
		     *
		     * Each column contains two triangles of width one quantum and
		     * total height of (x / WIDTH), thus the total area covered by all
		     * columns in each pixel is .5 * (x / WIDTH).
		     */

		    comac_move_to (cr, x + i / (double) PRECISION, y + dy);
		    comac_rel_line_to (cr, 0, step);
		    comac_rel_line_to (cr, 1 / (double) PRECISION, step);
		    comac_rel_line_to (cr, 0, -step);
		    comac_close_path (cr);
		}
		comac_fill (
		    cr); /* do these per-pixel due to the extra volume of edges */
	    }
	}
    }
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
row_triangles (comac_t *cr, int width, int height)
{
    int x, y, i, channel;

    state = 0x12345678;

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

#if GENERATE_REFERENCE
    for (x = 0; x < WIDTH; x++) {
	comac_set_source_rgba (cr, 1, 1, 1, x * 0.5 / WIDTH);
	comac_rectangle (cr, x, 0, 1, HEIGHT);
	comac_fill (cr);
    }
#else
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    for (channel = 0; channel < 3; channel++) {
	switch (channel) {
	default:
	case 0:
	    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
	    break;
	case 1:
	    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
	    break;
	case 2:
	    comac_set_source_rgb (cr, 0.0, 0.0, 1.0);
	    break;
	}

	for (x = 0; x < WIDTH; x++) {
	    double step = x / (double) (2 * WIDTH);
	    for (y = 0; y < HEIGHT; y++) {
		for (i = 0; i < PRECISION; i++) {
		    double dx = random_offset (WIDTH - x, FALSE);

		    /* See column_triangles() for a transposed description
		     * of this geometry.
		     */

		    comac_move_to (cr, x + dx, y + i / (double) PRECISION);
		    comac_rel_line_to (cr, step, 0);
		    comac_rel_line_to (cr, step, 1 / (double) PRECISION);
		    comac_rel_line_to (cr, -step, 0);
		    comac_close_path (cr);
		}
		comac_fill (
		    cr); /* do these per-pixel due to the extra volume of edges */
	    }
	}
    }
#endif

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (coverage_rectangles,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    rectangles)

COMAC_TEST (coverage_rhombus,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    2 * WIDTH,
	    2 * WIDTH,
	    NULL,
	    rhombus)

COMAC_TEST (coverage_intersecting_quads,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    intersecting_quads)

COMAC_TEST (coverage_intersecting_triangles,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    intersecting_triangles)
COMAC_TEST (coverage_row_triangles,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    row_triangles)
COMAC_TEST (coverage_column_triangles,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    column_triangles)
COMAC_TEST (coverage_triangles,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    triangles)
COMAC_TEST (coverage_abutting,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    16 * 16,
	    16 * 16,
	    NULL,
	    abutting)
