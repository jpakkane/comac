/*
 * Copyright (c) 2010 Intel Corporation
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

/* Test the handling of comac_push_group() with everything clipped. */
static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_paint (cr); /* opaque background */

    comac_rectangle (cr,  20, 20, 0, 0);
    comac_clip (cr);

    comac_push_group (cr); /* => 0x0 group */
    comac_reset_clip (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    comac_rectangle (cr, 0, 0, width, height);
    comac_set_source_rgba (cr, 0, 1, 0, .5);
    comac_fill (cr);

    comac_move_to (cr, 0, 20);
    comac_line_to (cr, width, 20);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_stroke (cr);

    comac_pop_group_to_source (cr);
    comac_reset_clip (cr);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_empty_group,
	    "Test handling of groups with everything clipped",
	    "clip, group", /* keywords */
	    NULL, /* requirements */
	    40, 40,
	    NULL, draw)
