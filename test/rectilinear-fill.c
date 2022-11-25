/*
 * Copyright © 2008 Chris Wilson
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

#define SIZE 24

static void
draw_rectangles (comac_t *cr)
{
    comac_save (cr);

    /* test constructing single rectangles */
    comac_rectangle (cr, 0, 0, SIZE/2, 2);
    comac_fill (cr);

    comac_rectangle (cr, 0, 5, SIZE/2, -2);
    comac_fill (cr);

    comac_rectangle (cr, SIZE/2, 6, -SIZE/2, 2);
    comac_fill (cr);

    comac_rectangle (cr, SIZE/2, 11, -SIZE/2, -2);
    comac_fill (cr);

    /* test constructing multiple rectangles */
    comac_translate (cr, 0, 12);
    comac_rectangle (cr, 0, 0, SIZE/2, 2);
    comac_rectangle (cr, 0, 5, SIZE/2, -2);
    comac_rectangle (cr, SIZE/2, 6, -SIZE/2, 2);
    comac_rectangle (cr, SIZE/2, 11, -SIZE/2, -2);
    comac_fill (cr);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* Paint background white, then draw in black. */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */

    draw_rectangles (cr);

    /* and check using cw winding */
    comac_translate (cr, SIZE, SIZE);
    comac_scale (cr, -1, 1);

    draw_rectangles (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (rectilinear_fill,
	    "Test rectilinear fill operations (covering only whole pixels)",
	    "fill, rectilinear", /* keywords */
	    NULL, /* requirements */
	    SIZE, 2 * SIZE,
	    NULL, draw)
