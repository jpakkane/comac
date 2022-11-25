/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright 2009 Chris Wilson
 * Copyright 2010 Andrea Canciani
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
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define WIDTH 60
#define HEIGHT 60

static void
stroke (comac_t *cr)
{
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    comac_set_operator (cr, COMAC_OPERATOR_IN);
    comac_set_source_rgb (cr, 0, 0.7, 0);
    comac_arc (cr, 10, 10, 7.5, 0, 2 * M_PI);
    comac_move_to (cr, 0, 20);
    comac_line_to (cr, 20, 0);
    comac_stroke (cr);
}

static void
fill (comac_t *cr)
{
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    comac_set_operator (cr, COMAC_OPERATOR_IN);
    comac_set_source_rgb (cr, 0, 0.7, 0);
    comac_new_sub_path (cr);
    comac_arc (cr, 10, 10, 8.5, 0, 2 * M_PI);
    comac_new_sub_path (cr);
    comac_arc_negative (cr, 10, 10, 6.5, 2 * M_PI, 0);

    comac_move_to (cr, -1, 19);
    comac_line_to (cr, 1, 21);
    comac_line_to (cr, 21, 1);
    comac_line_to (cr, 19, -1);
    comac_line_to (cr, -1, 19);

    comac_fill (cr);
}

static void
clip_simple (comac_t *cr)
{
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_clip (cr);
}

static void
clip_unaligned (comac_t *cr)
{
    comac_rectangle (cr, 0.5, 0.5, 20, 20);
    comac_clip (cr);
}

static void
clip_aligned (comac_t *cr)
{
    comac_fill_rule_t orig_rule;

    orig_rule = comac_get_fill_rule (cr);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_rectangle (cr, 3, 3, 10, 10);
    comac_rectangle (cr, 7, 7, 10, 10);
    comac_clip (cr);
    comac_set_fill_rule (cr, orig_rule);
}

static void
clip_mask (comac_t *cr)
{
    comac_arc (cr, 10, 10, 10, 0, 2 * M_PI);
    comac_new_sub_path (cr);
    comac_arc_negative (cr, 10, 10, 5, 2 * M_PI, 0);
    comac_new_sub_path (cr);
    comac_arc (cr, 10, 10, 2, 0, 2 * M_PI);
    comac_clip (cr);
}

static void (*const clip_funcs[]) (comac_t *cr) = {
    clip_simple, clip_unaligned, clip_aligned, clip_mask};

static double translations[][2] = {
    {10, 10}, {WIDTH, 0}, {-WIDTH, HEIGHT}, {WIDTH, 0}};

static comac_test_status_t
draw (comac_t *cr, void (*shapes) (comac_t *))
{
    unsigned int i;
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    for (i = 0; i < ARRAY_LENGTH (clip_funcs); i++) {
	comac_translate (cr, translations[i][0], translations[i][1]);

	comac_save (cr);
	comac_scale (cr, 2, 2);
	clip_funcs[i](cr);
	shapes (cr);
	comac_restore (cr);
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_stroke (comac_t *cr, int width, int height)
{
    return draw (cr, stroke);
}

static comac_test_status_t
draw_fill_nz (comac_t *cr, int width, int height)
{
    comac_set_fill_rule (cr, COMAC_FILL_RULE_WINDING);
    return draw (cr, fill);
}

static comac_test_status_t
draw_fill_eo (comac_t *cr, int width, int height)
{
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    return draw (cr, fill);
}

COMAC_TEST (clip_stroke_unbounded,
	    "Tests unbounded stroke through complex clips.",
	    "clip, stroke, unbounded", /* keywords */
	    NULL,		       /* requirements */
	    2 * WIDTH,
	    2 * HEIGHT,
	    NULL,
	    draw_stroke)

COMAC_TEST (
    clip_fill_nz_unbounded,
    "Tests unbounded fill through complex clips (with winding fill rule).",
    "clip, fill, unbounded", /* keywords */
    NULL,		     /* requirements */
    2 * WIDTH,
    2 * HEIGHT,
    NULL,
    draw_fill_nz)

COMAC_TEST (
    clip_fill_eo_unbounded,
    "Tests unbounded fill through complex clips (with even-odd fill rule).",
    "clip, fill, unbounded", /* keywords */
    NULL,		     /* requirements */
    2 * WIDTH,
    2 * HEIGHT,
    NULL,
    draw_fill_eo)
