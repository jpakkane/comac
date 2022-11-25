/*
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
 *
 * Based on a bug snippet by Jeremy Moles <jeremy@emperorlinux.com>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *mask;

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    comac_push_group_with_content (cr, COMAC_CONTENT_ALPHA);
    {
	comac_set_source_rgb (cr, 1, 1, 1);
	comac_paint (cr);

	comac_move_to (cr, 0, 0);
	comac_line_to (cr, width, height);
	comac_set_operator (cr, COMAC_OPERATOR_CLEAR);
	comac_set_line_width (cr, 10);
	comac_stroke (cr);
    }
    mask = comac_pop_group (cr);
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_mask (cr, mask);
    comac_pattern_destroy (mask);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (a8_clear,
	    "Test clear on an a8 surface",
	    "a8, clear", /* keywords */
	    NULL, /* requirements */
	    40, 40,
	    NULL, draw)

