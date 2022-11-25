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

/* An assertion failure found by Rico Tzschichholz */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_rectangle (cr, 10, 55, 165, 1);
    comac_rectangle (cr, 174, 55, 1, 413);
    comac_rectangle (cr, 10, 56, 1, 413);
    comac_rectangle (cr, 10, 469, 165, 1);
    comac_clip (cr);

    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    comac_move_to (cr, 10, 57);
    comac_curve_to (cr, 10, 55.894531, 10.894531, 55, 12, 55);
    comac_line_to (cr, 173, 55);
    comac_curve_to (cr, 174.105469, 55, 175, 55.894531, 175, 57);
    comac_line_to (cr, 175, 468);
    comac_curve_to (cr, 175, 469.105469, 174.105469, 470, 173, 470);
    comac_line_to (cr, 12, 470);
    comac_curve_to (cr, 10.894531, 470, 10, 469.105469, 10, 468);

    comac_move_to (cr, 11, 57);
    comac_curve_to (cr, 11, 56.449219, 11.449219, 56, 12, 56);
    comac_line_to (cr, 173, 56);
    comac_curve_to (cr, 173.550781, 56, 174, 56.449219, 174, 57);
    comac_line_to (cr, 174, 468);
    comac_curve_to (cr, 174, 468.550781, 173.550781, 469, 173, 469);
    comac_line_to (cr, 12, 469);
    comac_curve_to (cr, 11.449219, 469, 11, 468.550781, 11, 468);

    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_bo_ricotz,
	    "Exercises a bug discovered by Rico Tzschichholz",
	    "clip, fill", /* keywords */
	    NULL,	  /* requirements */
	    649,
	    480,
	    NULL,
	    draw)
