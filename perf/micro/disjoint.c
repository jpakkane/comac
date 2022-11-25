/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright (c) 2011 Intel Corporation
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
#include <assert.h>

#define STEP 5

static void
path (comac_t *cr, int width, int height)
{
    int i;

    comac_rectangle (cr, 0, 0, width, height);
    comac_clip (cr);

    comac_translate (cr, width / 2, height / 2);
    comac_rotate (cr, M_PI / 4);
    comac_translate (cr, -width / 2, -height / 2);

    for (i = 0; i < width; i += STEP) {
	comac_rectangle (cr, i, -2, 1, height + 4);
	comac_rectangle (cr, -2, i, width + 4, 1);
    }
}

static void
clip (comac_t *cr, int width, int height)
{
    int i, j;

    for (j = 0; j < height; j += 2 * STEP) {
	for (i = 0; i < width; i += 2 * STEP)
	    comac_rectangle (cr, i, j, STEP, STEP);

	j += 2 * STEP;
	for (i = 0; i < width; i += 2 * STEP)
	    comac_rectangle (cr, i + STEP / 2, j, STEP, STEP);
    }

    comac_clip (cr);
}

static comac_time_t
draw (comac_t *cr, int width, int height, int loops)
{
    comac_save (cr);
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
    comac_set_source_rgb (cr, 1, 0, 0);

    comac_perf_timer_start ();
    while (loops--) {
	comac_save (cr);
	clip (cr, width, height);
	path (cr, width, height);
	comac_fill (cr);
	comac_restore (cr);
    }
    comac_perf_timer_stop ();

    comac_restore (cr);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
disjoint_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "disjoint", NULL);
}

void
disjoint (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    if (! comac_perf_can_run (perf, "disjoint", NULL))
	return;

    comac_perf_run (perf, "disjoint", draw, NULL);
}
