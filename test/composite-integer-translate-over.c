/*
 * Copyright © 2006 Mozilla Corporation
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
#include <math.h>
#include <stdio.h>

#define SIZE 100
#define OFFSET 10
static const char *png_filename = "romedalen.png";

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *image;

    image = comac_test_create_surface_from_png (ctx, png_filename);

    comac_set_source_rgba (cr, 0, 0, 0, 1);
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_fill (cr);

    comac_translate (cr, OFFSET, OFFSET);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_surface (cr, image, 0, 0);
    comac_rectangle (cr, 0, 0, (SIZE - OFFSET), (SIZE - OFFSET));
    comac_fill (cr);

    comac_surface_destroy (image);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (composite_integer_translate_over,
	    "Test simple compositing: integer-translation 32->32 OVER",
	    "composite", /* keywords */
	    NULL,	 /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)
