/*
 * Copyright © 2009 M Joonas Pihlaja
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: M Joonas Pihlaja <jpihlaja@cc.helsinki.fi>
 */

#include "comac-test.h"

/* When faced with very small dash lengths the stroker is liable to
 * get stuck in an infinite loop when advancing the dash offset.  This
 * test attempts to hit each of the locations in the stroker code
 * where the dash offset is advanced in a loop.
 *
 * Reported to the comac mailing list by Hans Breuer.
 * https://lists.comacgraphics.org/archives/comac/2009-June/017506.html
 */

#define EPS 1e-30
/* This should be comfortably smaller than the unit epsilon of the
 * floating point type used to advance the dashing, yet not small
 * enough that it underflows to zero.  1e-30 works to foil up to 80
 * bit extended precision arithmetic.  We want to avoid zero dash
 * lengths because those trigger special processing in the stroker. */

static void
do_dash (comac_t *cr, double dx, double dy, double offset)
{
    /* Set the dash pattern to be predominantly ON so that we can
     * create a reference image by just ignoring the dashing. */
    static double dash[] = { EPS, EPS/512 };
    comac_set_dash (cr, dash, 2, offset);
    comac_move_to (cr, 10, 10);
    comac_rel_line_to (cr, dx, dy);
    comac_stroke (cr);
    comac_translate (cr, dx, dy);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    (void)width; (void)height;

    comac_set_source_rgb (cr, 1,1,1);
    comac_paint (cr);
    comac_set_source_rgb (cr, 0,0,0);

    comac_set_line_width (cr, 10);

    /* The following calls will wedge in various places that try
     * to advance the dashing in a loop inside the stroker. */
    do_dash (cr, 30, 30, 0); /* _comac_stroker_line_to_dashed */
    do_dash (cr, 30,  0, 0); /* _comac_rectilinear_stroker_line_to_dashed */
    do_dash (cr, 30, 30, 1); /* _comac_stroker_dash_start */

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (dash_infinite_loop,
            "Test dashing with extremely small dash lengths.",
            "dash",
            NULL,
            100, 100,
            NULL, draw);
