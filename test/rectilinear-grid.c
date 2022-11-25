/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright 2010 Andrea Canciani
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
 * Author: Andrea Canciani <ranma42@gmail.com>
 */

#include "comac-test.h"

#define SIZE 52
#define OFFSET 5
#define DISTANCE 10.25

/*
  This test checks that boxes not aligned to pixels are drawn
  correctly.

  In particular the corners of the boxes are drawn incorrectly by
  comac-image in comac 1.10.0, because overlapping boxes are passed to
  a span converter which assumes disjoint boxes as input.

  This results in corners to be drawn with the wrong shade.
*/

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_set_line_width (cr, 4);
    comac_translate (cr, 2 * OFFSET, 2 * OFFSET);

    for (i = 0; i < 4; i++) {
	double x = i * DISTANCE;

	comac_move_to (cr, x, -OFFSET - 0.75);
	comac_line_to (cr, x, SIZE - 3 * OFFSET - 0.25);

	comac_move_to (cr, -OFFSET - 0.75, x);
	comac_line_to (cr, SIZE - 3 * OFFSET - 0.25, x);
    }

    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
aligned (comac_t *cr, int width, int height)
{
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    return draw (cr, width, height);
}

COMAC_TEST (rectilinear_grid,
	    "Test rectilinear rasterizer (covering partial pixels)",
	    "rectilinear", /* keywords */
	    NULL,	   /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)

COMAC_TEST (a1_rectilinear_grid,
	    "Test rectilinear rasterizer (covering whole pixels)",
	    "rectilinear",   /* keywords */
	    "target=raster", /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    aligned)
