/*
 * Copyright © 2007 Björn Lindqvist
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
 * Author: Björn Lindqvist <bjourne@gmail.com>
 */

#include "comac-perf.h"

/* This test case measures how much time comac takes to render the
 * equivalent of the following gdk-pixbuf operation:
 *
 *    gdk_pixbuf_composite_color(dest,
 *                               0, 0, DST_SIZE, DST_SIZE,
 *                               0, 0,
 *                               SCALE, SCALE,
 *                               gdk.INTERP_NEAREST,
 *                               255,
 *                               0, 0,
 *                               8, 0x33333333, 0x88888888);
 *
 * Comac is (at the time of writing) about 2-3 times as slow as
 * gdk-pixbuf.
 */
#define PAT_SIZE 16
#define SRC_SIZE 64

static comac_pattern_t *checkerboard = NULL;
static comac_pattern_t *src_pattern = NULL;

static comac_time_t
do_composite_checker (comac_t *cr, int width, int height, int loops)
{
    /* Compute zoom so that the src_pattern covers the whole output image. */
    double xscale = width / (double) SRC_SIZE;
    double yscale = height / (double) SRC_SIZE;

    comac_perf_timer_start ();

    while (loops--) {
	/* Fill the surface with our background. */
	comac_identity_matrix (cr);
	comac_set_source (cr, checkerboard);
	comac_paint (cr);

	/* Draw the scaled image on top. */
	comac_scale (cr, xscale, yscale);
	comac_set_source (cr, src_pattern);
	comac_paint (cr);
    }

    comac_perf_timer_stop ();
    return comac_perf_timer_elapsed ();
}

comac_bool_t
composite_checker_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "composite-checker", NULL);
}

void
composite_checker (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_surface_t *image;

    /* Create the checker pattern. We don't actually need to draw
     * anything on it since that wouldn't affect performance.
     */
    image =
	comac_image_surface_create (COMAC_FORMAT_ARGB32, PAT_SIZE, PAT_SIZE);
    checkerboard = comac_pattern_create_for_surface (image);
    comac_pattern_set_filter (checkerboard, COMAC_FILTER_NEAREST);
    comac_pattern_set_extend (checkerboard, COMAC_EXTEND_REPEAT);
    comac_surface_destroy (image);

    /* Create the image source pattern. Again we use the NEAREST
     * filtering which should be fastest.
    */
    image =
	comac_image_surface_create (COMAC_FORMAT_ARGB32, SRC_SIZE, SRC_SIZE);
    src_pattern = comac_pattern_create_for_surface (image);
    comac_pattern_set_filter (src_pattern, COMAC_FILTER_NEAREST);
    comac_surface_destroy (image);

    comac_perf_run (perf, "composite-checker", do_composite_checker, NULL);

    comac_pattern_destroy (checkerboard);
    comac_pattern_destroy (src_pattern);
}
