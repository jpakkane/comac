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

#define STEP 5
#define WIDTH 100
#define HEIGHT 100

static void
diamond (comac_t *cr)
{
    comac_move_to (cr, WIDTH / 2, 0);
    comac_line_to (cr, WIDTH, HEIGHT / 2);
    comac_line_to (cr, WIDTH / 2, HEIGHT);
    comac_line_to (cr, 0, HEIGHT / 2);
    comac_close_path (cr);
}

static void
background (comac_t *cr)
{
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_rgb (cr, 0, 0, 0);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    background (cr);

    /* completely overlapping diamonds */
    comac_save (cr);
    diamond (cr);
    comac_clip (cr);
    diamond (cr);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    /* partial overlap */
    comac_save (cr);
    comac_translate (cr, -WIDTH / 4, 0);
    diamond (cr);
    comac_clip (cr);
    comac_translate (cr, WIDTH / 2, 0);
    diamond (cr);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    /* no overlap, but the bounding boxes must */
    comac_save (cr);
    comac_translate (cr, -WIDTH / 2 + 2, -2);
    diamond (cr);
    comac_clip (cr);
    comac_translate (cr, WIDTH - 4, 4);
    diamond (cr);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    /* completely disjoint */
    comac_save (cr);
    comac_translate (cr, -WIDTH / 2 - 1, 0);
    diamond (cr);
    comac_clip (cr);
    comac_translate (cr, WIDTH + 2, 0);
    diamond (cr);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_polygons,
	    "Test drawing through through an intersection of polygons",
	    "clip",	     /* keywords */
	    "target=raster", /* requirements */
	    4 * WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
