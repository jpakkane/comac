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
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"

#include <stdlib.h>
#include <stdio.h>

/*#define ROTATED 1*/

#define BORDER 10
#define TEXT_SIZE 64
#define WIDTH  (TEXT_SIZE * 15 + 2*BORDER)
#ifndef ROTATED
 #define HEIGHT ((TEXT_SIZE + 2*BORDER)*2)
#else
 #define HEIGHT WIDTH
#endif
#define END_GLYPH 0
#define TEXT   "comac"

/* Reverse the bits in a byte with 7 operations (no 64-bit):
 * Devised by Sean Anderson, July 13, 2001.
 * Source: http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
 */
#define COMAC_BITSWAP8(c) ((((c) * 0x0802LU & 0x22110LU) | ((c) * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16)

#ifdef WORDS_BIGENDIAN
#define COMAC_BITSWAP8_IF_LITTLE_ENDIAN(c) (c)
#else
#define COMAC_BITSWAP8_IF_LITTLE_ENDIAN(c) COMAC_BITSWAP8(c)
#endif



/* Simple glyph definition. data is an 8x8 bitmap.
 */
typedef struct {
    unsigned long ucs4;
    int width;
    char data[8];
} test_scaled_font_glyph_t;

static comac_user_data_key_t test_font_face_glyphs_key;

static comac_status_t
test_scaled_font_init (comac_scaled_font_t  *scaled_font,
		       comac_t              *cr,
		       comac_font_extents_t *metrics)
{
  metrics->ascent  = 1;
  metrics->descent = 0;
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
    unsigned char *data;
    comac_surface_t *image;
    comac_pattern_t *pattern;
    comac_matrix_t matrix;
    uint8_t byte;

    /* FIXME: We simply crash on out-of-bound glyph indices */

    metrics->x_advance = (glyphs[glyph].width + 1) / 8.0;

    image = comac_image_surface_create (COMAC_FORMAT_A1, glyphs[glyph].width, 8);
    if (comac_surface_status (image))
	return comac_surface_status (image);

    data = comac_image_surface_get_data (image);
    for (i = 0; i < 8; i++) {
	byte = glyphs[glyph].data[i];
	*data = COMAC_BITSWAP8_IF_LITTLE_ENDIAN (byte);
	data += comac_image_surface_get_stride (image);
    }
    comac_surface_mark_dirty (image);

    pattern = comac_pattern_create_for_surface (image);
    comac_surface_destroy (image);

    comac_matrix_init_identity (&matrix);
    comac_matrix_scale (&matrix, 1.0/8.0, 1.0/8.0);
    comac_matrix_translate (&matrix, 0, -8);
    comac_matrix_invert (&matrix);
    comac_pattern_set_matrix (pattern, &matrix);

    comac_set_source (cr, pattern);
    comac_mask (cr, pattern);
    comac_pattern_destroy (pattern);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_user_font_face_create (comac_font_face_t **out)
{
    static const test_scaled_font_glyph_t glyphs [] = {
	{ 'c',  6, { 0x00, 0x38, 0x44, 0x80, 0x80, 0x80, 0x44, 0x38 } },
	{ 'a',  6, { 0x00, 0x70, 0x88, 0x3c, 0x44, 0x84, 0x8c, 0x74 } },
	{ 'i',  1, { 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 } },
	{ 'r',  6, { 0x00, 0xb8, 0xc4, 0x80, 0x80, 0x80, 0x80, 0x80 } },
	{ 'o',  7, { 0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x44, 0x38 } },
	{  -1,  8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    };

    comac_font_face_t *user_font_face;
    comac_status_t status;

    user_font_face = comac_user_font_face_create ();
    comac_user_font_face_set_init_func             (user_font_face, test_scaled_font_init);
    comac_user_font_face_set_render_glyph_func     (user_font_face, test_scaled_font_render_glyph);
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
    const char text[] = TEXT;
    comac_font_extents_t font_extents;
    comac_text_extents_t extents;
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

    /* text in black */
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_move_to (cr, BORDER, BORDER + font_extents.ascent);
    comac_show_text (cr, text);


    /* filled version of text in blue */
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_move_to (cr, BORDER, BORDER + font_extents.height + 2*BORDER + font_extents.ascent);
    comac_text_path (cr, text);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (user_font_mask,
	    "Tests a user-font using comac_mask with bitmap images",
	    "user-font, mask", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
