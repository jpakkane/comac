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

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *mask;
    uint32_t data[] = {
	0x80000000,
	0x80000000,
	0x80000000,
	0x80000000,
    };

    mask = comac_image_surface_create_for_data ((unsigned char *) data,
						COMAC_FORMAT_ARGB32,
						2,
						2,
						8);

    comac_set_source_rgb (cr, 1.0, 0, 0);

    /* We can translate with the CTM, with the mask_surface offset, or
     * with both. */

    /* 1. CTM alone. */
    comac_save (cr);
    {
	comac_translate (cr, 2, 2);
	comac_mask_surface (cr, mask, 0, 0);
    }
    comac_restore (cr);

    /* 2. Offset alone. */
    comac_mask_surface (cr, mask, 4, 4);

    /* 3. CTM + offset */
    comac_translate (cr, 2, 2);
    comac_mask_surface (cr, mask, 4, 4);

    comac_surface_finish (mask); /* data goes out of scope */
    comac_surface_destroy (mask);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (mask_surface_ctm,
	    "Test that comac_mask_surface is affected properly by the CTM",
	    "mask", /* keywords */
	    NULL,   /* requirements */
	    10,
	    10,
	    NULL,
	    draw)
