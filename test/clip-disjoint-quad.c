/*
 * Copyright © 2012 Red Hat, Inc.
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
 * Author: Soren Sandmann <sandmann@redhat.com>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define WIDTH 100
#define HEIGHT 200

static void
draw_quad (comac_t *cr,
	   double x1, double y1,
	   double x2, double y2,
	   double x3, double y3,
	   double x4, double y4)
{
    comac_move_to (cr, x1, y1);
    comac_line_to (cr, x2, y2);
    comac_line_to (cr, x3, y3);
    comac_line_to (cr, x4, y4);
    comac_close_path (cr);

    comac_set_source_rgb (cr, 0, 0.6, 0);
    comac_fill (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int position[5] = {0, HEIGHT/2-10, HEIGHT/2-5, HEIGHT/2, HEIGHT-10 };
    int i;

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    for (i = 0; i < 5; i++) {
	comac_reset_clip (cr);
	comac_set_source_rgb (cr, 1, 1, 1);
	comac_rectangle (cr, 0, 0, WIDTH/2, HEIGHT/2);
	comac_rectangle (cr, WIDTH/2, position[i], WIDTH/2, 10);
	comac_fill_preserve (cr);
	comac_clip (cr);

	comac_set_source_rgb (cr, 1, 0, 1);
	draw_quad (cr, 50, 50, 75, 75, 50, 150, 25, 75);
	comac_fill (cr);

	comac_translate(cr, WIDTH, 0);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_disjoint_quad,
	    "Tests a simple fill through two disjoint clips.",
	    "clip, fill", /* keywords */
	    NULL, /* requirements */
	    5*WIDTH, HEIGHT,
	    NULL, draw)
