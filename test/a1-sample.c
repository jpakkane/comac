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

#define WIDTH	(256) //COMAC_FIXED_ONE
#define HEIGHT	(WIDTH)

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i, j;

    /* Fill background white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    comac_set_source_rgb (cr, 0, 0, 0);

    /* Only the single rectangle that covers the centre pixel should be filled*/
    for (i = 0; i < 256; i++)
	for (j = 0; j < 256; j++) {
	    comac_rectangle (cr, i + i/256., j + j/256., 1/256., 1/256.);
	    comac_fill (cr);
	}

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (a1_sample,
	    "Tests unantialiased rendering of a quantum box",
	    " alpha", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
