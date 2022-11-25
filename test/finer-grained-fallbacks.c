/*
 * Copyright © 2008 Adrian Johnson
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

#define CIRCLE_SIZE 10
#define PAD 2
#define WIDTH (CIRCLE_SIZE * 6.5 + PAD)
#define HEIGHT (CIRCLE_SIZE * 7.0 + PAD)

static void
draw_circle (comac_t *cr, double x, double y)
{
    comac_save (cr);
    comac_translate (cr, x, y);
    comac_arc (cr, 0, 0, CIRCLE_SIZE / 2, 0., 2. * M_PI);
    comac_fill (cr);
    comac_restore (cr);
}

static void
draw_image_circle (comac_t *cr, comac_surface_t *source, double x, double y)
{
    comac_save (cr);

    comac_set_source_surface (cr, source, x, y);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_REFLECT);
    comac_rectangle (cr, x, y, CIRCLE_SIZE, CIRCLE_SIZE);
    comac_fill (cr);

    comac_restore (cr);
}

static void
draw_circles (comac_t *cr)
{
    draw_circle (cr, 0, -CIRCLE_SIZE * 0.1);
    draw_circle (cr, CIRCLE_SIZE * 0.4, CIRCLE_SIZE * 0.25);

    draw_circle (cr, CIRCLE_SIZE * 2, 0);
    draw_circle (cr, CIRCLE_SIZE * 4, 0);
    draw_circle (cr, CIRCLE_SIZE * 6, 0);
}

static void
draw_image_circles (comac_t *cr, comac_surface_t *source)
{
    draw_image_circle (cr, source, 0, -CIRCLE_SIZE * 0.1);
    draw_image_circle (cr, source, CIRCLE_SIZE * 0.4, CIRCLE_SIZE * 0.25);

    draw_image_circle (cr, source, CIRCLE_SIZE * 2, 0);
    draw_image_circle (cr, source, CIRCLE_SIZE * 4, 0);
    draw_image_circle (cr, source, CIRCLE_SIZE * 6, 0);
}

/* For each of circle and fallback_circle we draw:
 *  - two overlapping
 *  - one isolated
 *  - one off the page
 *  - one overlapping the edge of the page.
 *
 * We also draw a circle and fallback_circle overlapping each other.
 *
 * Circles are drawn in green. An opaque color and COMAC_OPERATOR_OVER
 * is used to ensure they will be emitted as a vectors in PS/PDF.
 *
 * Fallback circles are drawn in red. COMAC_OPERATOR_ADD is used to
 * ensure they will be emitted as a fallback image in PS/PDF.
 *
 * In order to trigger a fallback for SVG, we need to use a surface with
 * REFLECT.
 */
static comac_surface_t *
surface_create (comac_t *target)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_surface_create_similar (comac_get_target (target),
					    COMAC_CONTENT_COLOR_ALPHA,
					    CIRCLE_SIZE,
					    CIRCLE_SIZE);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
    draw_circle (cr, CIRCLE_SIZE / 2, CIRCLE_SIZE / 2);

    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return surface;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;

    comac_translate (cr, PAD, PAD);

    comac_save (cr);

    /* Draw overlapping circle and fallback circle */
    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    draw_circle (cr, CIRCLE_SIZE * 0.5, CIRCLE_SIZE * 1.5);

    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    draw_circle (cr, CIRCLE_SIZE * 0.75, CIRCLE_SIZE * 1.75);

    /* Draw circles */
    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_translate (cr, CIRCLE_SIZE * 2.5, CIRCLE_SIZE * 0.6);
    draw_circles (cr);

    /* Draw fallback circles */
    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    comac_translate (cr, 0, CIRCLE_SIZE * 2);
    draw_circles (cr);

    comac_restore (cr);
    comac_translate (cr, 0, CIRCLE_SIZE * 3.5);

    /* Draw using fallback surface */
    surface = surface_create (cr);

    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    draw_circle (cr, CIRCLE_SIZE * 0.5, CIRCLE_SIZE * 1.5);

    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    draw_image_circle (cr,
		       surface,
		       CIRCLE_SIZE / 4,
		       CIRCLE_SIZE + CIRCLE_SIZE / 4);

    /* Draw circles */
    comac_set_source_rgb (cr, 0.0, 1.0, 0.0);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_translate (cr, CIRCLE_SIZE * 2.5, CIRCLE_SIZE * 0.6);
    draw_circles (cr);

    comac_set_operator (cr, COMAC_OPERATOR_ADD);
    comac_translate (cr, -CIRCLE_SIZE / 2, CIRCLE_SIZE * 1.5);
    draw_image_circles (cr, surface);

    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (finer_grained_fallbacks,
	    "Test that multiple PS/PDF fallback images in various locations "
	    "are correct",
	    "fallbacks", /* keywords */
	    NULL,	 /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
