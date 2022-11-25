/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright 2011 Red Hat Inc.
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
 * Author: Benjamin Otte <otte@redhat.com>
 */

/*
 * Test case taken from the WebKit test suite, failure originally reported
 * by Zan Dobersek <zandobersek@gmail.com>. WebKit test is
 * LayoutTests/canvas/philip/tests/2d.path.rect.selfintersect.html
 */

#include "comac-test.h"

#include <math.h>

#define LINE_WIDTH 60
#define SIZE 100
#define RECT_SIZE 10

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* fill with green so RGB and RGBA tests can share the ref image */
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_paint (cr);

    /* red to see eventual bugs immediately */
    comac_set_source_rgb (cr, 1, 0, 0);

    /* big line width */
    comac_set_line_width (cr, LINE_WIDTH);

    /* rectangle that is smaller than the line width in center of image */
    comac_rectangle (cr,
		     (SIZE - RECT_SIZE) / 2,
		     (SIZE - RECT_SIZE) / 2,
		     RECT_SIZE,
		     RECT_SIZE);

    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

/* and again slightly offset to trigger another path */
static comac_test_status_t
draw_offset (comac_t *cr, int width, int height)
{
    comac_translate (cr, .5, .5);
    return draw (cr, width, height);
}

static comac_test_status_t
draw_rotated (comac_t *cr, int width, int height)
{
    comac_translate (cr, SIZE / 2, SIZE / 2);
    comac_rotate (cr, M_PI / 4);
    comac_translate (cr, -SIZE / 2, -SIZE / 2);

    return draw (cr, width, height);
}

static comac_test_status_t
draw_flipped (comac_t *cr, int width, int height)
{
    comac_translate (cr, SIZE / 2, SIZE / 2);
    comac_scale (cr, -1, 1);
    comac_translate (cr, -SIZE / 2, -SIZE / 2);

    return draw (cr, width, height);
}

static comac_test_status_t
draw_flopped (comac_t *cr, int width, int height)
{
    comac_translate (cr, SIZE / 2, SIZE / 2);
    comac_scale (cr, 1, -1);
    comac_translate (cr, -SIZE / 2, -SIZE / 2);

    return draw (cr, width, height);
}

static comac_test_status_t
draw_dashed (comac_t *cr, int width, int height)
{
    const double dashes[] = {4};
    comac_set_dash (cr, dashes, 1, 0);
    comac_set_line_cap (cr, COMAC_LINE_CAP_BUTT);
    return draw (cr, width, height);
}

COMAC_TEST (line_width_overlap,
	    "Test overlapping lines due to large line width",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)
COMAC_TEST (line_width_overlap_offset,
	    "Test overlapping lines due to large line width",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw_offset)
COMAC_TEST (line_width_overlap_rotated,
	    "Test overlapping lines due to large line width",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw_rotated)
COMAC_TEST (line_width_overlap_flipped,
	    "Test overlapping lines due to large line width",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw_flipped)
COMAC_TEST (line_width_overlap_flopped,
	    "Test overlapping lines due to large line width",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw_flopped)
COMAC_TEST (line_width_overlap_dashed,
	    "Test overlapping lines due to large line width",
	    "stroke", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw_dashed)
