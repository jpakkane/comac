/*
 * Copyright © 2012 Adrian Johnson
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

#define SIZE 40
#define WIDTH (7*SIZE)
#define HEIGHT (5*SIZE)

#define FALLBACK_RES_X 300
#define FALLBACK_RES_Y 150

static void
rectangles (comac_t *cr)
{
    comac_save (cr);

    comac_rotate (cr, M_PI/8);
    comac_translate (cr, 2*SIZE, SIZE/16);
    comac_scale (cr, 1.5, 1.5);

    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_set_source_rgba (cr, 1, 0, 0, 0.5);
    comac_fill (cr);

    /* Select an operator not supported by PDF/PS/SVG to trigger fallback */
    comac_set_operator (cr, COMAC_OPERATOR_SATURATE);

    comac_rectangle (cr, SIZE/2, SIZE/2, SIZE, SIZE);
    comac_set_source_rgba (cr, 0, 1, 0, 0.5);
    comac_fill (cr);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_set_fallback_resolution (comac_get_target (cr), FALLBACK_RES_X, FALLBACK_RES_Y);

    rectangles (cr);
    comac_translate (cr, 3*SIZE, 0);
    comac_push_group (cr);
    rectangles (cr);
    comac_pop_group_to_source (cr);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (fallback,
	    "Check that fallback images are correct when fallback resolution is not 72ppi",
	    "fallback", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
