/*
 * Copyright © 2004 Red Hat, Inc.
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

#define STAR_SIZE 20

static void
star_path (comac_t *cr)
{
    comac_move_to (cr, 10, 0);
    comac_rel_line_to (cr, 6, 20);
    comac_rel_line_to (cr, -16, -12);
    comac_rel_line_to (cr, 20, 0);
    comac_rel_line_to (cr, -16, 12);
}

/* Use clipping to draw the same path twice, once with each fill rule */
static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 0, 0);

    comac_translate (cr, 1, 1);
    comac_save (cr);
    {
	star_path (cr);
	comac_set_fill_rule (cr, COMAC_FILL_RULE_WINDING);
	comac_clip (cr);
	comac_paint (cr);
    }
    comac_restore (cr);

    comac_translate (cr, STAR_SIZE + 1, 0);
    comac_save (cr);
    {
	star_path (cr);
	comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
	comac_clip (cr);
	comac_paint (cr);
    }
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
a1_draw (comac_t *cr, int width, int height)
{
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    return draw (cr, width, height);
}

COMAC_TEST (clip_fill_rule,
	    "Tests interaction of clipping with comac_set_fill_rule",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    STAR_SIZE * 2 + 2,
	    STAR_SIZE + 2,
	    NULL,
	    draw)
COMAC_TEST (a1_clip_fill_rule,
	    "Tests interaction of clipping with comac_set_fill_rule",
	    "clip",	     /* keywords */
	    "target=raster", /* requirements */
	    STAR_SIZE * 2 + 2,
	    STAR_SIZE + 2,
	    NULL,
	    a1_draw)
