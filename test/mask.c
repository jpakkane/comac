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
 * Authors: Owen Taylor <otaylor@redhat.com>
 *          Kristian Høgsberg <krh@redhat.com>
 */

#include "comac-test.h"
#include <math.h>
#include <stdio.h>

#define WIDTH 16
#define HEIGHT 16
#define PAD 2

static const char *png_filename = "romedalen.png";
static comac_surface_t *image;

static void
set_solid_pattern (const comac_test_context_t *ctx, comac_t *cr, int x, int y)
{
    comac_set_source_rgb (cr, 0, 0, 0.6);
}

static void
set_translucent_pattern (const comac_test_context_t *ctx,
			 comac_t *cr,
			 int x,
			 int y)
{
    comac_set_source_rgba (cr, 0, 0, 0.6, 0.5);
}

static void
set_gradient_pattern (const comac_test_context_t *ctx,
		      comac_t *cr,
		      int x,
		      int y)
{
    comac_pattern_t *pattern;

    pattern = comac_pattern_create_linear (x, y, x + WIDTH, y + HEIGHT);
    comac_pattern_add_color_stop_rgba (pattern, 0, 1, 1, 1, 1);
    comac_pattern_add_color_stop_rgba (pattern, 1, 0, 0, 0.4, 1);
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);
}

static void
set_image_pattern (const comac_test_context_t *ctx, comac_t *cr, int x, int y)
{
    comac_pattern_t *pattern;

    if (image == NULL || comac_surface_status (image)) {
	comac_surface_destroy (image);
	image = comac_test_create_surface_from_png (ctx, png_filename);
    }

    pattern = comac_pattern_create_for_surface (image);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_REPEAT);
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);
}

static void
mask_polygon (comac_t *cr, int x, int y)
{
    comac_surface_t *mask_surface;
    comac_t *cr2;

    mask_surface = comac_surface_create_similar (comac_get_group_target (cr),
						 COMAC_CONTENT_ALPHA,
						 WIDTH,
						 HEIGHT);
    cr2 = comac_create (mask_surface);
    comac_surface_destroy (mask_surface);

    comac_save (cr2);
    comac_set_operator (cr2, COMAC_OPERATOR_CLEAR);
    comac_paint (cr2);
    comac_restore (cr2);

    comac_set_source_rgb (cr2, 1, 1, 1); /* white */

    comac_new_path (cr2);
    comac_move_to (cr2, 0, 0);
    comac_line_to (cr2, 0, HEIGHT);
    comac_line_to (cr2, WIDTH / 2, 3 * HEIGHT / 4);
    comac_line_to (cr2, WIDTH, HEIGHT);
    comac_line_to (cr2, WIDTH, 0);
    comac_line_to (cr2, WIDTH / 2, HEIGHT / 4);
    comac_close_path (cr2);
    comac_fill (cr2);

    comac_mask_surface (cr, comac_get_target (cr2), x, y);
    comac_destroy (cr2);
}

static void
mask_alpha (comac_t *cr, int x, int y)
{
    comac_paint_with_alpha (cr, 0.75);
}

static void
mask_gradient (comac_t *cr, int x, int y)
{
    comac_pattern_t *pattern;

    pattern = comac_pattern_create_linear (x, y, x + WIDTH, y + HEIGHT);

    comac_pattern_add_color_stop_rgba (pattern, 0, 1, 1, 1, 1);
    comac_pattern_add_color_stop_rgba (pattern, 1, 1, 1, 1, 0);

    comac_mask (cr, pattern);

    comac_pattern_destroy (pattern);
}

static void
clip_none (comac_t *cr, int x, int y)
{
}

static void
clip_rects (comac_t *cr, int x, int y)
{
    int height = HEIGHT / 3;

    comac_new_path (cr);
    comac_rectangle (cr, x, y, WIDTH, height);
    comac_rectangle (cr, x, y + 2 * height, WIDTH, height);
    comac_clip (cr);
}

static void
clip_circle (comac_t *cr, int x, int y)
{
    comac_new_path (cr);
    comac_arc (cr, x + WIDTH / 2, y + HEIGHT / 2, WIDTH / 2, 0, 2 * M_PI);
    comac_clip (cr);
    comac_new_path (cr);
}

static void (*const pattern_funcs[]) (const comac_test_context_t *ctx,
				      comac_t *cr,
				      int x,
				      int y) = {
    set_solid_pattern,
    set_translucent_pattern,
    set_gradient_pattern,
    set_image_pattern,
};

static void (*const mask_funcs[]) (comac_t *cr, int x, int y) = {
    mask_alpha,
    mask_gradient,
    mask_polygon,
};

static void (*const clip_funcs[]) (comac_t *cr, int x, int y) = {
    clip_none,
    clip_rects,
    clip_circle,
};

#define IMAGE_WIDTH (ARRAY_LENGTH (pattern_funcs) * (WIDTH + PAD) + PAD)
#define IMAGE_HEIGHT                                                           \
    (ARRAY_LENGTH (mask_funcs) * ARRAY_LENGTH (clip_funcs) * (HEIGHT + PAD) +  \
     PAD)

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *tmp_surface;
    size_t i, j, k;
    comac_t *cr2;

    /* Some of our drawing is unbounded, so we draw each test to
     * a temporary surface and copy over.
     */
    tmp_surface = comac_surface_create_similar (comac_get_group_target (cr),
						COMAC_CONTENT_COLOR_ALPHA,
						IMAGE_WIDTH,
						IMAGE_HEIGHT);
    cr2 = comac_create (tmp_surface);
    comac_surface_destroy (tmp_surface);

    for (k = 0; k < ARRAY_LENGTH (clip_funcs); k++) {
	for (j = 0; j < ARRAY_LENGTH (mask_funcs); j++) {
	    for (i = 0; i < ARRAY_LENGTH (pattern_funcs); i++) {
		int x = i * (WIDTH + PAD) + PAD;
		int y =
		    (ARRAY_LENGTH (mask_funcs) * k + j) * (HEIGHT + PAD) + PAD;

		/* Clear intermediate surface we are going to be drawing onto */
		comac_save (cr2);
		comac_set_operator (cr2, COMAC_OPERATOR_CLEAR);
		comac_paint (cr2);
		comac_restore (cr2);

		/* draw */
		comac_save (cr2);

		clip_funcs[k](cr2, x, y);
		pattern_funcs[i](ctx, cr2, x, y);
		mask_funcs[j](cr2, x, y);

		comac_restore (cr2);

		/* Copy back to the main pixmap */
		comac_set_source_surface (cr, comac_get_target (cr2), 0, 0);
		comac_rectangle (cr, x, y, WIDTH, HEIGHT);
		comac_fill (cr);
	    }
	}
    }

    comac_destroy (cr2);

    comac_surface_destroy (image);
    image = NULL;

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (mask,
	    "Tests of comac_mask",
	    "mask", /* keywords */
	    NULL,   /* requirements */
	    IMAGE_WIDTH,
	    IMAGE_HEIGHT,
	    NULL,
	    draw)
