/*
 * Copyright © 2019 Uli Schlachter
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
 */

#include "comac-test.h"

/* This is basically gears.shape.arrow */
static void
shape (comac_t *cr, double width, double height)
{
    double shaft_length = height / 2;
    double shaft_width = width / 2;
    double head_width = width;
    double head_length = height - shaft_length;

    comac_move_to (cr, width / 2, 0);
    comac_rel_line_to (cr, head_width / 2, head_length);
    comac_rel_line_to (cr, -(head_width - shaft_width) / 2, 0);
    comac_rel_line_to (cr, 0, shaft_length);
    comac_rel_line_to (cr, -shaft_width, 0);
    comac_rel_line_to (cr, 0, -shaft_length);
    comac_rel_line_to (cr, -(head_width - shaft_width) / 2, 0);
    comac_close_path (cr);
}

/* This is basically the drawing routine of wibox.container.background */
static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    double bw = 1.5;

    comac_push_group_with_content (cr, COMAC_CONTENT_COLOR_ALPHA);

    /* Draw background */
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_paint (cr);

    /* Create shape */
    comac_translate (cr, bw, bw);
    shape (cr, width - 2 * bw, height - 2 * bw);
    comac_translate (cr, -bw, -bw);

    /* Now do the border */

    comac_push_group_with_content (cr, COMAC_CONTENT_ALPHA);

    /* Mark everything as "is border" */
    comac_set_source_rgba (cr, 0, 0, 0, 1);
    comac_paint (cr);

    /* Remove inside of the shape */
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgba (cr, 0, 0, 0, 0);
    comac_fill_preserve (cr);

    comac_pattern_t *mask = comac_pop_group (cr);

    /* Now actually draw the border via the mask */
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_mask (cr, mask);

    comac_pattern_destroy (mask);

    /* We now have the right content in a temporary surface. Copy it to the
     * target surface. Needs another mask.
     */
    comac_push_group_with_content (cr, COMAC_CONTENT_ALPHA);

    comac_set_line_width (cr, 2 * bw);
    comac_set_source_rgba (cr, 0, 0, 0, 1);
    comac_stroke_preserve (cr);
    comac_fill (cr);

    mask = comac_pop_group (cr);
    comac_pop_group_to_source (cr);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_mask (cr, mask);

    comac_pattern_destroy (mask);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_361,
	    "Bug 361 (Problem with SVG backend and masks)",
	    "mask, operator", /* keywords */
	    NULL,	      /* requirements */
	    100,
	    100,
	    NULL,
	    draw)
