/*
 * Copyright 2009 Chris Wilson
 * Copyright 2011 Intel Corporation
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

#define WIDTH 20
#define HEIGHT 20

static void
clip_mask (comac_t *cr)
{
    comac_move_to (cr, 10, 0);
    comac_line_to (cr, 0, 10);
    comac_line_to (cr, 10, 20);
    comac_line_to (cr, 20, 10);
    comac_clip (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    clip_mask (cr);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_paint (cr);
    comac_reset_clip (cr);

    comac_set_source_rgb (cr, 1, 0, 0);

    comac_rectangle (cr, 0, 0, 4, 4);
    comac_clip (cr);
    clip_mask (cr);
    comac_paint (cr);
    comac_reset_clip (cr);

    comac_rectangle (cr, 20, 0, -4, 4);
    comac_clip (cr);
    clip_mask (cr);
    comac_paint (cr);
    comac_reset_clip (cr);

    comac_rectangle (cr, 20, 20, -4, -4);
    comac_clip (cr);
    clip_mask (cr);
    comac_paint (cr);
    comac_reset_clip (cr);

    comac_rectangle (cr, 0, 20, 4, -4);
    comac_clip (cr);
    clip_mask (cr);
    comac_paint (cr);
    comac_reset_clip (cr);

    comac_set_source_rgb (cr, 0, 0, 1);

    comac_rectangle (cr, 8, 8, 4, 4);
    comac_clip (cr);
    clip_mask (cr);
    comac_paint (cr);
    comac_reset_clip (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_intersect,
	    "Tests intersection of a simple clip with a clip-mask",
	    "clip, paint", /* keywords */
	    NULL,	   /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
