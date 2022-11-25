/*
 * Copyright © 2006 M Joonas Pihlaja
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
 * Author: M Joonas Pihlaja <jpihlaja@cc.helsinki.fi>
 */
#include "comac-test.h"

#define SIZE 512
#define NUM_SEGMENTS 128

static uint32_t state;

static double
uniform_random (double minval, double maxval)
{
    static uint32_t const poly = 0x9a795537U;
    uint32_t n = 32;
    while (n-->0)
	state = 2*state < state ? (2*state ^ poly) : 2*state;
    return minval + state * (maxval - minval) / 4294967296.0;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    state = 0x12345678;
    comac_translate (cr, 1, 1);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    comac_move_to (cr, 0, 0);
    for (i = 0; i < NUM_SEGMENTS; i++) {
	double x = uniform_random (0, width);
	double y = uniform_random (0, height);
	comac_line_to (cr, x, y);
    }
    comac_close_path (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill_preserve (cr);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_set_line_width (cr, 0.5);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (random_intersections_eo,
	    "Tests the tessellator trapezoid generation and intersection computation",
	    "trap", /* keywords */
	    NULL, /* requirements */
	    SIZE+3, SIZE+3,
	    NULL, draw)

