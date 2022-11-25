/*
 * Copyright © 2005,2007 Red Hat, Inc.
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

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    uint32_t data[16] = {
	0xffffffff, 0xffffffff,		0xffff0000, 0xffff0000,
	0xffffffff, 0xffffffff,		0xffff0000, 0xffff0000,

	0xff00ff00, 0xff00ff00,		0xff0000ff, 0xff0000ff,
	0xff00ff00, 0xff00ff00,		0xff0000ff, 0xff0000ff
    };

    /* First paint opaque background (black) so we don't need separate
     * ARGB32 and RGB24 reference images. */
    comac_set_source_rgb (cr, 0, 0, 0); /* black */
    comac_paint (cr);

    surface = comac_image_surface_create_for_data ((unsigned char *) data,
						   COMAC_FORMAT_RGB24, 4, 4, 16);

    comac_scale (cr, 0.5, 0.5);

    comac_set_source_surface (cr, surface, 4, 4);
    comac_pattern_set_filter (comac_get_source (cr), COMAC_FILTER_NEAREST);
    comac_paint (cr);

    comac_surface_finish (surface); /* data will go out of scope */
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (scale_down_source_surface_paint,
	    "Test call sequence: comac_scale; comac_set_source_surface; comac_paint, with a scale < 1.0",
	    "paint, transform", /* keywords */
	    NULL, /* requirements */
	    6, 6,
	    NULL, draw)
