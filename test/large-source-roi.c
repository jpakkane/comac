/*
 * Copyright © 2009 Joonas Pihlaja
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "comac-test.h"

/* This test attempts to trigger failures in those clone_similar
 * backend methods that have size restrictions. */

static comac_surface_t *
create_large_source (int width, int height)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_image_surface_create (COMAC_FORMAT_RGB24, width, height);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 1, 0, 0); /* red */
    comac_paint (cr);
    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return surface;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *source;
    /* Since 1cc750ed92a936d84b47cac696aaffd226e1c02e pixman will not
     * paint on the source surface if source_width > 30582. */
    double source_width = 30000.0;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    /* Create an excessively wide source image, all red. */
    source = create_large_source (source_width, height);

    /* Set a transform so that the source is scaled down to fit in the
     * destination horizontally and then paint the entire source to
     * the context. */
    comac_scale (cr, width / source_width, 1.0);
    comac_set_source_surface (cr, source, 0, 0);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_paint (cr);

    comac_surface_destroy (source);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (large_source_roi,
	    "Uses a all of a large source image.",
	    "stress, source", /* keywords */
	    NULL,	      /* requirements */
	    7,
	    7,
	    NULL,
	    draw)
