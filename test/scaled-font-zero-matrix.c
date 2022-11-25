/*
 * Copyright © 2008 Mozilla Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Mozilla Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Mozilla Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * MOZILLA CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MOZILLA CORPORATION BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Jeff Muizelaar
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_font_face_t *font_face;
    comac_font_options_t *font_options;
    comac_scaled_font_t *scaled_font;
    comac_matrix_t identity;
    comac_matrix_t zero;

    comac_matrix_init_identity(&identity);

    zero = identity;
    comac_matrix_scale(&zero, 0, 0);

    font_face = comac_get_font_face (cr);
    font_options = comac_font_options_create ();
    comac_get_font_options (cr, font_options);
    scaled_font = comac_scaled_font_create (font_face,
	    &identity,
	    &zero,
	    font_options);
    comac_set_scaled_font (cr, scaled_font);
    comac_show_text (cr, "Hello");
    comac_scaled_font_destroy (scaled_font);
    comac_font_options_destroy (font_options);

    return comac_test_status_from_status (comac_test_get_context (cr),
                                          comac_status(cr));
}

COMAC_TEST (scaled_font_zero_matrix,
	    "Test that scaled fonts with a degenerate matrix work",
	    "zero, matrix, degenerate, scaled-font", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    NULL, draw)
