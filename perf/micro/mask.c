/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2009 Chris Wilson
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
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-perf.h"

static comac_time_t
do_mask_solid (comac_t *cr, int width, int height, int loops)
{
    comac_pattern_t *mask;

    mask = comac_pattern_create_rgba (0, 0, 0, .5);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_surface_t *
init_surface (comac_surface_t *surface, int width, int height)
{
    comac_t *cr;

    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgb (cr, 0, 0, 0); /* back */
    comac_paint (cr);

    comac_set_source_rgba (cr, 1, 1, 1, 0.5); /* 50% */
    comac_new_path (cr);
    comac_rectangle (cr, 0, 0, width/2.0, height/2.0);
    comac_rectangle (cr, width/2.0, height/2.0, width/2.0, height/2.0);
    comac_fill (cr);

    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return surface;
}

static comac_time_t
do_mask_image (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *surface;
    comac_pattern_t *mask;

    surface = comac_image_surface_create (COMAC_FORMAT_A8, width, height);
    mask = comac_pattern_create_for_surface (init_surface (surface,
							   width,
							   height));
    comac_surface_destroy (surface);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_mask_image_half (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *surface;
    comac_pattern_t *mask;
    comac_matrix_t matrix;

    surface = comac_image_surface_create (COMAC_FORMAT_A8, width, height);
    mask = comac_pattern_create_for_surface (init_surface (surface,
							   width,
							   height));
    comac_surface_destroy (surface);
    comac_matrix_init_scale (&matrix, .5, .5);
    comac_pattern_set_matrix (mask, &matrix);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_mask_image_double (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *surface;
    comac_pattern_t *mask;
    comac_matrix_t matrix;

    surface = comac_image_surface_create (COMAC_FORMAT_A8, width, height);
    mask = comac_pattern_create_for_surface (init_surface (surface,
							   width,
							   height));
    comac_surface_destroy (surface);
    comac_matrix_init_scale (&matrix, 2., 2.);
    comac_pattern_set_matrix (mask, &matrix);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_mask_similar (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *surface;
    comac_pattern_t *mask;

    surface = comac_surface_create_similar (comac_get_group_target (cr),
					    COMAC_CONTENT_ALPHA, width, height);
    mask = comac_pattern_create_for_surface (init_surface (surface,
							   width,
							   height));
    comac_surface_destroy (surface);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_mask_similar_half (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *surface;
    comac_pattern_t *mask;
    comac_matrix_t matrix;

    surface = comac_surface_create_similar (comac_get_group_target (cr),
					    COMAC_CONTENT_ALPHA, width, height);
    mask = comac_pattern_create_for_surface (init_surface (surface,
							   width,
							   height));
    comac_surface_destroy (surface);
    comac_matrix_init_scale (&matrix, .5, .5);
    comac_pattern_set_matrix (mask, &matrix);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_mask_similar_double (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *surface;
    comac_pattern_t *mask;
    comac_matrix_t matrix;

    surface = comac_surface_create_similar (comac_get_group_target (cr),
					    COMAC_CONTENT_ALPHA, width, height);
    mask = comac_pattern_create_for_surface (init_surface (surface,
							   width,
							   height));
    comac_surface_destroy (surface);
    comac_matrix_init_scale (&matrix, 2., 2.);
    comac_pattern_set_matrix (mask, &matrix);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_mask_linear (comac_t *cr, int width, int height, int loops)
{
    comac_pattern_t *mask;

    mask = comac_pattern_create_linear (0.0, 0.0, width, height);
    comac_pattern_add_color_stop_rgba (mask, 0.0, 0, 0, 0, 0.5); /*  50% */
    comac_pattern_add_color_stop_rgba (mask, 0.0, 0, 0, 0, 1.0); /* 100% */

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
do_mask_radial (comac_t *cr, int width, int height, int loops)
{
    comac_pattern_t *mask;

    mask = comac_pattern_create_radial (width/2.0, height/2.0, 0.0,
					width/2.0, height/2.0, width/2.0);
    comac_pattern_add_color_stop_rgba (mask, 0.0, 0, 0, 0, 0.5); /*  50% */
    comac_pattern_add_color_stop_rgba (mask, 0.0, 0, 0, 0, 1.0); /* 100% */

    comac_perf_timer_start ();

    while (loops--)
	comac_mask (cr, mask);

    comac_perf_timer_stop ();

    comac_pattern_destroy (mask);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
mask_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "mask", NULL);
}

void
mask (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    if (! comac_perf_can_run (perf, "mask", NULL))
	return;

    comac_perf_cover_sources_and_operators (perf, "mask-solid",
					    do_mask_solid, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-image",
					    do_mask_image, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-image-half",
					    do_mask_image_half, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-image-double",
					    do_mask_image_double, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-similar",
					    do_mask_similar, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-similar-half",
					    do_mask_similar_half, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-similar-double",
					    do_mask_similar_double, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-linear",
					    do_mask_linear, NULL);
    comac_perf_cover_sources_and_operators (perf, "mask-radial",
					    do_mask_radial, NULL);
}
