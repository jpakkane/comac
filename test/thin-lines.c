/*
 * Copyright © 2015 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"

#define CELL_WIDTH 20
#define CELL_HEIGHT 20
#define PAD 2
#define IMAGE_WIDTH (CELL_WIDTH*3 + PAD*4)
#define IMAGE_HEIGHT (CELL_HEIGHT*4 + PAD*5)


static void
draw_lines(comac_t *cr)
{
    /* horizontal line */
    comac_translate (cr, PAD, PAD);
    comac_move_to (cr, 0, CELL_HEIGHT/2);
    comac_line_to (cr, CELL_WIDTH, CELL_HEIGHT/2);
    comac_stroke (cr);

    /* vertical line */
    comac_translate (cr, 0, CELL_HEIGHT + PAD);
    comac_move_to (cr, CELL_WIDTH/2, 0);
    comac_line_to (cr, CELL_WIDTH/2, CELL_HEIGHT);
    comac_stroke (cr);

    /* diagonal line */
    comac_translate (cr, 0, CELL_HEIGHT + PAD);
    comac_move_to (cr, 0, CELL_HEIGHT);
    comac_line_to (cr, CELL_WIDTH, 0);
    comac_stroke (cr);

    /* curved line */
    comac_translate (cr, 0, CELL_HEIGHT + PAD);
    comac_move_to (cr, CELL_WIDTH, 0);
    comac_curve_to (cr, 0, 0,
		    CELL_WIDTH, CELL_HEIGHT,
		    0, CELL_HEIGHT);
    comac_stroke (cr);
}

#define FIXED_POINT_MIN (1.0/256)

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_save (cr);
    comac_set_line_width (cr, FIXED_POINT_MIN*10.0);
    draw_lines (cr);
    comac_restore (cr);

    comac_translate (cr, CELL_WIDTH + PAD, 0);
    comac_save (cr);
    comac_set_line_width (cr, FIXED_POINT_MIN);
    draw_lines (cr);
    comac_restore (cr);

    comac_translate (cr, CELL_WIDTH + PAD, 0);
    comac_save (cr);
    comac_set_line_width (cr, FIXED_POINT_MIN/10.0);
    draw_lines (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (thin_lines,
	    "Tests that very thin lines are output to vector surfaces",
	    "stroke", /* keywords */
	    NULL, /* requirements */
	    IMAGE_WIDTH, IMAGE_HEIGHT,
	    NULL, draw)
