/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2010 Intel Corporation
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
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define TEXT_SIZE 12

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* This is just the inverse of select-font-face.c */
    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1, 1, 1);

    comac_set_font_size (cr, TEXT_SIZE);
    comac_move_to (cr, 0, TEXT_SIZE);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Serif",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_show_text (cr, "i-am-serif");

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_show_text (cr, " i-am-sans");

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans Mono",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_show_text (cr, " i-am-mono");

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (inverse_text,
	    "Tests rendering of inverse text (white-on-black)",
	    "font, text", /* keywords */
	    NULL, /* requirements */
	    192, TEXT_SIZE + 4,
	    NULL, draw)
