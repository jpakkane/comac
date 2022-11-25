/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2007 Mozilla Corporation
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
 * Author: Vladimir Vukicevic <vladimir@pobox.com>
 */

#include "comac-perf.h"

static comac_time_t
do_long_dashed_lines (comac_t *cr, int width, int height, int loops)
{
    double dash[2] = {2.0, 2.0};
    int i;

    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);
    comac_set_dash (cr, dash, 2, 0.0);

    comac_new_path (cr);
    comac_set_line_width (cr, 1.0);

    for (i = 0; i < height - 1; i++) {
	double y0 = (double) i + 0.5;
	comac_move_to (cr, 0.0, y0);
	comac_line_to (cr, width, y0);
    }

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_restore (cr);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
long_dashed_lines_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "long-dashed-lines", NULL);
}

void
long_dashed_lines (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "long-dashed-lines", do_long_dashed_lines, NULL);
}
