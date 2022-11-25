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

/* This test exercises the various interactions between
 * comac_set_line_width and comac_scale. Specifically it shows how
 * separate transformations can affect the pen for stroking compared
 * to the path itself.
 *
 * This was inspired by an image by Maxim Shemanarev demonstrating the
 * flexible-pipeline nature of his Antigrain Geometry project:
 *
 *	http://antigrain.com/tips/line_alignment/conv_order.gif
 *
 * It also uncovered some behavior in comac that I found surprising.
 * Namely, comac_set_line_width was not transforming the width
 * according the the current CTM, but instead delaying that
 * transformation until the time of comac_stroke.
 *
 * This delayed behavior was released in comac 1.0 so we're going to
 * document this as the way comac_set_line_width works rather than
 * considering this a bug.
 */

#define LINE_WIDTH 13
#define SPLINE 50.0
#define XSCALE  0.5
#define YSCALE  2.0
#define WIDTH (XSCALE * SPLINE * 6.0)
#define HEIGHT (YSCALE * SPLINE * 2.0)

static void
spline_path (comac_t *cr)
{
    comac_save (cr);
    {
	comac_move_to (cr,
		       - SPLINE, 0);
	comac_curve_to (cr,
			- SPLINE / 4, - SPLINE,
			  SPLINE / 4,   SPLINE,
			  SPLINE, 0);
    }
    comac_restore (cr);
}

/* If we scale before setting the line width or creating the path,
 * then obviously both will be scaled. */
static void
scale_then_set_line_width_and_stroke (comac_t *cr)
{
    comac_scale (cr, XSCALE, YSCALE);
    comac_set_line_width (cr, LINE_WIDTH);
    spline_path (cr);
    comac_stroke (cr);
}

/* This is used to verify the results of
 * scale_then_set_line_width_and_stroke.
 *
 * It uses save/restore pairs to isolate the scaling of the path and
 * line_width and ensures that both are scaled.
 */
static void
scale_path_and_line_width (comac_t *cr)
{
    comac_save (cr);
    {
	comac_scale (cr, XSCALE, YSCALE);
	spline_path (cr);
    }
    comac_restore (cr);

    comac_save (cr);
    {
	comac_scale (cr, XSCALE, YSCALE);
	comac_set_line_width (cr, LINE_WIDTH);
	comac_stroke (cr);
    }
    comac_restore (cr);
}

/* This is the case that was surprising.
 *
 * Setting the line width before scaling doesn't change anything. The
 * line width will be interpreted under the CTM in effect at the time
 * of comac_stroke, so the line width will be scaled as well as the
 * path here.
 */
static void
set_line_width_then_scale_and_stroke (comac_t *cr)
{
    comac_set_line_width (cr, LINE_WIDTH);
    comac_scale (cr, XSCALE, YSCALE);
    spline_path (cr);
    comac_stroke (cr);
}

/* Here then is the way to achieve the alternate result.
 *
 * This uses save/restore pairs to isolate the scaling of the path and
 * line_width and ensures that the path is scaled while the line width
 * is not.
 */
static void
scale_path_not_line_width (comac_t *cr)
{
    comac_save (cr);
    {
	comac_scale (cr, XSCALE, YSCALE);
	spline_path (cr);
    }
    comac_restore (cr);

    comac_save (cr);
    {
	comac_set_line_width (cr, LINE_WIDTH);
	comac_stroke (cr);
    }
    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;
    void (* const figures[4]) (comac_t *cr) = {
	scale_then_set_line_width_and_stroke,
	scale_path_and_line_width,
	set_line_width_then_scale_and_stroke,
	scale_path_not_line_width
    };

    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */

    for (i = 0; i < 4; i++) {
	comac_save (cr);
	comac_translate (cr,
			 WIDTH/4  + (i % 2) * WIDTH/2,
			 HEIGHT/4 + (i / 2) * HEIGHT/2);
	(figures[i]) (cr);
	comac_restore (cr);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (line_width_scale,
	    "Tests interaction of comac_set_line_width with comac_scale",
	    "stroke", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
