/*
 * Copyright © 2006 Red Hat, Inc.
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

/* This test case is designed to illustrate a performance bug in
 * drawing very long lines, where most of the line is out of bounds of
 * the destination surface, (but some portion of the line is
 * visible). These results are in the "long-lines-uncropped" report.
 *
 * For comparison, this test also renders the visible portions of the
 * same lines, (this is the "long-lines-cropped" report).
 */

typedef enum {
    LONG_LINES_CROPPED = 0x1,
    LONG_LINES_ONCE = 0x2,
} long_lines_crop_t;
#define NUM_LINES 20
#define LONG_FACTOR 50.0

static comac_time_t
do_long_lines (
    comac_t *cr, int width, int height, int loops, long_lines_crop_t crop)
{
    int i;
    double x, y, dx, dy, min_x, min_y, max_x, max_y;
    double outer_width, outer_height;

    comac_save (cr);

    comac_translate (cr, width / 2, height / 2);

    if (crop & LONG_LINES_CROPPED) {
	outer_width = width;
	outer_height = height;
	comac_set_source_rgb (cr, 0.0, 1.0, 0.0); /* green */
    } else {
	outer_width = LONG_FACTOR * width;
	outer_height = LONG_FACTOR * height;
	comac_set_source_rgb (cr, 1.0, 0.0, 0.0); /* red */
    }

    min_x = x = -outer_width / 2.0;
    min_y = y = -outer_height / 2.0;
    max_x = outer_width / 2.0;
    max_y = outer_height / 2.0;
    dx = outer_width / NUM_LINES;
    dy = outer_height / NUM_LINES;

    comac_perf_timer_start ();

    while (loops--) {
	for (i = 0; i <= NUM_LINES; i++) {
	    comac_move_to (cr, 0, 0);
	    comac_line_to (cr, x, min_y);
	    if ((crop & LONG_LINES_ONCE) == 0)
		comac_stroke (cr);

	    comac_move_to (cr, 0, 0);
	    comac_line_to (cr, x, max_y);
	    if ((crop & LONG_LINES_ONCE) == 0)
		comac_stroke (cr);

	    comac_move_to (cr, 0, 0);
	    comac_line_to (cr, min_x, y);
	    if ((crop & LONG_LINES_ONCE) == 0)
		comac_stroke (cr);

	    comac_move_to (cr, 0, 0);
	    comac_line_to (cr, max_x, y);
	    if ((crop & LONG_LINES_ONCE) == 0)
		comac_stroke (cr);

	    x += dx;
	    y += dy;
	}
	if (crop & LONG_LINES_ONCE)
	    comac_stroke (cr);
    }

    comac_perf_timer_stop ();

    comac_restore (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
long_lines_uncropped (comac_t *cr, int width, int height, int loops)
{
    return do_long_lines (cr, width, height, loops, 0);
}

static comac_time_t
long_lines_uncropped_once (comac_t *cr, int width, int height, int loops)
{
    return do_long_lines (cr, width, height, loops, LONG_LINES_ONCE);
}

static comac_time_t
long_lines_cropped (comac_t *cr, int width, int height, int loops)
{
    return do_long_lines (cr, width, height, loops, LONG_LINES_CROPPED);
}

static comac_time_t
long_lines_cropped_once (comac_t *cr, int width, int height, int loops)
{
    return do_long_lines (cr,
			  width,
			  height,
			  loops,
			  LONG_LINES_CROPPED | LONG_LINES_ONCE);
}

comac_bool_t
long_lines_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "long-lines", NULL);
}

void
long_lines (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "long-lines-uncropped", long_lines_uncropped, NULL);
    comac_perf_run (perf,
		    "long-lines-uncropped-once",
		    long_lines_uncropped_once,
		    NULL);
    comac_perf_run (perf, "long-lines-cropped", long_lines_cropped, NULL);
    comac_perf_run (perf,
		    "long-lines-cropped-once",
		    long_lines_cropped_once,
		    NULL);
}
