/*
 * Copyright (c) 2011 Intel Corporation
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

#define SIZE 120

static void L(comac_t *cr, int w, int h)
{
	comac_move_to (cr, 0, 0);
	comac_line_to (cr, 0, h);
	comac_line_to (cr, w, h);
	comac_line_to (cr, w, h/2);
	comac_line_to (cr, w/2, h/2);
	comac_line_to (cr, w/2, 0);
	comac_close_path (cr);
}

static void LL(comac_t *cr, int w, int h)
{
    comac_save (cr);

    /* aligned */
    comac_rectangle (cr, 0, 0, w, h);
    comac_clip (cr);
    L (cr, w, h);
    comac_clip (cr);
    comac_paint (cr);
    comac_reset_clip (cr);

    /* unaligned */
    comac_translate (cr, w+.25, .25);
    comac_rectangle (cr, 0, 0, w, h);
    comac_clip (cr);
    L (cr, w, h);
    comac_clip (cr);
    comac_paint (cr);
    comac_reset_clip (cr);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int w = SIZE/2, h = SIZE/2;

    comac_paint (cr); /* opaque background */

    comac_set_source_rgb (cr, 1, 0, 0);
    LL (cr, w, h);

    comac_translate (cr, 0, h);
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    comac_set_source_rgb (cr, 0, 0, 1);
    LL (cr, w, h);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_rectilinear,
	    "Test handling of rectilinear clipping",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
