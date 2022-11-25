/*
 * Copyright © 2007 Red Hat, Inc.
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

#define SIZE   20
#define PAD    5
#define WIDTH  (PAD + 3 * (PAD + SIZE) + PAD)
#define HEIGHT (PAD + SIZE + PAD)

/* We're demonstrating here a bug originally reported by Benjamin Otte
 * on the comac mailing list here, (after he ran into this problem
 * with various flash animations):
 *
 *	[comac] Assertion `i < pen->num_vertices' failed in 1.4.10
 *	https://lists.comacgraphics.org/archives/comac/2007-August/011282.html
 *
 * The problem shows up with an extreme transformation matrix that
 * collapses the pen to a single line, (which means that
 * _comac_slope_compare cannot handle adjacent vertices in the pen
 * since they have parallel slope).
 *
 * This test case tests degenerate pens in several directions and uses
 * round caps to force the stroking code to attempt to walk around the
 * pen doing slope comparisons.
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);

    comac_translate (cr, PAD, PAD);

    /* First compress the pen to a vertical line. */
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_curve_to (cr, SIZE / 2, 0, SIZE, SIZE / 2, SIZE, SIZE);
    comac_save (cr);
    {
	comac_scale (cr, 0.000001, 1.0);
	comac_stroke (cr);
    }
    comac_restore (cr);

    comac_translate (cr, PAD + SIZE, 0);

    /* Then compress the pen to a horizontal line. */
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_curve_to (cr, SIZE / 2, 0, SIZE, SIZE / 2, SIZE, SIZE);
    comac_save (cr);
    {
	comac_scale (cr, 1.0, 0.000001);
	comac_stroke (cr);
    }
    comac_restore (cr);

    comac_translate (cr, PAD + SIZE, 0);

    /* Finally a line at an angle. */
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_curve_to (cr, SIZE / 2, 0, SIZE, SIZE / 2, SIZE, SIZE);
    comac_save (cr);
    {
	comac_rotate (cr, M_PI / 4.0);
	comac_scale (cr, 0.000001, 1.0);
	comac_stroke (cr);
    }
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (degenerate_pen,
	    "Test round joins with a pen that's transformed to a line",
	    "degenerate", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
