/*
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define WIDTH  2
#define HEIGHT 2

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    /* Four green pixels with different "alpha" values, (but which
     * should be entirely ignored). */
    uint32_t colors[4] = {
	0xff00ff00, 0x8800ff00,
	0x4400ff00, 0x0000ff00
    };

    surface = comac_image_surface_create_for_data ((unsigned char *) colors,
						   COMAC_FORMAT_RGB24, 2, 2, 8);

    comac_set_source_surface (cr, surface, 0, 0);
    comac_paint (cr);

    comac_surface_finish (surface); /* colors will go out of scope */
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (rgb24_ignore_alpha,
	    "Test that when using an RGB24 image as a source, there is no alpha channel",
	    "image, alpha", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
