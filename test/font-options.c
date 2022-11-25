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

#include <assert.h>

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_font_options_t *default_options;
    comac_font_options_t *nil_options;
    comac_surface_t *surface;
    comac_matrix_t identity;
    comac_t *cr;
    comac_scaled_font_t *scaled_font;

    /* first check NULL handling of comac_font_options_t */
    default_options = comac_font_options_create ();
    assert (comac_font_options_status (default_options) == COMAC_STATUS_SUCCESS);
    nil_options = comac_font_options_copy (NULL);
    assert (comac_font_options_status (nil_options) == COMAC_STATUS_NO_MEMORY);

    assert (comac_font_options_equal (default_options, default_options));
    assert (! comac_font_options_equal (default_options, nil_options));
    assert (! comac_font_options_equal (NULL, nil_options));
    assert (! comac_font_options_equal (nil_options, nil_options));
    assert (! comac_font_options_equal (default_options, NULL));
    assert (! comac_font_options_equal (NULL, default_options));

    assert (comac_font_options_hash (default_options) == comac_font_options_hash (nil_options));
    assert (comac_font_options_hash (NULL) == comac_font_options_hash (nil_options));
    assert (comac_font_options_hash (default_options) == comac_font_options_hash (NULL));

    comac_font_options_merge (NULL, NULL);
    comac_font_options_merge (default_options, NULL);
    comac_font_options_merge (default_options, nil_options);

    comac_font_options_set_antialias (NULL, COMAC_ANTIALIAS_DEFAULT);
    comac_font_options_get_antialias (NULL);
    assert (comac_font_options_get_antialias (default_options) == COMAC_ANTIALIAS_DEFAULT);

    comac_font_options_set_subpixel_order (NULL, COMAC_SUBPIXEL_ORDER_DEFAULT);
    comac_font_options_get_subpixel_order (NULL);
    assert (comac_font_options_get_subpixel_order (default_options) == COMAC_SUBPIXEL_ORDER_DEFAULT);

    comac_font_options_set_hint_style (NULL, COMAC_HINT_STYLE_DEFAULT);
    comac_font_options_get_hint_style (NULL);
    assert (comac_font_options_get_hint_style (default_options) == COMAC_HINT_STYLE_DEFAULT);

    comac_font_options_set_hint_metrics (NULL, COMAC_HINT_METRICS_DEFAULT);
    comac_font_options_get_hint_metrics (NULL);
    assert (comac_font_options_get_hint_metrics (default_options) == COMAC_HINT_METRICS_DEFAULT);

    comac_font_options_set_color_palette (NULL, COMAC_COLOR_PALETTE_DEFAULT);
    comac_font_options_get_color_palette (NULL);
    assert (comac_font_options_get_color_palette (default_options) == COMAC_COLOR_PALETTE_DEFAULT);

    comac_font_options_destroy (NULL);
    comac_font_options_destroy (default_options);
    comac_font_options_destroy (nil_options);


    /* Now try creating fonts with NULLs */
    surface = comac_image_surface_create (COMAC_FORMAT_RGB24, 0, 0);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_matrix_init_identity (&identity);
    scaled_font = comac_scaled_font_create (comac_get_font_face (cr),
	                                    &identity, &identity,
					    NULL);
    assert (comac_scaled_font_status (scaled_font) == COMAC_STATUS_NULL_POINTER);
    comac_scaled_font_get_font_options (scaled_font, NULL);
    comac_scaled_font_destroy (scaled_font);

    assert (comac_status (cr) == COMAC_STATUS_SUCCESS);
    comac_get_font_options (cr, NULL);
    comac_set_font_options (cr, NULL);
    assert (comac_status (cr) == COMAC_STATUS_NULL_POINTER);

    comac_destroy (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (font_options,
	    "Check setters and getters on comac_font_options_t.",
	    "font, api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
