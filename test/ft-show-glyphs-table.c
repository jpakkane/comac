/*
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Eugeniy Meshcheryakov <eugen@debian.org>
 *	    Adrian Johnson <ajohnson@redneon.com>
 *	    Carl Worth <cworth@cworth.org>
 */

#include "comac-test.h"
#include <comac-ft.h>

#define TEXT_SIZE 20
#define PAD 10
#define GRID_SIZE 30
#define GRID_ROWS 10
#define GRID_COLS 4
#define NUM_GLYPHS (GRID_ROWS * GRID_COLS)
#define WIDTH (PAD + GRID_COLS * GRID_SIZE + PAD)
#define HEIGHT (PAD + GRID_ROWS * GRID_SIZE + PAD)

/* This test was originally inspired by this bug report:
 *
 * Error when creating pdf charts for new FreeSerifItalic.ttf
 * http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=%23474136
 *
 * The original assertion failure was fairly boring, but the later
 * glyph mispositiing was quite interesting. And it turns out that the
 * _comac_pdf_operators_show_glyphs code is fairly convoluted with a
 * code path that wasn't being exercised at all by the test suite.
 *
 * So this is an attempt to exercise that code path. Apparently laying
 * glyphs out vertically in a table like this, (so that there's a
 * large change in Y position from one glyph to the next), exercises
 * the code well.
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_font_options_t *font_options;
    comac_scaled_font_t *scaled_font;
    FT_Face face;
    FT_ULong charcode;
    FT_UInt idx;
    int i = 0;
    comac_glyph_t glyphs[NUM_GLYPHS];

    /* paint white so we don't need separate ref images for
     * RGB24 and ARGB32 */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);
    comac_paint (cr);

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);

    font_options = comac_font_options_create ();
    comac_get_font_options (cr, font_options);
    comac_font_options_set_hint_metrics (font_options, COMAC_HINT_METRICS_OFF);
    comac_set_font_options (cr, font_options);
    comac_font_options_destroy (font_options);

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);

    scaled_font = comac_get_scaled_font (cr);
    face = comac_ft_scaled_font_lock_face (scaled_font);
    {
	charcode = FT_Get_First_Char (face, &idx);
	while (idx && (i < NUM_GLYPHS)) {
	    glyphs[i] =
		(comac_glyph_t){idx,
				PAD + GRID_SIZE * (i / GRID_ROWS),
				PAD + TEXT_SIZE + GRID_SIZE * (i % GRID_ROWS)};
	    i++;
	    charcode = FT_Get_Next_Char (face, charcode, &idx);
	}
    }
    comac_ft_scaled_font_unlock_face (scaled_font);

    comac_show_glyphs (cr, glyphs, i);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (ft_show_glyphs_table,
	    "Test comac_show_glyphs with comac-ft backend and glyphs laid out "
	    "in a table",
	    "ft, text", /* keywords */
	    NULL,	/* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
