/*
 * Copyright © 2005, 2007 Red Hat, Inc.
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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define SIZE 20
#define PAD 2

static comac_pattern_t *
create_image_source (int size)
{
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    comac_t *cr;

    /* Create an image surface with my favorite four colors in each
     * quadrant. */
    surface = comac_image_surface_create (COMAC_FORMAT_RGB24, size, size);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_rectangle (cr, 0, 0, size / 2, size / 2);
    comac_fill (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_rectangle (cr, size / 2, 0, size - size / 2, size / 2);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_rectangle (cr, 0, size / 2, size / 2, size - size / 2);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 0, 1);
    comac_rectangle (cr, size / 2, size / 2, size - size / 2, size - size / 2);
    comac_fill (cr);

    pattern = comac_pattern_create_for_surface (comac_get_target (cr));
    comac_destroy (cr);

    return pattern;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *source;
    int surface_size = sqrt ((SIZE - 2*PAD)*(SIZE - 2*PAD)/2);

    /* Use a gray (neutral) background, so we can spot if the backend pads
     * with any other colour.
     */
    comac_set_source_rgb (cr, .5, .5, .5);
    comac_paint (cr);

    comac_translate(cr, SIZE/2, SIZE/2);
    comac_rotate (cr, M_PI / 4.0);
    comac_translate (cr, -surface_size/2, -surface_size/2);

    source = create_image_source (surface_size);
    comac_pattern_set_filter (source, COMAC_FILTER_NEAREST);
    comac_set_source(cr, source);
    comac_pattern_destroy (source);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
clip_draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *source;
    int surface_size = sqrt ((SIZE - 2*PAD)*(SIZE - 2*PAD)/2);

    /* Use a gray (neutral) background, so we can spot if the backend pads
     * with any other colour.
     */
    comac_set_source_rgb (cr, .5, .5, .5);
    comac_paint (cr);

    comac_rectangle (cr, 2*PAD, 2*PAD, SIZE-4*PAD, SIZE-4*PAD);
    comac_clip (cr);

    comac_translate(cr, SIZE/2, SIZE/2);
    comac_rotate (cr, M_PI / 4.0);
    comac_translate (cr, -surface_size/2, -surface_size/2);

    source = create_image_source (surface_size);
    comac_pattern_set_filter (source, COMAC_FILTER_NEAREST);
    comac_set_source(cr, source);
    comac_pattern_destroy (source);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_clip (comac_t *cr, int width, int height)
{
    comac_pattern_t *source;
    int surface_size = sqrt ((SIZE - 2*PAD)*(SIZE - 2*PAD)/2);

    /* Use a gray (neutral) background, so we can spot if the backend pads
     * with any other colour.
     */
    comac_set_source_rgb (cr, .5, .5, .5);
    comac_paint (cr);

    comac_translate(cr, SIZE/2, SIZE/2);
    comac_rotate (cr, M_PI / 4.0);
    comac_translate (cr, -surface_size/2, -surface_size/2);

    comac_rectangle (cr, PAD, PAD, surface_size-2*PAD, surface_size-2*PAD);
    comac_clip (cr);

    source = create_image_source (surface_size);
    comac_pattern_set_filter (source, COMAC_FILTER_NEAREST);
    comac_set_source(cr, source);
    comac_pattern_destroy (source);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (rotate_image_surface_paint,
	    "Test call sequence: image_surface_create; rotate; set_source_surface; paint"
	    "\nThis test is known to fail on the ps backend currently",
	    "image, transform, paint", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)

COMAC_TEST (clip_rotate_image_surface_paint,
	    "Test call sequence: image_surface_create; rotate; set_source_surface; paint"
	    "\nThis test is known to fail on the ps backend currently",
	    "image, transform, paint", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, clip_draw)
COMAC_TEST (rotate_clip_image_surface_paint,
	    "Test call sequence: image_surface_create; rotate; set_source_surface; paint"
	    "\nThis test is known to fail on the ps backend currently",
	    "image, transform, paint", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw_clip)
