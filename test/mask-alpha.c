/*
 * Copyright © 2007 Adrian Johnson
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
#define PAD 2
#define WIDTH (PAD + SIZE + PAD)
#define HEIGHT WIDTH

/* This test is designed to test that PDF viewers use the correct
 * alpha values in an Alpha SMasks. Some viewers use the color values
 * instead of the alpha. The test draws a triangle and rectangle in a
 * group then draws the group using comac_mask(). The mask consists of
 * a circle with the rgba (0.4, 0.4, 0.4, 0.8) and the background rgba
 * (0.8, 0.8, 0.8, 0.4).
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;

    comac_translate (cr, PAD, PAD);

    /* mask */
    comac_push_group (cr);
    comac_set_source_rgba (cr, 0.8, 0.8, 0.8, 0.4);
    comac_paint (cr);
    comac_arc (cr, SIZE / 2, SIZE / 2, SIZE / 6, 0., 2. * M_PI);
    comac_set_source_rgba (cr, 0.4, 0.4, 0.4, 0.8);
    comac_fill (cr);
    pattern = comac_pop_group (cr);

    /* source */
    comac_push_group (cr);
    comac_rectangle (cr, 0.3 * SIZE, 0.2 * SIZE, 0.5 * SIZE, 0.5 * SIZE);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_fill (cr);
    comac_move_to     (cr,   0.0,          0.8 * SIZE);
    comac_rel_line_to (cr,   0.7 * SIZE,   0.0);
    comac_rel_line_to (cr, -0.375 * SIZE, -0.6 * SIZE);
    comac_close_path (cr);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_fill (cr);
    comac_pop_group_to_source (cr);

    comac_mask (cr, pattern);
    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (mask_alpha,
	    "A simple test painting a group through a circle mask",
	    "mask, alpha", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
