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

#include "comac-test.h"

/* An assertion failure found by Rico Tzschichholz */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_set_source_rgb (cr, 0, 1, 0);

    comac_arc (cr, 50, 50, 40, 0, 2 * M_PI);
    comac_clip_preserve (cr);

    comac_paint (cr);

    comac_rectangle (cr, 0, 0, 100, 100);
    comac_reset_clip (cr);
    comac_clip (cr);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (inverted_clip,
	    "Clip + InvertedClip should be opaque",
	    "clip, paint",   /* keywords */
	    "target=raster", /* requirements */
	    100,
	    100,
	    NULL,
	    draw)
