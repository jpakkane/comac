/*
 * Copyright © 2010 Mozilla Corporation
 * Copyright © 2010 Intel Corporation
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

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *mask;
    comac_t *cr2;

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_paint (cr);

    /* clip twice, note that the intersection is smaller then the extents */
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_rectangle (cr, 10, 10, 80, 80);
    comac_rectangle (cr, 20, 20, 60, 60);
    comac_clip (cr);

    comac_rectangle (cr, 0, 40, 40, 30);
    comac_clip (cr);

    /* and exercise the bug found by Jeff Muizelaar */
    mask = comac_surface_create_similar (comac_get_target (cr),
					 COMAC_CONTENT_ALPHA,
					 width-20, height-20);
    cr2 = comac_create (mask);
    comac_surface_destroy (mask);

    comac_set_source_rgba (cr2, 1, 1, 1, 1);
    comac_paint (cr2);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_mask_surface (cr, comac_get_target (cr2), 0, 0);
    comac_destroy (cr2);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_twice_rectangle,
	    "Tests clipping twice using rectangles",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    100, 100,
	    NULL, draw)
