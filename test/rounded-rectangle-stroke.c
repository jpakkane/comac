/*
 * Copyright © 2009 Chris Wilson
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

#define SIZE 80

/* A very simple test to exercise the scan rasterisers with constant regions. */

static void
rounded_rectangle (comac_t *cr, int x, int y, int w, int h, int r)
{
    comac_new_sub_path (cr);
    comac_arc (cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
    comac_arc (cr, x + w - r, y + r, r, 3 * M_PI / 2, 2 * M_PI);
    comac_arc (cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    comac_arc (cr, x + r, y + h - r, r, M_PI / 2, M_PI);
    comac_close_path (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* Paint background white, then draw in black. */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */

    comac_set_line_width (cr, 10);
    rounded_rectangle (cr, 10, 10, width - 20, height - 20, 10);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (rounded_rectangle_stroke,
	    "Tests handling of rounded rectangles, the UI designers favourite",
	    "stroke, rounded-rectangle", /* keywords */
	    NULL,			 /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)
