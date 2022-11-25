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

#define LINE_WIDTH 30.
#define SIZE (2 * LINE_WIDTH)
#define PAD (2 * LINE_WIDTH)

static void
make_path (comac_t *cr, double theta)
{
    double line_width = comac_get_line_width (cr) / 4;

    comac_move_to (cr, 0, 0);
    comac_rel_curve_to (cr, SIZE / 3, -SIZE / 4, SIZE / 3, -SIZE / 4, SIZE, 0);

    comac_rel_line_to (cr, cos (theta) * line_width, sin (theta) * line_width);
}

static void
draw_joins (comac_t *cr, double theta)
{
    make_path (cr, theta);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    comac_stroke (cr);

    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr, theta);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    comac_stroke (cr);

    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr, theta);
    comac_set_line_join (cr, COMAC_LINE_JOIN_MITER);
    comac_stroke (cr);

    comac_translate (cr, SIZE + PAD, 0.);
}

static void
draw_caps_joins (comac_t *cr, double theta)
{
    comac_translate (cr, PAD, 0);
    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    draw_joins (cr, theta);

    comac_translate (cr, PAD, 0);
    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);
    draw_joins (cr, theta);

    comac_translate (cr, PAD, 0);
    comac_set_line_cap (cr, COMAC_LINE_CAP_SQUARE);
    draw_joins (cr, theta);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const double theta[] = {-M_PI / 2, -M_PI / 4, 0, M_PI / 8, M_PI / 3, M_PI};
    unsigned int t;

    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_set_line_width (cr, LINE_WIDTH);

    for (t = 0; t < ARRAY_LENGTH (theta); t++) {
	comac_save (cr);
	comac_translate (cr, 0, t * (SIZE + PAD) + PAD);
	draw_caps_joins (cr, theta[t]);
	comac_restore (cr);

	comac_save (cr);
	/* and reflect to generate the opposite vertex ordering */
	comac_translate (cr, 0, height - t * (SIZE + PAD) - PAD);
	comac_scale (cr, 1, -1);
	draw_caps_joins (cr, theta[t]);
	comac_restore (cr);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (caps_tails_curve,
	    "Test caps and joins on short tail segments",
	    "stroke, cap, join", /* keywords */
	    NULL,		 /* requirements */
	    9 * (PAD + SIZE) + 4 * PAD,
	    12 * (PAD + SIZE) + PAD,
	    NULL,
	    draw)
