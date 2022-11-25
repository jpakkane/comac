/*
 * Copyright © 2016 Adrian Johnson
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
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"
#include <stdio.h>
#include <math.h>

#define PAT_SIZE  64
#define PAD (PAT_SIZE/8)
#define WIDTH (PAT_SIZE*4 + PAD*5)
#define HEIGHT (PAT_SIZE + PAD*2)

/* Test case based on bug 89232 - painting a recording surface to a pdf/ps surface
 * omits objects on the recording surface with negative coordinates even though
 * the pattern matrix has transformed the objects to within the page extents.
 * The bug is a result of pdf/ps assuming the surface extents are always
 * (0,0) to (page_width, page_height).
 *
 * Each test has four cases of painting a recording pattern where:
 * 1) recording surface origin is transformed to the center of the pattern
 * 2) same as 1) but also scaled up 10x
 * 3) same as 1) but also scaled down 10x
 * 4) same as 1) but also rotated 45 deg
 */


static void
transform_extents(comac_rectangle_t *extents, comac_matrix_t *mat)
{
    double x1, y1, x2, y2, x, y;

#define UPDATE_BBOX \
    x1 = x < x1 ? x : x1; \
    y1 = y < y1 ? y : y1; \
    x2 = x > x2 ? x : x2; \
    y2 = y > y2 ? y : y2;

    x = extents->x;
    y = extents->y;
    comac_matrix_transform_point (mat, &x, &y);
    x1 = x2 = x;
    y1 = y2 = y;

    x = extents->x + extents->width;
    y = extents->y;
    comac_matrix_transform_point (mat, &x, &y);
    UPDATE_BBOX;

    x = extents->x;
    y = extents->y + extents->height;
    comac_matrix_transform_point (mat, &x, &y);
    UPDATE_BBOX;

    x = extents->x + extents->width;
    y = extents->y + extents->height;
    comac_matrix_transform_point (mat, &x, &y);
    UPDATE_BBOX;

    extents->x = x1;
    extents->y = y1;
    extents->width = x2 - extents->x;
    extents->height = y2 - extents->y;

#undef UPDATE_BBOX
}

static comac_pattern_t *
create_pattern (comac_matrix_t *mat, comac_bool_t bounded)
{
    comac_surface_t *surf;
    comac_pattern_t *pat;
    comac_t *cr;
    int border;
    int square;

    if (bounded) {
	comac_rectangle_t extents = { 0, 0, PAT_SIZE, PAT_SIZE };
	transform_extents (&extents, mat);
	surf = comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, &extents);
    } else {
	surf = comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, NULL);
    }

    cr = comac_create (surf);
    comac_transform (cr, mat);

    border  = PAT_SIZE/8;
    square = (PAT_SIZE - 2*border)/2;

    comac_rectangle (cr, 0, 0, PAT_SIZE, PAT_SIZE);
    comac_clip (cr);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    comac_translate (cr, border, border);
    comac_rectangle (cr, 0, 0, square, square);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_fill (cr);

    comac_translate (cr, square, 0);
    comac_rectangle (cr, 0, 0, square, square);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_fill (cr);

    comac_translate (cr, 0, square);
    comac_rectangle (cr, 0, 0, square, square);
    comac_set_source_rgb (cr, 0, 1, 1);
    comac_fill (cr);

    comac_translate (cr, -square, 0);
    comac_rectangle (cr, 0, 0, square, square);
    comac_set_source_rgb (cr, 1, 1, 0);
    comac_fill (cr);

    comac_destroy (cr);

    pat = comac_pattern_create_for_surface (surf);
    comac_surface_destroy (surf);
    comac_pattern_set_matrix (pat, mat);

    return pat;
}

static comac_test_status_t
record_extents (comac_t *cr, int width, int height, comac_bool_t bounded)
{
    comac_pattern_t *pat;
    comac_matrix_t mat;

    /* record surface extents (-PAT_SIZE/2, -PAT_SIZE/2) to (PAT_SIZE/2, PAT_SIZE/2) */
    comac_translate (cr, PAD, PAD);
    comac_matrix_init_translate (&mat, -PAT_SIZE/2, -PAT_SIZE/2);
    pat = create_pattern (&mat, bounded);
    comac_set_source (cr, pat);
    comac_pattern_destroy (pat);
    comac_paint (cr);

    /* record surface extents (-10*PAT_SIZE/2, -10*PAT_SIZE/2) to (10*PAT_SIZE/2, 10*PAT_SIZE/2) */
    comac_translate (cr, PAT_SIZE + PAD, 0);
    comac_matrix_init_translate (&mat, -10.0*PAT_SIZE/2, -10.0*PAT_SIZE/2);
    comac_matrix_scale (&mat, 10, 10);
    pat = create_pattern (&mat, bounded);
    comac_set_source (cr, pat);
    comac_pattern_destroy (pat);
    comac_paint (cr);

    /* record surface extents (-0.1*PAT_SIZE/2, -0.1*PAT_SIZE/2) to (0.1*PAT_SIZE/2, 0.1*PAT_SIZE/2) */
    comac_translate (cr, PAT_SIZE + PAD, 0);
    comac_matrix_init_translate (&mat, -0.1*PAT_SIZE/2, -0.1*PAT_SIZE/2);
    comac_matrix_scale (&mat, 0.1, 0.1);
    pat = create_pattern (&mat, bounded);
    comac_set_source (cr, pat);
    comac_pattern_destroy (pat);
    comac_paint (cr);

    /* record surface centered on (0,0) and rotated 45 deg */
    comac_translate (cr, PAT_SIZE + PAD, 0);
    comac_matrix_init_translate (&mat, -PAT_SIZE/sqrt(2), -PAT_SIZE/sqrt(2));
    comac_matrix_rotate (&mat, M_PI/4.0);
    comac_matrix_translate (&mat, PAT_SIZE/2, -PAT_SIZE/2);
    pat = create_pattern (&mat, bounded);
    comac_set_source (cr, pat);
    comac_pattern_destroy (pat);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
record_neg_extents_bounded (comac_t *cr, int width, int height)
{
    return record_extents(cr, width, height, TRUE);
}

static comac_test_status_t
record_neg_extents_unbounded (comac_t *cr, int width, int height)
{
    return record_extents(cr, width, height, FALSE);
}


COMAC_TEST (record_neg_extents_unbounded,
	    "Paint unbounded recording pattern with untransformed extents outside of target extents",
	    "record,transform,pattern", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, record_neg_extents_unbounded)
COMAC_TEST (record_neg_extents_bounded,
	    "Paint bounded recording pattern with untransformed extents outside of target extents",
	    "record,transform,pattern", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, record_neg_extents_bounded)
