/*
 * Copyright 2009 Chris Wilson
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

#include "comac-test.h"

#define WIDTH 20
#define HEIGHT 20

static const char *png_filename = "romedalen.png";

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *image;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    image = comac_test_create_surface_from_png (ctx, png_filename);
    comac_set_source_surface (cr, image, 0, 0);
    comac_surface_destroy (image);

    /* simple clip */
    comac_save (cr);
    comac_rectangle (cr, 2, 2, 16, 16);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    /* unaligned clip */
    comac_save (cr);
    comac_rectangle (cr, 2.5, 2.5, 15, 15);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, -WIDTH, HEIGHT);

    /* aligned-clip */
    comac_save (cr);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    comac_rectangle (cr, 0, 0, 20, 20);
    comac_rectangle (cr, 3, 3, 10, 10);
    comac_rectangle (cr, 7, 7, 10, 10);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, WIDTH, 0);

    /* force a clip-mask */
    comac_save (cr);
    comac_arc (cr, 10, 10, 10, 0, 2 * M_PI);
    comac_new_sub_path (cr);
    comac_arc_negative (cr, 10, 10, 5, 2 * M_PI, 0);
    comac_new_sub_path (cr);
    comac_arc (cr, 10, 10, 2, 0, 2 * M_PI);
    comac_clip (cr);
    comac_paint (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_image,
	    "Tests painting an image through complex clips.",
	    "clip, paint", /* keywords */
	    NULL,	   /* requirements */
	    2 * WIDTH,
	    2 * HEIGHT,
	    NULL,
	    draw)
