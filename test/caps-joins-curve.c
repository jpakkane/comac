/*
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define LINE_WIDTH	10.
#define SIZE		(5 * LINE_WIDTH)
#define PAD		(3 * LINE_WIDTH)

static void
make_path (comac_t *cr)
{
    comac_move_to (cr, 0, 0);
    comac_rel_curve_to (cr,
			-SIZE/4, SIZE/3,
			-SIZE/4, SIZE/3,
			0, SIZE);
    comac_rel_curve_to (cr,
			SIZE/3, -SIZE/4,
			SIZE/3, -SIZE/4,
			SIZE, 0);
    comac_close_path (cr);

    comac_move_to (cr, 5 * LINE_WIDTH, 3 * LINE_WIDTH);
    comac_rel_curve_to (cr,
			0, -3 * LINE_WIDTH,
			0, -3 * LINE_WIDTH,
			-3 * LINE_WIDTH, -3 * LINE_WIDTH);
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
draw (comac_t *cr, int width, int height)
{
    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_set_line_width (cr, LINE_WIDTH);

    draw_caps_joins (cr);

    /* and reflect to generate the opposite vertex ordering */
    comac_translate (cr, 0, height);
    comac_scale (cr, 1, -1);

    draw_caps_joins (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (caps_joins_curve,
	    "Test caps and joins on curves",
	    "stroke, cap, join", /* keywords */
	    NULL, /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    2 * (PAD + SIZE) + PAD,
	    NULL, draw)

