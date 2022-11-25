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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define SIZE 30

/* At one point, an optimization was proposed for comac in which a
 * curve_to would be optimized as a line_to. The initial (buggy)
 * implementation verified that the slopes of several segments of the
 * spline's control polygon were identical, but left open the
 * possibility of an anti-parallel slope for one segment.
 *
 * For example, given a spline with collinear control points (A,B,C,D)
 * positioned as follows:
 *
 *	C--A--B--D
 *
 * The code verified identical slopes for AB, CD, and AD. The missing
 * check for the BC segment allowed it to be anti-parallel to the
 * others as above, and hence invalid to replace this spline with the
 * AD line segment.
 */
static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);

    comac_set_line_width (cr, 1.0);
    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);
    comac_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */

    comac_translate (cr, 0, 1.0);

    /* The CABD spline as described above. We ensure that the spline
     * folds over on itself outside the bounds of the image to avoid
     * the reference image having the curved portion of that fold,
     * (which would just be harder to match in all the backends than
     * we really want). */
    comac_move_to (cr,
		     10.5, 0.5);
    comac_curve_to (cr,
		     11.5, 0.5,
		    -25.0, 0.5,
		     31.0, 0.5);

    comac_stroke (cr);

    comac_translate (cr, 0, 2.0);

    /* A reflected version: DBAC */
    comac_move_to (cr,
		    19.5, 0.5);

    comac_curve_to (cr,
		    18.5, 0.5,
		    55.0, 0.5,
		    -1.0, 0.5);

    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (curve_to_as_line_to,
	    "Test optimization treating curve_to as line_to",
	    "path", /* keywords */
	    NULL, /* requirements */
	    30,
	    5,
	    NULL, draw)
