/*
 * Copyright © 2006 Jeff Muizelaar <jeff@infidigm.net>
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
 * Authors: Jeff Muizelaar <jeff@infidigm.net>
 *          Carl Worth <cworth@cworth.org>
 */

#include "comac-perf.h"

static comac_time_t
do_unaligned_clip (comac_t *cr, int width, int height, int loops)
{
    comac_perf_timer_start ();

    while (loops--) {
	comac_save (cr);

	/* First a triangular clip that obviously isn't along device-pixel
	 * boundaries. */
	comac_move_to (cr, 50, 50);
	comac_line_to (cr, 50, 90);
	comac_line_to (cr, 90, 90);
	comac_close_path (cr);
	comac_clip (cr);

	/* Then a rectangular clip that would be but for the non-integer
	 * scaling. */
	comac_scale (cr, 1.1, 1.1);
	comac_rectangle (cr, 55, 55, 35, 35);
	comac_clip (cr);

	/* And paint something to force the clip to be evaluated. */
	comac_paint (cr);

	comac_restore (cr);
    }
    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

comac_bool_t
unaligned_clip_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "unaligned-clip", NULL);
}

void
unaligned_clip (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "unaligned-clip", do_unaligned_clip, NULL);
}
