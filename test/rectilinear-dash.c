/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2008 Chris Wilson
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
 * Author: Carl D. Worth <cworth@cworth.org>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 *
 * Based on the original test/rectilinear-stroke.c by Carl D. Worth.
 */

#include "comac-test.h"

#define SIZE 50

static void
draw_dashes (comac_t *cr)
{
    const double dash_square[4] = {4, 2, 2, 2};
    const double dash_butt[4] = {5, 1, 3, 1};

    comac_save (cr);

    comac_set_dash (cr, dash_square, 4, 0);

    comac_set_line_width (cr, 1.0);
    comac_translate (cr, 1, 1);

    /* Draw everything first with square caps. */
    comac_set_line_cap (cr, COMAC_LINE_CAP_SQUARE);

    /* Draw horizontal and vertical segments, each in both
     * directions. */
    comac_move_to (cr, 4.5, 0.5);
    comac_rel_line_to (cr, 2.0, 0.0);

    comac_move_to (cr, 10.5, 4.5);
    comac_rel_line_to (cr, 0.0, 2.0);

    comac_move_to (cr, 6.5, 10.5);
    comac_rel_line_to (cr, -2.0, 0.0);

    comac_move_to (cr, 0.5, 6.5);
    comac_rel_line_to (cr, 0.0, -2.0);

    /* Draw right angle turns in four directions. */
    comac_move_to (cr, 0.5, 2.5);
    comac_rel_line_to (cr, 0.0, -2.0);
    comac_rel_line_to (cr, 2.0, 0.0);

    comac_move_to (cr, 8.5, 0.5);
    comac_rel_line_to (cr, 2.0, 0.0);
    comac_rel_line_to (cr, 0.0, 2.0);

    comac_move_to (cr, 10.5, 8.5);
    comac_rel_line_to (cr, 0.0, 2.0);
    comac_rel_line_to (cr, -2.0, 0.0);

    comac_move_to (cr, 2.5, 10.5);
    comac_rel_line_to (cr, -2.0, 0.0);
    comac_rel_line_to (cr, 0.0, -2.0);

    comac_stroke (cr);

    /* Draw a closed-path rectangle */
    comac_rectangle (cr, 0.5, 12.5, 10.0, 10.0);
    comac_set_dash (cr, dash_square, 4, 2);
    comac_stroke (cr);

    comac_translate (cr, 12, 0);

    /* Now draw the same results, but with butt caps. */
    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    comac_set_dash (cr, dash_butt, 4, 0.0);

    /* Draw horizontal and vertical segments, each in both
     * directions. */
    comac_move_to (cr, 4.0, 0.5);
    comac_rel_line_to (cr, 3.0, 0.0);

    comac_move_to (cr, 10.5, 4.0);
    comac_rel_line_to (cr, 0.0, 3.0);

    comac_move_to (cr, 7.0, 10.5);
    comac_rel_line_to (cr, -3.0, 0.0);

    comac_move_to (cr, 0.5, 7.0);
    comac_rel_line_to (cr, 0.0, -3.0);

    /* Draw right angle turns in four directions. */
    comac_move_to (cr, 0.5, 3.0);
    comac_rel_line_to (cr, 0.0, -2.5);
    comac_rel_line_to (cr, 2.5, 0.0);

    comac_move_to (cr, 8.0, 0.5);
    comac_rel_line_to (cr, 2.5, 0.0);
    comac_rel_line_to (cr, 0.0, 2.5);

    comac_move_to (cr, 10.5, 8.0);
    comac_rel_line_to (cr, 0.0, 2.5);
    comac_rel_line_to (cr, -2.5, 0.0);

    comac_move_to (cr, 3.0, 10.5);
    comac_rel_line_to (cr, -2.5, 0.0);
    comac_rel_line_to (cr, 0.0, -2.5);

    comac_stroke (cr);

    /* Draw a closed-path rectangle */
    comac_set_dash (cr, dash_butt, 4, 2.5);
    comac_rectangle (cr, 0.5, 12.5, 10.0, 10.0);
    comac_stroke (cr);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* Paint background white, then draw in black. */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    draw_dashes (cr);

    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
    comac_translate (cr, 0, height);
    comac_scale (cr, 1, -1);
    draw_dashes (cr);
    comac_restore (cr);

    comac_save (cr);
    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
    comac_translate (cr, width, 0);
    comac_scale (cr, -1, 1);
    draw_dashes (cr);
    comac_restore (cr);

    comac_save (cr);
    comac_set_source_rgb (cr, 0.0, 0.0, 1.0);
    comac_translate (cr, width, height);
    comac_scale (cr, -1, -1);
    draw_dashes (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    rectilinear_dash,
    "Test dashed rectilinear stroke operations (covering only whole pixels)",
    "stroke, dash", /* keywords */
    NULL,	    /* requirements */
    SIZE,
    SIZE,
    NULL,
    draw)
