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
#define SIZE		(8 * LINE_WIDTH)
#define PAD		(1 * LINE_WIDTH)


static void
make_path (comac_t *cr)
{
    comac_move_to (cr, 0, 0);
    comac_rel_curve_to (cr,
			SIZE, 0,
			0, SIZE,
			SIZE, SIZE);
    comac_rel_line_to (cr, -SIZE, 0);
    comac_rel_curve_to (cr,
			SIZE, 0,
			0, -SIZE,
			SIZE, -SIZE);
    comac_close_path (cr);
}

static void
draw_joins (comac_t *cr)
{
    comac_save (cr);
    comac_translate (cr, PAD, PAD);

    make_path (cr);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    comac_stroke (cr);
    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    comac_stroke (cr);
    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr);
    comac_set_line_join (cr, COMAC_LINE_JOIN_MITER);
    comac_stroke (cr);
    comac_translate (cr, SIZE + PAD, 0.);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_set_line_width (cr, LINE_WIDTH);

    draw_joins (cr);

    /* and reflect to generate the opposite vertex ordering */
    comac_translate (cr, 0, height);
    comac_scale (cr, 1, -1);

    draw_joins (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (joins_loop,
	    "A loopy concave shape",
	    "stroke", /* keywords */
	    NULL, /* requirements */
	    3*(SIZE+PAD)+PAD, 2*(SIZE+PAD)+2*PAD,
	    NULL, draw)

