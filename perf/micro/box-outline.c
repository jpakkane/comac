/*
 * Copyright © 2006 Red Hat, Inc.
 *
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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-perf.h"

/* This test case is designed to illustrate a performance bug that
 * exists in comac in which using comac_stroke is much slower than
 * comac_fill to draw an identical figure, (and in particular a figure
 * that is much more natural to draw with comac_stroke). The figure is
 * a 100x100 square outline 1-pixel wide, nicely pixel aligned.
 *
 * The performance bug should affect any path whose resulting contour
 * consists only of pixel-aligned horizontal and vertical elements.
 *
 * Initial testing on on machine shows stroke as 5x slower than fill
 * for the xlib backend and 16x slower for the image backend.
 */

static comac_time_t
box_outline_stroke (comac_t *cr, int width, int height, int loops)
{
    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    comac_rectangle (cr, 1.5, 1.5, width - 3, height - 3);
    comac_set_line_width (cr, 1.0);
    comac_set_source_rgb (cr, 1, 0, 0); /* red */

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
box_outline_alpha_stroke (comac_t *cr, int width, int height, int loops)
{
    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    comac_rectangle (cr, 1.5, 1.5, width - 3, height - 3);
    comac_set_line_width (cr, 1.0);
    comac_set_source_rgba (cr, 1, 0, 0, .5); /* red */

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
box_outline_aa_stroke (comac_t *cr, int width, int height, int loops)
{
    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    comac_translate (cr, .5, .5);
    comac_rectangle (cr, 1.5, 1.5, width - 3, height - 3);
    comac_set_line_width (cr, 1.0);
    comac_set_source_rgb (cr, 1, 0, 0); /* red */

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
box_outline_fill (comac_t *cr, int width, int height, int loops)
{
    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    comac_rectangle (cr, 1.0, 1.0, width - 2, height - 2);
    comac_rectangle (cr, 2.0, 2.0, width - 4, height - 4);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_set_source_rgb (cr, 0, 1, 0); /* green */

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
box_outline_alpha_fill (comac_t *cr, int width, int height, int loops)
{
    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    comac_rectangle (cr, 1.0, 1.0, width - 2, height - 2);
    comac_rectangle (cr, 2.0, 2.0, width - 4, height - 4);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_set_source_rgba (cr, 0, 1, 0, .5); /* green */

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
box_outline_aa_fill (comac_t *cr, int width, int height, int loops)
{
    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    comac_translate (cr, .5, .5);
    comac_rectangle (cr, 1.0, 1.0, width - 2, height - 2);
    comac_rectangle (cr, 2.0, 2.0, width - 4, height - 4);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_set_source_rgb (cr, 0, 1, 0); /* green */

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
box_outline_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "box-outline", NULL);
}

void
box_outline (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "box-outline-stroke", box_outline_stroke, NULL);
    comac_perf_run (perf, "box-outline-fill", box_outline_fill, NULL);

    comac_perf_run (perf,
		    "box-outline-alpha-stroke",
		    box_outline_alpha_stroke,
		    NULL);
    comac_perf_run (perf,
		    "box-outline-alpha-fill",
		    box_outline_alpha_fill,
		    NULL);

    comac_perf_run (perf, "box-outline-aa-stroke", box_outline_aa_stroke, NULL);
    comac_perf_run (perf, "box-outline-aa-fill", box_outline_aa_fill, NULL);
}
