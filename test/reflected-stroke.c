/*
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * the author not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The author makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHOR. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

static void
draw_symbol (comac_t *cr)
{
    double dash[] = {6, 3};

    comac_rectangle (cr, -25, -25, 50, 50);
    comac_stroke (cr);

    comac_move_to (cr, 0, -25);
    comac_curve_to (cr, 12.5, -12.5, 12.5, -12.5, 0, 0);
    comac_curve_to (cr, -12.5, 12.5, -12.5, 12.5, 0, 25);
    comac_curve_to (cr, 12.5, 12.5, 12.5, 12.5, 0, 0);
    comac_stroke (cr);

    comac_save (cr);
    comac_set_dash (cr, dash, ARRAY_LENGTH (dash), 0.);
    comac_move_to (cr, 0, 0);
    comac_arc (cr, 0, 0, 12.5, 0, 3 * M_PI / 2);
    comac_close_path (cr);
    comac_stroke (cr);
    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0, 0, 0);

    comac_save (cr);
    comac_translate (cr, 50, 50);
    comac_scale (cr, 1, 1);
    draw_symbol (cr);
    comac_restore (cr);

    comac_save (cr);
    comac_translate (cr, 150, 50);
    comac_scale (cr, -1, 1);
    draw_symbol (cr);
    comac_restore (cr);

    comac_save (cr);
    comac_translate (cr, 150, 150);
    comac_scale (cr, -1, -1);
    draw_symbol (cr);
    comac_restore (cr);

    comac_save (cr);
    comac_translate (cr, 50, 150);
    comac_scale (cr, 1, -1);
    draw_symbol (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (reflected_stroke,
	    "Exercises the stroker with a reflected ctm",
	    "stroke, transform", /* keywords */
	    NULL,		 /* requirements */
	    200,
	    200,
	    NULL,
	    draw)
