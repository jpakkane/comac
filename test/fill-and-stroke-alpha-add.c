/*
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Red Hat, Inc.
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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define PAD 2
#define SIZE 10

typedef void (*path_func_t) (comac_t *cr);

static void
rectangle (comac_t *cr)
{
    comac_rectangle (cr, PAD, PAD, SIZE, SIZE);
}

static void
circle (comac_t *cr)
{
    comac_arc (cr, PAD + SIZE / 2, PAD + SIZE / 2, SIZE / 2, 0, 2 * M_PI);
}

/* Given a path-generating function and two possibly translucent
 * patterns, fill and stroke the path with the patterns (to an
 * offscreen group), then blend the result into the destination.
 */
static void
fill_and_stroke (comac_t *cr,
		 path_func_t path_func,
		 comac_pattern_t *fill_pattern,
		 comac_pattern_t *stroke_pattern)
{
    comac_push_group (cr);
    {
	(path_func) (cr);
	comac_set_source (cr, fill_pattern);
	comac_fill_preserve (cr);

	/* Use DEST_OUT to subtract stroke from fill. */
	comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
	comac_set_operator (cr, COMAC_OPERATOR_DEST_OUT);
	comac_stroke_preserve (cr);

	/* Then use ADD to draw the stroke without a seam. */
	comac_set_source (cr, stroke_pattern);
	comac_set_operator (cr, COMAC_OPERATOR_ADD);
	comac_stroke (cr);
    }
    comac_pop_group_to_source (cr);
    comac_paint (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *blue;
    comac_pattern_t *red;

    blue = comac_pattern_create_rgba (0.0, 0.0, 1.0, 0.8);
    red = comac_pattern_create_rgba (1.0, 0.0, 0.0, 0.2);

    comac_test_paint_checkered (cr);

    fill_and_stroke (cr, rectangle, blue, red);

    comac_translate (cr, SIZE + 2 * PAD, 0);

    fill_and_stroke (cr, circle, red, blue);

    comac_pattern_destroy (blue);
    comac_pattern_destroy (red);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (fill_and_stroke_alpha_add,
	    "Use a group to fill/stroke a path (each with different alpha) "
	    "using DEST_OUT and ADD to combine",
	    "fill-and-stroke, fill, stroke", /* keywords */
	    NULL,			     /* requirements */
	    2 * SIZE + 4 * PAD,
	    SIZE + 2 * PAD,
	    NULL,
	    draw)
