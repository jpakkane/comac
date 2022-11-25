/*
 * Copyright © 2008 Red Hat, Inc.
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
 */

#include "comac-test.h"

#define POINTS	10
#define STEP	(1.0 / POINTS)
#define PAD	1
#define WIDTH	(PAD + POINTS * 2 + PAD)
#define HEIGHT	(WIDTH)

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i, j;

    /* Fill background white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    /* Draw in black */
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_translate (cr, PAD, PAD);
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    for (i = 0; i < POINTS; i++)
	for (j = 0; j < POINTS; j++) {
	    comac_rectangle (cr, 2 * i + i * STEP, 2 * j + j * STEP, 1, 1);
	    comac_fill (cr);
	}

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (a1_traps_sample,
	    "Test sample position when drawing trapezoids with ANTIALIAS_NONE",
	    "alpha, traps", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
