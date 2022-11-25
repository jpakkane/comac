/*
 * Copyright © 2006, 2008 Red Hat, Inc.
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
 * Contributor(s):
 *	Kristian Høgsberg <krh@redhat.com>
 *	Behdad Esfahbod <behdad@behdad.org>
 */

#include "comac-test.h"

#include <stdlib.h>
#include <stdio.h>

/*#define ROTATED 1*/

#define BORDER 10
#define TEXT_SIZE 64
#define WIDTH  (TEXT_SIZE * 15 + 2*BORDER)
#ifndef ROTATED
 #define HEIGHT ((TEXT_SIZE + 2*BORDER)*3)
#else
 #define HEIGHT WIDTH
#endif

#define TEXT1   "comac user-font."
#define TEXT2   " zg."

#define END_GLYPH 0
#define STROKE 126
#define CLOSE 127

/* Simple glyph definition: 1 - 15 means lineto (or moveto for first
 * point) for one of the points on this grid:
 *
 *      1  2  3
 *      4  5  6
 *      7  8  9
 * ----10 11 12----(baseline)
 *     13 14 15
 */
typedef struct {
    unsigned long ucs4;
    int width;
    char data[16];
} test_scaled_font_glyph_t;

static comac_user_data_key_t test_font_face_glyphs_key;

static comac_status_t
test_scaled_font_init (comac_scaled_font_t  *scaled_font,
		       comac_t              *cr,
		       comac_font_extents_t *metrics)
{
  metrics->ascent  = .75;
  metrics->descent = .25;
  return COMAC_STATUS_SUCCESS;
}

static comac_status_t
test_scaled_font_unicode_to_glyph (comac_scaled_font_t *scaled_font,
				   unsigned long        unicode,
				   unsigned long       *glyph)
{
    test_scaled_font_glyph_t *glyphs = comac_font_face_get_user_data (comac_scaled_font_get_font_face (scaled_font),
								      &test_font_face_glyphs_key);
    int i;

    for (i = 0; glyphs[i].ucs4 != (unsigned long) -1; i++)
	if (glyphs[i].ucs4 == unicode) {
	    *glyph = i;
	    return COMAC_STATUS_SUCCESS;
	}

    /* Not found.  Default to glyph 0 */
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
test_scaled_font_render_glyph (comac_scaled_font_t  *scaled_font,
			       unsigned long         glyph,
			       comac_t              *cr,
			       comac_text_extents_t *metrics)
{
    test_scaled_font_glyph_t *glyphs = comac_font_face_get_user_data (comac_scaled_font_get_font_face (scaled_font),
								      &test_font_face_glyphs_key);
    int i;
    const char *data;
    div_t d;
    double x, y;

    /* FIXME: We simply crash on out-of-bound glyph indices */

    metrics->x_advance = glyphs[glyph].width / 4.0;

    comac_set_line_width (cr, 0.1);
    comac_set_line_join (cr, COMAC_LINE_JOIN_ROUND);
    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);

    data = glyphs[glyph].data;
    for (i = 0; data[i] != END_GLYPH; i++) {
	switch (data[i]) {
	case STROKE:
	    comac_new_sub_path (cr);
	    break;

	case CLOSE:
	    comac_close_path (cr);
	    break;

	default:
	    d = div (data[i] - 1, 3);
	    x = d.rem / 4.0 + 0.125;
	    y = d.quot / 5.0 + 0.4 - 1.0;
	    comac_line_to (cr, x, y);
	}
    }
    comac_stroke (cr);

    return COMAC_STATUS_SUCCESS;
}


/* If color_render is TRUE, use the render_color_glyph callback
 * instead of the render_glyph callbac. The output should be identical
 * in this test since the render function does not alter the comac_t
 * source.
 */
