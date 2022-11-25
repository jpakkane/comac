/*
 * Copyright 2009 Chris Wilson
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

static comac_scaled_font_t *
create_twin (comac_t *cr, comac_antialias_t antialias)
{
    comac_font_options_t *options;

    comac_select_font_face (cr,
			    "@comac:",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    options = comac_font_options_create ();
    comac_font_options_set_antialias (options, antialias);
    comac_set_font_options (cr, options);
    comac_font_options_destroy (options);

    return comac_scaled_font_reference (comac_get_scaled_font (cr));
}


static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_scaled_font_t *subpixel, *gray, *none;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_set_font_size (cr, 16);
    subpixel = create_twin (cr, COMAC_ANTIALIAS_SUBPIXEL);
    gray = create_twin (cr, COMAC_ANTIALIAS_GRAY);
    none = create_twin (cr, COMAC_ANTIALIAS_NONE);

    comac_move_to (cr, 4, 14);
    comac_set_scaled_font (cr, subpixel);
    comac_show_text (cr, "Is comac's");
    comac_set_scaled_font (cr, gray);
    comac_show_text (cr, " twin");
    comac_set_scaled_font (cr, none);
    comac_show_text (cr, " giza?");

    comac_move_to (cr, 4, 34);
    comac_set_scaled_font (cr, gray);
    comac_show_text (cr, "Is comac's");
    comac_set_scaled_font (cr, none);
    comac_show_text (cr, " twin");
    comac_set_scaled_font (cr, subpixel);
    comac_show_text (cr, " giza?");

    comac_move_to (cr, 4, 54);
    comac_set_scaled_font (cr, none);
    comac_show_text (cr, "Is comac's");
    comac_set_scaled_font (cr, gray);
    comac_show_text (cr, " twin");
    comac_set_scaled_font (cr, subpixel);
    comac_show_text (cr, " giza?");

    comac_scaled_font_destroy (none);
    comac_scaled_font_destroy (gray);
    comac_scaled_font_destroy (subpixel);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (twin_antialias_mixed,
	    "Tests the internal font (with intermixed antialiasing)",
	    "twin, font", /* keywords */
	    "target=raster", /* requirements */
	    140, 60,
	    NULL, draw)
