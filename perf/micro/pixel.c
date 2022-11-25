/*
 * Copyright © 2011 Intel Corporation
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
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

/* Measure the overhead in setting a single pixel */

#include "comac-perf.h"

#include <pixman.h>

static comac_time_t
pixel_direct (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *surface, *image;
    uint32_t *data;
    int stride, bpp;

    surface = comac_get_target (cr);
    image = comac_surface_map_to_image (surface, NULL);
    data = (uint32_t *) comac_image_surface_get_data (image);
    stride = comac_image_surface_get_stride (image) / sizeof (uint32_t);

    switch (comac_image_surface_get_format (image)) {
    default:
    case COMAC_FORMAT_INVALID:
    case COMAC_FORMAT_A1: bpp = 0; break;
    case COMAC_FORMAT_A8: bpp = 8; break;
    case COMAC_FORMAT_RGB16_565: bpp = 16; break;
    case COMAC_FORMAT_RGB24:
    case COMAC_FORMAT_RGB30:
    case COMAC_FORMAT_ARGB32: bpp = 32; break;
    case COMAC_FORMAT_RGB96F: bpp = 96; break;
    case COMAC_FORMAT_RGBA128F: bpp = 128; break;
    }

    comac_perf_timer_start ();

    while (loops--)
	pixman_fill (data, stride, bpp, 0, 0, 1, 1, -1);

    comac_perf_timer_stop ();

    comac_surface_unmap_image (surface, image);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
pixel_paint (comac_t *cr, int width, int height, int loops)
{
    comac_perf_timer_start ();

    while (loops--)
	comac_paint (cr);

    comac_perf_timer_stop ();

    return comac_perf_timer_elapsed ();
}

static comac_time_t
pixel_mask (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *mask;
    comac_t *cr2;

    mask = comac_surface_create_similar (comac_get_target (cr),
					 COMAC_CONTENT_ALPHA,
					 1, 1);
    cr2 = comac_create (mask);
    comac_set_source_rgb (cr2, 1,1,1);
    comac_paint (cr2);
    comac_destroy (cr2);

    comac_perf_timer_start ();

    while (loops--)
	comac_mask_surface (cr, mask, 0, 0);

    comac_perf_timer_stop ();

    comac_surface_destroy (mask);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
pixel_rectangle (comac_t *cr, int width, int height, int loops)
{
    comac_new_path (cr);
    comac_rectangle (cr, 0, 0, 1, 1);

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);
    return comac_perf_timer_elapsed ();
}

static comac_time_t
pixel_subrectangle (comac_t *cr, int width, int height, int loops)
{
    comac_new_path (cr);
    comac_rectangle (cr, 0.1, 0.1, .8, .8);

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);
    return comac_perf_timer_elapsed ();
}

static comac_time_t
pixel_triangle (comac_t *cr, int width, int height, int loops)
{
    comac_new_path (cr);
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, 1, 1);
    comac_line_to (cr, 0, 1);

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);
    return comac_perf_timer_elapsed ();
}

static comac_time_t
pixel_circle (comac_t *cr, int width, int height, int loops)
{
    comac_new_path (cr);
    comac_arc (cr, 0.5, 0.5, 0.5, 0, 2 * M_PI);

    comac_perf_timer_start ();

    while (loops--)
	comac_fill_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);
    return comac_perf_timer_elapsed ();
}

static comac_time_t
pixel_stroke (comac_t *cr, int width, int height, int loops)
{
    comac_set_line_width (cr, 1.);
    comac_new_path (cr);
    comac_move_to (cr, 0, 0.5);
    comac_line_to (cr, 1, 0.5);

    comac_perf_timer_start ();

    while (loops--)
	comac_stroke_preserve (cr);

    comac_perf_timer_stop ();

    comac_new_path (cr);
    return comac_perf_timer_elapsed ();
}

comac_bool_t
pixel_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "pixel", NULL);
}

void
pixel (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1., 1., 1.);

    comac_perf_run (perf, "pixel-direct", pixel_direct, NULL);
    comac_perf_run (perf, "pixel-paint", pixel_paint, NULL);
    comac_perf_run (perf, "pixel-mask", pixel_mask, NULL);
    comac_perf_run (perf, "pixel-rectangle", pixel_rectangle, NULL);
    comac_perf_run (perf, "pixel-subrectangle", pixel_subrectangle, NULL);
    comac_perf_run (perf, "pixel-triangle", pixel_triangle, NULL);
    comac_perf_run (perf, "pixel-circle", pixel_circle, NULL);
    comac_perf_run (perf, "pixel-stroke", pixel_stroke, NULL);
}

comac_bool_t
a1_pixel_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "a1-pixel", NULL);
}

void
a1_pixel (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1., 1., 1.);
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);

    comac_perf_run (perf, "a1-pixel-direct", pixel_direct, NULL);
    comac_perf_run (perf, "a1-pixel-paint", pixel_paint, NULL);
    comac_perf_run (perf, "a1-pixel-mask", pixel_mask, NULL);
    comac_perf_run (perf, "a1-pixel-rectangle", pixel_rectangle, NULL);
    comac_perf_run (perf, "a1-pixel-subrectangle", pixel_subrectangle, NULL);
    comac_perf_run (perf, "a1-pixel-triangle", pixel_triangle, NULL);
    comac_perf_run (perf, "a1-pixel-circle", pixel_circle, NULL);
    comac_perf_run (perf, "a1-pixel-stroke", pixel_stroke, NULL);
}
