/*
 * Copyright © 2007 Adrian Johnson
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

#define PAT_WIDTH  120
#define PAT_HEIGHT 120
#define SIZE (PAT_WIDTH*2)
#define PAD 2
#define WIDTH (PAD + SIZE + PAD)
#define HEIGHT WIDTH


/* This test is designed to test painting a recording surface pattern with
 * COMAC_EXTEND_NONE and a non identity pattern matrix.
 */
static comac_pattern_t *create_pattern (comac_t *target)
{
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    comac_t *cr;

    surface = comac_surface_create_similar (comac_get_group_target (target),
					    COMAC_CONTENT_COLOR_ALPHA,
					    PAT_WIDTH, PAT_HEIGHT);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgba (cr, 1, 0, 1, 0.5);
    comac_rectangle (cr, PAT_WIDTH/6.0, PAT_HEIGHT/6.0, PAT_WIDTH/4.0, PAT_HEIGHT/4.0);
    comac_fill (cr);

    comac_set_source_rgba (cr, 0, 1, 1, 0.5);
    comac_rectangle (cr, PAT_WIDTH/2.0, PAT_HEIGHT/2.0, PAT_WIDTH/4.0, PAT_HEIGHT/4.0);
    comac_fill (cr);

    comac_set_line_width (cr, 1);
    comac_move_to (cr, PAT_WIDTH/6.0, 0);
    comac_line_to (cr, 0, 0);
    comac_line_to (cr, 0, PAT_HEIGHT/6.0);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_stroke (cr);
    comac_move_to (cr, PAT_WIDTH/6.0, PAT_HEIGHT);
    comac_line_to (cr, 0, PAT_HEIGHT);
    comac_line_to (cr, 0, 5*PAT_HEIGHT/6.0);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_stroke (cr);
    comac_move_to (cr, 5*PAT_WIDTH/6.0, 0);
    comac_line_to (cr, PAT_WIDTH, 0);
    comac_line_to (cr, PAT_WIDTH, PAT_HEIGHT/6.0);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_stroke (cr);
    comac_move_to (cr, 5*PAT_WIDTH/6.0, PAT_HEIGHT);
    comac_line_to (cr, PAT_WIDTH, PAT_HEIGHT);
    comac_line_to (cr, PAT_WIDTH, 5*PAT_HEIGHT/6.0);
    comac_set_source_rgb (cr, 1, 1, 0);
    comac_stroke (cr);

    comac_set_source_rgb (cr, 0.5, 0.5, 0.5);
    comac_set_line_width (cr, PAT_WIDTH/10.0);

    comac_move_to (cr, 0,         PAT_HEIGHT/4.0);
    comac_line_to (cr, PAT_WIDTH, PAT_HEIGHT/4.0);
    comac_stroke (cr);

    comac_move_to (cr, PAT_WIDTH/4.0,         0);
    comac_line_to (cr, PAT_WIDTH/4.0, PAT_WIDTH);
    comac_stroke (cr);

    pattern = comac_pattern_create_for_surface (comac_get_target (cr));
    comac_destroy (cr);

    return pattern;
}

static comac_test_status_t
over (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    comac_matrix_t   mat;

    comac_translate (cr, PAD, PAD);

    pattern = create_pattern (cr);

    comac_matrix_init_identity (&mat);
    comac_matrix_scale (&mat, 2, 1.5);
    comac_matrix_rotate (&mat, 1);
    comac_matrix_translate (&mat, -PAT_WIDTH/4.0, -PAT_WIDTH/2.0);
    comac_pattern_set_matrix (pattern, &mat);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_NONE);

    comac_set_source (cr, pattern);
    comac_paint (cr);

    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
source (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    comac_matrix_t   mat;

    comac_translate (cr, PAD, PAD);

    pattern = create_pattern (cr);

    comac_matrix_init_identity (&mat);
    comac_matrix_scale (&mat, 2, 1.5);
    comac_matrix_rotate (&mat, 1);
    comac_matrix_translate (&mat, -PAT_WIDTH/4.0, -PAT_WIDTH/2.0);
    comac_pattern_set_matrix (pattern, &mat);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_NONE);

    comac_set_source (cr, pattern);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_paint (cr);

    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (recording_surface_over,
	    "Paint recording surface pattern with non identity pattern matrix",
	    "recording", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, over)

COMAC_TEST (recording_surface_source,
	    "Paint recording surface pattern with non identity pattern matrix",
	    "recording", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, source)
