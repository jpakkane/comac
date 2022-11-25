/*
 * Copyright © 2011 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"

#define IMAGE_WIDTH 80
#define IMAGE_HEIGHT 80


static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;

    comac_test_paint_checkered (cr);

    comac_scale (cr, 0.3, 0.3);
    comac_translate (cr, 50, 50);

    pattern = comac_pattern_create_linear (70, 100, 130, 100);
    comac_pattern_add_color_stop_rgba (pattern, 0,  1, 0, 0,  1.0);
    comac_pattern_add_color_stop_rgba (pattern, 1,  0, 1, 0,  0.5);

    comac_pattern_set_extend (pattern, COMAC_EXTEND_PAD);
    comac_set_source (cr, pattern);

    comac_move_to(cr, 20, 20);
    comac_curve_to(cr,
                   130, 0,
                   70, 200,
                   180, 180);
    comac_set_line_width (cr, 20);
    comac_stroke (cr);

    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (stroke_pattern,
	    "Patterned stroke",
	    "stroke, pattern", /* keywords */
	    NULL, /* requirements */
	    IMAGE_WIDTH, IMAGE_HEIGHT,
	    NULL, draw)
