/*
 * Copyright © 2007 Mozilla Corporation
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
 * Author: Vladimir Vukicevic <vladimir@pobox.com>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define SIZE 100
#define SIZE2 20
#define OFFSET 10

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *image;
    comac_pattern_t *pat;
    comac_t *cr2;

    image = comac_image_surface_create (COMAC_FORMAT_ARGB32, SIZE2, SIZE2);
    cr2 = comac_create (image);
    comac_surface_destroy (image);

    comac_set_source_rgba (cr2, 1, 0, 0, 1);
    comac_rectangle (cr2, 0, 0, SIZE2/2, SIZE2/2);
    comac_fill (cr2);
    comac_set_source_rgba (cr2, 0, 1, 0, 1);
    comac_rectangle (cr2, SIZE2/2, 0, SIZE2/2, SIZE2/2);
    comac_fill (cr2);
    comac_set_source_rgba (cr2, 0, 0, 1, 1);
    comac_rectangle (cr2, 0, SIZE2/2, SIZE2/2, SIZE2/2);
    comac_fill (cr2);
    comac_set_source_rgba (cr2, 1, 1, 0, 1);
    comac_rectangle (cr2, SIZE2/2, SIZE2/2, SIZE2/2, SIZE2/2);
    comac_fill (cr2);

    pat = comac_pattern_create_for_surface (comac_get_target (cr2));
    comac_destroy (cr2);

    comac_pattern_set_extend (pat, COMAC_EXTEND_REPEAT);

    comac_set_source_rgba (cr, 0, 0, 0, 1);
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_fill (cr);

    comac_translate (cr, OFFSET, OFFSET);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source (cr, pat);
    comac_rectangle (cr, 0, 0, SIZE - OFFSET, SIZE - OFFSET);
    comac_fill (cr);

    comac_pattern_destroy (pat);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (composite_integer_translate_over_repeat,
	    "Test simple compositing: integer-translation 32->32 OVER, with repeat",
	    "composite", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
