/*
 * Copyright © Chris Wilson, 2008
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
 * Authors: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

/* This is a test case for the following bug:
 *
 *	crafted gif file will crash firefox
 *	[XError: 'BadAlloc (insufficient resources for operation)']
 *	https://bugzilla.mozilla.org/show_bug.cgi?id=424333
 */

#ifdef WORDS_BIGENDIAN
#define RED_MASK 0xA0
#define GREEN_MASK 0xA
#else
#define RED_MASK 0x5
#define GREEN_MASK 0x50
#endif

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    unsigned char *data;

    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    surface = comac_image_surface_create (COMAC_FORMAT_A1, 32000, 20);
    data = comac_image_surface_get_data (surface);
    if (data != NULL) {
	int stride = comac_image_surface_get_stride (surface);
	int width  = comac_image_surface_get_width  (surface);
	int height = comac_image_surface_get_height (surface);
	int x, y;

	for (y = 0; y < height; y++) {
	    for (x = 0; x < (width + 7) / 8; x++)
		data[x] = RED_MASK;
	    data += stride;
	}
        comac_surface_mark_dirty (surface);
    }

    comac_set_source_rgb (cr, 1, 0, 0); /* red */
    comac_mask_surface (cr, surface, 0, 0);
    comac_surface_destroy (surface);

    surface = comac_image_surface_create (COMAC_FORMAT_A1, 20, 32000);
    data = comac_image_surface_get_data (surface);
    if (data != NULL) {
	int stride = comac_image_surface_get_stride (surface);
	int width  = comac_image_surface_get_width  (surface);
	int height = comac_image_surface_get_height (surface);
	int x, y;

	for (y = 0; y < height; y++) {
	    for (x = 0; x < (width + 7) / 8; x++)
		data[x] = GREEN_MASK;
	    data += stride;
	}
        comac_surface_mark_dirty (surface);
    }

    comac_set_source_rgb (cr, 0, 1, 0); /* green */
    comac_mask_surface (cr, surface, 0, 0);
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (large_source,
	    "Exercises mozilla bug 424333 - handling of massive images",
	    "stress, source", /* keywords */
	    NULL, /* requirements */
	    20, 20,
	    NULL, draw)
