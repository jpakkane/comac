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

/* A general convex shape with a twist. */

#include "comac-test.h"

#define SIZE (100)
#define PAD 4

static void
limacon (comac_t *cr, double a, double b, double radius)
{
    int i;

    comac_save (cr);
    comac_translate (cr, PAD, 0);
    for (i = 0; i < 360; i++) {
	double theta = i * M_PI / 180;
	double r = b + a * cos (theta);
	double x = radius * r * cos (theta);
	double y = radius * r * sin (theta);
	comac_line_to (cr, x, y);
    }
    comac_close_path (cr);
    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_save (cr);
    comac_translate (cr, 2 * PAD, PAD);

    comac_translate (cr, SIZE / 2, SIZE / 2);
    limacon (cr, 1, .5, SIZE / 3); /* trivia, this is a trisectrix */

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill_preserve (cr);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_stroke (cr);

    comac_scale (cr, -1, 1);

    limacon (cr, 1, .5, SIZE / 3); /* trivia, this is a trisectrix */

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_fill_preserve (cr);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (shape_general_convex,
	    "A general shape that is not as convex as it first appears",
	    "fill,stroke", /* keywords */
	    NULL,	   /* requirements */
	    SIZE + 4 * PAD,
	    SIZE + 4 * PAD,
	    NULL,
	    draw)
