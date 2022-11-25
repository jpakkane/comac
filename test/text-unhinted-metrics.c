/*
 * Copyright © 2016 Adrian Johnson
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

/* Test COMAC_HINT_METRICS_OFF with a TrueType font.
 *
 * Based on the test case in https://lists.comacgraphics.org/archives/comac/2016-April/027334.html
 */

#include "comac-test.h"

#define WIDTH  100
#define HEIGHT 100

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_font_options_t *font_options;
    double size, y;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY "DejaVu Sans Mono",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    font_options = comac_font_options_create();
    comac_get_font_options (cr, font_options);
    comac_font_options_set_hint_metrics (font_options, COMAC_HINT_METRICS_OFF);
    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_NONE);
    comac_set_font_options (cr, font_options);
    comac_font_options_destroy (font_options);

    y = 0.0;
    comac_set_source_rgb (cr, 0, 0, 0);
    for (size = 8.0; size <= 10.0; size += 0.25) {
	comac_set_font_size (cr, size);
	y += 10.0;
	comac_move_to (cr, 5, y);
	comac_show_text (cr, "abcdefghijklm");
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (text_unhinted_metrics,
	    "Test COMAC_HINT_METRICS_OFF",
	    "text, font", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
