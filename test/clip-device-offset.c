/*
 * Copyright © 2009 Benjamin Otte
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "comac-test.h"

#define WIDTH 50
#define HEIGHT 50

static comac_pattern_t *
create_green_source (void)
{
    comac_surface_t *image;
    comac_pattern_t *pattern;
    comac_t *cr;

    image = comac_image_surface_create (COMAC_FORMAT_ARGB32, WIDTH, HEIGHT);
    cr = comac_create (image);
    comac_surface_destroy (image);

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_paint (cr);

    pattern = comac_pattern_create_for_surface (comac_get_target (cr));
    comac_destroy (cr);

    return pattern;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *source;
    double old_x, old_y;

    comac_surface_get_device_offset (comac_get_group_target (cr),
				     &old_x,
				     &old_y);
    comac_surface_set_device_offset (comac_get_group_target (cr),
				     old_x + 5,
				     old_y + 5);

    source = create_green_source ();
    comac_set_source (cr, source);
    comac_pattern_destroy (source);

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);
    comac_paint (cr);

    comac_surface_set_device_offset (comac_get_group_target (cr), old_x, old_y);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_device_offset,
	    "Test clipping on surfaces with device offsets",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    WIDTH + 10,
	    HEIGHT + 10,
	    NULL,
	    draw)
