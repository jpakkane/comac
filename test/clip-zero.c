/*
 * Copyright © 2007 Mozilla Corporation
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
 * Author: Vladimir Vukicevic <vladimir@pobox.com>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pat;
    comac_surface_t *surf;

    comac_new_path (cr);
    comac_rectangle (cr, 0, 0, 0, 0);
    comac_clip (cr);

    comac_push_group (cr);

    comac_set_source_rgb (cr, 1, 0, 0);

    comac_new_path (cr);
    comac_rectangle (cr, -10, 10, 20, 20);
    comac_fill_preserve (cr);
    comac_stroke_preserve (cr);
    comac_paint (cr);

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_show_text (cr, "ABC");

    comac_mask (cr, comac_get_source (cr));

    surf = comac_surface_create_similar (comac_get_group_target (cr),
					 COMAC_CONTENT_COLOR_ALPHA,
					 0,
					 0);
    pat = comac_pattern_create_for_surface (surf);
    comac_surface_destroy (surf);

    comac_mask (cr, pat);
    comac_pattern_destroy (pat);

    comac_pop_group_to_source (cr);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_zero,
	    "Verifies that 0x0 surfaces or clips don't cause problems.",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    0,
	    0,
	    NULL,
	    draw)
