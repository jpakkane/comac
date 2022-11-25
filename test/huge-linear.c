/*
 * Copyright © 2006 Benjamin Otte
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
 * Author: Benjamin Otte <otte@gnome.org>
 */

#include "comac-test.h"

/* set this to 0.1 to make this test work */
#define FACTOR 1.e6

/* XXX poppler-comac doesn't handle gradients very well... */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    comac_matrix_t mat = {0,
			  -4.5254285714285709 * FACTOR,
			  -2.6398333333333333 * FACTOR,
			  0,
			  0,
			  0};

    pattern =
	comac_pattern_create_linear (-16384 * FACTOR, 0, 16384 * FACTOR, 0);
    comac_pattern_add_color_stop_rgba (pattern,
				       0,
				       0.376471,
				       0.533333,
				       0.27451,
				       1);
    comac_pattern_add_color_stop_rgba (pattern, 1, 1, 1, 1, 1);
    comac_pattern_set_matrix (pattern, &mat);

    comac_scale (cr, 0.05, 0.05);
    comac_translate (cr, 6000, 3500);

    comac_set_source (cr, pattern);
    comac_rectangle (cr, -6000, -3500, 12000, 7000);
    comac_pattern_destroy (pattern);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (huge_linear,
	    "Test huge linear patterns",
	    "gradient, linear", /* keywords */
	    NULL,		/* requirements */
	    600,
	    350,
	    NULL,
	    draw)
