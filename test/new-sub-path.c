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

#define SIZE 10

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 0.0, 0.0, 1.0); /* blue */

    /* Test comac_new_sub_path followed by several different
     * path-modification functions in turn...
     */

    /* ... comac_move_to */
    comac_new_sub_path (cr);
    comac_move_to (cr, SIZE, SIZE);
    comac_line_to (cr, SIZE, 2 * SIZE);

    /* ... comac_line_to */
    comac_new_sub_path (cr);
    comac_line_to (cr, 2 * SIZE, 1.5 * SIZE);
    comac_line_to (cr, 3 * SIZE, 1.5 * SIZE);

    /* ... comac_curve_to */
    comac_new_sub_path (cr);
    comac_curve_to (cr,
		    4.0 * SIZE,
		    1.5 * SIZE,
		    4.5 * SIZE,
		    1.0 * SIZE,
		    5.0 * SIZE,
		    1.5 * SIZE);

    /* ... comac_arc */
    comac_new_sub_path (cr);
    comac_arc (cr, 6.5 * SIZE, 1.5 * SIZE, 0.5 * SIZE, 0.0, 2.0 * M_PI);

    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (new_sub_path,
	    "Test the comac_new_sub_path call",
	    "path", /* keywords */
	    NULL,   /* requirements */
	    8 * SIZE,
	    3 * SIZE,
	    NULL,
	    draw)
