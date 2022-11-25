/*
 * Copyright 2008 Kai-Uwe Behrmann
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Kai-Uwe Behrmann not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Kai-Uwe Behrmann makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * KAI_UWE BEHRMANN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL KAI_UWE BEHRMANN BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Kai-Uwe Behrmann <ku.b@gmx.de>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

static const char png_filename[] = "romedalen.png";

static comac_surface_t *
create_mask (comac_t *dst, int width, int height)
{
    comac_surface_t *mask;
    comac_t *cr;

    mask = comac_image_surface_create (COMAC_FORMAT_A8, width, height);
    cr = comac_create (mask);
    comac_surface_destroy (mask);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_rectangle (cr, width/4, height/4, width/2, height/2);
    comac_fill (cr);

    mask = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return mask;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *image, *mask;

    image = comac_test_create_surface_from_png (ctx, png_filename);
    mask = create_mask (cr, 40, 40);

    /* opaque background */
    comac_paint (cr);

    /* center */
    comac_translate (cr,
	             (width - comac_image_surface_get_width (image)) / 2.,
		     (height - comac_image_surface_get_height (image)) / 2.);

    /* rotate 30 degree around the center */
    comac_translate (cr, width/2., height/2.);
    comac_rotate (cr, -30 * 2 * M_PI / 360);
    comac_translate (cr, -width/2., -height/2.);

    /* place the image on our surface */
    comac_set_source_surface (cr, image, 0, 0);

    /* reset the drawing matrix */
    comac_identity_matrix (cr);

    /* fill nicely */
    comac_scale (cr, width / 40., height / 40.);

    /* apply the mask */
    comac_mask_surface (cr, mask, 0, 0);

    comac_surface_destroy (mask);
    comac_surface_destroy (image);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (mask_transformed_image,
	    "Test that comac_mask() is affected properly by the CTM and not the image",
	    "mask", /* keywords */
	    NULL, /* requirements */
	    80, 80,
	    NULL, draw)
