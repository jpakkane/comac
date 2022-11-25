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

#define PAT_WIDTH  170
#define PAT_HEIGHT 170
#define SIZE PAT_WIDTH
#define PAD 10
#define WIDTH 190
#define HEIGHT 140


/* This test is designed to paint a mesh pattern containing two
 * overlapping patches transformed in different ways. */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;

    comac_test_paint_checkered (cr);

    comac_translate (cr, PAD, PAD);

    pattern = comac_pattern_create_mesh ();

    comac_mesh_pattern_begin_patch (pattern);

    comac_mesh_pattern_move_to (pattern, 0, 0);
    comac_mesh_pattern_curve_to (pattern, 30, -30,  60,  30, 100, 0);
    comac_mesh_pattern_curve_to (pattern, 60,  30, 130,  60, 100, 100);
    comac_mesh_pattern_curve_to (pattern, 60,  70,  30, 130,   0, 100);
    comac_mesh_pattern_curve_to (pattern, 30,  70, -30,  30,   0, 0);

    comac_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0);

    comac_mesh_pattern_end_patch (pattern);

    comac_mesh_pattern_begin_patch (pattern);

    comac_mesh_pattern_move_to (pattern, 50, 50);
    comac_mesh_pattern_curve_to (pattern, 80, 20, 110, 80, 150, 50);

    comac_mesh_pattern_curve_to (pattern, 110, 80, 180, 110, 150, 150);

    comac_mesh_pattern_curve_to (pattern, 110, 120, 80, 180, 50, 150);

    comac_mesh_pattern_curve_to (pattern, 80, 120, 20, 80, 50, 50);

    comac_mesh_pattern_set_corner_color_rgba (pattern, 0, 1, 0, 0, 0.3);
    comac_mesh_pattern_set_corner_color_rgb  (pattern, 1, 0, 1, 0);
    comac_mesh_pattern_set_corner_color_rgba (pattern, 2, 0, 0, 1, 0.3);
    comac_mesh_pattern_set_corner_color_rgb  (pattern, 3, 1, 1, 0);

    comac_mesh_pattern_end_patch (pattern);

    comac_scale (cr, .5, .5);

    comac_set_source (cr, pattern);
    comac_paint (cr);

    comac_translate (cr, PAT_WIDTH, PAT_HEIGHT);
    comac_translate (cr, PAT_WIDTH/2, PAT_HEIGHT/2);
    comac_rotate (cr, M_PI/4);
    comac_translate (cr, -PAT_WIDTH, -PAT_HEIGHT);
    comac_set_source (cr, pattern);
    comac_paint (cr);

    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (mesh_pattern_transformed,
	    "Paint mesh pattern with a transformation",
	    "mesh, pattern", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)

