/*
 * Copyright © 2005 Red Hat, Inc.
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
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"
#include <stdlib.h>

static void
scale_by_two (double *x, double *y)
{
    *x = *x * 2.0;
    *y = *y * 2.0;
}

typedef void (*munge_func_t) (double *x, double *y);

static void
munge_and_set_path (comac_t *cr, comac_path_t *path, munge_func_t munge)
{
    int i;
    comac_path_data_t *p;
    double x1, y1, x2, y2, x3, y3;

    if (path->status) {
	comac_append_path (cr, path);
	return;
    }

    for (i = 0; i < path->num_data; i += path->data[i].header.length) {
	p = &path->data[i];
	switch (p->header.type) {
	case COMAC_PATH_MOVE_TO:
	    x1 = p[1].point.x;
	    y1 = p[1].point.y;
	    (munge) (&x1, &y1);
	    comac_move_to (cr, x1, y1);
	    break;
	case COMAC_PATH_LINE_TO:
	    x1 = p[1].point.x;
	    y1 = p[1].point.y;
	    (munge) (&x1, &y1);
	    comac_line_to (cr, x1, y1);
	    break;
	case COMAC_PATH_CURVE_TO:
	    x1 = p[1].point.x;
	    y1 = p[1].point.y;
	    x2 = p[2].point.x;
	    y2 = p[2].point.y;
	    x3 = p[3].point.x;
	    y3 = p[3].point.y;
	    (munge) (&x1, &y1);
	    (munge) (&x2, &y2);
	    (munge) (&x3, &y3);
	    comac_curve_to (cr, x1, y1, x2, y2, x3, y3);
	    break;
	case COMAC_PATH_CLOSE_PATH:
	    comac_close_path (cr);
	    break;
	}
    }
}

static void
make_path (comac_t *cr)
{
    comac_rectangle (cr, 0, 0, 5, 5);
    comac_move_to (cr, 15, 2.5);
    comac_arc (cr, 12.5, 2.5, 2.5, 0, 2 * M_PI);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_path_t *path;
    comac_t *cr_error;

    /* Ensure that calling comac_copy_path on an in-error comac_t will
     * propagate the error. */
    cr_error = comac_create (NULL);
    path = comac_copy_path (cr_error);
    if (path->status != COMAC_STATUS_NULL_POINTER) {
	comac_test_log (ctx,
			"Error: comac_copy_path returned status of %s rather "
			"than propagating %s\n",
			comac_status_to_string (path->status),
			comac_status_to_string (COMAC_STATUS_NULL_POINTER));
	comac_path_destroy (path);
	comac_destroy (cr_error);
	return COMAC_TEST_FAILURE;
    }
    comac_path_destroy (path);

    path = comac_copy_path_flat (cr_error);
    if (path->status != COMAC_STATUS_NULL_POINTER) {
	comac_test_log (ctx,
			"Error: comac_copy_path_flat returned status of %s "
			"rather than propagating %s\n",
			comac_status_to_string (path->status),
			comac_status_to_string (COMAC_STATUS_NULL_POINTER));
	comac_path_destroy (path);
	comac_destroy (cr_error);
	return COMAC_TEST_FAILURE;
    }
    comac_path_destroy (path);

    comac_destroy (cr_error);

    /* first check that we can copy an empty path */
    comac_new_path (cr);
    path = comac_copy_path (cr);
    if (path->status != COMAC_STATUS_SUCCESS) {
	comac_status_t status = path->status;
	comac_test_log (ctx,
			"Error: comac_copy_path returned status of %s\n",
			comac_status_to_string (status));
	comac_path_destroy (path);
	return comac_test_status_from_status (ctx, status);
    }
    if (path->num_data != 0) {
	comac_test_log (ctx,
			"Error: comac_copy_path did not copy an empty path, "
			"returned path contains %d elements\n",
			path->num_data);
	comac_path_destroy (path);
	return COMAC_TEST_FAILURE;
    }
    comac_append_path (cr, path);
    comac_path_destroy (path);
    if (comac_status (cr) != COMAC_STATUS_SUCCESS) {
	comac_test_log (ctx,
			"Error: comac_append_path failed with a copy of an "
			"empty path, returned status of %s\n",
			comac_status_to_string (comac_status (cr)));
	return comac_test_status_from_status (ctx, comac_status (cr));
    }

    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    /* copy path, munge, and fill */
    comac_translate (cr, 5, 5);
    make_path (cr);
    path = comac_copy_path (cr);

    comac_new_path (cr);
    munge_and_set_path (cr, path, scale_by_two);
    comac_path_destroy (path);
    comac_fill (cr);

    /* copy flattened path, munge, and fill */
    comac_translate (cr, 0, 15);
    make_path (cr);
    path = comac_copy_path_flat (cr);

    comac_new_path (cr);
    munge_and_set_path (cr, path, scale_by_two);
    comac_path_destroy (path);
    comac_fill (cr);

    /* append two copies of path, and fill */
    comac_translate (cr, 0, 15);
    comac_scale (cr, 2.0, 2.0);
    make_path (cr);
    path = comac_copy_path (cr);

    comac_new_path (cr);
    comac_append_path (cr, path);
    comac_translate (cr, 2.5, 2.5);
    comac_append_path (cr, path);

    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_fill (cr);

    comac_path_destroy (path);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_t *cr;
    comac_path_data_t data;
    comac_path_t path;
    comac_surface_t *surface;
    comac_status_t status;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1);
    status = comac_surface_status (surface);
    if (status) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    /* Test a few error cases for comac_append_path_data */
