/*
 * Copyright 2011 Red Hat Inc.
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
 * Author: Benjamin Otte <otte@redhat.com>
 */

#include "comac-perf.h"

static comac_surface_t *
generate_random_waveform(comac_t *target, int width, int height)
{
    comac_surface_t *surface;
    comac_t *cr;
    int i, r;

    srand (0xdeadbeef);

    surface = comac_surface_create_similar (comac_get_target (target),
					    COMAC_CONTENT_ALPHA,
					    width, height);
    cr = comac_create (surface);

    r = height / 2;

    for (i = 0; i < width; i++)
    {
	r += rand () % (height / 4) - (height / 8);
	if (r < 0)
	    r = 0;
	else if (r > height)
	    r = height;
	comac_rectangle (cr, i, (height - r) / 2, 1, r);
    }
    comac_fill (cr);
    comac_destroy (cr);

    return surface;
}

static comac_time_t
do_wave (comac_t *cr, int width, int height, int loops)
{
    comac_surface_t *wave;
    comac_pattern_t *mask;

    wave = generate_random_waveform (cr, width, height);

    comac_perf_timer_start ();

    while (loops--) {
	/* paint outline (and contents) */
	comac_set_source_rgb (cr, 1, 0, 0);
	comac_mask_surface (cr, wave, 0, 0);

	/* overdraw inline */
	/* first, create a mask */
	comac_push_group_with_content (cr, COMAC_CONTENT_ALPHA);
	comac_set_source_surface (cr, wave, 0, 0);
	comac_paint (cr);
	comac_set_operator (cr, COMAC_OPERATOR_IN);
	comac_set_source_surface (cr, wave, 1, 0);
	comac_paint (cr);
	comac_set_source_surface (cr, wave, -1, 0);
	comac_paint (cr);
	comac_set_source_surface (cr, wave, 0, 1);
	comac_paint (cr);
	comac_set_source_surface (cr, wave, 0, -1);
	comac_paint (cr);
	mask = comac_pop_group (cr);

	/* second, paint the mask */
	comac_set_source_rgb (cr, 0, 1, 0);
	comac_mask (cr, mask);

	comac_pattern_destroy (mask);
    }

    comac_perf_timer_stop ();

    comac_surface_destroy (wave);

    return comac_perf_timer_elapsed ();
}

comac_bool_t
wave_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "wave", NULL);
}

void
wave (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "wave", do_wave, NULL);
}
