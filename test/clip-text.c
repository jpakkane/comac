/*
 * Copyright 2009 Chris Wilson
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

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const char *comac = "Comac";
    comac_text_extents_t extents;
    double x0, y0;

    comac_text_extents (cr, comac, &extents);
    x0 = WIDTH / 2. - (extents.width / 2. + extents.x_bearing);
    y0 = HEIGHT / 2. - (extents.height / 2. + extents.y_bearing);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    /* aligned-clip */
    comac_save (cr);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_rectangle (cr, 3, 3, 10, 10);
    comac_rectangle (cr, 7, 7, 10, 10);
    comac_clip (cr);

    comac_set_source_rgb (cr, 0.7, 0, 0);
    comac_paint (cr);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_move_to (cr, x0, y0);
    comac_show_text (cr, comac);
    comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    /* force a clip-mask */
    comac_save (cr);
    comac_arc (cr, 10, 10, 10, 0, 2 * M_PI);
    comac_new_sub_path (cr);
    comac_arc_negative (cr, 10, 10, 5, 2 * M_PI, 0);
    comac_new_sub_path (cr);
    comac_arc (cr, 10, 10, 2, 0, 2 * M_PI);
    comac_clip (cr);

    comac_set_source_rgb (cr, 0, 0, 0.7);
    comac_paint (cr);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_move_to (cr, x0, y0);
    comac_show_text (cr, comac);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_text,
	    "Tests drawing text through complex clips.",
	    "clip, text", /* keywords */
	    NULL,	  /* requirements */
	    2 * WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
