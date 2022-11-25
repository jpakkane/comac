/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2006 Brian Ewins.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Brian Ewins not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Brian Ewins makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * BRIAN EWINS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL BRIAN EWINS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Brian Ewins <Brian.Ewins@gmail.com>
 */

/* Related to bug 9530
 *
 * comac_glyph_t can contain any unsigned long in its 'index', the intention
 * being that it is large enough to hold a pointer. However, this means that
 * it can specify many glyph indexes which don't exist in the font, and may
 * exceed the range of legal glyph indexes for the font backend. It may
 * also contain special values that are not usable as indexes - e.g. 0xffff is
 * kATSDeletedGlyphcode in ATSUI, a glyph that should not be drawn.
 * The font backends should handle all legal and out-of-range values
 * consistently.
 *
 * This test expects that operations on out-of-range and missing glyphs should
 * act as if they were zero-width.
 */

#include "comac-test.h"

#define WIDTH  100
#define HEIGHT 75
#define NUM_TEXT 20
#define TEXT_SIZE 12

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_text_extents_t extents;
    int i;
    /* Glyphs with no paths followed by 'comac', the additional
     * text is to make the space obvious.
     */
    long int index[] = {
	0, /* 'no matching glyph' */
	0xffff, /* kATSDeletedGlyphCode */
	0x1ffff, /* out of range */
	-1L, /* out of range */
	70, 68, 76, 85, 82 /* 'comac' */
    };

    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, 16);

    for (i = 0; i < 9; i++) {
	/* since we're just drawing glyphs directly we need to position them. */
	comac_glyph_t glyph = {
	    index[i], 10 * i, 25
	};

	/* test comac_glyph_extents. Every glyph index should
	 * have extents, invalid glyphs should be zero-width.
	 */
	comac_move_to (cr, glyph.x, glyph.y);
	comac_set_line_width (cr, 1.0);
	comac_glyph_extents (cr, &glyph, 1, &extents);
	comac_rectangle (cr,
			 glyph.x + extents.x_bearing - 0.5,
			 glyph.y + extents.y_bearing - 0.5,
			 extents.width + 1,
			 extents.height + 1);
	comac_set_source_rgb (cr, 1, 0, 0); /* red */
	comac_stroke (cr);

	/* test comac_show_glyphs. Every glyph index should be
	 * drawable, invalid glyph indexes should draw nothing.
	 */
	comac_set_source_rgb (cr, 0, 0, 0); /* black */
	comac_show_glyphs (cr, &glyph, 1);
	comac_move_to (cr, glyph.x, glyph.y);

	/* test comac_glyph_path. Every glyph index should produce
	 * a path, invalid glyph indexes should have empty paths.
	 */
	/* Change the glyph position
	 * so that the paths are visible.
	 */
	glyph.y = 55;
	comac_move_to (cr, glyph.x, glyph.y);
	comac_glyph_path (cr, &glyph, 1);
	comac_fill (cr);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (text_glyph_range,
	    "Tests show_glyphs, glyph_path, glyph_extents with out of range glyph ids."
	    "\nft and atsui font backends fail, misreporting errors from FT_Load_Glyph and ATSUGlyphGetCubicPaths",
	    "text, stress", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
