/*
 * Copyright © 2005 Red Hat, Inc.
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
 * Author: Carl Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define WIDTH 45
#define HEIGHT 30

static void
draw_L_shape (comac_t *cr)
{
    comac_move_to (cr, 0, 0);
    comac_rel_line_to (cr, 0, 10);
    comac_rel_line_to (cr, 5, 0);

    comac_save (cr);
    comac_identity_matrix (cr);
    comac_set_line_width (cr, 2.0);
    comac_stroke (cr);
    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, 5, 5);

    draw_L_shape (cr);

    comac_translate (cr, 10, 0);

    comac_save (cr);
    {
	comac_scale (cr, 2, 2);
	draw_L_shape (cr);
    }
    comac_restore (cr);

    comac_translate (cr, 15, 0);

    comac_save (cr);
    {
	comac_rotate (cr, M_PI / 2.0);
	draw_L_shape (cr);
    }
    comac_restore (cr);

    comac_translate (cr, 5, 0);

    comac_save (cr);
    {
	comac_matrix_t skew_y = {
	    1, -1,
	    0,  1,
	    0,  0
	};
	comac_transform (cr, &skew_y);
	draw_L_shape (cr);
    }
    comac_restore (cr);

    comac_translate (cr, 5, 10);

    comac_save (cr);
    {
	comac_matrix_t skew_x = {
	     1.0, 0.0,
	    -0.5, 1.0,
	     0.0, 0.0
	};
	comac_transform (cr, &skew_x);
	draw_L_shape (cr);
    }
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (transforms,
	    "Test various transformations.",
	    "transforms, api", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
