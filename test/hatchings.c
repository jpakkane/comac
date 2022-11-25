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

#include "comac-test.h"

#define STEP	5
#define WIDTH	100
#define HEIGHT	100

static void hatching (comac_t *cr)
{
    int i;

    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);

    for (i = 0; i < WIDTH; i += STEP) {
	comac_rectangle (cr, i-1, -2, 2, HEIGHT+4);
	comac_rectangle (cr, -2, i-1, WIDTH+4, 2);
    }
}

static void background (comac_t *cr)
{
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgb (cr, 1,1,1);
    comac_paint (cr);
}

static void clip_to_quadrant (comac_t *cr)
{
    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);
}

static void draw_hatching (comac_t *cr, void (*func) (comac_t *))
{
    comac_save (cr); {
	clip_to_quadrant (cr);
	hatching (cr);
	func (cr);
    } comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    comac_save (cr); {
	clip_to_quadrant (cr);
	comac_translate (cr, 0.25, 0.25);
	hatching (cr);
	func (cr);
    } comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    comac_save (cr); {
	clip_to_quadrant (cr);
	comac_translate (cr, WIDTH/2, HEIGHT/2);
	comac_rotate (cr, M_PI/4);
	comac_translate (cr, -WIDTH/2, -HEIGHT/2);
	hatching (cr);
	func (cr);
    } comac_restore (cr);

    comac_translate (cr, WIDTH, 0);
}

static void do_clip (comac_t *cr)
{
    comac_clip (cr);
    comac_paint (cr);
}

static void do_clip_alpha (comac_t *cr)
{
    comac_clip (cr);
    comac_paint_with_alpha (cr, .5);
}

static void hatchings (comac_t *cr, void (*func) (comac_t *))
{
    comac_save (cr); {
	comac_set_source_rgb(cr, 1, 0, 0);
	comac_set_antialias (cr, COMAC_ANTIALIAS_DEFAULT);
	draw_hatching (cr, func);
	comac_set_source_rgb(cr, 0, 0, 1);
	comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
	draw_hatching (cr, func);
    } comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    background (cr);


    /* aligned, misaligned, diagonal; mono repeat
     * x fill
     * x clip; paint
     * x clip; paint-alpha
     * repeated, for over/source
     */

    comac_set_operator (cr, COMAC_OPERATOR_OVER);

    hatchings (cr, comac_fill);
    comac_translate (cr, 0, HEIGHT);
    hatchings (cr, do_clip);
    comac_translate (cr, 0, HEIGHT);
    hatchings (cr, do_clip_alpha);
    comac_translate (cr, 0, HEIGHT);

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);

    hatchings (cr, comac_fill);
    comac_translate (cr, 0, HEIGHT);
    hatchings (cr, do_clip);
    comac_translate (cr, 0, HEIGHT);
    hatchings (cr, do_clip_alpha);
    comac_translate (cr, 0, HEIGHT);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (hatchings,
	    "Test drawing through various aligned/unaliged clips",
	    "clip, alpha", /* keywords */
	    "target=raster", /* requirements */
	    6*WIDTH, 6*HEIGHT,
	    NULL, draw)
