/*
 * Copyright © 2008 Mozilla Corporation
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
 * Author: Jeff Muizelaar
 */

#include "comac-test.h"

#define WIDTH 60
#define HEIGHT 70

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* fill with black so we don't need an rgb test case */
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    /* setting a scale will ensure that the device offset is transformed */
    comac_scale (cr, 2.1, 2.8);
    comac_set_source_rgb (cr, 1, .5,.4);

    /* all rectangles should look the same */

    /* plain rectangle */
    comac_rectangle (cr, 4, 4, 8, 8);
    comac_fill (cr);

    comac_translate (cr, 10, 0);

    /* clipped rectangle */
    comac_save (cr);
    comac_rectangle (cr, 3, 3, 9, 9);
    comac_clip (cr);
    comac_rectangle (cr, 4, 4, 8, 8);
    comac_fill (cr);
    comac_restore (cr);

    comac_translate (cr, 0, 10);

    /* clipped and grouped rectangle */
    comac_save (cr);
    comac_rectangle (cr, 3, 3, 9, 9);
    comac_clip (cr);
    comac_push_group (cr);
    comac_rectangle (cr, 4, 4, 8, 8);
    comac_fill (cr);
    comac_pop_group_to_source (cr);
    comac_paint (cr);
    comac_restore (cr);

    comac_translate (cr, -10, 0);

    /* grouped rectangle */
    comac_push_group (cr);
    comac_rectangle (cr, 4, 4, 8, 8);
    comac_fill (cr);
    comac_pop_group_to_source (cr);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clipped_group,
	    "Test that a clipped group ends up in the right place",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
