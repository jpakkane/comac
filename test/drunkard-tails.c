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

#define LINE_WIDTH	10.
#define SIZE		(5 * LINE_WIDTH)
#define PAD		(3 * LINE_WIDTH)

static uint32_t state;

static double
uniform_random (double minval, double maxval)
{
    static uint32_t const poly = 0x9a795537U;
    uint32_t n = 32;
    while (n-->0)
	state = 2*state < state ? (2*state ^ poly) : 2*state;
    return minval + state * (maxval - minval) / 4294967296.0;
}

static void
make_path (comac_t *cr)
{
    int i;

    state = 0xdeadbeef;

    comac_move_to (cr, SIZE/2, SIZE/2);
    for (i = 0; i < 200; i++) {
	double theta = uniform_random (-M_PI, M_PI);
	comac_rel_line_to (cr,
			   cos (theta) * LINE_WIDTH / 4,
			   sin (theta) * LINE_WIDTH / 4);
    }
}

static void
draw_caps_joins (comac_t *cr)
{
    comac_save (cr);

    comac_translate (cr, PAD, PAD);

    comac_reset_clip (cr);
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_clip (cr);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    comac_stroke (cr);

    comac_translate (cr, SIZE + PAD, 0.);

    comac_reset_clip (cr);
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_clip (cr);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    comac_stroke (cr);

    comac_translate (cr, SIZE + PAD, 0.);

    comac_reset_clip (cr);
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_clip (cr);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_SQUARE);
    comac_set_line_join (cr, COMAC_LINE_JOIN_MITER);
    comac_stroke (cr);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_set_line_width (cr, LINE_WIDTH);

    draw_caps_joins (cr);

    comac_save (cr);
    /* and reflect to generate the opposite vertex ordering */
    comac_translate (cr, 0, height);
    comac_scale (cr, 1, -1);

    draw_caps_joins (cr);
    comac_restore (cr);

    comac_translate (cr, 0, SIZE + PAD);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (drunkard_tails,
	    "Test caps and joins on short tail segments",
	    "stroke, cap, join", /* keywords */
	    NULL, /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    2 * (PAD + SIZE) + PAD,
	    NULL, draw)

