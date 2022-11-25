/*
 * Copyright © 2014 Google, Inc.
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
 */

#include "comac-test.h"

#include <assert.h>

static void
assert_point (comac_t *cr, double expected_x, double expected_y)
{
    double x, y;
    assert (comac_has_current_point (cr));
    comac_get_current_point (cr, &x, &y);
    assert (x == expected_x);
    assert (y == expected_y);
}

static void
assert_point_maintained (comac_t *cr, double expected_x, double expected_y)
{
    comac_path_t *path;

    assert_point (cr, expected_x, expected_y);

    path = comac_copy_path (cr);

    comac_new_path (cr);
    comac_rectangle (cr, 5, 5, 10, 20);
    comac_stroke (cr);

    comac_new_path (cr);
    comac_append_path (cr, path);
    comac_path_destroy (path);

    assert_point (cr, expected_x, expected_y);
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 20, 20);
    cr = comac_create (surface);

    comac_new_path (cr);
    comac_move_to (cr, 1., 2.);
    assert_point_maintained (cr, 1., 2.);

    comac_line_to (cr, 4., 5.);
    comac_move_to (cr, 2., 1.);
    assert_point_maintained (cr, 2., 1.);

    comac_move_to (cr, 5, 5);
    comac_arc (cr, 5, 5, 10, 0, M_PI / 3);
    comac_close_path (cr);
    assert_point_maintained (cr, 5, 5);

    comac_destroy (cr);
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (path_currentpoint,
	    "Test save/restore path maintains current point",
	    "api", /* keywords */
	    NULL,  /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
