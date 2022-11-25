/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright © 2005 Mozilla Corporation, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Mozilla Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Mozilla Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * MOZILLA CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MOZILLA CORPORATION BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Vladimir Vukicevic <vladimir@pobox.com>
 */

#include "comac-test.h"
#include <stdlib.h>

#define CHECK_SUCCESS do { \
    if (status) { \
	comac_pattern_destroy (pat); \
	return comac_test_status_from_status (ctx, status); \
    } \
} while (0)

static int
double_buf_equal (const comac_test_context_t *ctx, double *a, double *b, int nc)
{
    int i;
    for (i = 0; i < nc; i++) {
	if (!COMAC_TEST_DOUBLE_EQUALS(a[i],b[i])) {
	    comac_test_log (ctx, "Error: doubles not equal: %g, %g\n",
			    a[i], b[i]);
	    return 0;
	}
    }
    return 1;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_status_t status;
    comac_pattern_t *pat;

    /* Test pattern_get_rgba */
    {
	double r, g, b, a;
	pat = comac_pattern_create_rgba (0.2, 0.3, 0.4, 0.5);

	status = comac_pattern_get_rgba (pat, &r, &g, &b, &a);
	CHECK_SUCCESS;

	if (!COMAC_TEST_DOUBLE_EQUALS(r,0.2) ||
	    !COMAC_TEST_DOUBLE_EQUALS(g,0.3) ||
	    !COMAC_TEST_DOUBLE_EQUALS(b,0.4) ||
	    !COMAC_TEST_DOUBLE_EQUALS(a,0.5)) {
	    comac_test_log (ctx, "Error: comac_pattern_get_rgba returned unexpected results: %g, %g, %g, %g\n",
			    r, g, b, a);
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	comac_pattern_destroy (pat);
    }

    /* Test pattern_get_surface */
    {
	comac_surface_t *surf;

	pat = comac_pattern_create_for_surface (comac_get_target (cr));

	status = comac_pattern_get_surface (pat, &surf);
	CHECK_SUCCESS;

	if (surf != comac_get_target (cr)) {
	    comac_test_log (ctx, "Error: comac_pattern_get_resurface returned wrong surface\n");
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	comac_pattern_destroy (pat);
    }

    /* Test get_color_stops & linear_get_points */
    {
	int i;
	double x0, y0, x1, y1;
	double expected_values[15] = { 0.0, 0.2, 0.4, 0.2, 1.0,
				       0.5, 0.4, 0.5, 0.2, 0.5,
				       1.0, 0.2, 0.4, 0.5, 0.2 };
	double new_buf[15];

	pat = comac_pattern_create_linear (1.0, 2.0, 3.0, 4.0);

	for (i = 0; i < 3; i++) {
	    comac_pattern_add_color_stop_rgba (pat,
					       expected_values[i*5+0],
					       expected_values[i*5+1],
					       expected_values[i*5+2],
					       expected_values[i*5+3],
					       expected_values[i*5+4]);
	}

	status = comac_pattern_get_linear_points (pat, &x0, &y0, &x1, &y1);
	CHECK_SUCCESS;

	if (!COMAC_TEST_DOUBLE_EQUALS(x0,1.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(y0,2.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(x1,3.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(y1,4.0))
	{
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	status = comac_pattern_get_color_stop_count (pat, &i);
	CHECK_SUCCESS;

	if (i != 3) {
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	for (i = 0; i < 3; i++) {
	    status = comac_pattern_get_color_stop_rgba (pat, i,
							&new_buf[i*5+0],
							&new_buf[i*5+1],
							&new_buf[i*5+2],
							&new_buf[i*5+3],
							&new_buf[i*5+4]);
	    CHECK_SUCCESS;
	}

	status = comac_pattern_get_color_stop_rgba (pat, 5, NULL, NULL, NULL, NULL, NULL);
	if (status != COMAC_STATUS_INVALID_INDEX) {
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	if (!double_buf_equal (ctx, new_buf, expected_values,
			       ARRAY_LENGTH (expected_values)) != 0)
	{
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	comac_pattern_destroy (pat);
    }

    /* Test radial_get_circles */
    {
	double a, b, c, d, e, f;
	pat = comac_pattern_create_radial (1, 2, 3,
					   4, 5, 6);

	status = comac_pattern_get_radial_circles (pat, &a, &b, &c, &d, &e, &f);
	CHECK_SUCCESS;

	if (!COMAC_TEST_DOUBLE_EQUALS(a,1.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(b,2.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(c,3.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(d,4.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(e,5.0) ||
	    !COMAC_TEST_DOUBLE_EQUALS(f,6.0))
	{
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	comac_pattern_destroy (pat);
    }

    /* Test mesh getters */
    {
	unsigned int count;
	int i;
	pat = comac_pattern_create_mesh ();

	status = comac_mesh_pattern_get_patch_count (pat, &count);
	CHECK_SUCCESS;

	if (count != 0) {
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	comac_mesh_pattern_begin_patch (pat);
	comac_mesh_pattern_move_to (pat, 0, 0);
	comac_mesh_pattern_line_to (pat, 0, 3);
	comac_mesh_pattern_line_to (pat, 3, 3);
	comac_mesh_pattern_line_to (pat, 3, 0);

	status = comac_mesh_pattern_get_patch_count (pat, &count);
	CHECK_SUCCESS;

	if (count != 0) {
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	comac_mesh_pattern_end_patch (pat);

	status = comac_mesh_pattern_get_patch_count (pat, &count);
	CHECK_SUCCESS;

	if (count != 1) {
	    comac_pattern_destroy (pat);
	    return COMAC_TEST_FAILURE;
	}

	for (i = 0; i < 4; i++) {
	    double cp_x[4] = { 1, 1, 2, 2 };
	    double cp_y[4] = { 1, 2, 2, 1 };
	    double x, y;

	    status = comac_mesh_pattern_get_control_point (pat, 0, i, &x, &y);
	    CHECK_SUCCESS;

	    if (!COMAC_TEST_DOUBLE_EQUALS(x,cp_x[i]) ||
		!COMAC_TEST_DOUBLE_EQUALS(y,cp_y[i]))
	    {
		comac_pattern_destroy (pat);
		return COMAC_TEST_FAILURE;
	    }
	}

	comac_mesh_pattern_begin_patch (pat);
	comac_mesh_pattern_move_to (pat, 0, 0);
	comac_mesh_pattern_line_to (pat, 1, 0);
	comac_mesh_pattern_line_to (pat, 1, 1);
	comac_mesh_pattern_set_corner_color_rgb (pat, 0, 1, 1, 1);
	comac_mesh_pattern_end_patch (pat);

	for (i = 0; i < 4; i++) {
	    double corner_color[4] = { 1, 0, 0, 1 };
	    double a, r, g, b;

	    status = comac_mesh_pattern_get_corner_color_rgba (pat, 1, i,
							       &r, &g, &b, &a);
	    CHECK_SUCCESS;

	    if (!COMAC_TEST_DOUBLE_EQUALS(a,corner_color[i]) ||
		!COMAC_TEST_DOUBLE_EQUALS(r,corner_color[i]) ||
		!COMAC_TEST_DOUBLE_EQUALS(g,corner_color[i]) ||
		!COMAC_TEST_DOUBLE_EQUALS(b,corner_color[i]))
	    {
		comac_pattern_destroy (pat);
		return COMAC_TEST_FAILURE;
	    }
	}

	comac_pattern_destroy (pat);
    }

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (pattern_getters,
	    "Tests calls to pattern getter functions",
	    "pattern, api", /* keywords */
	    NULL, /* requirements */
	    1, 1,
	    NULL, draw)
