/*
 * Copyright © 2012 Intel Corporation
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
 */

#include "comac-test.h"

/* An error in xlib pattern transformation discovered by Albertas Vyšniauskas */

static comac_pattern_t *
source (void)
{
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    comac_matrix_t matrix;
    comac_t *cr;
    int i;

    surface = comac_image_surface_create (COMAC_FORMAT_RGB24, 32, 32);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, .5, .5, .5);
    comac_set_line_width (cr, 2);

    for (i = -1; i <= 8; i++) {
	comac_move_to (cr, -34 + 8 * i, 34);
	comac_rel_line_to (cr, 36, -36);
	comac_stroke (cr);
    }

    pattern = comac_pattern_create_for_surface (comac_get_target (cr));
    comac_destroy (cr);

    comac_pattern_set_extend (pattern, COMAC_EXTEND_REPEAT);

    comac_matrix_init_translate (&matrix, 14.1, 0);
    comac_pattern_set_matrix (pattern, &matrix);

    return pattern;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    int i;

    comac_paint (cr);

    pattern = source ();
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);

    for (i = 0; i < 8; i++) {
	comac_rectangle (cr, 3.5 * i, 32 * i, 256, 32);
	comac_fill (cr);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_51910,
	    "A bug in the xlib pattern transformation",
	    " paint", /* keywords */
	    NULL,     /* requirements */
	    256,
	    256,
	    NULL,
	    draw)
