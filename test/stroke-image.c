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

#define PAD 10
#define SIZE 100
#define IMAGE_SIZE (SIZE-PAD*2)
#define LINE_WIDTH 10

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *image;
    comac_t *cr_image;

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    image = comac_image_surface_create (COMAC_FORMAT_RGB24, IMAGE_SIZE, IMAGE_SIZE);
    cr_image = comac_create (image);
    comac_surface_destroy (image);

    /* Create the image */
    comac_set_source_rgb (cr_image, 0, 0, 0);
    comac_paint (cr_image);
    comac_set_source_rgb (cr_image, 0, 1, 0);
    comac_set_line_width (cr_image, LINE_WIDTH);
    comac_arc (cr_image, IMAGE_SIZE/2, IMAGE_SIZE/2, IMAGE_SIZE/2 - LINE_WIDTH/2, 0, M_PI * 2.0);
    comac_stroke (cr_image);

    /* Now stroke with it */
    comac_translate (cr, PAD, PAD);

    comac_set_source_surface (cr, comac_get_target (cr_image), 0, 0);
    comac_destroy (cr_image);

    comac_new_path (cr);
    comac_set_line_width (cr, LINE_WIDTH);
    comac_arc (cr, IMAGE_SIZE/2, IMAGE_SIZE/2, IMAGE_SIZE/2 - LINE_WIDTH/2, 0, M_PI * 2.0);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (stroke_image,
	    "Test stroking with an image source, with a non-identity CTM",
	    "stroke, image, transform", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
