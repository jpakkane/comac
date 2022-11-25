/*
 * Copyright 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    double dash[2] = { 8, 4 };
    double radius;

    radius = width;
    if (height > radius)
	radius = height;

    /* fill the background using a big circle */
    comac_arc (cr, 0, 0, 4 * radius, 0, 2 * M_PI);
    comac_fill (cr);

    /* a rotated square - overlapping the corners */
    comac_save (cr);
    comac_save (cr);
    comac_translate (cr, width/2, height/2);
    comac_rotate (cr, M_PI/4);
    comac_scale (cr, M_SQRT2, M_SQRT2);
    comac_rectangle (cr, -width/2, -height/2, width, height);
    comac_restore (cr);
    comac_set_source_rgba (cr, 0, 1, 0, .5);
    comac_set_line_width (cr, radius/2);
    comac_stroke (cr);
    comac_restore (cr);

    /* and put some circles in the corners */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_new_sub_path (cr);
    comac_arc (cr, 0, 0, radius/4, 0, 2 * M_PI);
    comac_new_sub_path (cr);
    comac_arc (cr, width, 0, radius/4, 0, 2 * M_PI);
    comac_new_sub_path (cr);
    comac_arc (cr, width, height, radius/4, 0, 2 * M_PI);
    comac_new_sub_path (cr);
    comac_arc (cr, 0, height, radius/4, 0, 2 * M_PI);
    comac_fill (cr);

    /* a couple of pixel-aligned lines */
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_move_to (cr, width/2, -height);
    comac_rel_line_to (cr, 0, 3*height);
    comac_move_to (cr, -width, height/2);
    comac_rel_line_to (cr, 3*width, 0);
    comac_stroke (cr);

    /* a couple of dashed diagonals */
    comac_save (cr);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_set_dash (cr, dash, 2, 0);
    comac_set_line_width (cr, 4.);
    comac_move_to (cr, -width, -height);
    comac_line_to (cr, width+width, height+height);
    comac_move_to (cr, width+width, -height);
    comac_line_to (cr, -width, height+height);
    comac_stroke (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clipped_trapezoids,
	    "Tests clipping of trapezoids larger than the surface",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    40, 40,
	    NULL, draw)
