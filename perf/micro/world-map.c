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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

typedef enum {
    WM_NEW_PATH,
    WM_MOVE_TO,
    WM_LINE_TO,
    WM_HLINE_TO,
    WM_VLINE_TO,
    WM_REL_LINE_TO,
    WM_END
} wm_type_t;

typedef struct _wm_element {
    wm_type_t type;
    double x;
    double y;
} wm_element_t;

#include "world-map.h"

enum {
    STROKE = 1,
    FILL = 2,
};

static comac_time_t
do_world_map (comac_t *cr, int width, int height, int loops, int mode)
{
    const wm_element_t *e;
    double cx, cy;

    comac_set_line_width (cr, 0.2);

    comac_perf_timer_start ();

    while (loops--) {
	comac_set_source_rgb (cr, .68, .85, .90); /* lightblue */
	comac_rectangle (cr, 0, 0, 800, 400);
	comac_fill (cr);

	e = &countries[0];
	while (1) {
	    switch (e->type) {
	    case WM_NEW_PATH:
	    case WM_END:
		if (mode & FILL) {
		    comac_set_source_rgb (cr, .75, .75, .75); /* silver */
		    comac_fill_preserve (cr);
		}
		if (mode & STROKE) {
		    comac_set_source_rgb (cr, .50, .50, .50); /* gray */
		    comac_stroke (cr);
		}
		comac_new_path (cr);
		comac_move_to (cr, e->x, e->y);
		break;
	    case WM_MOVE_TO:
		comac_close_path (cr);
		comac_move_to (cr, e->x, e->y);
		break;
	    case WM_LINE_TO:
		comac_line_to (cr, e->x, e->y);
		break;
	    case WM_HLINE_TO:
		comac_get_current_point (cr, &cx, &cy);
		comac_line_to (cr, e->x, cy);
		break;
	    case WM_VLINE_TO:
		comac_get_current_point (cr, &cx, &cy);
		comac_line_to (cr, cx, e->y);
		break;
	    case WM_REL_LINE_TO:
		comac_rel_line_to (cr, e->x, e->y);
		break;
	    }
	    if (e->type == WM_END)
		break;
	    e++;
	}

	comac_new_path (cr);
    }

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_world_map_stroke (comac_t *cr, int width, int height, int loops)
{
    return do_world_map (cr, width, height, loops, STROKE);
}

static comac_time_t
do_world_map_fill (comac_t *cr, int width, int height, int loops)
{
    return do_world_map (cr, width, height, loops, FILL);
}

static comac_time_t
do_world_map_both (comac_t *cr, int width, int height, int loops)
{
    return do_world_map (cr, width, height, loops, FILL | STROKE);
}

comac_bool_t
world_map_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "world-map", NULL);
}

void
world_map (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "world-map-stroke", do_world_map_stroke, NULL);
    comac_perf_run (perf, "world-map-fill", do_world_map_fill, NULL);
    comac_perf_run (perf, "world-map", do_world_map_both, NULL);
}
