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

/* This test case exercises a "Potential division by zero in comac_arc"
 * reported by Luiz Americo Pereira Camara <luizmed@oi.com.br>,
 * https://lists.cairographics.org/archives/comac/2008-May/014054.html.
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int n;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);

    comac_set_line_width (cr, 5);
    comac_set_source_rgb (cr, 0, 1, 0);
    for (n = 0; n < 8; n++) {
	double theta = n * 2 * M_PI / 8;
	comac_new_sub_path (cr);
	comac_arc (cr, 20, 20, 15, theta, theta);
	comac_close_path (cr);
    }
    comac_stroke (cr);

    comac_set_line_width (cr, 2);
    comac_set_source_rgb (cr, 0, 0, 1);
    for (n = 0; n < 8; n++) {
	double theta = n * 2 * M_PI / 8;
	comac_move_to (cr, 20, 20);
	comac_arc (cr, 20, 20, 15, theta, theta);
    }
    comac_stroke (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_arc (cr, 20, 20, 2, 0, 2 * M_PI);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (degenerate_arc,
	    "Tests the behaviour of degenerate arcs",
	    "degenerate", /* keywords */
	    NULL,	  /* requirements */
	    40,
	    40,
	    NULL,
	    draw)
