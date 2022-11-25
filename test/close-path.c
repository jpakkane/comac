/*
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
#include <stdlib.h>

static comac_test_draw_function_t draw;

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_path_t *path;

    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    /* This curious approach for drawing a circle (starting with a
     * closed arc) exercises a bug in which the "last move point" was
     * not being set so the close_path closes to (0,0). */
    comac_arc (cr, 8, 8, 4, 0, M_PI);
    comac_close_path (cr);
    comac_arc (cr, 8, 8, 4, M_PI, 2 * M_PI);

    comac_fill (cr);

    comac_translate (cr, 16, 0);

    /* Here a curve immediately after a close_to will begin from (0,0)
     * when the path is obtained with comac_copy_path_flat. */
    comac_move_to (cr, 8, 4);
    comac_arc_negative (cr, 8, 8, 4, 3 * M_PI / 2.0, M_PI / 2.0);
    comac_close_path (cr);
    comac_curve_to (cr,
		    12, 4,
		    12, 12,
		    8, 12);

    path = comac_copy_path_flat (cr);
    comac_new_path (cr);
    comac_append_path (cr, path);
    comac_path_destroy (path);

    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (close_path,
	    "Test some corner cases related to comac_close_path",
	    "path", /* keywords */
	    NULL, /* requirements */
	    32, 16,
	    NULL, draw)
