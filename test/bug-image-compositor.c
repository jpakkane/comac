/*
 * Copyright © 2020 Uli Schlachter, Heiko Lewin
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
 * Author: Heiko Lewin <hlewin@gmx.de>
 */
#include "comac-test.h"


/* This test reproduces an overflow of a mask-buffer in comac-image-compositor.c */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 0., 0., 0.);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1., 1., 1.);
    comac_set_line_width (cr, 1.);

    comac_pattern_t *p = comac_pattern_create_linear (0, 0, width, height);
    comac_pattern_add_color_stop_rgb (p, 0, 0.99, 1, 1);
    comac_pattern_add_color_stop_rgb (p, 1, 1, 1, 1);
    comac_set_source (cr, p);
    comac_pattern_destroy(p);

    comac_move_to (cr, 0.5, -1);
    for (int i = 0; i < width; i+=3) {
	comac_rel_line_to (cr, 2, 2);
	comac_rel_line_to (cr, 1, -2);
    }

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}


COMAC_TEST (bug_image_compositor,
	    "Crash in image-compositor",
	    "stroke, stress", /* keywords */
	    NULL, /* requirements */
	    10000, 1,
	    NULL, draw)
	    
