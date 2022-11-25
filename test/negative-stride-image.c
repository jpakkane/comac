/*
 * Copyright 2012 Andrea Canciani
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

static const char *png_filename = "romedalen.png";

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_format_t format = COMAC_FORMAT_ARGB32;
    comac_t *cr_inv;
    comac_surface_t *png, *inv;
    uint8_t *data;
    int stride;

    png = comac_test_create_surface_from_png (ctx, png_filename);

    stride = comac_format_stride_for_width (format, width);
    data = xcalloc (stride, height);
    inv = comac_image_surface_create_for_data (data + stride * (height - 1),
					       format,
					       width,
					       height,
					       -stride);

    cr_inv = comac_create (inv);
    comac_set_source_surface (cr_inv, png, 0, 0);
    comac_paint (cr_inv);
    comac_destroy (cr_inv);

    comac_set_source_surface (cr, inv, 0, 0);
    comac_paint (cr);

    comac_surface_destroy (png);

    comac_surface_finish (inv);
    comac_surface_destroy (inv);

    free (data);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (negative_stride_image,
	    "Test that images with a negative stride are handled correctly.",
	    "stride, image", /* keywords */
	    NULL,	     /* requirements */
	    256,
	    192,
	    NULL,
	    draw)
