/*
 * Copyright © Jeff Muizelaar
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
 * JEFF MUIZELAAR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Jeff Muizelaar <jeff@infidigm.net>
 *	    Carl Worth <cworth@cworth.org>
 *	    Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define MASK_WIDTH 10
#define MASK_HEIGHT 8

#ifdef WORDS_BIGENDIAN
#define MASK 0x28, 0x55
#else
#define MASK 0x14, 0xAA
#endif
static unsigned char mask[(MASK_WIDTH + 7) / 8 * MASK_HEIGHT] = {
    MASK,
    MASK,
    MASK,
    MASK,
    MASK,
    MASK,
    MASK,
    MASK,
};

static comac_test_status_t
check_status (const comac_test_context_t *ctx,
	      comac_status_t status,
	      comac_status_t expected)
{
    if (status == expected)
	return COMAC_TEST_SUCCESS;

    comac_test_log (ctx,
		    "Error: Expected status value %d (%s), received %d (%s)\n",
		    expected,
		    comac_status_to_string (expected),
		    status,
		    comac_status_to_string (status));
    return COMAC_TEST_FAILURE;
}

static comac_test_status_t
test_surface_with_width_and_stride (const comac_test_context_t *ctx,
				    int width, int stride,
				    comac_status_t expected)
{
    comac_test_status_t status;
    comac_surface_t *surface;
    comac_t *cr;
    int len;
    unsigned char *data;

    comac_test_log (ctx,
		    "Creating surface with width %d and stride %d\n",
		    width, stride);

    len = stride;
    if (len < 0)
	len = -len;
    data = xmalloc (len);

    surface = comac_image_surface_create_for_data (data, COMAC_FORMAT_A1,
						   width, 1, stride);
    cr = comac_create (surface);

    comac_paint (cr);

    status = check_status (ctx, comac_surface_status (surface), expected);
    if (status)
	goto BAIL;

    status = check_status (ctx, comac_status (cr), expected);
    if (status)
	goto BAIL;

  BAIL:
    comac_destroy (cr);
    comac_surface_destroy (surface);
    free (data);
    return status;
}

static comac_test_status_t
draw (comac_t *cr, int dst_width, int dst_height)
{
    unsigned char *mask_aligned;
    comac_surface_t *surface;

    surface = comac_image_surface_create (COMAC_FORMAT_A1,
					  MASK_WIDTH,
					  MASK_HEIGHT);

    mask_aligned = comac_image_surface_get_data (surface);
    if (mask_aligned != NULL) {
	int stride = comac_image_surface_get_stride (surface), row;
	const unsigned char *src = mask;
	unsigned char *dst = mask_aligned;
	for (row = 0; row < MASK_HEIGHT; row++) {
	    memcpy (dst, src, (MASK_WIDTH + 7) / 8);
	    src += (MASK_WIDTH + 7) / 8;
	    dst += stride;
	}
    }
    comac_surface_mark_dirty (surface);

    /* Paint background blue */
    comac_set_source_rgb (cr, 0, 0, 1); /* blue */
    comac_paint (cr);

    /* Then paint red through our mask */
    comac_set_source_rgb (cr, 1, 0, 0); /* red */
    comac_mask_surface (cr, surface, 0, 0);
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_test_status_t status = COMAC_TEST_SUCCESS;
    int test_width;

    /* first check the API strictness */
    for (test_width = 0; test_width < 40; test_width++) {
	int test_stride = (test_width + 7) / 8;
	int stride = comac_format_stride_for_width (COMAC_FORMAT_A1,
						    test_width);
	comac_status_t expected;

	/* First create a surface using the width as the stride,
	 * (most of these should fail).
	 */
	expected = (stride == test_stride) ?
	    COMAC_STATUS_SUCCESS : COMAC_STATUS_INVALID_STRIDE;

	status = test_surface_with_width_and_stride (ctx,
						     test_width,
						     test_stride,
						     expected);
	if (status)
	    return status;

	status = test_surface_with_width_and_stride (ctx,
						     test_width,
						     -test_stride,
						     expected);
	if (status)
	    return status;


	/* Then create a surface using the correct stride,
	 * (should always succeed).
	 */
	status = test_surface_with_width_and_stride (ctx,
						     test_width,
						     stride,
						     COMAC_STATUS_SUCCESS);
	if (status)
	    return status;

	status = test_surface_with_width_and_stride (ctx,
						     test_width,
						     -stride,
						     COMAC_STATUS_SUCCESS);
	if (status)
	    return status;
    }

    return status;
}

COMAC_TEST (a1_mask,
            "test masks of COMAC_FORMAT_A1",
	    "alpha, mask", /* keywords */
	    NULL, /* requirements */
	    MASK_WIDTH, MASK_HEIGHT,
	    preamble, draw)
