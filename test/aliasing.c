/*
 * Copyright 2010 Intel Corporation
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

/* A fun little test to explore color fringing in various experimental
 * subpixel rasterisation techniques.
 */

#define WIDTH 60
#define HEIGHT 40

static const struct color {
    double red, green, blue;
} color[] = {
    {1, 1, 1},
    {0, 0, 0},
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {1, 1, 0},
    {0, 1, 1},
    {1, 0, 1},
    {.5, .5, .5},
};

#define NUM_COLORS ARRAY_LENGTH (color)

static void
object (comac_t *cr, const struct color *fg, const struct color *bg)
{
    comac_set_source_rgb (cr, bg->red, bg->green, bg->blue);
    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_fill (cr);

    comac_set_source_rgb (cr, fg->red, fg->green, fg->blue);
    comac_save (cr);
    comac_scale (cr, WIDTH, HEIGHT);
    comac_arc (cr, .5, .5, .5 - 4. / MAX (WIDTH, HEIGHT), 0, 2 * M_PI);
    comac_fill (cr);
    comac_arc (cr, .5, .5, .5 - 2. / MAX (WIDTH, HEIGHT), 0, 2 * M_PI);
    comac_restore (cr);
    comac_set_line_width (cr, 1.);
    comac_stroke (cr);

    comac_set_source_rgb (cr, bg->red, bg->green, bg->blue);
    comac_set_line_width (cr, 4.);
    comac_move_to (cr, 4, HEIGHT - 4);
    comac_line_to (cr, WIDTH - 12, 4);
    comac_move_to (cr, 12, HEIGHT - 4);
    comac_line_to (cr, WIDTH - 4, 4);
    comac_stroke (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    unsigned int i, j;

    for (i = 0; i < NUM_COLORS; i++) {
	for (j = 0; j < NUM_COLORS; j++) {
	    comac_save (cr);
	    comac_translate (cr, i * WIDTH, j * HEIGHT);
	    object (cr, &color[i], &color[j]);
	    comac_restore (cr);
	}
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (aliasing,
	    "Check for subpixel aliasing and color fringing",
	    "rasterisation", /* keywords */
	    "target=raster", /* requirements */
	    NUM_COLORS *WIDTH,
	    NUM_COLORS *HEIGHT,
	    NULL,
	    draw)
