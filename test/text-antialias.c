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

#define WIDTH 31
#define HEIGHT 22
#define TEXT_SIZE 12

static comac_test_status_t
draw (comac_t *cr, comac_antialias_t antialias)
{
    comac_text_extents_t extents;
    comac_font_options_t *font_options;
    const char black[] = "black", blue[] = "blue";

    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);

    font_options = comac_font_options_create ();
    comac_get_font_options (cr, font_options);
    comac_font_options_set_antialias (font_options, antialias);
    comac_font_options_set_subpixel_order (font_options,
					   COMAC_SUBPIXEL_ORDER_RGB);
    comac_set_font_options (cr, font_options);

    comac_font_options_destroy (font_options);

    comac_set_source_rgb (cr, 0, 0, 0); /* black */
    comac_text_extents (cr, black, &extents);
    comac_move_to (cr, -extents.x_bearing, -extents.y_bearing);
    comac_show_text (cr, black);
    comac_translate (cr, 0, -extents.y_bearing + 1);

    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_text_extents (cr, blue, &extents);
    comac_move_to (cr, -extents.x_bearing, -extents.y_bearing);
    comac_show_text (cr, blue);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_gray (comac_t *cr, int width, int height)
{
    return draw (cr, COMAC_ANTIALIAS_GRAY);
}

static comac_test_status_t
draw_none (comac_t *cr, int width, int height)
{
    return draw (cr, COMAC_ANTIALIAS_NONE);
}

static comac_test_status_t
draw_subpixel (comac_t *cr, int width, int height)
{
    return draw (cr, COMAC_ANTIALIAS_SUBPIXEL);
}

COMAC_TEST (text_antialias_gray,
	    "Tests text rendering with grayscale antialiasing",
	    "text",	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw_gray)

COMAC_TEST (text_antialias_none,
	    "Tests text rendering with no antialiasing",
	    "text",	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw_none)

COMAC_TEST (text_antialias_subpixel,
	    "Tests text rendering with subpixel antialiasing",
	    "text",	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw_subpixel)
