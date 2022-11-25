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

#define PAD		1
#define LINE_WIDTH	2
#define HEIGHT		(PAD + 4 * (LINE_WIDTH + PAD))
#define WIDTH		16

static void
line (comac_t *cr)
{
    comac_move_to (cr, PAD, 0.0);
    comac_line_to (cr, WIDTH - PAD, 0.0);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    double dash = 2.0;

    /* We draw in black, so paint white first. */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);

    comac_set_source_rgb (cr, 0, 0, 0); /* black */

    comac_translate (cr, 0.0, PAD + LINE_WIDTH / 2);

    /* First draw a solid line... */
    line (cr);
    comac_stroke (cr);

    comac_translate (cr, 0.0, LINE_WIDTH + PAD);

    /* then a dashed line... */
    comac_set_dash (cr, &dash, 1, 0.0);
    line (cr);
    comac_stroke (cr);

    comac_translate (cr, 0.0, LINE_WIDTH + PAD);

    /* back to solid... */
    comac_set_dash (cr, NULL, 0, 0.0);
    line (cr);
    comac_stroke (cr);

    comac_translate (cr, 0.0, LINE_WIDTH + PAD);

    /* and finally, back to dashed. */
    comac_set_dash (cr, &dash, 1, 0.0);
    line (cr);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (dash_no_dash,
	    "Tests that we can actually turn dashing on and off again",
	    "dash, stroke", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
