/*
 * Copyright © 2008 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Novell, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Larry Ewing <lewing@novell.com>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1, 1, 0);
    comac_move_to (cr, 50, 50);
    comac_rel_line_to (cr, 50000, 50000);
    comac_stroke (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_move_to (cr, 50, 50);
    comac_rel_line_to (cr, -50000, 50000);
    comac_stroke (cr);

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_move_to (cr, 50, 50);
    comac_rel_line_to (cr, 50000, -50000);
    comac_stroke (cr);

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_move_to (cr, 50, 50);
    comac_rel_line_to (cr, -50000, -50000);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (big_line,
	    "Test drawing of simple lines with positive and negative coordinates > 2^16",
	    "stroke, line", /* keywords */
	    NULL, /* requirements */
	    100, 100,
	    NULL, draw)
