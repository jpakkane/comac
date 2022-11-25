/*
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2007 Emmanuel Pacaud
 * Copyright © 2008 Benjamin Otte
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Owen Taylor <otaylor@redhat.com>
 *          Kristian Høgsberg <krh@redhat.com>
 *          Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define STEPS 16
#define START_OPERATOR COMAC_OPERATOR_MULTIPLY
#define STOP_OPERATOR COMAC_OPERATOR_HSL_LUMINOSITY

#define SIZE 5
#define COUNT 4
#define FULL_WIDTH ((STEPS + 1) * COUNT - 1)
#define FULL_HEIGHT                                                            \
    ((COUNT + STOP_OPERATOR - START_OPERATOR) / COUNT) * (STEPS + 1)

static void
set_solid_pattern (comac_t *cr, int step, comac_bool_t bg, comac_bool_t alpha)
{
    double c, a;

    a = ((double) step) / (STEPS - 1);
    if (alpha) {
	c = 1;
    } else {
	c = a;
	a = 1;
    }

    if (bg) /* draw a yellow background fading in using discrete steps */
	comac_set_source_rgba (cr, c, c, 0, a);
    else /* draw a teal foreground pattern fading in using discrete steps */
	comac_set_source_rgba (cr, 0, c, c, a);
}

/* expects a STEP*STEP pixel rectangle */
static void
do_blend_solid (comac_t *cr, comac_operator_t op, comac_bool_t alpha)
{
    int x;

    comac_save (cr);
    comac_scale (cr, SIZE, SIZE);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    for (x = 0; x < STEPS; x++) {
	/* draw the background using discrete steps */
	set_solid_pattern (cr, x, TRUE, alpha);
	comac_rectangle (cr, x, 0, 1, STEPS);
	comac_fill (cr);
    }

    comac_set_operator (cr, op);
    for (x = 0; x < STEPS; x++) {
	/* draw an orthogonal foreground pattern using discrete steps */
	set_solid_pattern (cr, x, FALSE, alpha);
	comac_rectangle (cr, 0, x, STEPS, 1);
	comac_fill (cr);
    }

    comac_restore (cr);
}

static void
create_patterns (comac_t *cr,
		 comac_surface_t **bg,
		 comac_surface_t **fg,
		 comac_bool_t alpha)
{
    comac_t *bgcr, *fgcr;

    *bg = comac_surface_create_similar (comac_get_target (cr),
					COMAC_CONTENT_COLOR_ALPHA,
					SIZE * STEPS,
					SIZE * STEPS);
    *fg = comac_surface_create_similar (comac_get_target (cr),
					COMAC_CONTENT_COLOR_ALPHA,
					SIZE * STEPS,
					SIZE * STEPS);

    bgcr = comac_create (*bg);
    fgcr = comac_create (*fg);

    do_blend_solid (bgcr, COMAC_OPERATOR_DEST, alpha);
    do_blend_solid (fgcr, COMAC_OPERATOR_SOURCE, alpha);

    comac_destroy (bgcr);
    comac_destroy (fgcr);
}

/* expects a STEP*STEP pixel rectangle */
static void
do_blend (comac_t *cr, comac_operator_t op, comac_bool_t alpha)
{
    comac_surface_t *bg, *fg;

    create_patterns (cr, &bg, &fg, alpha);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_surface (cr, bg, 0, 0);
    comac_paint (cr);

    comac_set_operator (cr, op);
    comac_set_source_surface (cr, fg, 0, 0);
    comac_paint (cr);

    comac_surface_destroy (fg);
    comac_surface_destroy (bg);
}

static void
do_blend_mask (comac_t *cr, comac_operator_t op, comac_bool_t alpha)
{
    comac_surface_t *bg, *fg;

    create_patterns (cr, &bg, &fg, alpha);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_surface (cr, bg, 0, 0);
    comac_paint (cr);

    comac_set_operator (cr, op);
    comac_set_source_surface (cr, fg, 0, 0);
    comac_paint_with_alpha (cr, .5);

    comac_surface_destroy (fg);
    comac_surface_destroy (bg);
}

static comac_test_status_t
draw (comac_t *cr,
      comac_bool_t alpha,
      void (*blend) (comac_t *, comac_operator_t, comac_bool_t))
{
    size_t i = 0;
    comac_operator_t op;

    for (op = START_OPERATOR; op <= STOP_OPERATOR; op++, i++) {
	comac_save (cr);
	comac_translate (cr,
			 SIZE * (STEPS + 1) * (i % COUNT),
			 SIZE * (STEPS + 1) * (i / COUNT));
	blend (cr, op, alpha);
	comac_restore (cr);
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_extended_blend (comac_t *cr, int width, int height)
{
    return draw (cr, FALSE, do_blend);
}

static comac_test_status_t
draw_extended_blend_alpha (comac_t *cr, int width, int height)
{
    return draw (cr, TRUE, do_blend);
}

static comac_test_status_t
draw_extended_blend_solid (comac_t *cr, int width, int height)
{
    return draw (cr, FALSE, do_blend_solid);
}

static comac_test_status_t
draw_extended_blend_solid_alpha (comac_t *cr, int width, int height)
{
    return draw (cr, TRUE, do_blend_solid);
}

static comac_test_status_t
draw_extended_blend_mask (comac_t *cr, int width, int height)
{
    return draw (cr, FALSE, do_blend_mask);
}
static comac_test_status_t
draw_extended_blend_alpha_mask (comac_t *cr, int width, int height)
{
    return draw (cr, TRUE, do_blend_mask);
}

COMAC_TEST (extended_blend,
	    "Tests extended blend modes without alpha",
	    "operator", /* keywords */
	    NULL,	/* requirements */
	    FULL_WIDTH *SIZE,
	    FULL_HEIGHT *SIZE,
	    NULL,
	    draw_extended_blend)

COMAC_TEST (extended_blend_alpha,
	    "Tests extended blend modes with alpha",
	    "operator", /* keywords */
	    NULL,	/* requirements */
	    FULL_WIDTH *SIZE,
	    FULL_HEIGHT *SIZE,
	    NULL,
	    draw_extended_blend_alpha)

COMAC_TEST (extended_blend_mask,
	    "Tests extended blend modes with an alpha mask",
	    "operator,mask", /* keywords */
	    NULL,	     /* requirements */
	    FULL_WIDTH *SIZE,
	    FULL_HEIGHT *SIZE,
	    NULL,
	    draw_extended_blend_mask)
COMAC_TEST (extended_blend_alpha_mask,
	    "Tests extended blend modes with an alpha mask",
	    "operator,mask", /* keywords */
	    NULL,	     /* requirements */
	    FULL_WIDTH *SIZE,
	    FULL_HEIGHT *SIZE,
	    NULL,
	    draw_extended_blend_alpha_mask)

COMAC_TEST (extended_blend_solid,
	    "Tests extended blend modes on solid patterns without alpha",
	    "operator", /* keywords */
	    NULL,	/* requirements */
	    FULL_WIDTH *SIZE,
	    FULL_HEIGHT *SIZE,
	    NULL,
	    draw_extended_blend_solid)

COMAC_TEST (extended_blend_solid_alpha,
	    "Tests extended blend modes on solid patterns with alpha",
	    "operator", /* keywords */
	    NULL,	/* requirements */
	    FULL_WIDTH *SIZE,
	    FULL_HEIGHT *SIZE,
	    NULL,
	    draw_extended_blend_solid_alpha)
