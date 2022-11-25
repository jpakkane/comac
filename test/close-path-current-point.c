/*
 * Copyright © 2009 Nis Martensen
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of the copyright holder
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission. The
 * copyright holder makes no representations about the suitability of
 * this software for any purpose. It is provided "as is" without
 * express or implied warranty.
 *
 * THE COPYRIGHT HOLDER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Nis Martensen <nis.martensen@web.de>
 */

#include "comac-test.h"

#define SIZE 20

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    /* subpath starts with comac_move_to */
    comac_new_sub_path (cr);
    comac_move_to (cr, SIZE, SIZE);
    comac_rel_line_to (cr, SIZE, 0);
    comac_rel_line_to (cr, 0, SIZE);
    comac_close_path (cr);
    comac_rel_line_to (cr, 0.5 * SIZE, SIZE);

    /* subpath starts with comac_line_to */
    comac_new_sub_path (cr);
    comac_line_to (cr, SIZE, 3 * SIZE);
    comac_rel_line_to (cr, SIZE, 0);
    comac_rel_line_to (cr, 0, SIZE);
    comac_close_path (cr);
    comac_rel_line_to (cr, 0, SIZE);

    /* subpath starts with comac_curve_to */
    comac_new_sub_path (cr);
    comac_curve_to (cr,
		    SIZE,
		    5 * SIZE,
		    1.5 * SIZE,
		    6 * SIZE,
		    2 * SIZE,
		    5 * SIZE);
    comac_rel_line_to (cr, 0, SIZE);
    comac_close_path (cr);
    comac_rel_line_to (cr, -0.5 * SIZE, SIZE);

    /* subpath starts with comac_arc */
    comac_new_sub_path (cr);
    comac_arc (cr, 1.5 * SIZE, 7 * SIZE, 0.5 * SIZE, M_PI, 2 * M_PI);
    comac_rel_line_to (cr, 0, SIZE);
    comac_close_path (cr);
    comac_rel_line_to (cr, -0.7 * SIZE, 0.7 * SIZE);

    /* subpath starts with comac_arc_negative */
    comac_new_sub_path (cr);
    comac_arc_negative (cr, 1.5 * SIZE, 9 * SIZE, 0.5 * SIZE, M_PI, 2 * M_PI);
    comac_rel_line_to (cr, 0, SIZE);
    comac_close_path (cr);
    comac_rel_line_to (cr, -0.8 * SIZE, 0.3 * SIZE);

    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (close_path_current_point,
	    "Test some corner cases related to comac path operations and the "
	    "current point",
	    "path", /* keywords */
	    NULL,   /* requirements */
	    3 * SIZE,
	    11 * SIZE,
	    NULL,
	    draw)
