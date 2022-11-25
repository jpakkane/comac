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

#define SIZE 40

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;
    comac_matrix_t matrix;

    /* Paint a diagonal division as a test image */
    comac_set_source_rgb (cr, 1, 1, 1);	/* White */
    comac_paint (cr);

    comac_move_to (cr, SIZE,    0);
    comac_line_to (cr, SIZE, SIZE);
    comac_line_to (cr, 0,    SIZE);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_fill (cr);

    /* Create a pattern with the target surface as the source,
     * offset by SIZE/2
     */
    pattern = comac_pattern_create_for_surface (comac_get_group_target (cr));

    comac_matrix_init_translate (&matrix, - SIZE / 2, - SIZE / 2);
    comac_pattern_set_matrix (pattern, &matrix);

    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);

    /* Copy two rectangles from the upper-left quarter of the image to
     * the lower right.  It will work if we use comac_fill(), but the
     * comac_clip() comac_paint() combination fails because the clip
     * on the surface as a destination affects it as the source as
     * well.
     */
    comac_rectangle (cr,
		     2 * SIZE / 4, 2 * SIZE / 4,
		     SIZE / 4,     SIZE / 4);
    comac_rectangle (cr,
		     3 * SIZE / 4, 3 * SIZE / 4,
		     SIZE / 4,     SIZE / 4);
    comac_clip (cr);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;

}

COMAC_TEST (self_copy,
	    "Test copying from a surface to itself with a clip",
	    "paint", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
