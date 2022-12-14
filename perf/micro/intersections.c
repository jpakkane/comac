/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright (c) 2008  M Joonas Pihlaja
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "comac-perf.h"

#define NUM_SEGMENTS 256

static unsigned state;
static double
uniform_random (double minval, double maxval)
{
    static unsigned const poly = 0x9a795537U;
    unsigned n = 32;
    while (n-- > 0)
	state = 2 * state < state ? (2 * state ^ poly) : 2 * state;
    return minval + state * (maxval - minval) / 4294967296.0;
}

static comac_time_t
draw_random (
    comac_t *cr, comac_fill_rule_t fill_rule, int width, int height, int loops)
{
    double x[NUM_SEGMENTS];
    double y[NUM_SEGMENTS];
    int i;

    comac_save (cr);
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    for (i = 0; i < NUM_SEGMENTS; i++) {
	x[i] = uniform_random (0, width);
	y[i] = uniform_random (0, height);
    }

    state = 0x12345678;
    comac_translate (cr, 1, 1);
    comac_set_fill_rule (cr, fill_rule);
    comac_set_source_rgb (cr, 1, 0, 0);

    comac_new_path (cr);
    comac_move_to (cr, 0, 0);
    for (i = 0; i < NUM_SEGMENTS; i++)
	comac_line_to (cr, x[i], y[i]);
    comac_close_path (cr);

    comac_perf_timer_start ();
    while (loops--)
	comac_fill_preserve (cr);
    comac_perf_timer_stop ();

    comac_restore (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
draw_random_curve (
    comac_t *cr, comac_fill_rule_t fill_rule, int width, int height, int loops)
{
    double x[3 * NUM_SEGMENTS];
    double y[3 * NUM_SEGMENTS];
    int i;

    comac_save (cr);
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    for (i = 0; i < 3 * NUM_SEGMENTS; i++) {
	x[i] = uniform_random (0, width);
	y[i] = uniform_random (0, height);
    }

    state = 0x12345678;
    comac_translate (cr, 1, 1);
    comac_set_fill_rule (cr, fill_rule);
    comac_set_source_rgb (cr, 1, 0, 0);

    comac_new_path (cr);
    comac_move_to (cr, 0, 0);
    for (i = 0; i < NUM_SEGMENTS; i++) {
	comac_curve_to (cr,
			x[3 * i + 0],
			y[3 * i + 0],
			x[3 * i + 1],
			y[3 * i + 1],
			x[3 * i + 2],
			y[3 * i + 2]);
    }
    comac_close_path (cr);

    comac_perf_timer_start ();
    while (loops--)
	comac_fill_preserve (cr);
    comac_perf_timer_stop ();

    comac_restore (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
random_eo (comac_t *cr, int width, int height, int loops)
{
    return draw_random (cr, COMAC_FILL_RULE_EVEN_ODD, width, height, loops);
}

static comac_time_t
random_nz (comac_t *cr, int width, int height, int loops)
{
    return draw_random (cr, COMAC_FILL_RULE_WINDING, width, height, loops);
}

static comac_time_t
random_curve_eo (comac_t *cr, int width, int height, int loops)
{
    return draw_random_curve (cr,
			      COMAC_FILL_RULE_EVEN_ODD,
			      width,
			      height,
			      loops);
}

static comac_time_t
random_curve_nz (comac_t *cr, int width, int height, int loops)
{
    return draw_random_curve (cr,
			      COMAC_FILL_RULE_WINDING,
			      width,
			      height,
			      loops);
}

comac_bool_t
intersections_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "intersections", NULL);
}

void
intersections (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "intersections-nz-fill", random_nz, NULL);
    comac_perf_run (perf, "intersections-eo-fill", random_eo, NULL);

    comac_perf_run (perf, "intersections-nz-curve-fill", random_curve_nz, NULL);
    comac_perf_run (perf, "intersections-eo-curve-fill", random_curve_eo, NULL);
}
