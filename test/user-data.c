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
 * Author: Kristian Høgsberg <krh@redhat.com>
 */

#include "comac-test.h"

#include <assert.h>

static void
destroy_data1 (void *p)
{
    *(int *) p = 1;
}

static void
destroy_data2 (void *p)
{
    *(int *) p = 2;
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    static const comac_user_data_key_t key1, key2;
    comac_surface_t *surface;
    comac_status_t status;
    int data1, data2;

    data1 = 0;
    data2 = 0;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1);
    status =
	comac_surface_set_user_data (surface, &key1, &data1, destroy_data1);
    if (status)
	goto error;

    status =
	comac_surface_set_user_data (surface, &key2, &data2, destroy_data2);
    if (status)
	goto error;

    assert (comac_surface_get_user_data (surface, &key1) == &data1);
    status = comac_surface_set_user_data (surface, &key1, NULL, NULL);
    if (status)
	goto error;

    assert (comac_surface_get_user_data (surface, &key1) == NULL);
    assert (data1 == 1);
    assert (data2 == 0);

    status = comac_surface_set_user_data (surface, &key2, NULL, NULL);
    if (status)
	goto error;

    assert (data2 == 2);

    data1 = 0;
    status = comac_surface_set_user_data (surface, &key1, &data1, NULL);
    if (status)
	goto error;

    status = comac_surface_set_user_data (surface, &key1, NULL, NULL);
    if (status)
	goto error;

    assert (data1 == 0);
    assert (comac_surface_get_user_data (surface, &key1) == NULL);

    status =
	comac_surface_set_user_data (surface, &key1, &data1, destroy_data1);
    if (status)
	goto error;

    comac_surface_destroy (surface);

    assert (data1 == 1);
    assert (data2 == 2);

    return COMAC_TEST_SUCCESS;

error:
    comac_surface_destroy (surface);
    return comac_test_status_from_status (ctx, status);
}

COMAC_TEST (user_data,
	    "Test setting and getting random bits of user data.",
	    "api", /* keywords */
	    NULL,  /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
