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

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_test_status_t status = COMAC_TEST_SUCCESS;
    comac_surface_t *surface;
    comac_t *cr;
    comac_font_face_t *font_face;
    comac_scaled_font_t *scaled_font;

    comac_test_log (ctx, "Creating comac context and obtaining a font face\n");

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1);
    cr = comac_create (surface);

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    comac_test_log (ctx, "Testing return value of comac_font_face_get_type\n");

    font_face = comac_get_font_face (cr);

    if (comac_font_face_get_type (font_face) != COMAC_FONT_TYPE_TOY) {
	comac_test_log (
	    ctx,
	    "Unexpected value %d from comac_font_face_get_type (expected %d)\n",
	    comac_font_face_get_type (font_face),
	    COMAC_FONT_TYPE_TOY);
	status = COMAC_TEST_FAILURE;
	goto done;
    }

    comac_test_log (ctx, "Testing return value of comac_get_scaled_font\n");

    scaled_font = comac_get_scaled_font (cr);

    if (comac_scaled_font_get_font_face (scaled_font) != font_face) {
	comac_test_log (ctx,
			"Font face returned from the scaled font is different "
			"from that returned by the context\n");
	status = COMAC_TEST_FAILURE;
	goto done;
    }

done:
    comac_destroy (cr);
    comac_surface_destroy (surface);

    return status;
}

COMAC_TEST (font_face_get_type,
	    "Check the returned type from comac_select_font_face.",
	    "font", /* keywords */
	    NULL,   /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
