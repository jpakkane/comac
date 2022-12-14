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
    uint32_t data[] = {
	0x80000000,
	0x80000000,
	0x80000000,
	0x80000000,
    };

    comac_surface_t *mask, *mask2;
    comac_pattern_t *pattern;
    comac_t *cr2;
    comac_text_extents_t extents;

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

    pattern = comac_pattern_create_linear (0, 0, 0, height);
    comac_pattern_add_color_stop_rgba (pattern, 0.00, 0., 0., 0., 0.0);
    comac_pattern_add_color_stop_rgba (pattern, 0.80, 0., 0., 0., 0.0);
    comac_pattern_add_color_stop_rgba (pattern, 0.90, 1., 1., 1., 0.25);
    comac_pattern_add_color_stop_rgba (pattern, 1.00, 1., 1., 1., 1.0);
    comac_set_source (cr2, pattern);
    comac_pattern_destroy (pattern);

    comac_paint (cr2);

    pattern = comac_pattern_create_linear (0, 0, width, height);
    comac_pattern_add_color_stop_rgba (pattern, 0.00, 0., 0., 0., 0.);
    comac_pattern_add_color_stop_rgba (pattern, 0.25, 1., 1., 1., 1.);
    comac_pattern_add_color_stop_rgba (pattern, 0.50, 1., 1., 1., .5);
    comac_pattern_add_color_stop_rgba (pattern, 0.75, 1., 1., 1., 1.);
    comac_pattern_add_color_stop_rgba (pattern, 1.00, 0., 0., 0., 0.);
    comac_set_source (cr2, pattern);
    comac_pattern_destroy (pattern);

    mask2 = comac_image_surface_create_for_data ((unsigned char *) data,
						 COMAC_FORMAT_ARGB32,
						 2,
						 2,
						 8);
    pattern = comac_pattern_create_for_surface (mask2);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_REPEAT);
    comac_mask (cr2, pattern);
    comac_pattern_destroy (pattern);

    comac_arc (cr2, 0.5 * width, 0.5 * height - 10, 0.2 * height, 0, 2 * M_PI);
    comac_fill (cr2);

    comac_arc (cr2, 0.5 * width, 0.5 * height - 10, 0.25 * height, 0, 2 * M_PI);
    comac_stroke (cr2);

    comac_select_font_face (cr2,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr2, 0.3 * height);

    comac_text_extents (cr2, "FG", &extents);
    comac_move_to (
	cr2,
	floor ((width - extents.width) / 2 + 0.5) - extents.x_bearing,
	floor (height - extents.height - 0.5) - extents.y_bearing - 5);
    comac_show_text (cr2, "FG");

    comac_set_source_rgb (cr, 0, 0, 1.0);
    comac_paint (cr);

    pattern = comac_pattern_create_radial (0.5 * width,
					   0.5 * height,
					   0,
					   0.5 * width,
					   0.5 * height,
					   0.5 * height);
    comac_pattern_add_color_stop_rgba (pattern, 0.00, 0., 0., 0., 0.);
    comac_pattern_add_color_stop_rgba (pattern, 0.25, 1., 0., 0., 1.);
    comac_pattern_add_color_stop_rgba (pattern, 0.50, 1., 0., 0., .5);
    comac_pattern_add_color_stop_rgba (pattern, 1.00, 1., 0., 0., 1.);
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);

    comac_mask_surface (cr, comac_get_target (cr2), 0, 0);
    comac_destroy (cr2);

    comac_surface_finish (mask2); /* data will go out of scope */
    comac_surface_destroy (mask2);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (smask,
	    "Test the support of \"soft\" masks",
	    "smask", /* keywords */
	    NULL,    /* requirements */
	    60,
	    60,
	    NULL,
	    draw)
