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

typedef struct {
    double x;
    double y;
} point_t;

#include "zrusin-another.h"

static void
zrusin_another_path (comac_t *cr)
{
    unsigned int i;

    for (i = 0; i < ARRAY_LENGTH (zrusin_another); i++)
	comac_line_to (cr, zrusin_another[i].x, zrusin_another[i].y);
}

static comac_time_t
zrusin_another_tessellate (comac_t *cr, int width, int height, int loops)
{
    zrusin_another_path (cr);

    comac_perf_timer_start ();

    /* We'd like to measure just tessellation without
     * rasterization. For now, we can do that with comac_in_fill. But
     * we'll have to be careful since comac_in_fill might eventually
     * be optimized to have an implementation that doesn't necessarily
     * include tessellation. */
    while (loops--)
	comac_in_fill (cr, 50, 50);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
zrusin_another_fill (comac_t *cr, int width, int height, int loops)
{
    zrusin_another_path (cr);
    comac_set_source_rgb (cr, 0.0, 0.0, 0.8); /* blue */

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
zrusin_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "zrusin", NULL);
}

void
zrusin (comac_perf_t *perf, comac_t *cr, int width, int height)
{

    comac_perf_run (perf,
		    "zrusin-another-tessellate",
		    zrusin_another_tessellate,
		    NULL);
    comac_perf_run (perf, "zrusin-another-fill", zrusin_another_fill, NULL);
}
