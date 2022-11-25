/*
 * Copyright © 2005 Owen Taylor
 * Copyright © 2007 Dan Amelang
 * Copyright © 2007 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * the authors not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The authors make no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Chris Wilson <chris@chris-wilson.co.uk>
 */

/* This perf case is derived from the bug report
 *   Gradient on 'rounded rectangle' MUCH slower than normal rectangle
 *   https://bugs.freedesktop.org/show_bug.cgi?id=4263.
 */

#include "comac-perf.h"

#define RECTANGLE_COUNT (1000)

#if 0
#define MODE comac_perf_run
#else
#define MODE comac_perf_cover_sources_and_operators
#endif

static struct {
    double x;
    double y;
    double width;
    double height;
} rects[RECTANGLE_COUNT];

static void
rounded_rectangle (
    comac_t *cr, double x, double y, double w, double h, double radius)
{
    comac_move_to (cr, x + radius, y);
    comac_arc (cr,
	       x + w - radius,
	       y + radius,
	       radius,
	       M_PI + M_PI / 2,
	       M_PI * 2);
    comac_arc (cr, x + w - radius, y + h - radius, radius, 0, M_PI / 2);
    comac_arc (cr, x + radius, y + h - radius, radius, M_PI / 2, M_PI);
    comac_arc (cr, x + radius, y + radius, radius, M_PI, 270 * M_PI / 180);
}

static comac_time_t
do_rectangle (comac_t *cr, int width, int height, int loops)
{
    comac_perf_timer_start ();

    while (loops--) {
	rounded_rectangle (cr, 0, 0, width, height, 3.0);
	comac_fill (cr);
    }

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_rectangles (comac_t *cr, int width, int height, int loops)
{
    int i;

    comac_perf_timer_start ();

    while (loops--) {
	for (i = 0; i < RECTANGLE_COUNT; i++) {
	    rounded_rectangle (cr,
			       rects[i].x,
			       rects[i].y,
			       rects[i].width,
			       rects[i].height,
			       3.0);
	    comac_fill (cr);
	}
    }

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_rectangles_once (comac_t *cr, int width, int height, int loops)
{
    int i;

    comac_perf_timer_start ();

    while (loops--) {
	for (i = 0; i < RECTANGLE_COUNT; i++) {
	    rounded_rectangle (cr,
			       rects[i].x,
			       rects[i].y,
			       rects[i].width,
			       rects[i].height,
			       3.0);
	}
	comac_fill (cr);
    }

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

comac_bool_t
rounded_rectangles_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "rounded-rectangles", NULL);
}

void
rounded_rectangles (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    int i;

    srand (8478232);
    for (i = 0; i < RECTANGLE_COUNT; i++) {
	rects[i].x = rand () % width;
	rects[i].y = rand () % height;
	rects[i].width = (rand () % (width / 10)) + 1;
	rects[i].height = (rand () % (height / 10)) + 1;
    }

    MODE (perf, "one-rounded-rectangle", do_rectangle, NULL);
    MODE (perf, "rounded-rectangles", do_rectangles, NULL);
    MODE (perf, "rounded-rectangles-once", do_rectangles_once, NULL);
}
