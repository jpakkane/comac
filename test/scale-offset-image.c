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
 * Test case derived from the bug report by Michel Iwaniec:
 * https://lists.comacgraphics.org/archives/comac/2008-November/015660.html
 */

#include "comac-test.h"

static comac_surface_t *
create_source (comac_surface_t *target, int width, int height)
{
    comac_surface_t *similar;
    comac_t *cr;

    similar = comac_image_surface_create (COMAC_FORMAT_RGB24, width, height);
    cr = comac_create (similar);
    comac_surface_destroy (similar);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_rectangle (cr, width - 4, height - 4, 2, 2);
    comac_fill (cr);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_rectangle (cr, width - 2, height - 4, 2, 2);
    comac_fill (cr);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_rectangle (cr, width - 4, height - 2, 2, 2);
    comac_fill (cr);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_rectangle (cr, width - 2, height - 2, 2, 2);
    comac_fill (cr);

    similar = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return similar;
}

static void
draw_grid (comac_t *cr, comac_pattern_t *pattern, int dst_x, int dst_y)
{
    comac_matrix_t m;

    comac_save (cr);
    comac_translate (cr, dst_x, dst_y);
    comac_scale (cr, 16, 16);
    comac_rotate (cr, 1);

    comac_matrix_init_translate (&m, 2560 - 4, 1280 - 4);
    comac_pattern_set_matrix (pattern, &m);
    comac_set_source (cr, pattern);
    comac_rectangle (cr, 0, 0, 4, 4);
    comac_fill (cr);

    comac_set_source_rgb (cr, .7, .7, .7);
    comac_set_line_width (cr, 1. / 16);
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, 4, 0);
    comac_move_to (cr, 0, 2);
    comac_line_to (cr, 4, 2);
    comac_move_to (cr, 0, 4);
    comac_line_to (cr, 4, 4);
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, 0, 4);
    comac_move_to (cr, 2, 0);
    comac_line_to (cr, 2, 4);
    comac_move_to (cr, 4, 0);
    comac_line_to (cr, 4, 4);
    comac_stroke (cr);

    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *source;
    comac_pattern_t *pattern;

    comac_paint (cr);

    source = create_source (comac_get_target (cr), 2560, 1280);
    pattern = comac_pattern_create_for_surface (source);
    comac_surface_destroy (source);

    comac_pattern_set_filter (pattern, COMAC_FILTER_NEAREST);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_NONE);

    draw_grid (cr, pattern, 50, 0);
    draw_grid (cr, pattern, 130, 0);
    draw_grid (cr, pattern, 210, 0);
    draw_grid (cr, pattern, 290, 0);

    draw_grid (cr, pattern, 50, 230);
    draw_grid (cr, pattern, 130, 230);
    draw_grid (cr, pattern, 210, 230);
    draw_grid (cr, pattern, 290, 230);

    comac_pattern_destroy (pattern);

    return COMAC_TEST_SUCCESS;
}

/* XFAIL: loss of precision converting a comac matrix to */
COMAC_TEST (scale_offset_image,
	    "Tests drawing surfaces under various scales and transforms",
	    "surface, scale-offset", /* keywords */
	    NULL,		     /* requirements */
	    320,
	    320,
	    NULL,
	    draw)
