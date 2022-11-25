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
 * Author: Kristian Høgsberg <krh@redhat.com>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define WIDTH 16
#define HEIGHT 16
#define PAD 2

static void
draw_mask (comac_t *cr, int x, int y)
{
    comac_surface_t *mask_surface;
    comac_t *cr2;

    double width = (int)(0.9 * WIDTH);
    double height = (int)(0.9 * HEIGHT);
    x += 0.05 * WIDTH;
    y += 0.05 * HEIGHT;

    mask_surface = comac_surface_create_similar (comac_get_group_target (cr),
						 COMAC_CONTENT_ALPHA,
						 width, height);
    cr2 = comac_create (mask_surface);
    comac_surface_destroy (mask_surface);

    comac_save (cr2);
    comac_set_source_rgba (cr2, 0, 0, 0, 0); /* transparent */
    comac_set_operator (cr2, COMAC_OPERATOR_SOURCE);
    comac_paint (cr2);
    comac_restore (cr2);

    comac_set_source_rgb (cr2, 1, 1, 1); /* white */

    comac_arc (cr2, 0.5 * width, 0.5 * height, 0.45 * height, 0, 2 * M_PI);
    comac_fill (cr2);

    comac_mask_surface (cr, comac_get_target (cr2), x, y);
    comac_destroy (cr2);
}

static void
draw_glyphs (comac_t *cr, int x, int y)
{
    comac_text_extents_t extents;

    comac_set_font_size (cr, 0.8 * HEIGHT);

    comac_text_extents (cr, "FG", &extents);
    comac_move_to (cr,
		   x + floor ((WIDTH - extents.width) / 2 + 0.5) - extents.x_bearing,
		   y + floor ((HEIGHT - extents.height) / 2 + 0.5) - extents.y_bearing);
    comac_show_text (cr, "FG");
}

static void
draw_polygon (comac_t *cr, int x, int y)
{
    double width = (int)(0.9 * WIDTH);
    double height = (int)(0.9 * HEIGHT);
    x += 0.05 * WIDTH;
    y += 0.05 * HEIGHT;

    comac_new_path (cr);
    comac_move_to (cr, x, y);
    comac_line_to (cr, x, y + height);
    comac_line_to (cr, x + width / 2, y + 3 * height / 4);
    comac_line_to (cr, x + width, y + height);
    comac_line_to (cr, x + width, y);
    comac_line_to (cr, x + width / 2, y + height / 4);
    comac_close_path (cr);
    comac_fill (cr);
}

static void
draw_rects (comac_t *cr, int x, int y)
{
    double block_width = (int)(0.33 * WIDTH + 0.5);
    double block_height = (int)(0.33 * HEIGHT + 0.5);
    int i, j;

    for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	    if ((i + j) % 2 == 0)
		comac_rectangle (cr,
				 x + block_width * i, y + block_height * j,
				 block_width,         block_height);

    comac_fill (cr);
}

static void (* const draw_funcs[])(comac_t *cr, int x, int y) = {
    draw_mask,
    draw_glyphs,
    draw_polygon,
    draw_rects
};

#define N_OPERATORS (1 + COMAC_OPERATOR_SATURATE - COMAC_OPERATOR_CLEAR)

#define IMAGE_WIDTH (N_OPERATORS * (WIDTH + PAD) + PAD)
#define IMAGE_HEIGHT (ARRAY_LENGTH (draw_funcs) * (HEIGHT + PAD) + PAD)

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    size_t j, x, y;
    comac_operator_t op;
    comac_pattern_t *pattern;

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, 0.9 * HEIGHT);

    for (j = 0; j < ARRAY_LENGTH (draw_funcs); j++) {
	for (op = COMAC_OPERATOR_CLEAR; op < N_OPERATORS; op++) {
	    x = op * (WIDTH + PAD) + PAD;
	    y = j * (HEIGHT + PAD) + PAD;

	    comac_save (cr);

	    pattern = comac_pattern_create_linear (x + WIDTH, y,
						   x,         y + HEIGHT);
	    comac_pattern_add_color_stop_rgba (pattern, 0.2,
					       0.0, 0.0, 1.0, 1.0); /* Solid blue */
	    comac_pattern_add_color_stop_rgba (pattern, 0.8,
					       0.0, 0.0, 1.0, 0.0); /* Transparent blue */
	    comac_set_source (cr, pattern);
	    comac_pattern_destroy (pattern);

	    comac_rectangle (cr, x, y, WIDTH, HEIGHT);
	    comac_fill (cr);

	    comac_set_operator (cr, op);
	    comac_set_source_rgb (cr, 1.0, 0.0, 0.0);

	    comac_move_to (cr, x, y);
	    comac_line_to (cr, x + WIDTH, y);
	    comac_line_to (cr, x, y + HEIGHT);
	    comac_clip (cr);

	    draw_funcs[j] (cr, x, y);
	    if (comac_status (cr))
		comac_test_log (ctx, "%d %d HERE!\n", op, (int)j);

	    comac_restore (cr);
	}
    }

    if (comac_status (cr) != COMAC_STATUS_SUCCESS)
	comac_test_log (ctx, "%d %d .HERE!\n", op, (int)j);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_operator,
	    "Surface clipping with different operators",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    IMAGE_WIDTH, IMAGE_HEIGHT,
	    NULL, draw)

