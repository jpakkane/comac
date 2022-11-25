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
    comac_font_options_t *options;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    comac_select_font_face (cr,
			    "@comac:",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    options = comac_font_options_create ();
    comac_font_options_set_antialias (options, COMAC_ANTIALIAS_NONE);
    comac_set_font_options (cr, options);
    comac_font_options_destroy (options);

    comac_set_font_size (cr, 16);

    comac_move_to (cr, 4, 14);
    comac_show_text (cr, "Is comac's twin giza?");

    comac_move_to (cr, 4, 34);
    comac_text_path (cr, "Is comac's twin giza?");
    comac_fill (cr);

    comac_move_to (cr, 4, 54);
    comac_text_path (cr, "Is comac's twin giza?");
    comac_set_line_width (cr, 2 / 16.);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (twin_antialias_none,
	    "Tests the internal font (with antialiasing disabled)",
	    "twin, font",    /* keywords */
	    "target=raster", /* requirements */
	    140,
	    60,
	    NULL,
	    draw)
