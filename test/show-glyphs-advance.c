/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2011 Andrea Canciani
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
 * Author: Carl D. Worth <cworth@cworth.org>
 *         Andrea Canciani <ranma42@gmail.com>
 */

#include "comac-test.h"

static comac_test_status_t
get_glyph (const comac_test_context_t *ctx,
	   comac_scaled_font_t *scaled_font,
	   const char *utf8,
	   comac_glyph_t *glyph)
{
    comac_glyph_t *text_to_glyphs;
    comac_status_t status;
    int i;

    text_to_glyphs = glyph;
    i = 1;
    status = comac_scaled_font_text_to_glyphs (scaled_font,
					       0, 0,
					       utf8, -1,
					       &text_to_glyphs, &i,
					       NULL, NULL,
					       0);
    if (status != COMAC_STATUS_SUCCESS)
	return comac_test_status_from_status (ctx, status);

    if (text_to_glyphs != glyph) {
	*glyph = text_to_glyphs[0];
	comac_glyph_free (text_to_glyphs);
    }

    return COMAC_TEST_SUCCESS;
}

static const char *characters[] = { "A", "B", "C", "D" };

#define NUM_CHARS ARRAY_LENGTH (characters)

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_scaled_font_t *scaled_font;
    comac_glyph_t *glyphs = xmalloc (NUM_CHARS  * sizeof (comac_glyph_t));
    int i;
    comac_test_status_t status;

    /* Paint white background. */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    scaled_font = comac_get_scaled_font (cr);

    for (i = 0; i < NUM_CHARS; i++) {
	status = get_glyph (ctx, scaled_font, characters[i], &glyphs[i]);
	if (status)
	    goto BAIL;

	glyphs[i].x = 10.0 + 10.0 * (i % 2);
	glyphs[i].y = 20.0 + 10.0 * (i / 2);
    }

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_show_glyphs (cr, glyphs, 4);

    comac_translate (cr, 40., 20.);
    comac_rotate (cr, M_PI / 4.);

    comac_show_glyphs (cr, glyphs, NUM_CHARS);

  BAIL:
    free(glyphs);

    return status;
}

COMAC_TEST (show_glyphs_advance,
	    "Test that glyph advances work as expected along both axes",
	    "text, matrix", /* keywords */
	    NULL, /* requirements */
	    64, 64,
	    NULL, draw)
