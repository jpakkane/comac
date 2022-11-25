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
#define OFFSET 10

static const char *png_filename = "romedalen.png";

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *image;

    image = comac_test_create_surface_from_png (ctx, png_filename);

    comac_set_source_rgba (cr, 0, 0, 0, 1);
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_fill (cr);

    comac_translate (cr, OFFSET, OFFSET);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_surface (cr, image, 0, 0);
    comac_rectangle (cr, 0, 0, SIZE - OFFSET, SIZE - OFFSET);
    comac_fill (cr);

    comac_surface_destroy (image);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (composite_integer_translate_source,
	    "Test simple compositing: integer-translation 32->32 SOURCE",
	    "composite", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
