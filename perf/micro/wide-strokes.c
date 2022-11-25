/*
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
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-perf.h"

static uint32_t state;

static double
uniform_random (double minval, double maxval)
{
    static uint32_t const poly = 0x9a795537U;
    uint32_t n = 32;
    while (n-->0)
	state = 2*state < state ? (2*state ^ poly) : 2*state;
    return minval + state * (maxval - minval) / 4294967296.0;
}

static comac_time_t
do_wide_strokes_ha (comac_t *cr, int width, int height, int loops)
{
    int count;

    state = 0xc0ffee;
    for (count = 0; count < 1000; count++) {
	double h = floor (uniform_random (0, height));
	comac_move_to (cr, floor (uniform_random (0, width)), h);
	comac_line_to (cr, ceil (uniform_random (0, width)), h);
    }

    comac_set_line_width (cr, 5.);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_wide_strokes_h (comac_t *cr, int width, int height, int loops)
{
    int count;

    state = 0xc0ffee;
    for (count = 0; count < 1000; count++) {
	double h = uniform_random (0, height);
	comac_move_to (cr, uniform_random (0, width), h);
	comac_line_to (cr, uniform_random (0, width), h);
    }

    comac_set_line_width (cr, 5.);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_wide_strokes_va (comac_t *cr, int width, int height, int loops)
{
    int count;

    state = 0xc0ffee;
    for (count = 0; count < 1000; count++) {
	double v = floor (uniform_random (0, width));
	comac_move_to (cr, v, floor (uniform_random (0, height)));
	comac_line_to (cr, v, ceil (uniform_random (0, height)));
    }

    comac_set_line_width (cr, 5.);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_wide_strokes_v (comac_t *cr, int width, int height, int loops)
{
    int count;

    state = 0xc0ffee;
    for (count = 0; count < 1000; count++) {
	double v = uniform_random (0, width);
	comac_move_to (cr, v, uniform_random (0, height));
	comac_line_to (cr, v, uniform_random (0, height));
    }

    comac_set_line_width (cr, 5.);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_wide_strokes (comac_t *cr, int width, int height, int loops)
{
    int count;

    /* lots and lots of overlapping strokes */
    state = 0xc0ffee;
    for (count = 0; count < 1000; count++) {
	comac_line_to (cr,
		       uniform_random (0, width),
		       uniform_random (0, height));
    }

    comac_set_line_width (cr, 5.);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
wide_strokes_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "wide-strokes", NULL);
}

void
wide_strokes (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1., 1., 1.);

    comac_perf_run (perf, "wide-strokes-halign", do_wide_strokes_ha, NULL);
    comac_perf_run (perf, "wide-strokes-valign", do_wide_strokes_va, NULL);
    comac_perf_run (perf, "wide-strokes-horizontal", do_wide_strokes_h, NULL);
    comac_perf_run (perf, "wide-strokes-vertical", do_wide_strokes_v, NULL);
    comac_perf_run (perf, "wide-strokes-random", do_wide_strokes, NULL);
}