#define COMAC_CREATE()                                                         \
    do {                                                                       \
	cr = comac_create (surface);                                           \
	status = comac_status (cr);                                            \
	if (status) {                                                          \
	    comac_destroy (cr);                                                \
	    comac_surface_destroy (surface);                                   \
	    return comac_test_status_from_status (ctx, status);                \
	}                                                                      \
    } while (0)
    COMAC_CREATE ();
    comac_append_path (cr, NULL);
    status = comac_status (cr);
    comac_destroy (cr);
    if (status != COMAC_STATUS_NULL_POINTER) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    COMAC_CREATE ();
    path.status = -1;
    comac_append_path (cr, &path);
    status = comac_status (cr);
    comac_destroy (cr);
    if (status != COMAC_STATUS_INVALID_STATUS) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    COMAC_CREATE ();
    path.status = COMAC_STATUS_NO_MEMORY;
    comac_append_path (cr, &path);
    status = comac_status (cr);
    comac_destroy (cr);
    if (status != COMAC_STATUS_NO_MEMORY) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    COMAC_CREATE ();
    path.data = NULL;
    path.num_data = 0;
    path.status = COMAC_STATUS_SUCCESS;
    comac_append_path (cr, &path);
    status = comac_status (cr);
    comac_destroy (cr);
    if (status != COMAC_STATUS_SUCCESS) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    COMAC_CREATE ();
    path.data = NULL;
    path.num_data = 1;
    path.status = COMAC_STATUS_SUCCESS;
    comac_append_path (cr, &path);
    status = comac_status (cr);
    comac_destroy (cr);
    if (status != COMAC_STATUS_NULL_POINTER) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    COMAC_CREATE ();
    /* Intentionally insert bogus header.length value (otherwise would be 2) */
    data.header.type = COMAC_PATH_MOVE_TO;
    data.header.length = 1;
    path.data = &data;
    path.num_data = 1;
    comac_append_path (cr, &path);
    status = comac_status (cr);
    comac_destroy (cr);
    if (status != COMAC_STATUS_INVALID_PATH_DATA) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    /* And test the degenerate case */
    COMAC_CREATE ();
    path.num_data = 0;
    comac_append_path (cr, &path);
    status = comac_status (cr);
    comac_destroy (cr);
    if (status != COMAC_STATUS_SUCCESS) {
	comac_surface_destroy (surface);
	return comac_test_status_from_status (ctx, status);
    }

    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (copy_path,
	    "Tests calls to path_data functions: comac_copy_path, "
	    "comac_copy_path_flat, and comac_append_path",
	    "path", /* keywords */
	    NULL,   /* requirements */
	    45,
	    53,
	    preamble,
	    draw)
