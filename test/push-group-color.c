/*
 * Copyright © 2005 Mozilla Corporation
 * Copyright © 2009 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Mozilla Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Mozilla Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * MOZILLA CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MOZILLA CORPORATION BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Vladimir Vukicevic <vladimir@pobox.com>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define UNIT_SIZE 100
#define PAD 5
#define INNER_PAD 10

#define WIDTH (UNIT_SIZE + PAD) + PAD
#define HEIGHT (UNIT_SIZE + PAD) + PAD

static comac_pattern_t *
argb32_source (void)
{
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    comac_t *cr;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 16, 16);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_rectangle (cr, 8, 0, 8, 8);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_rectangle (cr, 0, 8, 8, 8);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_rectangle (cr, 8, 8, 8, 8);
    comac_fill (cr);

    pattern = comac_pattern_create_for_surface (comac_get_target (cr));
    comac_destroy (cr);

    return pattern;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *gradient, *image;

    comac_set_source_rgb (cr, 1,1,1);
    comac_paint (cr);

    comac_translate (cr, PAD, PAD);

    /* clip to the unit size */
    comac_rectangle (cr, 0, 0,
		     UNIT_SIZE, UNIT_SIZE);
    comac_clip (cr);

    comac_rectangle (cr, 0, 0,
		     UNIT_SIZE, UNIT_SIZE);
    comac_set_source_rgba (cr, 0, 0, 0, 1);
    comac_set_line_width (cr, 2);
    comac_stroke (cr);

    /* start a group */
    comac_push_group_with_content (cr, COMAC_CONTENT_COLOR);

    /* draw a gradient background */
    comac_save (cr);
    comac_translate (cr, INNER_PAD, INNER_PAD);
    comac_new_path (cr);
    comac_rectangle (cr, 0, 0,
		     UNIT_SIZE - (INNER_PAD*2), UNIT_SIZE - (INNER_PAD*2));
    gradient = comac_pattern_create_linear (UNIT_SIZE - (INNER_PAD*2), 0,
                                            UNIT_SIZE - (INNER_PAD*2), UNIT_SIZE - (INNER_PAD*2));
    comac_pattern_add_color_stop_rgba (gradient, 0.0, 0.3, 0.3, 0.3, 1.0);
    comac_pattern_add_color_stop_rgba (gradient, 1.0, 1.0, 1.0, 1.0, 1.0);
    comac_set_source (cr, gradient);
    comac_pattern_destroy (gradient);
    comac_fill (cr);
    comac_restore (cr);

    /* draw diamond */
    comac_move_to (cr, UNIT_SIZE / 2, 0);
    comac_line_to (cr, UNIT_SIZE    , UNIT_SIZE / 2);
    comac_line_to (cr, UNIT_SIZE / 2, UNIT_SIZE);
    comac_line_to (cr, 0            , UNIT_SIZE / 2);
    comac_close_path (cr);
    comac_set_source_rgba (cr, 0, 0, 1, 1);
    comac_fill (cr);

    /* draw circle */
    comac_arc (cr,
	       UNIT_SIZE / 2, UNIT_SIZE / 2,
	       UNIT_SIZE / 3.5,
	       0, M_PI * 2);
    comac_set_source_rgba (cr, 1, 0, 0, 1);
    comac_fill (cr);

    /* and put the image on top */
    comac_translate (cr, UNIT_SIZE/2 - 8, UNIT_SIZE/2 - 8);
    image = argb32_source ();
    comac_set_source (cr, image);
    comac_pattern_destroy (image);
    comac_paint (cr);

    comac_pop_group_to_source (cr);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (push_group_color,
	    "Verify that comac_push_group_with_content works.",
	    "group", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
