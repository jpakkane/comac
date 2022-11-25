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

static const double m_1_sqrt_3 = 0.577359269;

static void
T (comac_t *cr, int size)
{
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, size, 0);
    comac_line_to (cr, size/2, size*m_1_sqrt_3);

    size /= 2;
    if (size >= 4) {
	T (cr, size);
	comac_save (cr); {
	    comac_translate (cr, size, 0);
	    T (cr, size);
	} comac_restore (cr);
	comac_save (cr); {
	    comac_translate (cr, size/2, size*m_1_sqrt_3);
	    T (cr, size);
	} comac_restore (cr);
    }
}

static comac_time_t
draw (comac_t *cr, int width, int height, int loops)
{
    int t_height = height/2;
    int t_width = t_height / m_1_sqrt_3;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_set_line_width (cr, 1.);

    comac_perf_timer_start ();

    while (loops--) {
	comac_save (cr);
	T (cr, t_width);

	comac_translate (cr, 0, height);
	comac_scale (cr, 1, -1);

	T (cr, t_width);

	comac_stroke (cr);
	comac_restore (cr);
    }

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

comac_bool_t
sierpinski_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "sierpinski", NULL);
}

void
sierpinski (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "sierpinski", draw, NULL);
}
