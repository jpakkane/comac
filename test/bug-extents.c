/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright 2010 Andrea Canciani
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
 * Author: Andrea Canciani <ranma42@gmail.com>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_translate (cr, 0, -25);

    comac_move_to  (cr,   50, 200);
    comac_curve_to (cr,   50, 150, 100,  50, 150,  50);
    comac_curve_to (cr,  200,  50, 250, 250, 200, 250);
    comac_curve_to (cr,  150, 250, 200,  50,  50, 100);
    comac_curve_to (cr, -100, 150, 200, 150, 200, 200);
    comac_curve_to (cr,  200, 250,  50, 250,  50, 200);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_fill_preserve (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_extents,
	    "Tests a bug in the computation of approximate extents",
	    "extents", /* keywords */
	    NULL, /* requirements */
	    250, 250,
	    NULL, draw)
