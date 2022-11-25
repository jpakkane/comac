/*
 * Copyright © 2008 Red Hat, Inc.
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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

/* Bug history:
 *
 * 2008-05-23: Caolan McNamara noticed a bug in OpenOffice.org where,
 *             when using a very large font, space would be left for a
 *	       glyph but it would actually be rendered in the wrong
 *	       place. He wrote a minimal test case and posted the bug
 *	       here:
 *
 *			corrupt glyph positions with large font
 *			https://bugzilla.redhat.com/show_bug.cgi?id=448104
 *
 * 2008-05-23: Carl Worth wrote this test for the comac test suite to
 *             exercise the bug.
 */

#include "comac-test.h"

#define WIDTH  800
#define HEIGHT 800
#define TEXT_SIZE 10000

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* paint white so we don't need separate ref images for
     * RGB24 and ARGB32 */
    comac_set_source_rgb (cr, 1., 1., 1.);
    comac_paint (cr);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_move_to (cr, -TEXT_SIZE / 2, TEXT_SIZE / 2);
    comac_show_text (cr, "xW");

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (large_font,
	    "Draws a very large font to exercise a glyph-positioning bug",
	    "stress, font", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
