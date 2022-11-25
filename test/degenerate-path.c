/*
 * Copyright © 2006 Jeff Muizelaar
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Jeff Muizelaar not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Jeff Muizelaar makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * JEFF MUIZELAAR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL JEFF MUIZELAAR BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Jeff Muizelaar <jeff@infidigm.net>
 */

#include "comac-test.h"

#define PAD 3.0
#define LINE_WIDTH 6.0

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_line_cap_t cap[] = {COMAC_LINE_CAP_ROUND,
				    COMAC_LINE_CAP_SQUARE,
				    COMAC_LINE_CAP_BUTT};
    size_t i;
    double dash[] = {2, 2};
    double dash_long[] = {6, 6};

    comac_set_source_rgb (cr, 1, 0, 0);

    for (i = 0; i < ARRAY_LENGTH (cap); i++) {
	comac_save (cr);

	comac_set_line_cap (cr, cap[i]);

	/* simple degenerate paths */
	comac_set_line_width (cr, LINE_WIDTH);
	comac_move_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_line_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_stroke (cr);

	comac_translate (cr, 0, 3 * PAD);
	comac_move_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_close_path (cr);
	comac_stroke (cr);

	/* degenerate paths starting with dash on */
	comac_set_dash (cr, dash, 2, 0.);

	comac_translate (cr, 0, 3 * PAD);
	comac_move_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_line_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_stroke (cr);

	comac_translate (cr, 0, 3 * PAD);
	comac_move_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_close_path (cr);
	comac_stroke (cr);

	/* degenerate paths starting with dash off */
	/* these should not draw anything */
	comac_set_dash (cr, dash, 2, 2.);

	comac_translate (cr, 0, 3 * PAD);
	comac_move_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_line_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_stroke (cr);

	comac_translate (cr, 0, 3 * PAD);
	comac_move_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_close_path (cr);
	comac_stroke (cr);

	/* this should draw a single degenerate sub-path
	 * at the end of the path */
	comac_set_dash (cr, dash_long, 2, 6.);

	comac_translate (cr, 0, 3 * PAD);
	comac_move_to (cr, LINE_WIDTH + 6.0, LINE_WIDTH);
	comac_line_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_stroke (cr);

	/* this should draw a single degenerate sub-path
	 * at the end of the path. The difference between this
	 * and the above is that this ends with a degenerate sub-path*/
	comac_set_dash (cr, dash_long, 2, 6.);

	comac_translate (cr, 0, 3 * PAD);
	comac_move_to (cr, LINE_WIDTH + 6.0, LINE_WIDTH);
	comac_line_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_line_to (cr, LINE_WIDTH, LINE_WIDTH);
	comac_stroke (cr);

	comac_restore (cr);

	comac_translate (cr, PAD + LINE_WIDTH + PAD, 0);
    }
    return COMAC_TEST_SUCCESS;
}

/*
 * XFAIL: undefined behaviour in PS, needs path editing to convert degenerate
 * segments into circles/rectangles as expected by comac
 */
COMAC_TEST (degenerate_path,
	    "Tests the behaviour of degenerate paths with different cap types",
	    "degenerate", /* keywords */
	    NULL,	  /* requirements */
	    3 * (PAD + LINE_WIDTH + PAD),
	    8 * (LINE_WIDTH + PAD) + PAD,
	    NULL,
	    draw)
