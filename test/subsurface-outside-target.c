/*
 * Copyright 2010 Red Hat Inc.
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
 * Author: Benjamin Otte <otte@gnome.org>
 */

#include "comac-test.h"

#define TARGET_SIZE 10

#define SUB_SIZE 15
#define SUB_OFFSET -5

#define PAINT_OFFSET SUB_SIZE
#define PAINT_SIZE (3 * SUB_SIZE)

static comac_content_t contents[] = {
    COMAC_CONTENT_ALPHA, COMAC_CONTENT_COLOR, COMAC_CONTENT_COLOR_ALPHA};

#define N_CONTENTS ARRAY_LENGTH (contents)
#define N_PADS (COMAC_EXTEND_PAD + 1)

static comac_surface_t *
create_target (comac_surface_t *similar_to, comac_content_t content)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_surface_create_similar (similar_to,
					    content,
					    TARGET_SIZE,
					    TARGET_SIZE);

    cr = comac_create (surface);
    comac_test_paint_checkered (cr);
    comac_destroy (cr);

    return surface;
}

static comac_test_status_t
check_surface_extents (const comac_test_context_t *ctx,
		       comac_surface_t *surface,
		       double x,
		       double y,
		       double width,
		       double height)
{
    double x1, y1, x2, y2;
    comac_t *cr;

    cr = comac_create (surface);
    comac_clip_extents (cr, &x1, &y1, &x2, &y2);
    comac_destroy (cr);

    if (x != x1 || y != y1 || width != x2 - x1 || height != y2 - y1) {
	comac_test_log (ctx,
			"surface extents should be (%g, %g, %g, %g), but are "
			"(%g, %g, %g, %g)\n",
			x,
			y,
			width,
			height,
			x1,
			y1,
			x2 - x1,
			y2 - y1);
	return COMAC_TEST_FAILURE;
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_for_size (comac_t *cr, double x, double y)
{
    comac_surface_t *target, *subsurface;
    comac_extend_t extend;
    comac_test_status_t check, result = COMAC_TEST_SUCCESS;
    unsigned int content;

    for (content = 0; content < N_CONTENTS; content++) {
	comac_save (cr);

	/* create a target surface for our subsurface */
	target = create_target (comac_get_target (cr), contents[content]);

	/* create a subsurface that extends the target surface */
	subsurface = comac_surface_create_for_rectangle (target,
							 x,
							 y,
							 SUB_SIZE,
							 SUB_SIZE);

	/* ensure the extents are ok */
	check = check_surface_extents (comac_test_get_context (cr),
				       subsurface,
				       0,
				       0,
				       SUB_SIZE,
				       SUB_SIZE);
	if (result == COMAC_TEST_SUCCESS)
	    result = check;

	/* paint this surface with all extend modes. */
	for (extend = 0; extend < N_PADS; extend++) {
	    comac_save (cr);

	    comac_rectangle (cr, 0, 0, PAINT_SIZE, PAINT_SIZE);
	    comac_clip (cr);

	    comac_set_source_surface (cr,
				      subsurface,
				      PAINT_OFFSET,
				      PAINT_OFFSET);
	    comac_pattern_set_extend (comac_get_source (cr), extend);
	    comac_paint (cr);

	    comac_restore (cr);

	    comac_translate (cr, PAINT_SIZE + TARGET_SIZE, 0);
	}

	comac_surface_destroy (subsurface);
	comac_surface_destroy (target);

	comac_restore (cr);

	comac_translate (cr, 0, PAINT_SIZE + TARGET_SIZE);
    }

    return result;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_test_status_t check, result = COMAC_TEST_SUCCESS;

    /* paint background in nice gray */
    comac_set_source_rgb (cr, 0.51613, 0.55555, 0.51613);
    comac_paint (cr);

    /* Use COMAC_OPERATOR_SOURCE in the tests so we get the actual
     * contents of the subsurface */
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);

    result = draw_for_size (cr, SUB_OFFSET, SUB_OFFSET);

    check = draw_for_size (cr, 0, 0);
    if (result == COMAC_TEST_SUCCESS)
	result = check;

    return result;
}

COMAC_TEST (
    subsurface_outside_target,
    "Tests contents of subsurfaces outside target area",
    "subsurface, pad", /* keywords */
    "target=raster",
    /* FIXME! recursion bug in subsurface/snapshot (with pdf backend) */ /* requirements */
	    (PAINT_SIZE + TARGET_SIZE) * N_PADS -
	TARGET_SIZE,
    (PAINT_SIZE + TARGET_SIZE) * N_CONTENTS * 2 - TARGET_SIZE,
    NULL,
    draw)
