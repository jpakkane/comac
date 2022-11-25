/*
 * Copyright © 2009 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"
#include <float.h>

#define SIZE 256

/* This test is designed to test the accuracy of the rendering of mesh
 * patterns.
 *
 * Color accuracy is tested by a square patch covering the whole
 * surface with black and white corners.
 *
 * Extents accuracy is checked by a small red square patch at the
 * center of the surface which should measure 2x2 pixels.
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    double offset;

    comac_test_paint_checkered (cr);

    pattern = comac_pattern_create_mesh ();

    comac_mesh_pattern_begin_patch (pattern);

    comac_mesh_pattern_move_to (pattern, 0, 0);
    comac_mesh_pattern_line_to (pattern, 1, 0);
    comac_mesh_pattern_line_to (pattern, 1, 1);
    comac_mesh_pattern_line_to (pattern, 0, 1);

    comac_mesh_pattern_set_corner_color_rgb (pattern, 0, 0, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 1, 1, 1);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 1);

    comac_mesh_pattern_end_patch (pattern);

    comac_mesh_pattern_begin_patch (pattern);

    /* A small 1x1 red patch, that should be rendered as a 2x2 red
     * square in the center of the image */

    offset = 0.5 / SIZE;

    comac_mesh_pattern_move_to (pattern, 0.5 + offset, 0.5 + offset);
    comac_mesh_pattern_line_to (pattern, 0.5 + offset, 0.5 - offset);
    comac_mesh_pattern_line_to (pattern, 0.5 - offset, 0.5 - offset);
    comac_mesh_pattern_line_to (pattern, 0.5 - offset, 0.5 + offset);

    comac_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 1, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 2, 1, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 0, 0);

    comac_mesh_pattern_end_patch (pattern);

    comac_scale (cr, SIZE, SIZE);

    comac_set_source (cr, pattern);
    comac_paint (cr);
    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (mesh_pattern_accuracy,
	    "Paint mesh pattern",
	    "mesh, pattern", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
