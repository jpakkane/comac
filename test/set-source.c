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

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;
    uint32_t color = 0x8019334c;
    comac_surface_t *surface;
    comac_pattern_t *pattern;

    surface = comac_image_surface_create_for_data ((unsigned char *) &color,
						   COMAC_FORMAT_ARGB32,
						   1,
						   1,
						   4);
    pattern = comac_pattern_create_for_surface (surface);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_REPEAT);

    /* Several different means of making mostly the same color (though
     * we can't get anything but alpha==1.0 out of
     * comac_set_source_rgb. */
    for (i = 0; i < width; i++) {
	switch (i) {
	case 0:
	    comac_set_source_rgb (cr, .6, .7, .8);
	    break;
	case 1:
	    comac_set_source_rgba (cr, .2, .4, .6, 0.5);
	    break;
	case 2:
#if WE_HAD_SUPPORT_FOR_PREMULTIPLIED
	    comac_set_source_rgba_premultiplied (cr, .1, .2, .3, 0.5);
#else
	    comac_set_source_rgba (cr, .2, .4, .6, 0.5);
#endif
	    break;
	case 3:
	default:
	    comac_set_source (cr, pattern);
	    break;
	}

	comac_rectangle (cr, i, 0, 1, height);
	comac_fill (cr);
    }

    comac_pattern_destroy (pattern);
    comac_surface_finish (surface); /* data will go out of scope */
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (set_source,
	    "Tests calls to various set_source functions",
	    "api", /* keywords */
	    NULL,  /* requirements */
	    5,
	    5,
	    NULL,
	    draw)
