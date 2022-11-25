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

/* Check comac_recording_surface_ink_extents() returns correct extents. */


static comac_test_status_t
check_extents (comac_test_context_t *cr,
	       comac_surface_t *recording_surface,
	       const char * func_name,
	       double expected_x, double expected_y, double expected_w, double expected_h)
{
    double x, y, w, h;
    comac_recording_surface_ink_extents (recording_surface, &x, &y, &w, &h);
    if (x != expected_x ||
	y != expected_y ||
	w != expected_w ||
	h != expected_h)
    {
	comac_test_log (cr,
			"%s: x: %f, y: %f, w: %f, h: %f\n"
			"    expected: x: %f, y: %f, w: %f, h: %f\n",
			func_name,
			x, y, w, h,
			expected_x, expected_y,
			expected_w, expected_h);
       return COMAC_TEST_ERROR;
    }
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
unbounded_fill (comac_test_context_t *test_cr)
{
    comac_test_status_t status;
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, NULL);
    cr = comac_create (surface);

    comac_rectangle (cr, -300, -150, 900, 600);
    comac_fill (cr);

    comac_destroy(cr);

    status = check_extents (test_cr, surface,  __func__,
			    -300, -150, 900, 600);
    comac_surface_destroy (surface);
    return status;
}

static comac_test_status_t
bounded_fill (comac_test_context_t *test_cr)
{
    comac_test_status_t status;
    comac_surface_t *surface;
    comac_t *cr;
    comac_rectangle_t extents = { -150, -100, 300, 200 };

    surface = comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, &extents);
    cr = comac_create (surface);

    comac_rectangle (cr, -300, -300, 650, 600);
    comac_fill (cr);

    comac_destroy(cr);

    status = check_extents (test_cr, surface,  __func__,
			    -150, -100, 300, 200);
    comac_surface_destroy (surface);
    return status;
}

static comac_test_status_t
unbounded_paint (comac_test_context_t *test_cr)
{
    comac_test_status_t status;
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, NULL);
    cr = comac_create (surface);

    comac_paint (cr);

    comac_destroy(cr);

    status = check_extents (test_cr, surface,  __func__,
			    -(1 << 23), -(1 << 23), -1, -1);
    comac_surface_destroy (surface);
    return status;
}

static comac_test_status_t
bounded_paint (comac_test_context_t *test_cr)
{
    comac_test_status_t status;
    comac_surface_t *surface;
    comac_t *cr;
    comac_rectangle_t extents = { -150, -100, 300, 200 };

    surface = comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, &extents);
    cr = comac_create (surface);

    comac_paint (cr);

    comac_destroy(cr);

    status = check_extents (test_cr, surface,  __func__,
			    -150, -100, 300, 200);
    comac_surface_destroy (surface);
    return status;
}

static comac_test_status_t
preamble (comac_test_context_t *cr)
{
    comac_test_status_t status;

    status = unbounded_fill (cr);
    if (status != COMAC_TEST_SUCCESS)
	return status;

    status = bounded_fill (cr);
    if (status != COMAC_TEST_SUCCESS)
	return status;

    status = unbounded_paint (cr);
    if (status != COMAC_TEST_SUCCESS)
	return status;

    status = bounded_paint (cr);
    if (status != COMAC_TEST_SUCCESS)
	return status;

    return COMAC_TEST_SUCCESS;
}


COMAC_TEST (recording_ink_extents,
	    "Test comac_recording_surface_ink_extents()",
	    "api,recording,extents", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
