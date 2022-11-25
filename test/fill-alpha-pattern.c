/*
 * Copyright © 2008 Chris Wilson
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

#define SIZE 60 /* needs to be big to check large area effects (dithering) */
#define PAD 2

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const double alpha = 1./3;
    comac_pattern_t *pattern;
    int n;

    /* draw a simple pattern behind */
    pattern = comac_pattern_create_linear (0, 0, width, height);
    comac_pattern_add_color_stop_rgb (pattern, 0, 1, 1, 0);
    comac_pattern_add_color_stop_rgb (pattern, 1, 1, 1, 1);
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);
    comac_paint (cr);

    /* square */
    comac_rectangle (cr, PAD, PAD, SIZE, SIZE);
    comac_set_source_rgba (cr, 1, 0, 0, alpha);
    comac_fill (cr);

    /* circle */
    comac_translate (cr, SIZE + 2 * PAD, 0);
    comac_arc (cr, PAD + SIZE / 2., PAD + SIZE / 2., SIZE / 2., 0, 2 * M_PI);
    comac_set_source_rgba (cr, 0, 1, 0, alpha);
    comac_fill (cr);

    /* triangle */
    comac_translate (cr, 0, SIZE + 2 * PAD);
    comac_move_to (cr, PAD + SIZE / 2, PAD);
    comac_line_to (cr, PAD + SIZE, PAD + SIZE);
    comac_line_to (cr, PAD, PAD + SIZE);
    comac_set_source_rgba (cr, 0, 0, 1, alpha);
    comac_fill (cr);

    /* star */
    comac_translate (cr, -(SIZE + 2 * PAD) + SIZE/2., SIZE/2.);
    for (n = 0; n < 5; n++) {
	comac_line_to (cr,
		       SIZE/2 * cos (2*n * 2*M_PI / 10),
		       SIZE/2 * sin (2*n * 2*M_PI / 10));

	comac_line_to (cr,
		       SIZE/4 * cos ((2*n+1)*2*M_PI / 10),
		       SIZE/4 * sin ((2*n+1)*2*M_PI / 10));
    }
    comac_set_source_rgba (cr, 0, 0, 0, alpha);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (fill_alpha_pattern,
	    "Tests using set_rgba();fill() over a linear gradient",
	    "fill, alpha", /* keywords */
	    NULL, /* requirements */
	    2*SIZE + 4*PAD, 2*SIZE + 4*PAD,
	    NULL, draw)
