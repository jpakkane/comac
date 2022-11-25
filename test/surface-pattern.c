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

#define SIZE 140
/* Note GhostScript does not support /Interpolate on rotated images, so the PS
 * output looks terrible, but is a known issue. */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    comac_t * cr_surface;
    int surface_size = 6;

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    /* Create an image surface with my favorite four colors in each
     * quadrant. */
    surface = comac_image_surface_create (COMAC_FORMAT_RGB24,
					  surface_size, surface_size);
    cr_surface = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr_surface, 1, 1, 1);
    comac_rectangle (cr_surface,
		     0, 0,
		     surface_size / 2, surface_size / 2);
    comac_fill (cr_surface);
    comac_set_source_rgb (cr_surface, 1, 0, 0);
    comac_rectangle (cr_surface,
		     surface_size / 2, 0,
		     surface_size / 2, surface_size / 2);
    comac_fill (cr_surface);
    comac_set_source_rgb (cr_surface, 0, 1, 0);
    comac_rectangle (cr_surface,
		     0, surface_size / 2,
		     surface_size / 2, surface_size / 2);
    comac_fill (cr_surface);
    comac_set_source_rgb (cr_surface, 0, 0, 1);
    comac_rectangle (cr_surface,
		     surface_size / 2, surface_size / 2,
		     surface_size / 2, surface_size / 2);
    comac_fill (cr_surface);

    comac_scale (cr, 10, 10);
    comac_rotate (cr, 1.);
    comac_set_source_surface (cr, comac_get_target (cr_surface), 1.5, 1.5);
    comac_destroy (cr_surface);

    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_REPEAT);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (surface_pattern,
	    "Test transformed repeated surface patterns"
	    "\nExhibiting a strange (very minor) failure in ps backend with device-offset",
	    "transform", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
