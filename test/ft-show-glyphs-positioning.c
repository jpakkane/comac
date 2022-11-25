/*
 * Copyright © 2008 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"
#include <comac-ft.h>

#define TEXT_SIZE 12

typedef struct {
    comac_glyph_t glyph_list[100];
    int num_glyphs;
    double x;
    double y;
} glyph_array_t;

static void
glyph_array_init (glyph_array_t *glyphs, double x, double y)
{
    glyphs->num_glyphs = 0;
    glyphs->x = x;
    glyphs->y = y;
}

static void
glyph_array_rel_move_to (glyph_array_t *glyphs, double x, double y)
{
    glyphs->x += x;
    glyphs->y += y;
}

static void
glyph_array_show (glyph_array_t *glyphs, comac_t *cr)
{
    comac_show_glyphs (cr, glyphs->glyph_list, glyphs->num_glyphs);
}

#define DOUBLE_FROM_26_6(t) ((double)(t) / 64.0)

static comac_status_t
glyph_array_add_text(glyph_array_t *glyphs, comac_t *cr, const char *s, double spacing)
{
    comac_scaled_font_t *scaled_font;
    comac_status_t status;
    FT_Face face;
    unsigned long charcode;
    unsigned int index;
    comac_text_extents_t extents;
    const char *p;
    FT_Vector kerning;
    double kern_x;
    int first = TRUE;

    scaled_font = comac_get_scaled_font (cr);
    status = comac_scaled_font_status (scaled_font);
    if (status)
	return status;

    face = comac_ft_scaled_font_lock_face (scaled_font);
    if (face == NULL)
	return COMAC_STATUS_FONT_TYPE_MISMATCH;

    p = s;
    while (*p)
    {
        charcode = *p;
        index = FT_Get_Char_Index (face, charcode);
        glyphs->glyph_list[glyphs->num_glyphs].index = index;
        if (first) {
            first = FALSE;
            glyphs->glyph_list[glyphs->num_glyphs].x = glyphs->x;
            glyphs->glyph_list[glyphs->num_glyphs].y = glyphs->y;
        } else {
            comac_glyph_extents (cr, &glyphs->glyph_list[glyphs->num_glyphs - 1], 1, &extents);
            FT_Get_Kerning (face,
                            glyphs->glyph_list[glyphs->num_glyphs - 1].index,
                            glyphs->glyph_list[glyphs->num_glyphs].index,
                            FT_KERNING_UNSCALED,
                            &kerning);
            kern_x = DOUBLE_FROM_26_6(kerning.x);
            glyphs->glyph_list[glyphs->num_glyphs].x =
		glyphs->glyph_list[glyphs->num_glyphs - 1].x + extents.x_advance + kern_x + spacing;
            glyphs->glyph_list[glyphs->num_glyphs].y =
		glyphs->glyph_list[glyphs->num_glyphs - 1].y + extents.y_advance;
	}

	comac_glyph_extents (cr, &glyphs->glyph_list[glyphs->num_glyphs], 1, &extents);
	glyphs->x = glyphs->glyph_list[glyphs->num_glyphs].x + extents.x_advance + spacing;
	glyphs->y = glyphs->glyph_list[glyphs->num_glyphs].y + extents.y_advance;
	p++;
        glyphs->num_glyphs++;
    }

    comac_ft_scaled_font_unlock_face (scaled_font);
    return COMAC_STATUS_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    glyph_array_t glyphs;
    comac_font_options_t *font_options;
    comac_status_t status;

    /* paint white so we don't need separate ref images for
     * RGB24 and ARGB32 */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);
    comac_paint (cr);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);

    font_options = comac_font_options_create ();
    comac_get_font_options (cr, font_options);
    comac_font_options_set_hint_metrics (font_options, COMAC_HINT_METRICS_OFF);
    comac_set_font_options (cr, font_options);
    comac_font_options_destroy (font_options);

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);

    glyph_array_init (&glyphs, 1, TEXT_SIZE);

    status = glyph_array_add_text(&glyphs, cr, "AWAY again", 0.0);
    if (status)
	return comac_test_status_from_status (ctx, status);

    glyph_array_rel_move_to (&glyphs, TEXT_SIZE*1, 0.0);
    status = glyph_array_add_text(&glyphs, cr, "character space", TEXT_SIZE*0.3);
    if (status)
	return comac_test_status_from_status (ctx, status);

    glyph_array_show (&glyphs, cr);


    glyph_array_init (&glyphs, 1, TEXT_SIZE*2 + 4);

    status = glyph_array_add_text(&glyphs, cr, "Increasing", 0.0);
    if (status)
	return comac_test_status_from_status (ctx, status);

    glyph_array_rel_move_to (&glyphs, TEXT_SIZE*0.5, 0.0);
    status = glyph_array_add_text(&glyphs, cr, "space", 0.0);
    if (status)
	return comac_test_status_from_status (ctx, status);

    glyph_array_rel_move_to (&glyphs, TEXT_SIZE*1.0, 0.0);
    status = glyph_array_add_text(&glyphs, cr, "between", 0.0);
    if (status)
	return comac_test_status_from_status (ctx, status);

    glyph_array_rel_move_to (&glyphs, TEXT_SIZE*1.5, 0.0);
    status = glyph_array_add_text(&glyphs, cr, "words", 0.0);
    if (status)
	return comac_test_status_from_status (ctx, status);

    glyph_array_show (&glyphs, cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (ft_show_glyphs_positioning,
	    "Test that the PS/PDF glyph positioning optimizations are correct",
	    "ft, text", /* keywords */
	    NULL, /* requirements */
	    235, (TEXT_SIZE + 4)*2,
	    NULL, draw)
