/*
 * Copyright © 2005 Billy Biggs
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Billy Biggs not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Billy Biggs makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * BILLY BIGGS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL BILLY BIGGS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Billy Biggs <vektor@dumbterm.net>
 */

#include "comac-test.h"

/* The star shape from the SVG test suite, from the fill rule test */
static void
big_star_path (comac_t *cr)
{
    comac_move_to (cr, 40, 0);
    comac_rel_line_to (cr, 25, 80);
    comac_rel_line_to (cr, -65, -50);
    comac_rel_line_to (cr, 80, 0);
    comac_rel_line_to (cr, -65, 50);
    comac_close_path (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    /* Try a circle */
    comac_arc (cr, 40, 40, 20, 0, 2 * M_PI);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill (cr);

    /* Try using clipping to draw a circle */
    comac_arc (cr, 100, 40, 20, 0, 2 * M_PI);
    comac_clip (cr);
    comac_rectangle (cr, 80, 20, 40, 40);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_fill (cr);

    /* Reset the clipping */
    comac_reset_clip (cr);

    /* Draw a bunch of lines */
    comac_set_line_width (cr, 1.0);
    comac_set_source_rgb (cr, 0, 1, 0);
    for (i = 0; i < 10; i++) {
        comac_move_to (cr, 10, 70 + (i * 4));
        comac_line_to (cr, 120, 70 + (i * 18));
        comac_stroke (cr);
    }

    /* Try filling a poly */
    comac_translate (cr, 160, 120);
    comac_set_source_rgb (cr, 1, 1, 0);
    big_star_path (cr);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_fill (cr);
    comac_translate (cr, -160, -120);

    /* How about some curves? */
    comac_set_source_rgb (cr, 1, 0, 1);
    for (i = 0; i < 10; i++) {
        comac_move_to (cr, 150, 50 + (i * 5));
        comac_curve_to (cr, 250, 50, 200, (i * 10), 300, 50 + (i * 10));
        comac_stroke (cr);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (unantialiased_shapes,
	    "Test shape drawing without antialiasing",
	    "fill, stroke", /* keywords */
	    "target=raster", /* requirements */
	    320, 240,
	    NULL, draw)
