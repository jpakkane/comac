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
 * Author: Owen Taylor <otaylor@redhat.com>
 */

#include "comac-test.h"

#define IMAGE_WIDTH 19
#define IMAGE_HEIGHT 19

/* Basic test of dashed strokes, including a test for the negative
 * dash offset bug:
 *
 *	https://bugs.freedesktop.org/show_bug.cgi?id=2729
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    double dashes[] = { 1 };

    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_set_line_width (cr, 2);

    /* Basic 1-1 dash pattern */
    comac_set_dash (cr, dashes, 1, 0.);

    comac_move_to (cr,  1, 2);
    comac_line_to (cr, 18, 2);
    comac_stroke (cr);

    /* Adjust path by 0.5. Ideally this would give a constant 50%
     * gray, (but does not due to the location of the regular sample
     * grid points. */
    comac_move_to (cr, 1.5, 5);
    comac_line_to (cr, 18., 5);
    comac_stroke (cr);

    /* Offset dash by 0.5, rather than the path */
    comac_set_dash (cr, dashes, 1, 0.5);

    comac_move_to (cr,  1, 8);
    comac_line_to (cr, 18, 8);
    comac_stroke (cr);

    /* Now, similar tests with negative dash offsets. */

    /* Basic 1-1 dash pattern dashing */
    comac_set_dash (cr, dashes, 1, -4);

    comac_move_to (cr,  1, 11);
    comac_line_to (cr, 18, 11);
    comac_stroke (cr);

    /* Adjust path by 0.5 */
    comac_move_to (cr, 1.5, 14);
    comac_line_to (cr, 18., 14);
    comac_stroke (cr);

    /* Offset dash by 0.5 */
    comac_set_dash (cr, dashes, 1, -3.5);

    comac_move_to (cr,  1, 17);
    comac_line_to (cr, 18, 17);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (dash_offset_negative,
	    "Tests comac_set_dash with a negative offset",
	    "dash, stroke", /* keywords */
	    NULL, /* requirements */
	    IMAGE_WIDTH, IMAGE_HEIGHT,
	    NULL, draw)
