/*
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *mask;
    comac_pattern_t *pattern;
    comac_t *cr2;
    comac_text_extents_t extents;

    comac_set_source_rgb (cr, 0, 0, 1.0);
    comac_paint (cr);

    mask = comac_surface_create_similar (comac_get_group_target (cr),
					 COMAC_CONTENT_ALPHA,
					 width,
					 height);
    cr2 = comac_create (mask);
    comac_surface_destroy (mask);

    comac_save (cr2);
    {
	comac_set_operator (cr2, COMAC_OPERATOR_CLEAR);
	comac_paint (cr2);
    }
    comac_restore (cr2);

    pattern = comac_pattern_create_linear (0, 0, width, height);
    comac_pattern_add_color_stop_rgba (pattern, 0.00, 0., 0., 0., 0.);
    comac_pattern_add_color_stop_rgba (pattern, 0.25, 1., 1., 1., 1.);
    comac_pattern_add_color_stop_rgba (pattern, 0.50, 1., 1., 1., .5);
    comac_pattern_add_color_stop_rgba (pattern, 0.75, 1., 1., 1., 1.);
    comac_pattern_add_color_stop_rgba (pattern, 1.00, 0., 0., 0., 0.);
    comac_set_source (cr2, pattern);
    comac_pattern_destroy (pattern);

    comac_select_font_face (cr2,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr2, 0.5 * height);

    comac_text_extents (cr2, "comac", &extents);
    comac_move_to (
	cr2,
	floor ((width - extents.width) / 2 + 0.5) - extents.x_bearing,
	floor ((height - extents.height) / 2 - 0.5) - extents.y_bearing);
    comac_show_text (cr2, "comac");

    comac_set_source_rgb (cr, 1.0, 0, 0);
    comac_mask_surface (cr, comac_get_target (cr2), 0, 0);
    comac_destroy (cr2);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (smask_text,
	    "Test the support of \"soft\" masks with text",
	    "smask, text", /* keywords */
	    NULL,	   /* keywords */
	    120,
	    60,
	    NULL,
	    draw)
