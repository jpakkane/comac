/*
 * Copyright © 2007 Tim Rowley
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
 * Author: Tim Rowley
 */

#include "comac-test.h"
#include "stdio.h"

#define WIDTH 50
#define HEIGHT 50

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;

    comac_set_source_rgb (cr, 0.5, 0.5, 0.5);
    comac_paint (cr);

    comac_save (cr);

    pattern = comac_pattern_create_linear (0, 0, 10.0, 0);

    comac_pattern_add_color_stop_rgb (pattern, 0.0, 0.0, 0.0, 1.0);
    comac_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 0.0, 0.0);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_REFLECT);

    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);
    comac_rectangle (cr, 0.0, 0.0, WIDTH, HEIGHT);
    comac_fill (cr);

    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (linear_gradient_reflect,
	    "Tests the drawing of linear gradient with reflect",
	    "gradient", /* keywords */
	    NULL,	/* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
