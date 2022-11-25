/*
 * Copyright © 2005, 2007 Red Hat, Inc.
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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define NUM_GRADIENTS 7
#define NUM_EXTEND 4
#define SIZE 120
#define WIDTH (SIZE * NUM_GRADIENTS)
#define HEIGHT (SIZE * NUM_EXTEND)

typedef void (*composite_t)(comac_t *cr, comac_pattern_t *pattern);
typedef void (*add_stops_t)(comac_pattern_t *pattern);

/*
 * We want to test all the possible relative positions of the start
 * and end circle:
 *
 *  - The start circle can be smaller/equal/bigger than the end
 *    circle. A radial gradient can be classified in one of these
 *    three cases depending on the sign of dr.
 *
 *  - The smaller circle can be completely inside/internally
 *    tangent/outside (at least in part) of the bigger circle. This
 *    classification is the same as the one which can be computed by
 *    examining the sign of a = (dx^2 + dy^2 - dr^2).
 *
 *  - If the two circles have the same size, neither can be inside or
 *    internally tangent
 *
 * This test draws radial gradients whose circles always have the same
 * centers (0, 0) and (1, 0), but with different radiuses. From left
 * to right:
 *
 * - Small start circle completely inside the end circle
 *     0.25 -> 1.75; dr =  1.5 > 0; a = 1 - 1.50^2 < 0
 *
 * - Small start circle internally tangent to the end circle
 *     0.50 -> 1.50; dr =  1.0 > 0; a = 1 - 1.00^2 = 0
 *
 * - Small start circle outside of the end circle
 *     0.50 -> 1.00; dr =  0.5 > 0; a = 1 - 0.50^2 > 0
 *
 * - Start circle with the same size as the end circle
 *     1.00 -> 1.00; dr =  0.0 = 0; a = 1 - 0.00^2 > 0
 *
 * - Small end circle outside of the start circle
 *     1.00 -> 0.50; dr = -0.5 > 0; a = 1 - 0.50^2 > 0
 *
 * - Small end circle internally tangent to the start circle
 *     1.50 -> 0.50; dr = -1.0 > 0; a = 1 - 1.00^2 = 0
 *
 * - Small end circle completely inside the start circle
 *     1.75 -> 0.25; dr = -1.5 > 0; a = 1 - 1.50^2 < 0
 *
 */

static const double radiuses[NUM_GRADIENTS] = {
    0.25,
    0.50,
    0.50,
    1.00,
    1.00,
    1.50,
    1.75
};

static comac_pattern_t *
create_pattern (int index)
{
    double x0, x1, radius0, radius1, left, right, center;

    x0 = 0;
    x1 = 1;
    radius0 = radiuses[index];
    radius1 = radiuses[NUM_GRADIENTS - index - 1];

    /* center the gradient */
    left = MIN (x0 - radius0, x1 - radius1);
    right = MAX (x0 + radius0, x1 + radius1);
    center = (left + right) * 0.5;
    x0 -= center;
    x1 -= center;

    /* scale to make it fit within a 1x1 rect centered in (0,0) */
    x0 *= 0.25;
    x1 *= 0.25;
    radius0 *= 0.25;
    radius1 *= 0.25;

    return comac_pattern_create_radial (x0, 0, radius0, x1, 0, radius1);
}

static void
pattern_add_stops (comac_pattern_t *pattern)
{
    comac_pattern_add_color_stop_rgba (pattern, 0.0,        1, 0, 0, 0.75);
    comac_pattern_add_color_stop_rgba (pattern, sqrt (0.5), 0, 1, 0, 0);
    comac_pattern_add_color_stop_rgba (pattern, 1.0,        0, 0, 1, 1);
}

static void
pattern_add_single_stop (comac_pattern_t *pattern)
{
    comac_pattern_add_color_stop_rgba (pattern, 0.25, 1, 0, 0, 1);
}


static comac_test_status_t
draw (comac_t *cr, add_stops_t add_stops, composite_t composite)
{
    int i, j;
    comac_extend_t extend[NUM_EXTEND] = {
	COMAC_EXTEND_NONE,
	COMAC_EXTEND_REPEAT,
	COMAC_EXTEND_REFLECT,
	COMAC_EXTEND_PAD
    };

    comac_scale (cr, SIZE, SIZE);
    comac_translate (cr, 0.5, 0.5);

    for (j = 0; j < NUM_EXTEND; j++) {
	comac_save (cr);
	for (i = 0; i < NUM_GRADIENTS; i++) {
	    comac_pattern_t *pattern;

	    pattern = create_pattern (i);
	    add_stops (pattern);
	    comac_pattern_set_extend (pattern, extend[j]);

	    comac_save (cr);
	    comac_rectangle (cr, -0.5, -0.5, 1, 1);
	    comac_clip (cr);
	    composite (cr, pattern);
	    comac_restore (cr);
	    comac_pattern_destroy (pattern);

	    comac_translate (cr, 1, 0);
	}
	comac_restore (cr);
	comac_translate (cr, 0, 1);
    }

    return COMAC_TEST_SUCCESS;
}


static void
composite_simple (comac_t *cr, comac_pattern_t *pattern)
{
    comac_set_source (cr, pattern);
    comac_paint (cr);
}

static void
composite_mask (comac_t *cr, comac_pattern_t *pattern)
{
    comac_set_source_rgb (cr, 1, 0, 1);
    comac_mask (cr, pattern);
}


static comac_test_status_t
draw_simple (comac_t *cr, int width, int height)
{
    comac_test_paint_checkered (cr);
    return draw (cr, pattern_add_stops, composite_simple);
}

static comac_test_status_t
draw_mask (comac_t *cr, int width, int height)
{
    comac_test_paint_checkered (cr);
    return draw (cr, pattern_add_stops, composite_mask);
}

static comac_test_status_t
draw_source (comac_t *cr, int width, int height)
{
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    return draw (cr, pattern_add_stops, composite_simple);
}


static comac_test_status_t
draw_mask_source (comac_t *cr, int width, int height)
{
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    return draw (cr, pattern_add_stops, composite_mask);
}

static comac_test_status_t
draw_one_stop (comac_t *cr, int width, int height)
{
    comac_test_paint_checkered (cr);
    return draw (cr, pattern_add_single_stop, composite_simple);
}

COMAC_TEST (radial_gradient,
	    "Simple test of radial gradients",
	    "gradient", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw_simple)

COMAC_TEST (radial_gradient_mask,
	    "Simple test of radial gradients using a MASK",
	    "gradient,mask", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw_mask)

COMAC_TEST (radial_gradient_source,
	    "Simple test of radial gradients using the SOURCE operator",
	    "gradient,source", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw_source)

COMAC_TEST (radial_gradient_mask_source,
	    "Simple test of radial gradients using a MASK with a SOURCE operator",
	    "gradient,mask,source", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw_mask_source)

COMAC_TEST (radial_gradient_one_stop,
	    "Tests radial gradients with a single stop",
	    "gradient,radial", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw_one_stop)
