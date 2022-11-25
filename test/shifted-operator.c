/*
 * Copyright © 2021 Anton Danilkin
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

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *recording_surface =
	comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, NULL);
    comac_t *cr2 = comac_create (recording_surface);
    comac_translate (cr2, -30, -30);
    comac_rectangle (cr2, 0, 0, 120, 90);
    comac_set_source_rgba (cr2, 0.7, 0, 0, 0.8);
    comac_fill (cr2);
    comac_set_operator (cr2, COMAC_OPERATOR_XOR);
    comac_rectangle (cr2, 40, 30, 120, 90);
    comac_set_source_rgba (cr2, 0, 0, 0.9, 0.4);
    comac_fill (cr2);
    comac_destroy (cr2);

    comac_pattern_t *pattern =
	comac_pattern_create_for_surface (recording_surface);
    comac_surface_destroy (recording_surface);
    comac_matrix_t matrix;
    comac_matrix_init_translate (&matrix, -30, -30);
    comac_pattern_set_matrix (pattern, &matrix);
    comac_set_source (cr, pattern);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (shifted_operator,
	    "Test drawing a rectangle shifted into negative coordinates with "
	    "an operator",
	    "operator, transform, record", /* keywords */
	    NULL,			   /* requirements */
	    160,
	    120,
	    NULL,
	    draw)
