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
 * Author: Behdad Esfahbod <behdad@behdad.org>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define SIZE 90

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    comac_t *cr_surface;

    /* Create a 4-pixel similar surface with my favorite four colors in each
     * quadrant. */
    surface = comac_surface_create_similar (comac_get_group_target (cr),
					    COMAC_CONTENT_COLOR,
					    2,
					    2);
    cr_surface = comac_create (surface);
    comac_surface_destroy (surface);

    /* upper-left = white */
    comac_set_source_rgb (cr_surface, 1, 1, 1);
    comac_rectangle (cr_surface, 0, 0, 1, 1);
    comac_fill (cr_surface);

    /* upper-right = red */
    comac_set_source_rgb (cr_surface, 1, 0, 0);
    comac_rectangle (cr_surface, 1, 0, 1, 1);
    comac_fill (cr_surface);

    /* lower-left = green */
    comac_set_source_rgb (cr_surface, 0, 1, 0);
    comac_rectangle (cr_surface, 0, 1, 1, 1);
    comac_fill (cr_surface);

    /* lower-right = blue */
    comac_set_source_rgb (cr_surface, 0, 0, 1);
    comac_rectangle (cr_surface, 1, 1, 1, 1);
    comac_fill (cr_surface);

    /* Now use extend pad to cover the entire surface with those 4 colors */
    comac_set_source_surface (cr,
			      comac_get_target (cr_surface),
			      width / 2 - 1,
			      height / 2 - 1);
    comac_destroy (cr_surface);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_PAD);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (extend_pad_similar,
	    "Test COMAC_EXTEND_PAD for similar surface patterns",
	    "extend", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)
