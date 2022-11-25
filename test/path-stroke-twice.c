/*
 * Copyright 2010 Chris Wilson
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

/* Exercises a bug found by alois@astro.ch:
 *   https://bugs.freedesktop.org/show_bug.cgi?id=26010
 *   comac_line_to optimizes away path segment
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_move_to (cr, 10, 10);
    comac_line_to (cr, 10, 20);
    comac_line_to (cr, 20, 30);
    comac_line_to (cr, 10, 20);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (path_stroke_twice,
	    "Tests stroking of a path containing a segment drawn twice",
	    "path, stroke", /* keywords */
	    NULL, /* requirements */
	    40, 40,
	    NULL, draw)
