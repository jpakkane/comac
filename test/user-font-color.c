/*
 * Copyright © 2006, 2008 Red Hat, Inc.
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
 * Contributor(s):
 *	Kristian Høgsberg <krh@redhat.com>
 *	Behdad Esfahbod <behdad@behdad.org>
 */

/* Test that user-fonts can handle color and non-color glyphs in the
 * same font.
 */

#include "comac-test.h"

#include <stdlib.h>
#include <stdio.h>

#define BORDER 10
#define TEXT_SIZE 64
#define WIDTH (TEXT_SIZE * 6 + 2 * BORDER)
#define HEIGHT (TEXT_SIZE + 2 * BORDER)

#define TEXT "abcdef"

static comac_status_t
test_scaled_font_init (comac_scaled_font_t *scaled_font,
		       comac_t *cr,
		       comac_font_extents_t *metrics)
{
    metrics->ascent = .75;
    metrics->descent = .25;
    return COMAC_STATUS_SUCCESS;
}

static void
render_glyph_solid (comac_t *cr,
		    double width,
		    double height,
		    comac_bool_t color)
{
    comac_pattern_t *pattern = comac_pattern_reference (comac_get_source (cr));

    if (color)
	comac_set_source_rgba (cr, 0, 1, 1, 0.5);
    comac_rectangle (cr, 0, 0, width / 2, height / 2);
    comac_fill (cr);

    if (color)
	comac_set_source (cr, pattern);
    comac_rectangle (cr, width / 4, height / 4, width / 2, height / 2);
    comac_fill (cr);

    if (color)
	comac_set_source_rgba (cr, 1, 1, 0, 0.5);
    comac_rectangle (cr, width / 2, height / 2, width / 2, height / 2);
    comac_fill (cr);

    comac_pattern_destroy (pattern);
}

static void
render_glyph_linear (comac_t *cr,
		     double width,
		     double height,
		     comac_bool_t color)
{
    comac_pattern_t *pat;

    pat = comac_pattern_create_linear (0.0, 0.0, width, height);
    comac_pattern_add_color_stop_rgba (pat, 0, 1, 0, 0, 1);
    comac_pattern_add_color_stop_rgba (pat, 0.5, 0, 1, 0, color ? 0.5 : 1);
    comac_pattern_add_color_stop_rgba (pat, 1, 0, 0, 1, 1);
    comac_set_source (cr, pat);

    comac_rectangle (cr, 0, 0, width, height);
    comac_fill (cr);
}

static void
render_glyph_text (comac_t *cr, double width, double height, comac_bool_t color)
{
    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, 0.5);

    if (color)
	comac_set_source_rgb (cr, 0.5, 0.5, 0);
    comac_move_to (cr, width * 0.1, height / 2);
    comac_show_text (cr, "a");

    if (color)
	comac_set_source_rgb (cr, 0, 0.5, 0.5);
    comac_move_to (cr, width * 0.4, height * 0.9);
    comac_show_text (cr, "z");
}

static comac_status_t
test_scaled_font_render_color_glyph (comac_scaled_font_t *scaled_font,
				     unsigned long glyph,
				     comac_t *cr,
				     comac_text_extents_t *metrics)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    double width = 0.5;
    double height = 0.8;

    metrics->x_advance = 0.75;
    comac_translate (cr, 0.125, -0.6);
    switch (glyph) {
    case 'a':
	render_glyph_solid (cr, width, height, TRUE);
	break;
    case 'b':
	render_glyph_linear (cr, width, height, TRUE);
	break;
    case 'c':
	render_glyph_text (cr, width, height, TRUE);
	break;
    case 'd':
	render_glyph_solid (cr, width, height, TRUE);
	status = COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED;
	break;
    case 'e':
	render_glyph_linear (cr, width, height, TRUE);
	status = COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED;
	break;
    case 'f':
	render_glyph_solid (cr, width, height, TRUE);
	status = COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED;
	break;
    }

    return status;
}

static comac_status_t
test_scaled_font_render_glyph (comac_scaled_font_t *scaled_font,
			       unsigned long glyph,
			       comac_t *cr,
			       comac_text_extents_t *metrics)
{
    double width = 0.5;
    double height = 0.8;
    metrics->x_advance = 0.75;
    comac_translate (cr, 0.125, -0.6);
    switch (glyph) {
    case 'd':
	render_glyph_solid (cr, width, height, FALSE);
	break;
    case 'e':
	render_glyph_linear (cr, width, height, FALSE);
	break;
    case 'f':
	render_glyph_text (cr, width, height, FALSE);
	break;
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_user_font_face_create (comac_font_face_t **out)
{

    comac_font_face_t *user_font_face;

    user_font_face = comac_user_font_face_create ();
    comac_user_font_face_set_init_func (user_font_face, test_scaled_font_init);
    comac_user_font_face_set_render_color_glyph_func (
	user_font_face,
	test_scaled_font_render_color_glyph);
    comac_user_font_face_set_render_glyph_func (user_font_face,
						test_scaled_font_render_glyph);

    *out = user_font_face;
    return COMAC_STATUS_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_font_face_t *font_face;
    const char text[] = TEXT;
    comac_font_extents_t font_extents;
    comac_text_extents_t extents;
    comac_status_t status;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    status = _user_font_face_create (&font_face);
    if (status) {
	return comac_test_status_from_status (comac_test_get_context (cr),
					      status);
    }

    comac_set_font_face (cr, font_face);
    comac_font_face_destroy (font_face);

    comac_set_font_size (cr, TEXT_SIZE);

    comac_font_extents (cr, &font_extents);
    comac_text_extents (cr, text, &extents);

    /* logical boundaries in red */
    comac_move_to (cr, 0, BORDER);
    comac_rel_line_to (cr, WIDTH, 0);
    comac_move_to (cr, 0, BORDER + font_extents.ascent);
    comac_rel_line_to (cr, WIDTH, 0);
    comac_move_to (cr, 0, BORDER + font_extents.ascent + font_extents.descent);
    comac_rel_line_to (cr, WIDTH, 0);
    comac_move_to (cr, BORDER, 0);
    comac_rel_line_to (cr, 0, 2 * BORDER + TEXT_SIZE);
    comac_move_to (cr, BORDER + extents.x_advance, 0);
    comac_rel_line_to (cr, 0, 2 * BORDER + TEXT_SIZE);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_set_line_width (cr, 2);
    comac_stroke (cr);

    /* ink boundaries in green */
    comac_rectangle (cr,
		     BORDER + extents.x_bearing,
		     BORDER + font_extents.ascent + extents.y_bearing,
		     extents.width,
		     extents.height);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_set_line_width (cr, 2);
    comac_stroke (cr);

    /* text in color */
    comac_set_source_rgb (cr, 0, 0.3, 0);
    comac_move_to (cr, BORDER, BORDER + font_extents.ascent);
    comac_show_text (cr, text);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (user_font_color,
	    "Tests user font color feature",
	    "font, user-font", /* keywords */
	    "comac >= 1.17.4", /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
