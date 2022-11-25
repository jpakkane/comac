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

/* Test case for bug #4409:
 *
 *	Dashes are missing initial caps
 *	https://bugs.freedesktop.org/show_bug.cgi?id=4409
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
    double dash[] = {LINE_WIDTH, 1.5 * LINE_WIDTH};
    double dash_offset = -2 * LINE_WIDTH;
    int i;

    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    for (i = 0; i < 2; i++) {
	comac_save (cr);
	comac_set_line_width (cr, LINE_WIDTH);
	comac_set_dash (cr, dash, ARRAY_LENGTH (dash), dash_offset);

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
	comac_translate (cr, 0., SIZE + PAD);
	dash_offset = 0;
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (dash_caps_joins,
	    "Test caps and joins when dashing",
	    "dash, stroke", /* keywords */
	    NULL,	    /* requirements */
	    3 * (PAD + SIZE) + PAD,
	    PAD + SIZE + PAD + SIZE + PAD,
	    NULL,
	    draw)
