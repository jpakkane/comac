/*
 * Copyright © Chris Wilson, 2008
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
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
 * Authors: Chris Wilson <chris@chris-wilson.co.uk>
 *          Larry Ewing <lewing@novell.com>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_rectangle (cr, 25, 25, width, height);
    comac_clip_preserve (cr);
    comac_push_group (cr);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_fill (cr);
    comac_rectangle (cr, 0, 0, width, height);
    comac_pop_group_to_source (cr);
    comac_paint (cr);

    comac_reset_clip (cr);
    comac_clip_preserve (cr);
    comac_set_source_rgba (cr, 1, 0, 0, .5);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (group_clip,
	    "test preserving paths across groups",
	    "group", /* keywords */
	    NULL, /* requirements */
	    40 + 25, 40 + 25,
	    NULL, draw)
