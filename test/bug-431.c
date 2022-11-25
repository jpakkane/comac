/*
 * Copyright © 2021 Lome More
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
    comac_matrix_t test_matrix;
    test_matrix.xx = 614;
    test_matrix.yx = 0;
    test_matrix.xy = 0;
    test_matrix.yy = -794;
    test_matrix.x0 = -1.3831;
    test_matrix.y0 = 793;
    comac_set_matrix(cr, &test_matrix);

    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *png_surface = comac_test_create_surface_from_png (ctx, "romedalen.png");
    comac_pattern_t *png_pattern = comac_pattern_create_for_surface(png_surface);
    comac_matrix_t matrix;
    matrix.xx = 1228;
    matrix.yx = 0;
    matrix.xy = 0;
    matrix.yy = -1590;
    matrix.x0 = 0;
    matrix.y0 = 1590;
    comac_pattern_set_matrix (png_pattern, &matrix);
    comac_pattern_t *mask_pattern = comac_pattern_create_rgba (1.0, 1.0, 1.0, 0.15);
    comac_save(cr);
    comac_set_source(cr, png_pattern);
    comac_mask(cr, mask_pattern);
    comac_restore(cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_431,
	    "Bug 431 (Different result on SVG surface)",
	    "", /* keywords */
	    NULL, /* requirements */
	    128, 96,
	    NULL, draw)
