/*
 * Copyright © 2005 Red Hat, Inc.
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
 */

#include "comac-test.h"

#define STAMP_WIDTH 4
#define STAMP_HEIGHT 4
#define PAD 1

#define STEPS 10

#define IMAGE_WIDTH (PAD + STEPS * (STAMP_WIDTH + PAD) + PAD)
#define IMAGE_HEIGHT (PAD + STEPS * (STAMP_HEIGHT + PAD) + PAD)

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    uint32_t data[STAMP_WIDTH * STAMP_HEIGHT] = {0xffffffff,
						 0xffffffff,
						 0xffff0000,
						 0xffff0000,
						 0xffffffff,
						 0xffffffff,
						 0xffff0000,
						 0xffff0000,

						 0xff00ff00,
						 0xff00ff00,
						 0xff0000ff,
						 0xff0000ff,
						 0xff00ff00,
						 0xff00ff00,
						 0xff0000ff,
						 0xff0000ff};
    int i, j;

    /* fill with off-white to avoid a separate rgb24 ref image */
    comac_save (cr);
    comac_set_source_rgb (cr, .7, .7, .7);
    comac_paint (cr);
    comac_restore (cr);

    /* Draw reference lines where the jump should be. */
    comac_move_to (cr, PAD + STEPS / 2 * (STAMP_WIDTH + PAD), 0);
    comac_rel_line_to (cr, 0, IMAGE_HEIGHT);
    comac_move_to (cr, 0, PAD + STEPS / 2 * (STAMP_HEIGHT + PAD));
    comac_rel_line_to (cr, IMAGE_WIDTH, 0);
    comac_set_line_width (cr, 2.0);
    comac_stroke (cr);

    surface = comac_image_surface_create_for_data ((unsigned char *) data,
						   COMAC_FORMAT_RGB24,
						   STAMP_WIDTH,
						   STAMP_HEIGHT,
						   STAMP_WIDTH * 4);

    for (j = 0; j < STEPS; j++) {
	double j_step;

	for (i = 0; i < STEPS; i++) {
	    double i_step;

#define GENERATE_REFERENCE_IMAGE 0
#if GENERATE_REFERENCE_IMAGE
	    i_step = i >= STEPS / 2 ? 1 : 0;
	    j_step = j >= STEPS / 2 ? 1 : 0;
#else
	    i_step = i * 1.0 / STEPS;
	    j_step = j * 1.0 / STEPS;
#endif

	    comac_save (cr);

	    comac_set_source_surface (cr,
				      surface,
				      PAD + i * (STAMP_WIDTH + PAD) + i_step,
				      PAD + j * (STAMP_HEIGHT + PAD) + j_step);
	    comac_pattern_set_filter (comac_get_source (cr),
				      COMAC_FILTER_NEAREST);
	    comac_paint (cr);

	    comac_restore (cr);
	}
    }

    comac_surface_finish (surface); /* data goes out of scope */
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (filter_nearest_offset,
	    "Test sampling offset of COMAC_FILTER_NEAREST"
	    "\nwrong sampling location for nearest-neighbor filter in "
	    "libpixman and Render",
	    "filter", /* keywords */
	    NULL,     /* requirements */
	    IMAGE_WIDTH,
	    IMAGE_HEIGHT,
	    NULL,
	    draw)
