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
#define PAD		(2 * LINE_WIDTH)

static void
make_path (comac_t *cr)
{
    int i;

    comac_save (cr);
    for (i = 0; i <= 3; i++) {
	comac_new_sub_path (cr);
	comac_move_to (cr, -SIZE / 2, 0.);
	comac_line_to (cr,  SIZE / 2, 0.);
	comac_rotate (cr, M_PI / 4.);
    }
    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr)
{
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, PAD + SIZE / 2., PAD + SIZE / 2.);

    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    make_path (cr);
    comac_stroke (cr);

    comac_translate (cr, 0, SIZE + PAD);

    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    make_path (cr);
    comac_stroke (cr);

    comac_translate (cr, 0, SIZE + PAD);

    comac_set_line_cap (cr, COMAC_LINE_CAP_SQUARE);
    comac_set_line_join (cr, COMAC_LINE_JOIN_MITER);
    make_path (cr);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_10 (comac_t *cr, int width, int height)
{
    comac_set_line_width (cr, LINE_WIDTH);
    return draw (cr);
}

static comac_test_status_t
draw_2 (comac_t *cr, int width, int height)
{
    comac_set_line_width (cr, 2);
    return draw (cr);
}

static comac_test_status_t
draw_1 (comac_t *cr, int width, int height)
{
    comac_set_line_width (cr, 1);
    return draw (cr);
}

static comac_test_status_t
draw_05 (comac_t *cr, int width, int height)
{
    comac_set_line_width (cr, 0.5);
    return draw (cr);
}

COMAC_TEST (caps,
	    "Test caps",
	    "stroke, caps", /* keywords */
	    NULL, /* requirements */
	    PAD + SIZE + PAD,
	    3 * (PAD + SIZE) + PAD,
	    NULL, draw_10)

COMAC_TEST (caps_2,
	    "Test normal caps",
	    "stroke, caps", /* keywords */
	    NULL, /* requirements */
	    PAD + SIZE + PAD,
	    3 * (PAD + SIZE) + PAD,
	    NULL, draw_2)

COMAC_TEST (caps_1,
	    "Test hairline caps",
	    "stroke, caps", /* keywords */
	    NULL, /* requirements */
	    PAD + SIZE + PAD,
	    3 * (PAD + SIZE) + PAD,
	    NULL, draw_1)

COMAC_TEST (caps_05,
	    "Test fine caps",
	    "stroke, caps", /* keywords */
	    NULL, /* requirements */
	    PAD + SIZE + PAD,
	    3 * (PAD + SIZE) + PAD,
	    NULL, draw_05)

