/*
 * Copyright © 2017 Uli Schlachter
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
 * Author: Uli Schlachter <psychon@znc.in>
 */

#include "comac-test.h"

#include <stdlib.h>

static const unsigned char broken_png_data[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x08, 0x02, 0x00, 0x00, 0x00, 0xfd, 0xd4, 0x9a, 0xff, 0x00, 0x00, 0x00,
};
static const size_t broken_png_data_length =
    sizeof (broken_png_data) / sizeof (broken_png_data[0]);

static comac_status_t
read_png_from_data (void *closure, unsigned char *data, unsigned int length)
{
    size_t *offset = closure;
    size_t remaining = broken_png_data_length - *offset;

    if (remaining < length)
	return COMAC_STATUS_READ_ERROR;

    memcpy (data, &broken_png_data[*offset], length);
    *offset += length;

    return COMAC_STATUS_SUCCESS;
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface;
    comac_status_t status;
    comac_status_t expected;
    size_t offset = 0;

    surface = comac_image_surface_create_from_png_stream (read_png_from_data,
							  &offset);

    /* XXX: The actual error is COMAC_STATUS_PNG_ERROR, but
     * _comac_surface_create_in_error() does not support that.
     */
    expected = COMAC_STATUS_NO_MEMORY;
    status = comac_surface_status (surface);
    comac_surface_destroy (surface);
    if (status != COMAC_STATUS_NO_MEMORY) {
	comac_test_log (ctx,
			"Error: expected error %s, but got %s\n",
			comac_status_to_string (expected),
			comac_status_to_string (status));

	return COMAC_TEST_FAILURE;
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (create_from_broken_png_stream,
	    "Tests the creation of a PNG from malformed data",
	    "png", /* keywords */
	    NULL,  /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
