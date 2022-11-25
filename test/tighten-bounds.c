/*
 * Copyright © 2011 Uli Schlachter
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Uli Schlachter <psychon@znc.in>
 */

#include "comac-test.h"

static void
path_none (comac_t *cr, int size)
{
}

static void
path_box (comac_t *cr, int size)
{
    comac_rectangle (cr, 0, 0, size, size);
}

static void
path_box_unaligned (comac_t *cr, int size)
{
    comac_rectangle (cr, 0.5, 0.5, size - 1, size - 1);
}

static void
path_triangle (comac_t *cr, int size)
{
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, size / 2, size);
    comac_line_to (cr, size, 0);
    comac_close_path (cr);
}

static void
path_circle (comac_t *cr, int size)
{
    comac_arc (cr, size / 2.0, size / 2.0, size / 2.0, 0, 2 * M_PI);
}

static void (*const path_funcs[]) (comac_t *cr, int size) = {
    path_none, path_box, path_box_unaligned, path_triangle, path_circle};

#define SIZE 20
#define PAD 2
#define TYPES 6
/* All-clipped is boring, thus we skip path_none for clipping */
#define CLIP_OFFSET 1
#define IMAGE_WIDTH                                                            \
    ((ARRAY_LENGTH (path_funcs) - CLIP_OFFSET) * TYPES * (SIZE + PAD) - PAD)
#define IMAGE_HEIGHT (ARRAY_LENGTH (path_funcs) * (SIZE + PAD) - PAD)

static void
draw_idx (comac_t *cr, int i, int j, int type)
{
    comac_bool_t little_path;
    comac_bool_t empty_clip;
    comac_bool_t little_clip;

    /* The lowest bit controls the path, the rest the clip */
    little_path = type & 1;

    /* We don't want the combination "empty_clip = TRUE, little_clip = FALSE"
     * (== all clipped).
     */
    switch (type >> 1) {
    case 0:
	empty_clip = FALSE;
	little_clip = FALSE;
	break;
    case 1:
	empty_clip = FALSE;
	little_clip = TRUE;
	break;
    case 2:
	empty_clip = TRUE;
	little_clip = TRUE;
	break;
    default:
	return;
    }

    comac_save (cr);

    /* Thanks to the fill rule, drawing something twice removes it again */
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    path_funcs[i](cr, SIZE);
    if (empty_clip)
	path_funcs[i](cr, SIZE);
    if (little_clip) {
	comac_save (cr);
	comac_translate (cr, SIZE / 4, SIZE / 4);
	path_funcs[i](cr, SIZE / 2);
	comac_restore (cr);
    }
    comac_clip (cr);

    path_funcs[j](cr, SIZE);
    path_funcs[j](cr, SIZE);
    if (little_path) {
	/* Draw the object again in the center of itself */
	comac_save (cr);
	comac_translate (cr, SIZE / 4, SIZE / 4);
	path_funcs[j](cr, SIZE / 2);
	comac_restore (cr);
    }
    comac_fill (cr);
    comac_restore (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    size_t i, j, k;

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_paint (cr);

    /* Set an unbounded operator so that we can see how accurate the bounded
     * extents were.
     */
    comac_set_operator (cr, COMAC_OPERATOR_IN);
    comac_set_source_rgb (cr, 1, 1, 1);

    for (j = 0; j < ARRAY_LENGTH (path_funcs); j++) {
	comac_save (cr);
	for (i = CLIP_OFFSET; i < ARRAY_LENGTH (path_funcs); i++) {
	    for (k = 0; k < TYPES; k++) {
		comac_save (cr);
		comac_rectangle (cr, 0, 0, SIZE, SIZE);
		comac_clip (cr);
		draw_idx (cr, i, j, k);
		comac_restore (cr);
		comac_translate (cr, SIZE + PAD, 0);
	    }
	}
	comac_restore (cr);
	comac_translate (cr, 0, SIZE + PAD);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (tighten_bounds,
	    "Tests that we tighten the bounds after tessellation.",
	    "fill", /* keywords */
	    NULL,   /* requirements */
	    IMAGE_WIDTH,
	    IMAGE_HEIGHT,
	    NULL,
	    draw)
