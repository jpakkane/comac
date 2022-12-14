/*
 * Copyright 2010 Red Hat
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Intel not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Intel makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Benjamin Otte <otte@gnome.com>
 */

#include "comac-test.h"

static void
rect (comac_t *cr, int x1, int y1, int x2, int y2)
{
    comac_rectangle (cr, x1, y1, x2 - x1, y2 - y1);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    comac_scale (cr, 1. / 256, 1. / 256);

    rect (cr, 0, 0, 29696, 7680);
    rect (cr, 0, 0, -15360, 15360);
    comac_clip (cr);

    comac_set_source_rgb (cr, 1, 0.5, 0);
    comac_paint (cr);

    rect (cr, 9984, 0, 2969, 3840);
    rect (cr, 0, 3840, 9472, 7680);
    comac_clip (cr);

    rect (cr, 0, 3840, 3584, 7680);
    comac_set_source_rgb (cr, 1, 0, 0.5);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_bo_rectangular,
	    "Tests a bug found by Benjamin Otte in the rectangular tessellator",
	    "tessellator", /* keywords */
	    NULL,	   /* requirements */
	    300,
	    300,
	    NULL,
	    draw)
