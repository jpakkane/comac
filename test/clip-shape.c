/*
 * Copyright (c) 2010 Intel Corporation
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

/* Adapted from a bug report by <comacuser@yahoo.com> */

#include "comac-test.h"

static const struct xy {
   double x;
   double y;
} gp[] = {
    { 100, 250 },
    { 100, 100 },
    { 150, 230 },
    { 239, 100 },
    { 239, 250 },
};

static const double vp[3] = { 100, 144, 238.5 };

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;

    comac_paint (cr); /* opaque background */

    for (i = 0; i < 5; ++i)
	comac_line_to (cr, gp[i].x, gp[i].y);
    comac_close_path (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_set_line_width (cr, 1.5);
    comac_stroke_preserve (cr);
    comac_clip (cr);

    for (i = 1; i < 3; ++i) {
	double x1 = vp[i - 1];
	double x2 = vp[i];

	comac_move_to (cr, x1, 0);
	comac_line_to (cr, x1, height);
	comac_line_to (cr, x2, height);
	comac_line_to (cr, x2, 0);
	comac_close_path (cr);

	if (i & 1)
	    comac_set_source_rgb (cr, 0, 1, 0);
	else
	    comac_set_source_rgb (cr, 1, 1, 0);

	comac_fill (cr);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_shape,
	    "Test handling of clipping with a non-aligned shape",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    400, 300,
	    NULL, draw)
