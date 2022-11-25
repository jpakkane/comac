/*
 * Copyright © 2011 Intel Corporation
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
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

/* Exercise https://bugs.freedesktop.org/show_bug.cgi?id=31604 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *a1;
    comac_t *cr2;

    a1 = comac_image_surface_create (COMAC_FORMAT_A1, 100, 100);
    cr2 = comac_create (a1);
    comac_surface_destroy (a1);

    comac_set_operator (cr2, COMAC_OPERATOR_SOURCE);
    comac_rectangle (cr2, 10, 10, 80, 80);
    comac_set_source_rgb (cr2, 1, 1, 1);
    comac_fill (cr2);
    comac_rectangle (cr2, 20, 20, 60, 60);
    comac_set_source_rgb (cr2, 0, 0, 0);
    comac_fill (cr2);

    a1 = comac_surface_reference (comac_get_target (cr2));
    comac_destroy (cr2);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_mask_surface (cr, a1, 0, 0);
    comac_surface_destroy (a1);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (a1_fill,
	    "Test filling of an a1-surface and use as mask",
	    "a1, alpha, fill, mask", /* keywords */
	    "target=raster",	     /* requirements */
	    100,
	    100,
	    NULL,
	    draw)
