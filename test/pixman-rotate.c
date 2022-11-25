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
 * Author: Kristian Høgsberg <krh@redhat.com>
 */

#include "comac-test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <comac.h>

#define WIDTH 32
#define HEIGHT WIDTH

#define IMAGE_WIDTH (3 * WIDTH)
#define IMAGE_HEIGHT IMAGE_WIDTH

/* Draw the word comac at NUM_TEXT different angles */
static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *stamp;
    comac_t *cr2;

    /* Draw a translucent rectangle for reference where the rotated
     * image should be. */
    comac_new_path (cr);
    comac_rectangle (cr, WIDTH, HEIGHT, WIDTH, HEIGHT);
    comac_set_source_rgba (cr, 1, 1, 0, 0.3);
    comac_fill (cr);

#if 1 /* Set to 0 to generate reference image */
    comac_translate (cr, 2 * WIDTH, 2 * HEIGHT);
    comac_rotate (cr, M_PI);
#else
    comac_translate (cr, WIDTH, HEIGHT);
#endif

    stamp = comac_surface_create_similar (comac_get_group_target (cr),
					  COMAC_CONTENT_COLOR_ALPHA,
					  WIDTH,
					  HEIGHT);
    cr2 = comac_create (stamp);
    comac_surface_destroy (stamp);
    {
	comac_new_path (cr2);
	comac_rectangle (cr2, WIDTH / 4, HEIGHT / 4, WIDTH / 2, HEIGHT / 2);
	comac_set_source_rgba (cr2, 1, 0, 0, 0.8);
	comac_fill (cr2);

	comac_rectangle (cr2, 0, 0, WIDTH, HEIGHT);
	comac_set_line_width (cr2, 2);
	comac_set_source_rgb (cr2, 0, 0, 0);
	comac_stroke (cr2);
    }
    comac_set_source_surface (cr, comac_get_target (cr2), 0, 0);
    comac_destroy (cr2);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (pixman_rotate,
	    "Exposes pixman off-by-one error when rotating",
	    "image, transform", /* keywords */
	    NULL,		/* requirements */
	    IMAGE_WIDTH,
	    IMAGE_HEIGHT,
	    NULL,
	    draw)
