/*
 * Copyright © 2010 Red Hat, Inc.
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
 * Author: Benjamin Otte <otte@gnome.org>
 */

#include "comac-test.h"

#define RECT 10
#define SPACE 5

static void
paint_with_alpha (comac_t *cr)
{
    comac_paint_with_alpha (cr, 0.0);
}

static void
mask_with_solid (comac_t *cr)
{
    comac_pattern_t *pattern = comac_pattern_create_rgba (1, 0, 0, 0);

    comac_mask (cr, pattern);
    
    comac_pattern_destroy (pattern);
}

static void
mask_with_empty_gradient (comac_t *cr)
{
    comac_pattern_t *pattern = comac_pattern_create_linear (1, 2, 3, 4);

    comac_mask (cr, pattern);
    
    comac_pattern_destroy (pattern);
}

static void
mask_with_gradient (comac_t *cr)
{
    comac_pattern_t *pattern = comac_pattern_create_radial (1, 2, 3, 4, 5, 6);

    comac_pattern_add_color_stop_rgba (pattern, 0, 1, 0, 0, 0);
    comac_pattern_add_color_stop_rgba (pattern, 0, 0, 0, 1, 0);

    comac_mask (cr, pattern);
    
    comac_pattern_destroy (pattern);
}

static void
mask_with_surface (comac_t *cr)
{
    comac_surface_t *surface = comac_surface_create_similar (comac_get_target (cr),
                                                             COMAC_CONTENT_COLOR_ALPHA,
                                                             RECT,
                                                             RECT);

    comac_mask_surface (cr, surface, 0, 0);
    
    comac_surface_destroy (surface);
}

static void
mask_with_alpha_surface (comac_t *cr)
{
    comac_surface_t *surface = comac_surface_create_similar (comac_get_target (cr),
                                                             COMAC_CONTENT_ALPHA,
                                                             RECT / 2,
                                                             RECT / 2);
    comac_pattern_t *pattern = comac_pattern_create_for_surface (surface);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_REFLECT);

    comac_mask (cr, pattern);

    comac_pattern_destroy (pattern);
    comac_surface_destroy (surface);
}

static void
mask_with_nonclear_surface (comac_t *cr)
{
    static unsigned char data[8 * 4] = { 0, };
    comac_surface_t *surface = comac_image_surface_create_for_data (data,
                                                                    COMAC_FORMAT_A1,
                                                                    16, 8, 4);

    comac_mask_surface (cr, surface, 0, 0);

    comac_surface_destroy (surface);
}

static void
mask_with_0x0_surface (comac_t *cr)
{
    comac_surface_t *surface = comac_surface_create_similar (comac_get_target (cr),
                                                             COMAC_CONTENT_COLOR_ALPHA,
                                                             0, 0);

    comac_mask_surface (cr, surface, 0, 0);
    
    comac_surface_destroy (surface);
}

static void
mask_with_extend_none (comac_t *cr)
{
    comac_surface_t *surface = comac_surface_create_similar (comac_get_target (cr),
                                                             COMAC_CONTENT_COLOR_ALPHA,
                                                             RECT,
                                                             RECT);

    comac_mask_surface (cr, surface, 2 * RECT, 2 * RECT);
    
    comac_surface_destroy (surface);
}

typedef void (* mask_func_t) (comac_t *);

static mask_func_t mask_funcs[] = {
  paint_with_alpha,
  mask_with_solid,
  mask_with_empty_gradient,
  mask_with_gradient,
  mask_with_surface,
  mask_with_alpha_surface,
  mask_with_nonclear_surface,
  mask_with_0x0_surface,
  mask_with_extend_none
};

static comac_operator_t operators[] = {
  COMAC_OPERATOR_CLEAR,
  COMAC_OPERATOR_SOURCE,
  COMAC_OPERATOR_OVER,
  COMAC_OPERATOR_IN,
  COMAC_OPERATOR_DEST_ATOP,
  COMAC_OPERATOR_SATURATE,
  COMAC_OPERATOR_MULTIPLY
};

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    unsigned int i, op;

    /* 565-compatible gray background */
    comac_set_source_rgb (cr, 0.51613, 0.55555, 0.51613);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0.0, 1.0, 0.0); /* green */
    /* mask with zero-alpha in several ways */

    comac_translate (cr, SPACE, SPACE);

    for (op = 0; op < ARRAY_LENGTH (operators); op++) {
        comac_set_operator (cr, operators[op]);

        for (i = 0; i < ARRAY_LENGTH (mask_funcs); i++) {
            comac_save (cr);
            comac_translate (cr, i * (RECT + SPACE), op * (RECT + SPACE));
            comac_rectangle (cr, 0, 0, RECT, RECT);
            comac_clip (cr);
            mask_funcs[i] (cr);
            comac_restore (cr);
        }
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (zero_mask,
	    "Testing that masking with zero alpha works",
	    "alpha, mask", /* keywords */
	    NULL, /* requirements */
	    SPACE + (RECT + SPACE) * ARRAY_LENGTH (mask_funcs),
	    SPACE + (RECT + SPACE) * ARRAY_LENGTH (operators),
	    NULL, draw)
