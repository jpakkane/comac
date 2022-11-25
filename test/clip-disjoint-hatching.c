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

#define STEP 5
#define WIDTH 100
#define HEIGHT 100

static void
hatching (comac_t *cr)
{
    int i;

    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);

    comac_translate (cr, WIDTH / 2, HEIGHT / 2);
    comac_rotate (cr, M_PI / 4);
    comac_translate (cr, -WIDTH / 2, -HEIGHT / 2);

    for (i = 0; i < WIDTH; i += STEP) {
	comac_rectangle (cr, i, -2, 1, HEIGHT + 4);
	comac_rectangle (cr, -2, i, WIDTH + 4, 1);
    }
}

static void
background (comac_t *cr)
{
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
}

static void
clip_to_grid (comac_t *cr)
{
    int i, j;

    for (j = 0; j < HEIGHT; j += 2 * STEP) {
	for (i = 0; i < WIDTH; i += 2 * STEP)
	    comac_rectangle (cr, i, j, STEP, STEP);

	j += 2 * STEP;
	for (i = 0; i < WIDTH; i += 2 * STEP)
	    comac_rectangle (cr, i + STEP / 2, j, STEP, STEP);
    }

    comac_clip (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    background (cr);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);

    comac_save (cr);
    {
	clip_to_grid (cr);
	hatching (cr);
	comac_set_source_rgb (cr, 1, 0, 0);
	comac_fill (cr);
    }
    comac_restore (cr);

    comac_translate (cr, 0.25, HEIGHT + .25);

    comac_save (cr);
    {
	clip_to_grid (cr);
	hatching (cr);
	comac_set_source_rgb (cr, 0, 0, 1);
	comac_fill (cr);
    }
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_disjoint_hatching,
	    "Test drawing through through an array of clips",
	    "clip",	     /* keywords */
	    "target=raster", /* requirements */
	    WIDTH,
	    2 * HEIGHT,
	    NULL,
	    draw)
