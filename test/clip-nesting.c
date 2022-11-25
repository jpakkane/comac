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
 * Author: Owen Taylor <otaylor@redhat.com>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define SIZE 100
#define BORDER 10
#define LINE_WIDTH 20

static void
_propagate_status (comac_t *dst, comac_t *src)
{
    comac_path_t path;

    path.status = comac_status (src);
    if (path.status) {
	path.num_data = 0;
	path.data = NULL;
	comac_append_path (dst, &path);
    }
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *target_surface;
    comac_t *cr2, *cr3;

    target_surface = comac_get_group_target (cr);

    cr2 = comac_create (target_surface);

    /* Draw a diagonal line and clip to it */

    comac_move_to (cr2, BORDER, BORDER);
    comac_line_to (cr2, BORDER + LINE_WIDTH, BORDER);
    comac_line_to (cr2, SIZE - BORDER, SIZE - BORDER);
    comac_line_to (cr2, SIZE - BORDER - LINE_WIDTH, SIZE - BORDER);

    comac_clip (cr2);
    comac_set_source_rgb (cr2, 0, 0, 1); /* Blue */
    comac_paint (cr2);

    /* Clipping affects this comac_t */

    comac_set_source_rgb (cr2, 1, 1, 1); /* White */
    comac_rectangle (cr2,
		     SIZE / 2 - LINE_WIDTH / 2,
		     BORDER,
		     LINE_WIDTH,
		     SIZE - 2 * BORDER);
    comac_fill (cr2);

    /* But doesn't affect another comac_t that we create temporarily for
     * the same surface
     */
    cr3 = comac_create (target_surface);
    comac_set_source_rgb (cr3, 1, 1, 1); /* White */
    comac_rectangle (cr3,
		     SIZE - BORDER - LINE_WIDTH,
		     BORDER,
		     LINE_WIDTH,
		     SIZE - 2 * BORDER);
    comac_fill (cr3);

    _propagate_status (cr, cr3);
    comac_destroy (cr3);

    _propagate_status (cr, cr2);
    comac_destroy (cr2);

    /* And doesn't affect anything after this comac_t is destroyed */

    comac_set_source_rgb (cr, 1, 1, 1); /* White */
    comac_rectangle (cr, BORDER, BORDER, LINE_WIDTH, SIZE - 2 * BORDER);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_nesting,
	    "Test clipping with multiple contexts for the same surface",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    SIZE,
	    SIZE,
	    NULL,
	    draw)
