/*
 * Copyright 2008 Chris Wilson
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
 */

/*
 * Michael Natterer (mitch) reported a bad regression with post-1.8 trunk
 * with artifacts drawn whilst repainting exposed areas.
 */

#include "comac-test.h"

static const char png_filename[] = "romedalen.png";

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *image;

    image = comac_test_create_surface_from_png (ctx, png_filename);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    comac_rectangle (cr, 20, 20, 10, 10);
    comac_clip (cr);

    comac_set_source_surface (cr, image, 10, 10);
    comac_surface_destroy (image);

    comac_rectangle (cr, 10, 10, 20, 20);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clipped_surface,
	    "Tests application of a clip to a source surface",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    40,
	    40,
	    NULL,
	    draw)
