/*
 * Copyright © 2006 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-perf.h"

/* This case exposes a performance bug found by Christopher "Monty"
 * Montgomery in that copying a tiny portion of an image surface to an
 * X surface causes the entire image surface to be copied to an
 * intermediate surface.
 *
 * If the performance bug is fixed, then the time this test takes
 * should be independent of the source and destination surface sizes.
 */

static comac_time_t
do_subimage_copy (comac_t *cr, int width, int height, int loops)
{
    comac_rectangle (cr, 2, 2, 4, 4);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
subimage_copy_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "subimage-copy", NULL);
}

void
subimage_copy (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_surface_t *image;
    comac_t *cr2;

    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    image = comac_image_surface_create (COMAC_FORMAT_ARGB32, width, height);
    cr2 = comac_create (image);
    comac_set_source_rgb (cr2, 1, 0, 0); /* red */
    comac_paint (cr2);
    comac_destroy (cr2);

    comac_set_source_surface (cr, image, 0, 0);
    comac_surface_destroy (image);

    comac_perf_run (perf, "subimage-copy", do_subimage_copy, NULL);
}
