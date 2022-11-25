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

#define NUM_EXTEND 4
#define HEIGHT 32
#define WIDTH 32
#define PAD 6

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    unsigned int i, j;

    comac_extend_t extend[NUM_EXTEND] = {COMAC_EXTEND_NONE,
					 COMAC_EXTEND_REPEAT,
					 COMAC_EXTEND_REFLECT,
					 COMAC_EXTEND_PAD};

    comac_test_paint_checkered (cr);

    comac_translate (cr, PAD, PAD);

    for (i = 0; i < 3; i++) {
	comac_save (cr);

	for (j = 0; j < NUM_EXTEND; j++) {
	    comac_reset_clip (cr);
	    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
	    comac_clip (cr);

	    if (i == 0)
		pattern = comac_pattern_create_radial (WIDTH / 2,
						       HEIGHT / 2,
						       0,
						       WIDTH / 2,
						       HEIGHT / 2,
						       0);
	    else if (i == 1)
		pattern = comac_pattern_create_radial (WIDTH / 2,
						       HEIGHT / 2,
						       2 * PAD,
						       WIDTH / 2,
						       HEIGHT / 2,
						       2 * PAD);
	    else if (i == 2)
		pattern = comac_pattern_create_radial (PAD,
						       PAD,
						       0,
						       WIDTH - PAD,
						       HEIGHT - PAD,
						       0);

	    comac_pattern_add_color_stop_rgba (pattern, 0, 1, 0, 0, 1);
	    comac_pattern_add_color_stop_rgba (pattern,
					       sqrt (1.0 / 2.0),
					       0,
					       1,
					       0,
					       0);
	    comac_pattern_add_color_stop_rgba (pattern, 1, 0, 0, 1, 0.5);

	    comac_pattern_set_extend (pattern, extend[j]);

	    comac_set_source (cr, pattern);
	    comac_paint (cr);

	    comac_pattern_destroy (pattern);

	    comac_translate (cr, WIDTH + PAD, 0);
	}

	comac_restore (cr);
	comac_translate (cr, 0, HEIGHT + PAD);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (degenerate_radial_gradient,
	    "Tests degenerate radial gradients",
	    "radial, pattern, extend", /* keywords */
	    NULL,		       /* requirements */
	    (WIDTH + PAD) * NUM_EXTEND + PAD,
	    3 * (HEIGHT + PAD) + PAD,
	    NULL,
	    draw)
