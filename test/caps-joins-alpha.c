/*
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Red Hat, Inc.
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

    comac_move_to (cr, 2 * LINE_WIDTH, 0.);
    comac_rel_line_to (cr, 3 * LINE_WIDTH, 0.);
    comac_rel_line_to (cr, 0., 3 * LINE_WIDTH);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* First draw a checkered background */
    comac_test_paint_checkered (cr);

    /* Then draw the original caps-joins test but with a bit of alphs thrown in. */
    comac_set_line_width (cr, LINE_WIDTH);

    comac_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.5); /* 50% red */
    comac_translate (cr, PAD, PAD);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    comac_stroke (cr);

    comac_set_source_rgba (cr, 0.0, 1.0, 0.0, 0.5); /* 50% green */
    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    comac_stroke (cr);

    comac_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.5); /* 50% blue */

    comac_translate (cr, SIZE + PAD, 0.);

    make_path (cr);
    comac_set_line_cap (cr, COMAC_LINE_CAP_SQUARE);
    comac_set_line_join (cr, COMAC_LINE_JOIN_MITER);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (caps_joins_alpha,
	    "Test caps and joins with some source alpha",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    PAD + SIZE + PAD,
	    NULL,
	    draw)
