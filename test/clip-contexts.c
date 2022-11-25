/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Intel Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Intel Corporation makes no representations about the
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

/*
 * Jeff Muizelaar found a bug on Quartz with comac-surface-clipper, which was
 * the topmost clip path from two different contexts and finding them equally
 * incorrectly concluding that the operation was a no-op.
 */

#define SIZE 10
#define CLIP_SIZE 2

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_t *cr2;

    /* opaque background */
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    /* first create an empty, non-overlappiny clip */
    cr2 = comac_create (comac_get_target (cr));
    comac_rectangle (cr2, 0, 0, SIZE/2-2, SIZE/2-2);
    comac_clip (cr2);

    comac_rectangle (cr2, SIZE/2+2, SIZE/2+2, SIZE/2-2, SIZE/2-2);
    comac_clip (cr2);

    /* and apply the clip onto the surface, empty nothing should be painted */
    comac_set_source_rgba (cr2, 1, 0, 0, .5);
    comac_paint (cr2);

    /* switch back to the original, and set only the last clip */
    comac_rectangle (cr, SIZE/2+2, SIZE/2+2, SIZE/2-2, SIZE/2-2);
    comac_clip (cr);

    comac_set_source_rgba (cr, 0, 0, 1, .5);
    comac_paint (cr);

    comac_destroy (cr2);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_contexts,
	    "Test clipping with 2 separate contexts",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
