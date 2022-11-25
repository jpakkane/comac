/*
 * Copyright © 2009 Chris Wilson
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

#define TEXT_SIZE 12
#define HEIGHT (TEXT_SIZE + 4)
#define WIDTH 50

#define MAX_GLYPHS 80

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_glyph_t glyphs_stack[MAX_GLYPHS], *glyphs;
    const char *comac = "Comac";
    const char *giza = "Giza";
    comac_text_extents_t comac_extents;
    comac_text_extents_t giza_extents;
    int count, num_glyphs;
    double x0, y0;

    /* We draw in the default black, so paint white first. */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);

    /* We want to overlap two strings, so compute overlapping glyphs.  */

    comac_text_extents (cr, comac, &comac_extents);
    comac_text_extents (cr, giza, &giza_extents);

    x0 = WIDTH / 2. - (comac_extents.width / 2. + comac_extents.x_bearing);
    y0 = HEIGHT / 2. - (comac_extents.height / 2. + comac_extents.y_bearing);
    glyphs = glyphs_stack;
    count = MAX_GLYPHS;
    comac_scaled_font_text_to_glyphs (comac_get_scaled_font (cr),
				      x0,
				      y0,
				      comac,
				      strlen (comac),
				      &glyphs,
				      &count,
				      NULL,
				      NULL,
				      NULL);
    assert (glyphs == glyphs_stack);
    num_glyphs = count;

    x0 = WIDTH / 2. - (giza_extents.width / 2. + giza_extents.x_bearing);
    y0 = HEIGHT / 2. - (giza_extents.height / 2. + giza_extents.y_bearing);
    glyphs = glyphs_stack + count;
    count = MAX_GLYPHS - count;
    comac_scaled_font_text_to_glyphs (comac_get_scaled_font (cr),
				      x0,
				      y0,
				      giza,
				      strlen (giza),
				      &glyphs,
				      &count,
				      NULL,
				      NULL,
				      NULL);
    assert (glyphs == glyphs_stack + num_glyphs);
    glyphs = glyphs_stack;
    num_glyphs += count;

    comac_set_source_rgba (cr, 0, 0, 0, .5); /* translucent black, gray! */
    comac_show_glyphs (cr, glyphs, num_glyphs);

    /* and compare with filling */
    comac_translate (cr, 0, HEIGHT);
    comac_glyph_path (cr, glyphs, num_glyphs);
    comac_fill (cr);

    /* switch to using an unbounded operator for added complexity */
    comac_set_operator (cr, COMAC_OPERATOR_IN);

    comac_translate (cr, WIDTH, -HEIGHT);
    comac_save (cr);
    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);
    comac_show_glyphs (cr, glyphs, num_glyphs);
    comac_restore (cr);

    comac_translate (cr, 0, HEIGHT);
    comac_save (cr);
    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);
    comac_glyph_path (cr, glyphs, num_glyphs);
    comac_fill (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (overlapping_glyphs,
	    "Test handing of overlapping glyphs",
	    "text, glyphs", /* keywords */
	    NULL,	    /* requirements */
	    2 * WIDTH,
	    2 * HEIGHT,
	    NULL,
	    draw)