static comac_status_t
_user_font_face_create (comac_font_face_t **out, comac_bool_t color_render)
{
    /* Simple glyph definition: 1 - 15 means lineto (or moveto for first
     * point) for one of the points on this grid:
     *
     *      1  2  3
     *      4  5  6
     *      7  8  9
     * ----10 11 12----(baseline)
     *     13 14 15
     */
    static const test_scaled_font_glyph_t glyphs [] = {
	{ 'a',  3, { 4, 6, 12, 10, 7, 9, STROKE, END_GLYPH } },
	{ 'c',  3, { 6, 4, 10, 12, STROKE, END_GLYPH } },
	{ 'e',  3, { 12, 10, 4, 6, 9, 7, STROKE, END_GLYPH } },
	{ 'f',  3, { 3, 2, 11, STROKE, 4, 6, STROKE, END_GLYPH } },
	{ 'g',  3, { 12, 10, 4, 6, 15, 13, STROKE, END_GLYPH } },
	{ 'h',  3, { 1, 10, STROKE, 7, 5, 6, 12, STROKE, END_GLYPH } },
	{ 'i',  1, { 1, 1, STROKE, 4, 10, STROKE, END_GLYPH } },
	{ 'l',  1, { 1, 10, STROKE, END_GLYPH } },
	{ 'n',  3, { 10, 4, STROKE, 7, 5, 6, 12, STROKE, END_GLYPH } },
	{ 'o',  3, { 4, 10, 12, 6, CLOSE, END_GLYPH } },
	{ 'r',  3, { 4, 10, STROKE, 7, 5, 6, STROKE, END_GLYPH } },
	{ 's',  3, { 6, 4, 7, 9, 12, 10, STROKE, END_GLYPH } },
	{ 't',  3, { 2, 11, 12, STROKE, 4, 6, STROKE, END_GLYPH } },
	{ 'u',  3, { 4, 10, 12, 6, STROKE, END_GLYPH } },
	{ 'z',  3, { 4, 6, 10, 12, STROKE, END_GLYPH } },
	{ ' ',  1, { END_GLYPH } },
	{ '-',  2, { 7, 8, STROKE, END_GLYPH } },
	{ '.',  1, { 10, 10, STROKE, END_GLYPH } },
	{  -1,  0, { END_GLYPH } },
    };

    comac_font_face_t *user_font_face;
    comac_status_t status;

    user_font_face = comac_user_font_face_create ();
    comac_user_font_face_set_init_func (user_font_face, test_scaled_font_init);
    if (color_render)
        comac_user_font_face_set_render_color_glyph_func (user_font_face, test_scaled_font_render_glyph);
    else
        comac_user_font_face_set_render_glyph_func (user_font_face, test_scaled_font_render_glyph);

    comac_user_font_face_set_unicode_to_glyph_func (user_font_face, test_scaled_font_unicode_to_glyph);

    status = comac_font_face_set_user_data (user_font_face,
					    &test_font_face_glyphs_key,
					    (void*) glyphs, NULL);
    if (status) {
	comac_font_face_destroy (user_font_face);
	return status;
    }

    *out = user_font_face;
    return COMAC_STATUS_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_font_face_t *font_face;
    char full_text[100];
    comac_font_extents_t font_extents;
    comac_text_extents_t extents;
    comac_status_t status;

    strcpy(full_text, TEXT1);
    strcat(full_text, TEXT2);
    strcat(full_text, TEXT2);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

#ifdef ROTATED
    comac_translate (cr, TEXT_SIZE, 0);
    comac_rotate (cr, .6);
#endif

    status = _user_font_face_create (&font_face, FALSE);
    if (status) {
	return comac_test_status_from_status (comac_test_get_context (cr),
					      status);
    }

    comac_set_font_face (cr, font_face);
    comac_font_face_destroy (font_face);

    comac_set_font_size (cr, TEXT_SIZE);

    comac_font_extents (cr, &font_extents);
    comac_text_extents (cr, full_text, &extents);

    /* logical boundaries in red */
    comac_move_to (cr, 0, BORDER);
    comac_rel_line_to (cr, WIDTH, 0);
    comac_move_to (cr, 0, BORDER + font_extents.ascent);
    comac_rel_line_to (cr, WIDTH, 0);
    comac_move_to (cr, 0, BORDER + font_extents.ascent + font_extents.descent);
    comac_rel_line_to (cr, WIDTH, 0);
    comac_move_to (cr, BORDER, 0);
    comac_rel_line_to (cr, 0, 2*BORDER + TEXT_SIZE);
    comac_move_to (cr, BORDER + extents.x_advance, 0);
    comac_rel_line_to (cr, 0, 2*BORDER + TEXT_SIZE);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_set_line_width (cr, 2);
    comac_stroke (cr);

    /* ink boundaries in green */
    comac_rectangle (cr,
		     BORDER + extents.x_bearing, BORDER + font_extents.ascent + extents.y_bearing,
		     extents.width, extents.height);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_set_line_width (cr, 2);
    comac_stroke (cr);

    /* First line. Text in black, except first "zg." in green */
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_move_to (cr, BORDER, BORDER + font_extents.ascent);
    comac_show_text (cr, TEXT1);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_show_text (cr, TEXT2);
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_show_text (cr, TEXT2);

    /* Now draw the second line using the render_color_glyph callback. The
     * output should be the same because same render function is used
     * and the render function does not set a color. This exercises
     * the paint color glyph with foreground color code path and
     * ensures comac updates the glyph image when the foreground color
     * changes.
     */
    status = _user_font_face_create (&font_face, TRUE);
    if (status) {
	return comac_test_status_from_status (comac_test_get_context (cr),
					      status);
    }

    comac_set_font_face (cr, font_face);
    comac_font_face_destroy (font_face);

    comac_set_font_size (cr, TEXT_SIZE);

    /* text in black, except first "zg." in green */
    comac_move_to (cr, BORDER, BORDER + font_extents.height + 2*BORDER + font_extents.ascent);
    comac_show_text (cr, TEXT1);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_show_text (cr, TEXT2);
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_show_text (cr, TEXT2);

    /* Third line. Filled version of text in blue */
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_move_to (cr, BORDER, BORDER + font_extents.height + 4*BORDER + 2*font_extents.ascent);
    comac_text_path (cr, full_text);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (user_font,
	    "Tests user font feature",
	    "font, user-font", /* keywords */
	    "comac >= 1.7.4", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
