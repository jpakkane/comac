/*
 * Copyright © 2005 Mozilla Corporation
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

#define SIZE 12

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *source;
    comac_t *cr2;

    source = comac_surface_create_similar (comac_get_group_target (cr),
					   COMAC_CONTENT_COLOR_ALPHA,
					   SIZE,
					   SIZE);
    cr2 = comac_create (source);
    comac_surface_destroy (source);

    /* Fill the source surface with green */
    comac_set_source_rgb (cr2, 0, 1, 0);
    comac_paint (cr2);

    /* Draw a blue square in the middle of the source with clipping.
     * Note that we are only clipping within a save/restore block but
     * the buggy behavior demonstrates that the clip remains present
     * on the surface. */
    comac_save (cr2);
    comac_rectangle (cr2, SIZE / 4, SIZE / 4, SIZE / 2, SIZE / 2);
    comac_clip (cr2);
    comac_set_source_rgb (cr2, 0, 0, 1);
    comac_paint (cr2);
    comac_restore (cr2);

    /* Fill the destination surface with solid red (should not appear
     * in final result) */
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    /* Now draw the source surface onto the destination with scaling. */
    comac_scale (cr, 2.0, 1.0);

    comac_set_source_surface (cr, comac_get_target (cr2), 0, 0);
    comac_destroy (cr2);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (source_clip_scale,
	    "Test that a source surface is not affected by a clip when scaling",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    SIZE * 2,
	    SIZE,
	    NULL,
	    draw)
