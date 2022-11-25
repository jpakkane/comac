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
#define PAD		(2 * LINE_WIDTH)

static void
make_path (comac_t *cr)
{
    comac_move_to (cr, 0, 0);
    comac_rel_line_to (cr, -SIZE/2, SIZE);
    comac_rel_line_to (cr, SIZE, 0);
    /* back to the start, but do not close */
    comac_rel_line_to (cr, -SIZE/2, -SIZE);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_set_line_width (cr, LINE_WIDTH);
    comac_translate (cr, PAD + SIZE / 2., PAD);

    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    make_path (cr);
    comac_stroke (cr);

    comac_translate (cr, 0, SIZE + PAD);

    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    make_path (cr);
    comac_stroke (cr);

    comac_translate (cr, 0, SIZE + PAD);

    comac_set_line_cap (cr, COMAC_LINE_CAP_SQUARE);
    comac_set_line_join (cr, COMAC_LINE_JOIN_MITER);
    make_path (cr);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (unclosed_strokes,
	    "Test coincident end-points are capped and not joined",
	    "stroke, caps", /* keywords */
	    NULL, /* requirements */
	    PAD + SIZE + PAD,
	    3 * (PAD + SIZE) + PAD,
	    NULL, draw)

