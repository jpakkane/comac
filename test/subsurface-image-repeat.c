/*
 * Copyright 2009 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Intel not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Intel makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * INTEL CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

static const char *png_filename = "romedalen.png";

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *image, *region;
    comac_t *cr_region;

    comac_set_source_rgb (cr, .5, .5, .5);
    comac_paint (cr);

    /* fill the centre */
    region = comac_surface_create_for_rectangle (comac_get_target (cr),
						 20,
						 20,
						 20,
						 20);
    cr_region = comac_create (region);
    comac_surface_destroy (region);

    image = comac_test_create_surface_from_png (ctx, png_filename);
    comac_set_source_surface (cr_region,
			      image,
			      10 - comac_image_surface_get_width (image) / 2,
			      10 - comac_image_surface_get_height (image) / 2);
    comac_paint (cr_region);
    comac_surface_destroy (image);

    comac_set_source_surface (cr, comac_get_target (cr_region), 20, 20);
    comac_destroy (cr_region);

    /* repeat the pattern around the outside, but do not overwrite...*/
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_REPEAT);
    comac_rectangle (cr, 0, 0, width, height);
    comac_rectangle (cr, 20, 40, 20, -20);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    subsurface_image_repeat,
    "Tests source (image) clipping with repeat",
    "subsurface, image, repeat", /* keywords */
    "target=raster",
    /* FIXME! recursion bug in subsurface/snapshot (with pdf backend) */ /* requirements */
    60,
    60,
    NULL,
    draw)
