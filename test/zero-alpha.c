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

#define SIZE 3
#define REPS 10

/* History:
 *
 * 2006-06-13 Paul Giblock reports a "Strange alpha channel problem" on
 * the comac mailing list.
 *
 * 2006-06-13 Carl Worth writes this test in an attempt to reproduce
 * the problem. The test first fills in a 3x3 rectangle with red, then
 * starts pounding on that center pixel with various forms of
 * zero-alpha rendering to see if its value can ever be made to
 * change.
 *
 * 2006-06-13 Paul Giblock reports that this only happens with the
 * xlib backend, and then only on some systems.
 */
static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;
    comac_surface_t *surface;
    uint32_t zero = 0;

    /* First paint background red. */
    comac_set_source_rgb (cr, 1.0, 0.0, 0.0); /* red */
    comac_paint (cr);

    /* Then we paint zero-alpha in several ways (always REPS times): */

    /* 1. fill a rectangle with a zero-alpha solid source. */
    comac_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0); /* transparent */
    comac_rectangle (cr, 1.0, 1.0, 1.0, 1.0);
    for (i = 0; i < REPS; i++)
	comac_fill_preserve (cr);
    comac_new_path (cr);

    /* 2. paint with a zero-alpha image surface source. */
    surface = comac_image_surface_create_for_data ((unsigned char *) &zero,
						   COMAC_FORMAT_ARGB32,
						   1,
						   1,
						   4);
    comac_set_source_surface (cr, surface, 1, 1);
    for (i = 0; i < REPS; i++)
	comac_paint (cr);

    /* 3. clip to rectangle then paint with zero-alpha solid source. */
    comac_rectangle (cr, 1.0, 1.0, 1.0, 1.0);
    comac_clip (cr);
    comac_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0); /* transparent */
    for (i = 0; i < REPS; i++)
	comac_paint (cr);

    /* 4. With the clip still there, paint our image surface. */
    comac_set_source_surface (cr, surface, 1, 1);
    for (i = 0; i < REPS; i++)
	comac_paint (cr);

    comac_surface_finish (surface); /* zero will go out of scope */
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (zero_alpha,
	    "Testing that drawing with zero alpha has no effect",
	    "alpha", /* keywords */
	    NULL,    /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)
