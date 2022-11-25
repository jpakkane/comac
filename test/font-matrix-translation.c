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

#define TEXT_SIZE 12
#define PAD 4
#define TEXT "text"

static comac_bool_t
text_extents_equal (const comac_text_extents_t *A,
	            const comac_text_extents_t *B)
{
    return A->x_bearing == B->x_bearing &&
	   A->y_bearing == B->y_bearing &&
	   A->width     == B->width     &&
	   A->height    == B->height    &&
	   A->x_advance == B->x_advance &&
	   A->y_advance == B->y_advance;
}

static comac_test_status_t
box_text (const comac_test_context_t *ctx, comac_t *cr,
	  const char *utf8,
	  double x, double y)
{
    double line_width;
    comac_text_extents_t extents = {0}, scaled_extents = {0};
    comac_scaled_font_t *scaled_font;
    comac_status_t status;

    comac_save (cr);

    comac_text_extents (cr, utf8, &extents);

    scaled_font = comac_get_scaled_font (cr);
    comac_scaled_font_text_extents (scaled_font, TEXT, &scaled_extents);
    status = comac_scaled_font_status (scaled_font);
    if (status)
	return comac_test_status_from_status (ctx, status);

    if (! text_extents_equal (&extents, &scaled_extents)) {
        comac_test_log (ctx,
			"Error: extents differ when they shouldn't:\n"
			"comac_text_extents(); extents (%g, %g, %g, %g, %g, %g)\n"
			"comac_scaled_font_text_extents(); extents (%g, %g, %g, %g, %g, %g)\n",
		        extents.x_bearing, extents.y_bearing,
			extents.width, extents.height,
			extents.x_advance, extents.y_advance,
		        scaled_extents.x_bearing, scaled_extents.y_bearing,
			scaled_extents.width, scaled_extents.height,
			scaled_extents.x_advance, scaled_extents.y_advance);
        return COMAC_TEST_FAILURE;
    }

    line_width = comac_get_line_width (cr);
    comac_rectangle (cr,
		     x + extents.x_bearing - line_width / 2,
		     y + extents.y_bearing - line_width / 2,
		     extents.width  + line_width,
		     extents.height + line_width);
    comac_stroke (cr);

    comac_move_to (cr, x, y);
    comac_show_text (cr, utf8);

    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_test_status_t status;
    comac_text_extents_t extents;
    comac_matrix_t matrix;

    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);

    comac_translate (cr, PAD, PAD);
    comac_set_line_width (cr, 1.0);

    comac_text_extents (cr, TEXT, &extents);

    /* Draw text and bounding box */
    comac_set_source_rgb (cr, 0, 0, 0); /* black */
    status = box_text (ctx, cr, TEXT, 0, - extents.y_bearing);
    if (status)
	return status;

    /* Then draw again with the same coordinates, but with a font
     * matrix to position the text below and shifted a bit to the
     * right. */
    comac_matrix_init_translate (&matrix, TEXT_SIZE / 2, TEXT_SIZE + PAD);
    comac_matrix_scale (&matrix, TEXT_SIZE, TEXT_SIZE);
    comac_set_font_matrix (cr, &matrix);

    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    status = box_text (ctx, cr, TEXT, 0, - extents.y_bearing);
    if (status)
	return status;

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (font_matrix_translation,
	    "Test that translation in a font matrix can be used to offset a string",
	    "font", /* keywords */
	    NULL, /* requirements */
	    38, 34,
	    NULL, draw)
