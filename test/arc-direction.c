/*
 * Copyright © 2012 Intel Corporation
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

#define SIZE (2 * 20)
#define PAD (2)

static comac_test_status_t
draw_arcs (comac_t *cr)
{
    double start = M_PI / 12, stop = 2 * start;

    comac_move_to (cr, SIZE / 2, SIZE / 2);
    comac_arc (cr, SIZE / 2, SIZE / 2, SIZE / 2, start, stop);
    comac_fill (cr);

    comac_translate (cr, SIZE + PAD, 0);
    comac_move_to (cr, SIZE / 2, SIZE / 2);
    comac_arc (cr,
	       SIZE / 2,
	       SIZE / 2,
	       SIZE / 2,
	       2 * M_PI - stop,
	       2 * M_PI - start);
    comac_fill (cr);

    comac_translate (cr, 0, SIZE + PAD);
    comac_move_to (cr, SIZE / 2, SIZE / 2);
    comac_arc_negative (cr,
			SIZE / 2,
			SIZE / 2,
			SIZE / 2,
			2 * M_PI - stop,
			2 * M_PI - start);
    comac_fill (cr);

    comac_translate (cr, -SIZE - PAD, 0);
    comac_move_to (cr, SIZE / 2, SIZE / 2);
    comac_arc_negative (cr, SIZE / 2, SIZE / 2, SIZE / 2, start, stop);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_save (cr);
    comac_translate (cr, PAD, PAD);
    draw_arcs (cr);
    comac_restore (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_translate (cr, 2 * SIZE + 3 * PAD, 0);
    comac_save (cr);
    comac_translate (cr, 2 * SIZE + 2 * PAD, PAD);
    comac_scale (cr, -1, 1);
    draw_arcs (cr);
    comac_restore (cr);

    comac_set_source_rgb (cr, 1, 0, 1);
    comac_translate (cr, 0, 2 * SIZE + 3 * PAD);
    comac_save (cr);
    comac_translate (cr, 2 * SIZE + 2 * PAD, 2 * SIZE + 2 * PAD);
    comac_scale (cr, -1, -1);
    draw_arcs (cr);
    comac_restore (cr);

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_translate (cr, -(2 * SIZE + 3 * PAD), 0);
    comac_save (cr);
    comac_translate (cr, PAD, 2 * SIZE + 2 * PAD);
    comac_scale (cr, 1, -1);
    draw_arcs (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (arc_direction,
	    "Test drawing positive/negative arcs",
	    "arc, fill", /* keywords */
	    NULL,	 /* requirements */
	    2 * (3 * PAD + 2 * SIZE),
	    2 * (3 * PAD + 2 * SIZE),
	    NULL,
	    draw)
