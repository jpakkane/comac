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

#define SIZE 40

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_move_to (cr, SIZE, 0);
    comac_rel_line_to (cr, 0, SIZE);
    comac_rel_line_to (cr, -SIZE, 0);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_fill_preserve (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (implicit_close,
	    "Test implicitly closing paths",
	    "fill", /* keywords */
	    NULL,   /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)
