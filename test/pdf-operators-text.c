/*
 * Copyright © 2021 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

/* Test pdf-operators text positioning. This test is designed to expose rounding
 * errors in the PDF operations for relative positioning in very long strings.
 *
 * The text width is expected to match the width of the rectangle
 * enclosing the text.
 */

#include "comac-test.h"

#define WIDTH  10000
#define HEIGHT 60

/* Using a non integer size helps expose rounding errors */
#define FONT_SIZE 10.12345678912345

#define WORD "Text"
#define NUM_WORDS 450

#define BORDER 10

static comac_user_data_key_t font_face_key;

static comac_status_t
user_font_init (comac_scaled_font_t  *scaled_font,
                comac_t              *cr,
                comac_font_extents_t *metrics)
{
    comac_font_face_t *font_face = comac_font_face_get_user_data (comac_scaled_font_get_font_face (scaled_font),
                                                                  &font_face_key);
    comac_set_font_face (cr, font_face);
    comac_font_extents (cr, metrics);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
user_font_render_glyph (comac_scaled_font_t  *scaled_font,
                        unsigned long         index,
                        comac_t              *cr,
                        comac_text_extents_t *metrics)
{
    char text[2];
    comac_font_face_t *font_face = comac_font_face_get_user_data (comac_scaled_font_get_font_face (scaled_font),
                                                                  &font_face_key);

    text[0] = index; /* Only using ASCII for this test */
    text[1] = 0;
    comac_set_font_face (cr, font_face);
    comac_text_extents (cr, text, metrics);
    comac_text_path (cr, text);
    comac_fill (cr);
    return COMAC_STATUS_SUCCESS;
}

static comac_font_face_t *
create_user_font_face (comac_font_face_t *orig_font)
{
    comac_font_face_t *user_font_face;

    user_font_face = comac_user_font_face_create ();
    comac_user_font_face_set_init_func (user_font_face, user_font_init);
    comac_user_font_face_set_render_glyph_func (user_font_face, user_font_render_glyph);
    comac_font_face_set_user_data (user_font_face, &font_face_key, (void*) orig_font, NULL);
    return user_font_face;
}

static void
draw_text (comac_t *cr, const char *text)
{
    comac_text_extents_t extents;

    comac_move_to (cr, BORDER, BORDER);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_show_text (cr, text);
    comac_text_extents (cr, text,&extents);

    comac_rectangle (cr,
                     BORDER + extents.x_bearing,
                     BORDER + extents.y_bearing,
                     extents.width,
                     extents.height);
    comac_set_line_width (cr, 1);
    comac_stroke (cr);
}


static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;
    char *text;
    comac_font_face_t *font_face;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_select_font_face (cr, "Dejavu Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    comac_set_font_size (cr, FONT_SIZE);

    text = malloc (strlen(WORD) * NUM_WORDS + 1);
    text[0] = '\0';
    for (i = 0; i < NUM_WORDS; i++)
	strcat (text, WORD);

    comac_save (cr);
    comac_translate (cr, BORDER, BORDER);
    draw_text (cr, text);
    comac_restore (cr);

    font_face = create_user_font_face (comac_get_font_face (cr));
    comac_set_font_face (cr, font_face);
    comac_font_face_destroy (font_face);
    comac_set_font_size (cr, FONT_SIZE);

    comac_save (cr);
    comac_translate (cr, BORDER, BORDER*3);
    draw_text (cr, text);
    comac_restore (cr);

    free (text);
    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (pdf_operators_text,
	    "Test pdf-operators.c glyph positioning",
	    "pdf", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
