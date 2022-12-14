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

static comac_time_t
horizontal (comac_t *cr, int width, int height, int loops)
{
    double h = height / 2 + .5;

    comac_move_to (cr, 0, h);
    comac_line_to (cr, width, h);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
horizontal_hair (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 1.);
    return horizontal (cr, width, height, loops);
}

static comac_time_t
horizontal_wide (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 5.);
    return horizontal (cr, width, height, loops);
}

static comac_time_t
nearly_horizontal (comac_t *cr, int width, int height, int loops)
{
    double h = height / 2;

    comac_move_to (cr, 0, h);
    comac_line_to (cr, width, h + 1);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
nearly_horizontal_hair (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 1.);
    return nearly_horizontal (cr, width, height, loops);
}

static comac_time_t
nearly_horizontal_wide (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 5.);
    return nearly_horizontal (cr, width, height, loops);
}

static comac_time_t
vertical (comac_t *cr, int width, int height, int loops)
{
    double w = width / 2 + .5;

    comac_move_to (cr, w, 0);
    comac_line_to (cr, w, height);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
vertical_hair (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 1.);
    return vertical (cr, width, height, loops);
}

static comac_time_t
vertical_wide (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 5.);
    return vertical (cr, width, height, loops);
}

static comac_time_t
nearly_vertical (comac_t *cr, int width, int height, int loops)
{
    double w = width / 2;

    comac_move_to (cr, w, 0);
    comac_line_to (cr, w + 1, height);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
nearly_vertical_hair (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 1.);
    return nearly_vertical (cr, width, height, loops);
}

static comac_time_t
nearly_vertical_wide (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 5.);
    return nearly_vertical (cr, width, height, loops);
}

static comac_time_t
diagonal (comac_t *cr, int width, int height, int loops)
{
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, width, height);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
diagonal_hair (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 1.);
    return diagonal (cr, width, height, loops);
}

static comac_time_t
diagonal_wide (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 5.);
    return diagonal (cr, width, height, loops);
}

comac_bool_t
a1_line_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "a1-line", NULL);
}

void
a1_line (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1., 1., 1.);
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    comac_perf_run (perf, "a1-line-hh", horizontal_hair, NULL);
    comac_perf_run (perf, "a1-line-hw", horizontal_wide, NULL);
    comac_perf_run (perf, "a1-line-nhh", nearly_horizontal_hair, NULL);
    comac_perf_run (perf, "a1-line-nhw", nearly_horizontal_wide, NULL);

    comac_perf_run (perf, "a1-line-vh", vertical_hair, NULL);
    comac_perf_run (perf, "a1-line-vw", vertical_wide, NULL);
    comac_perf_run (perf, "a1-line-nvh", nearly_vertical_hair, NULL);
    comac_perf_run (perf, "a1-line-nvw", nearly_vertical_wide, NULL);

    comac_perf_run (perf, "a1-line-dh", diagonal_hair, NULL);
    comac_perf_run (perf, "a1-line-dw", diagonal_wide, NULL);
}
