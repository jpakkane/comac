/*
 * Copyright © 2007 Red Hat, Inc.
 * Copyright © 2009 Adrian Johnson
 * Copyright © 2011 Intel Corporation
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
 * Authors:
 *	Behdad Esfahbod <behdad@behdad.org>
 *	Adrian Johnson <ajohnson@redneon.com>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define PAT_WIDTH 170
#define PAT_HEIGHT 170
#define SIZE PAT_WIDTH
#define PAD 2
#define WIDTH (PAD + SIZE + PAD)
#define HEIGHT WIDTH

/* This test is designed to paint a mesh pattern. The mesh contains
 * two overlapping patches */

static comac_pattern_t *
mesh (void)
{
    comac_pattern_t *pattern;

    pattern = comac_pattern_create_mesh ();

    comac_mesh_pattern_begin_patch (pattern);

    comac_mesh_pattern_move_to (pattern, 0, 0);
    comac_mesh_pattern_curve_to (pattern, 30, -30, 60, 30, 100, 0);
    comac_mesh_pattern_curve_to (pattern, 60, 30, 130, 60, 100, 100);
    comac_mesh_pattern_curve_to (pattern, 60, 70, 30, 130, 0, 100);
    comac_mesh_pattern_curve_to (pattern, 30, 70, -30, 30, 0, 0);

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
    comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0);
    comac_mesh_pattern_set_corner_color_rgba (pattern, 2, 0, 0, 1, 0.3);
    comac_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0);

    comac_mesh_pattern_end_patch (pattern);

    return pattern;
}

static comac_t *
draw (comac_t *cr)
{
    comac_pattern_t *source;

    comac_set_source_rgb (cr, 0, 1, 1);
    comac_paint (cr);

    source = mesh ();
    comac_set_source (cr, source);
    comac_pattern_destroy (source);

    comac_rectangle (cr, 10, 10, SIZE - 20, SIZE - 20);
    comac_clip (cr);
    comac_paint (cr);

    return cr;
}

static comac_t *
record_create (comac_t *target)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_recording_surface_create (
	comac_surface_get_content (comac_get_target (target)),
	NULL);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    return cr;
}

static comac_surface_t *
record_get (comac_t *target)
{
    comac_surface_t *surface;

    surface = comac_surface_reference (comac_get_target (target));
    comac_destroy (target);

    return surface;
}

static comac_test_status_t
record_replay (comac_t *cr, comac_t *(*func) (comac_t *), int width, int height)
{
    comac_surface_t *surface;
    int x, y;

    surface = record_get (func (record_create (cr)));

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_surface (cr, surface, 0, 0);
    comac_surface_destroy (surface);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_NONE);

    for (y = 0; y < height; y += 2) {
	for (x = 0; x < width; x += 2) {
	    comac_rectangle (cr, x, y, 2, 2);
	    comac_clip (cr);
	    comac_paint (cr);
	    comac_reset_clip (cr);
	}
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
record_mesh (comac_t *cr, int width, int height)
{
    return record_replay (cr, draw, width, height);
}

COMAC_TEST (record_mesh,
	    "Paint mesh pattern through a recording surface",
	    "record,mesh,pattern", /* keywords */
	    NULL,		   /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    record_mesh)
