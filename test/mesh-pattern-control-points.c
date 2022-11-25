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

#define SIZE 90
#define PAD 10
#define WIDTH (PAD + 2 * (SIZE + PAD))
#define HEIGHT (PAD + SIZE + PAD)


/*
 * This test is designed to paint a two mesh patches. One with default
 * control points and one with a control point at a no default
 * location.  The control points of both of them are drawn as squares
 * to make them visible.
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    unsigned int i, j;
    unsigned int num_patches;
    double x, y;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_translate (cr, PAD, PAD);

    pattern = comac_pattern_create_mesh ();
    comac_mesh_pattern_begin_patch (pattern);

    comac_mesh_pattern_move_to (pattern,    0,    0);
    comac_mesh_pattern_line_to (pattern, SIZE,    0);
    comac_mesh_pattern_line_to (pattern, SIZE, SIZE);
    comac_mesh_pattern_line_to (pattern,    0, SIZE);

    comac_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0);

    comac_mesh_pattern_set_control_point (pattern, 0, SIZE * .7, SIZE * .7);
    comac_mesh_pattern_set_control_point (pattern, 1, SIZE * .9, SIZE * .7);
    comac_mesh_pattern_set_control_point (pattern, 2, SIZE * .9, SIZE * .9);
    comac_mesh_pattern_set_control_point (pattern, 3, SIZE * .7, SIZE * .9);

    comac_mesh_pattern_end_patch (pattern);

    comac_mesh_pattern_begin_patch (pattern);

    comac_mesh_pattern_move_to (pattern,   SIZE + PAD,    0);
    comac_mesh_pattern_line_to (pattern, 2*SIZE + PAD,    0);
    comac_mesh_pattern_line_to (pattern, 2*SIZE + PAD, SIZE);
    comac_mesh_pattern_line_to (pattern,   SIZE + PAD, SIZE);

    comac_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0);

    comac_mesh_pattern_end_patch (pattern);

    comac_set_source (cr, pattern);
    comac_paint (cr);

    /* mark the location of the control points */
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_mesh_pattern_get_patch_count (pattern, &num_patches);
    for (i = 0; i < num_patches; i++) {
	for (j = 0; j < 4; j++) {
	    comac_mesh_pattern_get_control_point (pattern, i, j, &x, &y);
	    comac_rectangle (cr, x - 5, y - 5, 10, 10);
	    comac_fill (cr);
	}
    }

    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}


COMAC_TEST (mesh_pattern_control_points,
	    "Paint mesh pattern with non default control points",
	    "mesh, pattern", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
