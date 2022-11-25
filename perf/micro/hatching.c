/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright (c) 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "comac-perf.h"
#include <assert.h>


#define STEP	5
#define WIDTH	100
#define HEIGHT	100

static void path (comac_t *cr, unsigned int width, unsigned int height)
{
    unsigned int i;

    for (i = 0; i < width+1; i += STEP) {
	comac_rectangle (cr, i-1, -1, 2, height+2);
	comac_rectangle (cr, -1, i-1, width+2, 2);
    }
}

static void aa (comac_t *cr)
{
    comac_set_antialias (cr, COMAC_ANTIALIAS_DEFAULT);
}

static void mono (comac_t *cr)
{
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
}

static void aligned (comac_t *cr, int width, int height)
{
}

static void misaligned (comac_t *cr, int width, int height)
{
    comac_translate (cr, 0.25, 0.25);
}

static void rotated (comac_t *cr, int width, int height)
{
    comac_translate (cr, width/2, height/2);
    comac_rotate (cr, M_PI/4);
    comac_translate (cr, -width/2, -height/2);
}

static void clip (comac_t *cr)
{
    comac_clip (cr);
    comac_paint (cr);
}

static void clip_alpha (comac_t *cr)
{
    comac_clip (cr);
    comac_paint_with_alpha (cr, .5);
}

static comac_time_t
draw (comac_t *cr,
      void (*prepare) (comac_t *cr),
      void (*transform) (comac_t *cr, int width, int height),
      void (*op) (comac_t *cr),
      int width, int height, int loops)
{
    comac_save (cr);
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
    comac_set_source_rgb (cr, 1, 0, 0);

    prepare (cr);

    comac_perf_timer_start ();
    while (loops--) {
	comac_save (cr);
	transform (cr, width, height);
	path (cr, width, height);
	op (cr);
	comac_restore (cr);
    }
    comac_perf_timer_stop ();

    comac_restore (cr);

    return comac_perf_timer_elapsed ();
}

static comac_time_t
draw_aligned_aa (comac_t *cr, int width, int height, int loops)
{
    return draw(cr, aa, aligned, comac_fill,
		width, height, loops);
}

static comac_time_t
draw_misaligned_aa (comac_t *cr, int width, int height, int loops)
{
    return draw(cr, aa, misaligned, comac_fill,
		width, height, loops);
}

static comac_time_t
draw_rotated_aa (comac_t *cr, int width, int height, int loops)
{
    return draw(cr, aa, rotated, comac_fill,
		width, height, loops);
}

static comac_time_t
draw_aligned_mono (comac_t *cr, int width, int height, int loops)
{
    return draw(cr, mono, aligned, comac_fill,
		width, height, loops);
}

static comac_time_t
draw_misaligned_mono (comac_t *cr, int width, int height, int loops)
{
    return draw(cr, mono, misaligned, comac_fill,
		width, height, loops);
}

static comac_time_t
draw_rotated_mono (comac_t *cr, int width, int height, int loops)
{
    return draw(cr, mono, rotated, comac_fill,
		width, height, loops);
}

#define F(name, op,transform,aa) \
static comac_time_t \
draw_##name (comac_t *cr, int width, int height, int loops) \
{ return draw(cr, (aa), (transform), (op), width, height, loops); }

F(clip_aligned, clip, aligned, aa)
F(clip_misaligned, clip, misaligned, aa)
F(clip_rotated, clip, rotated, aa)
F(clip_aligned_mono, clip, aligned, mono)
F(clip_misaligned_mono, clip, misaligned, mono)
F(clip_rotated_mono, clip, rotated, mono)

F(clip_alpha_aligned, clip_alpha, aligned, aa)
F(clip_alpha_misaligned, clip_alpha, misaligned, aa)
F(clip_alpha_rotated, clip_alpha, rotated, aa)
F(clip_alpha_aligned_mono, clip_alpha, aligned, mono)
F(clip_alpha_misaligned_mono, clip_alpha, misaligned, mono)
F(clip_alpha_rotated_mono, clip_alpha, rotated, mono)

comac_bool_t
hatching_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "hatching", NULL);
}

void
hatching (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "hatching-aligned-aa", draw_aligned_aa, NULL);
    comac_perf_run (perf, "hatching-misaligned-aa", draw_misaligned_aa, NULL);
    comac_perf_run (perf, "hatching-rotated-aa", draw_rotated_aa, NULL);
    comac_perf_run (perf, "hatching-aligned-mono", draw_aligned_mono, NULL);
    comac_perf_run (perf, "hatching-misaligned-mono", draw_misaligned_mono, NULL);
    comac_perf_run (perf, "hatching-rotated-mono", draw_rotated_mono, NULL);

    comac_perf_run (perf, "hatching-clip-aligned-aa", draw_clip_aligned, NULL);
    comac_perf_run (perf, "hatching-clip-misaligned-aa", draw_clip_misaligned, NULL);
    comac_perf_run (perf, "hatching-clip-rotated-aa", draw_clip_rotated, NULL);
    comac_perf_run (perf, "hatching-clip-aligned-mono", draw_clip_aligned_mono, NULL);
    comac_perf_run (perf, "hatching-clip-misaligned-mono", draw_clip_misaligned_mono, NULL);
    comac_perf_run (perf, "hatching-clip-rotated-mono", draw_clip_rotated_mono, NULL);

    comac_perf_run (perf, "hatching-clip-alpha-aligned-aa", draw_clip_alpha_aligned, NULL);
    comac_perf_run (perf, "hatching-clip-alpha-misaligned-aa", draw_clip_alpha_misaligned, NULL);
    comac_perf_run (perf, "hatching-clip-alpha-rotated-aa", draw_clip_alpha_rotated, NULL);
    comac_perf_run (perf, "hatching-clip-alpha-aligned-mono", draw_clip_alpha_aligned_mono, NULL);
    comac_perf_run (perf, "hatching-clip-alpha-misaligned-mono", draw_clip_alpha_misaligned_mono, NULL);
    comac_perf_run (perf, "hatching-clip-alpha-rotated-mono", draw_clip_alpha_rotated_mono, NULL);
}
