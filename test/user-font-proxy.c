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
#define WIDTH  (TEXT_SIZE * 12 + 2*BORDER)
#ifndef ROTATED
 #define HEIGHT ((TEXT_SIZE + 2*BORDER)*2)
#else
 #define HEIGHT WIDTH
#endif
#define TEXT   "geez... comac user-font"

static comac_user_data_key_t fallback_font_key;

static comac_status_t
test_scaled_font_init (comac_scaled_font_t  *scaled_font,
		       comac_t              *cr,
		       comac_font_extents_t *extents)
{
    comac_status_t status;

    comac_set_font_face (cr,
			 comac_font_face_get_user_data (comac_scaled_font_get_font_face (scaled_font),
							&fallback_font_key));

    status = comac_scaled_font_set_user_data (scaled_font,
					      &fallback_font_key,
					      comac_scaled_font_reference (comac_get_scaled_font (cr)),
					      (comac_destroy_func_t) comac_scaled_font_destroy);
    if (unlikely (status)) {
	comac_scaled_font_destroy (comac_get_scaled_font (cr));
	return status;
    }

    comac_font_extents (cr, extents);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
test_scaled_font_render_glyph (comac_scaled_font_t  *scaled_font,
			       unsigned long         glyph,
			       comac_t              *cr,
			       comac_text_extents_t *extents)
{
    comac_glyph_t comac_glyph;

    comac_glyph.index = glyph;
    comac_glyph.x = 0;
    comac_glyph.y = 0;

    comac_set_font_face (cr,
			 comac_font_face_get_user_data (comac_scaled_font_get_font_face (scaled_font),
							&fallback_font_key));

    comac_show_glyphs (cr, &comac_glyph, 1);
    comac_glyph_extents (cr, &comac_glyph, 1, extents);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
test_scaled_font_text_to_glyphs (comac_scaled_font_t        *scaled_font,
				 const char	            *utf8,
				 int		             utf8_len,
				 comac_glyph_t	           **glyphs,
				 int		            *num_glyphs,
				 comac_text_cluster_t      **clusters,
				 int		            *num_clusters,
				 comac_text_cluster_flags_t *cluster_flags)
{
  comac_scaled_font_t *fallback_scaled_font;

  fallback_scaled_font = comac_scaled_font_get_user_data (scaled_font,
							  &fallback_font_key);

  return comac_scaled_font_text_to_glyphs (fallback_scaled_font, 0, 0,
					   utf8, utf8_len,
					   glyphs, num_glyphs,
					   clusters, num_clusters, cluster_flags);
}

static comac_status_t
_user_font_face_create (comac_font_face_t **out)
{
    comac_font_face_t *user_font_face;
    comac_font_face_t *fallback_font_face;
    comac_status_t status;

    user_font_face = comac_user_font_face_create ();
    comac_user_font_face_set_init_func             (user_font_face, test_scaled_font_init);
    comac_user_font_face_set_render_glyph_func     (user_font_face, test_scaled_font_render_glyph);
    comac_user_font_face_set_text_to_glyphs_func   (user_font_face, test_scaled_font_text_to_glyphs);

    /* This also happens to be default font face on comac_t, so does
     * not make much sense here.  For demonstration only.
     */
    fallback_font_face = comac_toy_font_face_create (COMAC_TEST_FONT_FAMILY " Sans",
						     COMAC_FONT_SLANT_NORMAL,
						     COMAC_FONT_WEIGHT_NORMAL);

    status = comac_font_face_set_user_data (user_font_face,
					    &fallback_font_key,
					    fallback_font_face,
					    (comac_destroy_func_t) comac_font_face_destroy);
    if (status) {
	comac_font_face_destroy (fallback_font_face);
	comac_font_face_destroy (user_font_face);
	return status;
    }

    *out = user_font_face;
    return COMAC_STATUS_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const char text[] = TEXT;
    comac_font_extents_t font_extents;
    comac_text_extents_t extents;
    comac_font_face_t *font_face;
    comac_status_t status;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

#ifdef ROTATED
    comac_translate (cr, TEXT_SIZE, 0);
    comac_rotate (cr, .6);
#endif

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

    /* text in gray */
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_move_to (cr, BORDER, BORDER + font_extents.ascent);
    comac_show_text (cr, text);


    /* filled version of text in light blue */
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_move_to (cr, BORDER, BORDER + font_extents.height + BORDER + font_extents.ascent);
    comac_text_path (cr, text);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (user_font_proxy,
	    "Tests a user-font using a native font in its render_glyph",
	    "font, user-font", /* keywords */
	    "comac >= 1.7.4", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
