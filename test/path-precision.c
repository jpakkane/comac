/*
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 *
 * Based on an example by Dirk "krit" Schulze found during WebKit integration.
 */
 
 /*  Copyright (C) 2021 Heiko Lewin <hlewin@gmx.de> 
  *  Added error margin for point comparisons */

#include "comac-test.h"
#include "comac-fixed-type-private.h"

/* we know that this is an inherent limitation in comac */
#define FAIL COMAC_TEST_XFAILURE

/* Test the idempotency of path construction and copying */


/* The error to be expected from double<->fixed conversions */
#define EXPECTED_ERROR (0.5 / (1<<COMAC_FIXED_FRAC_BITS))

static int compare_points( double *p1, double *p2 ) {
    for(int i=0; i<2; ++i) {
	double error = fabs(p2[i]-p1[i]);
	if(error > EXPECTED_ERROR) {
	    return 1;
	}
    }
    return 0;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_path_data_t path_data[] = {
	{ { COMAC_PATH_MOVE_TO, 2 }, },
	{ .point={ 95.000000, 40.000000 }, },

	{ { COMAC_PATH_LINE_TO, 2 }, },
	{ .point={ 94.960533, 41.255810 }, },

	{ { COMAC_PATH_LINE_TO, 2 }, },
	{ .point={ 94.842293, 42.50666 }, },

	{ { COMAC_PATH_LINE_TO, 2 }, },
	{ .point={ 94.645744, 43.747627 }, },

	{ { COMAC_PATH_LINE_TO, 2 }, },
	{ .point={ 94.371666, 44.973797 }, },
    };
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_path_t path, *path_copy;
    int i, j, n;
    comac_test_status_t result = COMAC_TEST_SUCCESS;

    path.status = COMAC_STATUS_SUCCESS;
    path.num_data = ARRAY_LENGTH (path_data);
    path.data = path_data;

    comac_new_path (cr);
    comac_append_path (cr, &path);
    path_copy = comac_copy_path (cr);

    if (path_copy->status)
	return comac_test_status_from_status (ctx, path_copy->status);

    for (i = j = n = 0;
	 i < path.num_data && j < path_copy->num_data;
	 i += path.data[i].header.length,
	 j += path_copy->data[j].header.length,
	 n++)
    {
	const comac_path_data_t *src, *dst;

	src = &path.data[i];
	dst = &path_copy->data[j];

	if (src->header.type != dst->header.type) {
	    comac_test_log (ctx,
			    "Paths differ in header type after %d operations.\n"
			    "Expected path operation %d, found %d.\n",
			    n, src->header.type, dst->header.type);
	    result = FAIL;
	    break;
	}

	if (compare_points ((double*)&src[1].point, (double*)&dst[1].point)) {
	    comac_test_log (ctx,
			    "Paths differ in coordinates after %d operations.\n"
			    "Expected point (%f, %f), found (%f, %f).\n",
			    n,
			    src[1].point.x, src[1].point.y,
			    dst[1].point.x, dst[1].point.y);
	    result = FAIL;
	    break;
	}
    }

    comac_path_destroy (path_copy);
    return result;
}

COMAC_TEST (path_precision,
	    "Check that the path append/copy is idempotent.",
	    "api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    NULL, draw)
