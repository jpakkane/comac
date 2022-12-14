/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright 2010 Krzysztof Kosiński
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
 * Author: Krzysztof Kosiński <tweenk.pl@gmail.com>
 */

#include "comac-test.h"

/* originally reported in https://bugs.freedesktop.org/show_bug.cgi?id=29470 */

#define OFFSET 50
#define SIZE 1000

static void
mark_point (comac_t *ct, double x, double y)
{
    comac_rectangle (ct, x - 2, y - 2, 4, 4);
    comac_set_source_rgb (ct, 1, 0, 0);
    comac_fill (ct);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *gr = comac_pattern_create_linear (SIZE - OFFSET,
						       OFFSET,
						       OFFSET,
						       SIZE - OFFSET);

    comac_pattern_add_color_stop_rgb (gr, 0.0, 1, 1, 1);
    comac_pattern_add_color_stop_rgb (gr, 0.0, 0, 0, 0);
    comac_pattern_add_color_stop_rgb (gr, 1.0, 0, 0, 0);
    comac_pattern_add_color_stop_rgb (gr, 1.0, 1, 1, 1);

    comac_set_source (cr, gr);
    comac_pattern_destroy (gr);
    comac_paint (cr);

    mark_point (cr, SIZE - OFFSET, OFFSET);
    mark_point (cr, OFFSET, SIZE - OFFSET);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    linear_gradient_large,
    "Tests that large linear gradients get rendered at the correct place",
    "linear, pattern", /* keywords */
    NULL,	       /* requirements */
    SIZE,
    SIZE,
    NULL,
    draw)
