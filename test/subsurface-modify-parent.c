/*
 * Copyright 2010 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Intel not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Intel makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * INTEL CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *region;

    comac_set_source_rgb (cr, .5, .5, .5);
    comac_paint (cr);

    /* fill the centre, but through the *original* surface */
    region = comac_surface_create_for_rectangle (comac_get_target (cr),
						 20,
						 20,
						 20,
						 20);

    /* first trigger a snapshot of the region... */
    comac_set_source_surface (cr, region, 20, 20);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_REPEAT);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_rectangle (cr, 20, 20, 10, 10);
    comac_fill (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_rectangle (cr, 30, 20, 10, 10);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_rectangle (cr, 20, 30, 10, 10);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_rectangle (cr, 30, 30, 10, 10);
    comac_fill (cr);

    comac_set_source_surface (cr, region, 20, 20);
    comac_surface_destroy (region);

    /* repeat the pattern around the outside, but do not overwrite...*/
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_REPEAT);
    comac_rectangle (cr, 0, 0, width, height);
    comac_rectangle (cr, 20, 40, 20, -20);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    subsurface_modify_parent,
    "Tests source clipping with later modifications",
    "subsurface", /* keywords */
    "target=raster",
    /* FIXME! recursion bug in subsurface/snapshot (with pdf backend) */ /* requirements */
    60,
    60,
    NULL,
    draw)
