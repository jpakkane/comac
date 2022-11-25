/*
 * Copyright © 2005 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define LINE_WIDTH 10.
#define SIZE (5 * LINE_WIDTH)
#define PAD (2 * LINE_WIDTH)

static void
make_path (comac_t *cr)
{
    comac_move_to (cr, 0., 0.);
    comac_rel_line_to (cr, 0., SIZE);
    comac_rel_line_to (cr, SIZE, 0.);
    comac_close_path (cr);

    comac_move_to (cr, 5 * LINE_WIDTH, 3 * LINE_WIDTH);
    comac_rel_line_to (cr, 0., -3 * LINE_WIDTH);
    comac_rel_line_to (cr, -3 * LINE_WIDTH, 0.);
}

static void
draw_caps_joins (comac_t *cr)
{
    comac_save (cr);

    comac_translate (cr, PAD, PAD);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    comac_stroke (cr);

    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    comac_stroke (cr);

    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_SQUARE);
    comac_set_line_join (cr, COMAC_LINE_JOIN_MITER);
    comac_stroke (cr);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, float line_width)
{
    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_set_line_width (cr, line_width);

    draw_caps_joins (cr);

    /* and reflect to generate the opposite vertex ordering */
    comac_translate (cr, 0, 2 * (PAD + SIZE) + PAD);
    comac_scale (cr, 1, -1);

    draw_caps_joins (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_10 (comac_t *cr, int width, int height)
{
    return draw (cr, LINE_WIDTH);
}

static comac_test_status_t
draw_2 (comac_t *cr, int width, int height)
{
    return draw (cr, 2.0);
}

static comac_test_status_t
draw_1 (comac_t *cr, int width, int height)
{
    return draw (cr, 1.0);
}

static comac_test_status_t
draw_05 (comac_t *cr, int width, int height)
{
    return draw (cr, 0.5);
}

COMAC_TEST (caps_joins,
	    "Test caps and joins",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    2 * (PAD + SIZE) + PAD,
	    NULL,
	    draw_10)

COMAC_TEST (caps_joins_2,
	    "Test caps and joins with default line width",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    2 * (PAD + SIZE) + PAD,
	    NULL,
	    draw_2)

COMAC_TEST (caps_joins_1,
	    "Test caps and joins with hairlines",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    2 * (PAD + SIZE) + PAD,
	    NULL,
	    draw_1)

COMAC_TEST (caps_joins_05,
	    "Test caps and joins with fine lines",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    2 * (PAD + SIZE) + PAD,
	    NULL,
	    draw_05)
