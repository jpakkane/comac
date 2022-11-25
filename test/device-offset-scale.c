/*
 * Copyright © 2008 Mozilla Corporation
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
 * Authors: Michael Ventnor, Jeff Muizelaar
 */

#include "comac-test.h"

#define WIDTH 20
#define HEIGHT 20

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *second;
    comac_t *second_cr;

    /* fill with black so we don't need an rgb test case */
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    comac_scale (cr, 0.5, 0.5);

    /* draw the first rectangle */
    comac_set_source_rgb (cr, 0, 0, 0.4);
    comac_rectangle (cr, 6, 6, 10, 10);
    comac_fill (cr);

    /* adjust the offset so that the second rectangle will fit on the surface */
    second = comac_image_surface_create (COMAC_FORMAT_A8, 10, 10);
    comac_surface_set_device_offset (second, -6, -6);

    /* draw the second rectangle:
     * this rectangle should end up in the same place as the rectangle above
     * independent of the device offset of the surface it is painted on*/
    second_cr = comac_create (second);
    comac_surface_destroy (second);
    comac_rectangle (second_cr, 6, 6, 10, 10);
    comac_fill (second_cr);

    /* paint the second rectangle on top of the first rectangle */
    comac_set_source_rgb (cr, 0.5, 0.5, 0);
    comac_mask_surface (cr, comac_get_target (second_cr), 0, 0);
    comac_destroy (second_cr);

    return COMAC_TEST_SUCCESS;
}

/*
 * XFAIL: complication of pre-multiplying device_offset into the pattern_matrix
 * and then requiring further manipulation for SVG
 */
COMAC_TEST (device_offset_scale,
	    "Test that the device-offset transform is transformed by the ctm.",
	    "device-offset", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
