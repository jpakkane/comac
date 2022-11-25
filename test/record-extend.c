/*
 * Copyright © 2007 Red Hat, Inc.
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
 * Authors:
 *	Behdad Esfahbod <behdad@behdad.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define SIZE 90

/* This is written using clip+paint to exercise a bug that once was in the
 * recording surface.
 */

static comac_surface_t *
source (comac_surface_t *surface)
{
    comac_t *cr;

    /* Create a 4-pixel image surface with my favorite four colors in each
     * quadrant. */
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    /* upper-left = white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_rectangle (cr, 0, 0, 1, 1);
    comac_fill (cr);

    /* upper-right = red */
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_rectangle (cr, 1, 0, 1, 1);
    comac_fill (cr);

    /* lower-left = green */
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_rectangle (cr, 0, 1, 1, 1);
    comac_fill (cr);

    /* lower-right = blue */
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_rectangle (cr, 1, 1, 1, 1);
    comac_fill (cr);

    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return surface;
}

static comac_surface_t *
image (comac_t *cr)
{
    return source (comac_image_surface_create (COMAC_FORMAT_RGB24, 2, 2));
}

static comac_surface_t *
similar (comac_t *cr)
{
    return source (comac_surface_create_similar (comac_get_target (cr),
						 COMAC_CONTENT_COLOR, 2, 2));
}

static comac_t *
extend (comac_t *cr, comac_surface_t *(*surface)(comac_t *), comac_extend_t mode)
{
    comac_surface_t *s;

    comac_set_source_rgb (cr, 0, 1, 1);
    comac_paint (cr);

    /* Now use extend modes to cover most of the surface with those 4 colors */
    s = surface (cr);
    comac_set_source_surface (cr, s, SIZE/2 - 1, SIZE/2 - 1);
    comac_surface_destroy (s);

    comac_pattern_set_extend (comac_get_source (cr), mode);

    comac_rectangle (cr, 10, 10, SIZE-20, SIZE-20);
    comac_clip (cr);
    comac_paint (cr);

    return cr;
}

static comac_t *
extend_none (comac_t *cr,
	     comac_surface_t *(*pattern)(comac_t *))
{
    return extend (cr, pattern, COMAC_EXTEND_NONE);
}

static comac_t *
extend_pad (comac_t *cr,
	    comac_surface_t *(*pattern)(comac_t *))
{
    return extend (cr, pattern, COMAC_EXTEND_PAD);
}

static comac_t *
extend_repeat (comac_t *cr,
	       comac_surface_t *(*pattern)(comac_t *))
{
    return extend (cr, pattern, COMAC_EXTEND_REPEAT);
}

static comac_t *
extend_reflect (comac_t *cr,
	       comac_surface_t *(*pattern)(comac_t *))
{
    return extend (cr, pattern, COMAC_EXTEND_REFLECT);
}

static comac_t *
record_create (comac_t *target)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_recording_surface_create (comac_surface_get_content (comac_get_target (target)), NULL);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    return cr;
}

static comac_surface_t *
record_get (comac_t *target)
{
    comac_surface_t *surface;

    surface = comac_surface_reference (comac_get_target (target));
    comac_destroy (target);

    return surface;
}

static comac_test_status_t
record_replay (comac_t *cr,
	       comac_t *(*func)(comac_t *,
				comac_surface_t *(*pattern)(comac_t *)),
	       comac_surface_t *(*pattern)(comac_t *),
	       int width, int height)
{
    comac_surface_t *surface;
    int x, y;

    surface = record_get (func (record_create (cr), pattern));

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_surface (cr, surface, 0, 0);
    comac_surface_destroy (surface);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_NONE);

    for (y = 0; y < height; y += 2) {
	for (x = 0; x < width; x += 2) {
	    comac_rectangle (cr, x, y, 2, 2);
	    comac_clip (cr);
	    comac_paint (cr);
	    comac_reset_clip (cr);
	}
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
record_extend_none (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_none, image, width, height);
}

static comac_test_status_t
record_extend_pad (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_pad, image, width, height);
}

static comac_test_status_t
record_extend_repeat (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_repeat, image, width, height);
}

static comac_test_status_t
record_extend_reflect (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_reflect, image, width, height);
}

static comac_test_status_t
record_extend_none_similar (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_none, similar, width, height);
}

static comac_test_status_t
record_extend_pad_similar (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_pad, similar, width, height);
}

static comac_test_status_t
record_extend_repeat_similar (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_repeat, similar, width, height);
}

static comac_test_status_t
record_extend_reflect_similar (comac_t *cr, int width, int height)
{
    return record_replay (cr, extend_reflect, similar, width, height);
}

COMAC_TEST (record_extend_none,
	    "Test COMAC_EXTEND_NONE for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_none)
COMAC_TEST (record_extend_pad,
	    "Test COMAC_EXTEND_PAD for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_pad)
COMAC_TEST (record_extend_repeat,
	    "Test COMAC_EXTEND_REPEAT for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_repeat)
COMAC_TEST (record_extend_reflect,
	    "Test COMAC_EXTEND_REFLECT for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_reflect)

COMAC_TEST (record_extend_none_similar,
	    "Test COMAC_EXTEND_NONE for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_none_similar)
COMAC_TEST (record_extend_pad_similar,
	    "Test COMAC_EXTEND_PAD for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_pad_similar)
COMAC_TEST (record_extend_repeat_similar,
	    "Test COMAC_EXTEND_REPEAT for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_repeat_similar)
COMAC_TEST (record_extend_reflect_similar,
	    "Test COMAC_EXTEND_REFLECT for recorded surface patterns",
	    "record, extend", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, record_extend_reflect_similar)
