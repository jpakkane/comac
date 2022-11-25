/*
 * Copyright © 2013 Uli Schlachter
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
 * Author: Uli Schlachter <psychon@znc.in>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    comac_move_to (cr, 85, -465);
    comac_line_to (cr, 3, 4.1);
    comac_line_to (cr, -145, -25);
    comac_close_path (cr);
    comac_clip (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_DEFAULT);
    comac_move_to (cr, -139, -524);
    comac_line_to (cr, 78, 44);
    comac_line_to (cr, -229, -10);
    comac_close_path (cr);
    comac_clip (cr);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_complex_bug61592,
	    "Exercise a bug found in 1.12",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    8,
	    5,
	    NULL,
	    draw)
