/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2008 Chris Wilson
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
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define SIZE 10
#define PAD 4
#define COUNT 4

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    comac_t *cr2;
    int i, j;

    /* Fill the background */
    comac_set_source_rgb (cr, 1, 1, 1); /* white */
    comac_paint (cr);

    surface = comac_image_surface_create (COMAC_FORMAT_RGB24, SIZE, SIZE);
    cr2 = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr2, 0, 0, 1); /* blue */
    comac_paint (cr2);

    comac_set_source_surface (cr, comac_get_target (cr2), 0, 0);
    comac_destroy (cr2);

    for (i = 0; i < COUNT; i++) {
	for (j = 0; j < COUNT; j++) {
	    comac_surface_set_device_offset (surface,
					     -i * (SIZE + PAD + .5) - PAD,
					     -j * (SIZE + PAD + .5) - PAD);
	    comac_paint (cr);
	}
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    device_offset_fractional,
    "Test using image surfaces with fractional device-offsets as sources.",
    "device-offset", /* keywords */
    NULL,	     /* requirements */
    COUNT *(SIZE + PAD + .5) + PAD,
    COUNT *(SIZE + PAD + .5) + PAD,
    NULL,
    draw)
