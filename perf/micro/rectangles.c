/*
 * Copyright © 2006 Dan Amelang
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
 * Authors: Dan Amelang <dan@amelang.net>
 */
#include "comac-perf.h"

#if 0
#define MODE comac_perf_run
#else
#define MODE comac_perf_cover_sources_and_operators
#endif

#define RECTANGLE_COUNT (1000)

static struct {
    double x;
    double y;
    double width;
    double height;
} rects[RECTANGLE_COUNT];

static comac_time_t
do_rectangles (comac_t *cr, int width, int height, int loops)
{
    int i;

    comac_perf_timer_start ();

    while (loops--) {
	for (i = 0; i < RECTANGLE_COUNT; i++) {
	    comac_rectangle (cr,
			     rects[i].x,
			     rects[i].y,
			     rects[i].width,
			     rects[i].height);
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
	    comac_rectangle (cr,
			     rects[i].x,
			     rects[i].y,
			     rects[i].width,
			     rects[i].height);
	}

	comac_fill (cr);
    }

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_rectangle (comac_t *cr, int width, int height, int loops)
{
    comac_perf_timer_start ();

    while (loops--) {
	comac_rectangle (cr, 0, 0, width, height);
	comac_fill (cr);
    }

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

comac_bool_t
rectangles_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "rectangles", NULL);
}

void
rectangles (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    int i;

    srand (8478232);
    for (i = 0; i < RECTANGLE_COUNT; i++) {
	rects[i].x = rand () % width;
	rects[i].y = rand () % height;
	rects[i].width = (rand () % (width / 10)) + 1;
	rects[i].height = (rand () % (height / 10)) + 1;
    }

    MODE (perf, "one-rectangle", do_rectangle, NULL);
    MODE (perf, "rectangles", do_rectangles, NULL);
    MODE (perf, "rectangles-once", do_rectangles_once, NULL);
}
