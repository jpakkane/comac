/*
 * Copyright © 2009 Chris Wilson
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

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_text_extents_t extents;

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_paint (cr);

    comac_set_operator (cr, COMAC_OPERATOR_CLEAR);

    comac_translate (cr, 2, 2);
    comac_save (cr);
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_clip (cr);
    comac_rectangle (cr, 5, 5, 10, 10);
    comac_fill (cr);
    comac_restore (cr);

    comac_translate (cr, 20, 0);
    comac_save (cr);
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_clip (cr);
    comac_arc (cr, 10, 10, 8, 0, 2 * M_PI);
    comac_fill (cr);
    comac_restore (cr);

    comac_translate (cr, 0, 20);
    comac_save (cr);
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_clip (cr);
    comac_text_extents (cr, "Comac", &extents);
    comac_move_to (cr,
		   10 - (extents.width / 2. + extents.x_bearing),
		   10 - (extents.height / 2. + extents.y_bearing));
    comac_text_path (cr, "Comac");
    comac_fill (cr);
    comac_restore (cr);

    comac_translate (cr, -20, 0);
    comac_save (cr);
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_clip (cr);
    comac_move_to (cr, 10, 2);
    comac_line_to (cr, 18, 18);
    comac_line_to (cr, 2, 18);
    comac_close_path (cr);
    comac_fill (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clear,
	    "Test masked clears",
	    "paint, clear", /* keywords */
	    NULL,	    /* requirements */
	    44,
	    44,
	    NULL,
	    draw)
