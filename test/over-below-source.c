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

#define SIZE 40
#define PAD 2
#define WIDTH (PAD + SIZE + PAD)
#define HEIGHT WIDTH

/* This test is designed to explore the interactions of "native" and
 * "fallback" objects. For the ps surface, OVER with non-1.0 opacity
 * will be a fallback while SOURCE will be native. For the pdf
 * surface, it's the reverse where OVER is native while SOURCE is a
 * fallback. */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_translate (cr, PAD, PAD);

    /* A green circle with OVER */
    comac_arc (cr, SIZE / 2, SIZE / 2, SIZE / 4, 0., 2. * M_PI);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_rgba (cr, 0., 1., 0., 0.5); /* 50% green */

    comac_fill (cr);

    /* A red triangle with SOURCE */
    comac_move_to (cr, SIZE / 2, SIZE / 2);
    comac_line_to (cr, SIZE, SIZE / 2);
    comac_line_to (cr, SIZE / 2, SIZE);
    comac_close_path (cr);

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgba (cr, 1., 0., 0., 0.5); /* 50% red */

    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (over_below_source,
	    "A simple test drawing a circle with OVER before a triangle drawn "
	    "with SOURCE",
	    "operator", /* keywords */
	    NULL,	/* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
