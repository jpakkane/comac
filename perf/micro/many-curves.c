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
do_many_curves_stroked (comac_t *cr, int width, int height, int loops)
{
    int count;

    state = 0xc0ffee;
    comac_move_to (cr, uniform_random (0, width), uniform_random (0, height));
    for (count = 0; count < 1000; count++) {
	double x1 = uniform_random (0, width);
	double x2 = uniform_random (0, width);
	double x3 = uniform_random (0, width);
	double y1 = uniform_random (0, height);
	double y2 = uniform_random (0, height);
	double y3 = uniform_random (0, height);
	comac_curve_to (cr, x1, y1, x2, y2, x3, y3);
    }

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_many_curves_hair_stroked (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 1.);
    return do_many_curves_stroked (cr, width, height, loops);
}

static comac_time_t
do_many_curves_wide_stroked (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 5.);
    return do_many_curves_stroked (cr, width, height, loops);
}

static comac_time_t
do_many_curves_filled (comac_t *cr, int width, int height, int loops)
{
    int count;

    state = 0xc0ffee;
    for (count = 0; count < 1000; count++) {
	double x0 = uniform_random (0, width);
	double x1 = uniform_random (0, width);
	double x2 = uniform_random (0, width);
	double x3 = uniform_random (0, width);
	double xm = uniform_random (0, width);
	double xn = uniform_random (0, width);
	double y0 = uniform_random (0, height);
	double y1 = uniform_random (0, height);
	double y2 = uniform_random (0, height);
	double y3 = uniform_random (0, height);
	double ym = uniform_random (0, height);
	double yn = uniform_random (0, height);
	comac_move_to (cr, xm, ym);
	comac_curve_to (cr, x1, y1, x2, y2, xn, yn);
	comac_curve_to (cr, x3, y3, x0, y0, xm, ym);
	comac_close_path (cr);
    }

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
many_curves_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "many-curves", NULL);
}

void
many_curves (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1., 1., 1.);

    comac_perf_run (perf, "many-curves-hair-stroked", do_many_curves_hair_stroked, NULL);
    comac_perf_run (perf, "many-curves-wide-stroked", do_many_curves_wide_stroked, NULL);
    comac_perf_run (perf, "many-curves-filled", do_many_curves_filled, NULL);
}
