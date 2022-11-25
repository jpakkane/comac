/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright © 2005 Red Hat, Inc.
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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

typedef struct {
    comac_operator_t op;
    double tolerance;
    comac_fill_rule_t fill_rule;
    double line_width;
    comac_line_cap_t line_cap;
    comac_line_join_t line_join;
    double miter_limit;
    comac_matrix_t matrix;
    double dash[5];
    double dash_offset;
} settings_t;

/* Two sets of settings, no defaults */
static const settings_t settings[] = {
    {
	COMAC_OPERATOR_IN,
	2.0,
	COMAC_FILL_RULE_EVEN_ODD,
	7.7,
	COMAC_LINE_CAP_SQUARE,
	COMAC_LINE_JOIN_ROUND,
	3.14,
	{2.0, 0.0, 0.0, 2.0, 5.0, 5.0},
	{0.1, 0.2, 0.3, 0.4, 0.5},
	2.0
    },
    {
	COMAC_OPERATOR_ATOP,
	5.25,
	COMAC_FILL_RULE_WINDING,
	2.17,
	COMAC_LINE_CAP_ROUND,
	COMAC_LINE_JOIN_BEVEL,
	1000.0,
	{-3.0, 1.0, 1.0, -3.0, -4, -4},
	{1.0, 2.0, 3.0, 4.0, 5.0},
	3.0
    }
};

static void
settings_set (comac_t *cr, const settings_t *settings)
{
    comac_set_operator (cr, settings->op);
    comac_set_tolerance (cr, settings->tolerance);
    comac_set_fill_rule (cr, settings->fill_rule);
    comac_set_line_width (cr, settings->line_width);
    comac_set_line_cap (cr, settings->line_cap);
    comac_set_line_join (cr, settings->line_join);
    comac_set_miter_limit (cr, settings->miter_limit);
    comac_set_matrix (cr, &settings->matrix);
    comac_set_dash (cr, settings->dash, 5, settings->dash_offset);
}

static int
settings_get (comac_t *cr, settings_t *settings)
{
    int count;

    settings->op = comac_get_operator (cr);
    settings->tolerance = comac_get_tolerance (cr);
    settings->fill_rule = comac_get_fill_rule (cr);
    settings->line_width = comac_get_line_width (cr);
    settings->line_cap = comac_get_line_cap (cr);
    settings->line_join = comac_get_line_join (cr);
    settings->miter_limit = comac_get_miter_limit (cr);
    comac_get_matrix (cr, &settings->matrix);

    count = comac_get_dash_count (cr);
    if (count != 5)
	return -1;

    comac_get_dash (cr, settings->dash, &settings->dash_offset);

    return 0;
}

static int
settings_equal (const settings_t *a, const settings_t *b)
{
    return (a->op == b->op &&
	    a->tolerance == b->tolerance &&
	    a->fill_rule == b->fill_rule &&
	    a->line_width == b->line_width &&
	    a->line_cap == b->line_cap &&
	    a->line_join == b->line_join &&
	    a->miter_limit == b->miter_limit &&
	    a->matrix.xx == b->matrix.xx &&
	    a->matrix.xy == b->matrix.xy &&
	    a->matrix.x0 == b->matrix.x0 &&
	    a->matrix.yx == b->matrix.yx &&
	    a->matrix.yy == b->matrix.yy &&
	    a->matrix.y0 == b->matrix.y0 &&
	    memcmp(a->dash, b->dash, sizeof(a->dash)) == 0 &&
	    a->dash_offset == b->dash_offset);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    settings_t check;

    settings_set (cr, &settings[0]);

    comac_save (cr);
    {
	settings_set (cr, &settings[1]);
	if (settings_get (cr, &check))
	    return COMAC_TEST_FAILURE;

	if (!settings_equal (&settings[1], &check))
	    return COMAC_TEST_FAILURE;
    }
    comac_restore (cr);

    if (settings_get (cr, &check))
	return COMAC_TEST_FAILURE;

    if (!settings_equal (&settings[0], &check))
	return COMAC_TEST_FAILURE;

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (get_and_set,
	    "Tests calls to the most trivial comac_get and comac_set functions",
	    "api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    NULL, draw)
