/*
 * Copyright 2009 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Intel not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Intel makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * INTEL CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY SPECIAL,
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
    comac_surface_t *region[5];
    const char *text = "Comac";
    int i;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_rectangle (cr, 0, 20, 200, 60);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 0, 0);

    for (i = 0; i < 5; i++) {
	comac_t *cr_region;
	comac_text_extents_t extents;
	char buf[2] = {text[i], '\0'};

	region[i] = comac_surface_create_for_rectangle (comac_get_target (cr),
							20 * i,
							0,
							20,
							20);

	cr_region = comac_create (region[i]);
	comac_surface_destroy (region[i]);

	comac_select_font_face (cr_region,
				"@comac:",
				COMAC_FONT_WEIGHT_NORMAL,
				COMAC_FONT_SLANT_NORMAL);
	comac_set_font_size (cr_region, 20);
	comac_text_extents (cr_region, buf, &extents);
	comac_move_to (cr_region,
		       10 - (extents.width / 2 + extents.x_bearing),
		       10 - (extents.height / 2 + extents.y_bearing));
	comac_show_text (cr_region, buf);

	region[i] = comac_surface_reference (comac_get_target (cr_region));
	comac_destroy (cr_region);
    }

    comac_scale (cr, 2, 2);
    for (i = 0; i < 5; i++) {
	comac_set_source_surface (cr, region[5 - i - 1], 20 * i, 20);
	comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_PAD);
	comac_rectangle (cr, 20 * i, 20, 20, 20);
	comac_fill (cr);
    }

    for (i = 0; i < 5; i++) {
	comac_set_source_surface (cr, region[5 - i - 1], 20 * i, 40);
	comac_paint_with_alpha (cr, .5);
    }

    for (i = 0; i < 5; i++)
	comac_surface_destroy (region[i]);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    subsurface_scale,
    "Tests clipping of both source and destination using subsurfaces",
    "subsurface", /* keywords */
    "target=raster",
    /* FIXME! recursion bug in subsurface/snapshot (with pdf backend) */ /* requirements */
    200,
    120,
    NULL,
    draw)
