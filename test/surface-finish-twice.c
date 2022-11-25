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
 * Author: Carl Worth <cworth@cworth.org>
 */

/* Bug history
 *
 * 2005-04-10 stevech1097@yahoo.com.au
 *
 *   Subject: [Bug 2950]  New: *** glibc detected *** double free or corruption
 *   URL: https://bugs.freedesktop.org/show_bug.cgi?id=2950
 *
 *   The following short program gives the error message:
 *
 *     *** glibc detected *** double free or corruption: 0x082a7268 ***
 *     Aborted
 *
 * 2005-04-13 Carl Worth <cworth@cworth.org>
 *
 *   Looks like surface->finished was never being set. Now fixed.
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *surface;
    comac_status_t status;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1);

    comac_surface_finish (surface);
    status = comac_surface_status (surface);
    if (status != COMAC_STATUS_SUCCESS)
	return comac_test_status_from_status (ctx, status);

    comac_surface_finish (surface);
    status = comac_surface_status (surface);
    if (status != COMAC_STATUS_SUCCESS)
	return comac_test_status_from_status (ctx, status);

    comac_surface_finish (surface);
    status = comac_surface_status (surface);
    if (status != COMAC_STATUS_SUCCESS)
	return comac_test_status_from_status (ctx, status);

    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (surface_finish_twice,
	    "Test to exercise a crash when calling comac_surface_finish twice on the same surface.",
	    "api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    NULL, draw)
