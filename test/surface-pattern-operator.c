/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright 2009 Andrea Canciani
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

#define N_OPERATORS (COMAC_OPERATOR_SATURATE + 1)
#define HEIGHT 16
#define WIDTH 16
#define PAD 3

static comac_pattern_t*
_create_pattern (comac_surface_t *target, comac_content_t content, int width, int height)
{
    comac_pattern_t *pattern;
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_surface_create_similar (target, content, width, height);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_arc (cr, 0.5 * width, 0.5 * height, 0.45 * height, -M_PI / 4, 3 * M_PI / 4);
    comac_fill (cr);

    pattern = comac_pattern_create_for_surface (comac_get_target (cr));
    comac_destroy (cr);

    return pattern;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *alpha_pattern, *color_alpha_pattern, *pattern;
    unsigned int n, i;

    alpha_pattern = _create_pattern (comac_get_target (cr),
				     COMAC_CONTENT_ALPHA,
				     0.9 * WIDTH, 0.9 * HEIGHT);
    color_alpha_pattern = _create_pattern (comac_get_target (cr),
					   COMAC_CONTENT_COLOR_ALPHA,
					   0.9 * WIDTH, 0.9 * HEIGHT);

    pattern = comac_pattern_create_linear (WIDTH, 0, 0, HEIGHT);
    comac_pattern_add_color_stop_rgba (pattern, 0.2, 0, 0, 1, 1);
    comac_pattern_add_color_stop_rgba (pattern, 0.8, 0, 0, 1, 0);

    comac_translate (cr, PAD, PAD);

    for (n = 0; n < N_OPERATORS; n++) {
	comac_save (cr);
	for (i = 0; i < 4; i++) {
	    comac_reset_clip (cr);
	    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
	    comac_clip (cr);

	    comac_set_source (cr, pattern);
	    comac_set_operator (cr, COMAC_OPERATOR_OVER);
	    if (i & 2) {
	        comac_paint (cr);
	    } else {
	        comac_rectangle (cr, WIDTH/2, HEIGHT/2, WIDTH, HEIGHT);
		comac_fill (cr);
	    }

	    comac_set_source (cr, i & 1 ? alpha_pattern : color_alpha_pattern);
	    comac_set_operator (cr, n);
	    if (i & 2) {
	        comac_paint (cr);
	    } else {
	        comac_rectangle (cr, WIDTH/2, HEIGHT/2, WIDTH, HEIGHT);
		comac_fill (cr);
	    }

	    comac_translate (cr, 0, HEIGHT+PAD);
	}
	comac_restore (cr);

	comac_translate (cr, WIDTH+PAD, 0);
    }

    comac_pattern_destroy (pattern);
    comac_pattern_destroy (alpha_pattern);
    comac_pattern_destroy (color_alpha_pattern);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (surface_pattern_operator,
	    "Tests alpha-only and alpha-color sources with all operators",
	    "surface, pattern, operator", /* keywords */
	    NULL, /* requirements */
	    (WIDTH+PAD) * N_OPERATORS + PAD, 4*HEIGHT + 5*PAD,
	    NULL, draw)
