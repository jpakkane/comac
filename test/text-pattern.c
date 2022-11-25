/*
 * Copyright © 2005 Tim Rowley
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
 * Author: Tim Rowley
 */

#include "comac-test.h"

#define IMAGE_WIDTH 128
#define IMAGE_HEIGHT 64

static void
draw_text_pattern (comac_t *cr, double alpha)
{
    comac_pattern_t *pat;

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    pat = comac_pattern_create_linear (0.0, 0.0, 1, 1);
    comac_pattern_add_color_stop_rgba (pat, 1, 1, 0, 0, alpha);
    comac_pattern_add_color_stop_rgba (pat, 0, 0, 0, 1, alpha);
    comac_set_source (cr, pat);

    /* test rectangle - make sure the gradient is set correctly */
    comac_rectangle (cr, 0, 0, 0.1, 1);
    comac_fill (cr);

    comac_set_font_size (cr, 0.4);
    comac_move_to (cr, 0.1, 0.6);
    comac_show_text (cr, "comac");

    comac_pattern_destroy (pat);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_scale (cr, width / 2, height);
    draw_text_pattern (cr, 1.0);
    comac_translate (cr, 1, 0);
    draw_text_pattern (cr, 0.5);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (text_pattern,
	    "Patterned Text",
	    "text, pattern", /* keywords */
	    NULL,	     /* requirements */
	    IMAGE_WIDTH,
	    IMAGE_HEIGHT,
	    NULL,
	    draw)
