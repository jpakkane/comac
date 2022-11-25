/*
 * Copyright © 2011 Nis Martensen
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
 */

#include "comac-test.h"

#define OPERATORS_COUNT 29
#define WIDTH 160
#define HEIGHT 120

static void
example (comac_t *cr, const char *name)
{
    comac_save (cr);
    comac_push_group_with_content (cr, comac_surface_get_content (comac_get_target (cr)));

    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);

    comac_rectangle (cr, 0, 0, 120, 90);
    comac_set_source_rgba (cr, 0.7, 0, 0, 0.8);
    comac_fill (cr);

    if (strcmp (name, "clear") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_CLEAR);

    else if (strcmp (name, "source") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    else if (strcmp (name, "over") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_OVER);
    else if (strcmp (name, "in") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_IN);
    else if (strcmp (name, "out") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_OUT);
    else if (strcmp (name, "atop") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_ATOP);

    else if (strcmp (name, "dest") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_DEST);
    else if (strcmp (name, "dest_over") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_DEST_OVER);
    else if (strcmp (name, "dest_in") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_DEST_IN);
    else if (strcmp (name, "dest_out") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_DEST_OUT);
    else if (strcmp (name, "dest_atop") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_DEST_ATOP);

    else if (strcmp (name, "xor") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_XOR);
    else if (strcmp (name, "add") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_ADD);
    else if (strcmp (name, "saturate") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_SATURATE);
    else if (strcmp (name, "multiply") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_MULTIPLY);
    else if (strcmp (name, "screen") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_SCREEN);
    else if (strcmp (name, "overlay") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_OVERLAY);
    else if (strcmp (name, "darken") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_DARKEN);
    else if (strcmp (name, "lighten") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_LIGHTEN);
    else if (strcmp (name, "color_dodge") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_COLOR_DODGE);
    else if (strcmp (name, "color_burn") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_COLOR_BURN);
    else if (strcmp (name, "hard_light") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_HARD_LIGHT);
    else if (strcmp (name, "soft_light") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_SOFT_LIGHT);
    else if (strcmp (name, "difference") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_DIFFERENCE);
    else if (strcmp (name, "exclusion") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_EXCLUSION);
    else if (strcmp (name, "hsl_hue") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_HSL_HUE);
    else if (strcmp (name, "hsl_saturation") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_HSL_SATURATION);
    else if (strcmp (name, "hsl_color") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_HSL_COLOR);
    else if (strcmp (name, "hsl_luminosity") == 0)
	comac_set_operator (cr, COMAC_OPERATOR_HSL_LUMINOSITY);

    comac_rectangle (cr, 40, 30, 120, 90);
    comac_set_source_rgba (cr, 0, 0, 0.9, 0.4);
    comac_fill (cr);

    comac_pattern_t *pattern = comac_pop_group (cr);
    comac_rectangle (cr, 0, 0, WIDTH, HEIGHT);
    comac_clip (cr);
    // Make problems with COMAC_CONTENT_COLOR visible
    if (comac_surface_get_content (comac_get_target (cr)) == COMAC_CONTENT_COLOR) {
	comac_set_source_rgb (cr, 1, 1, 1);
	comac_paint (cr);
    }
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);
    comac_paint (cr);
    comac_restore (cr);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, 17);
    comac_move_to (cr, WIDTH + 20, 70);
    comac_set_source_rgb (cr, 1, 1, 0);
    comac_show_text (cr, name);

    comac_translate (cr, 0, HEIGHT);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    example (cr, "clear");

    example (cr, "source");
    example (cr, "over");
    example (cr, "in");
    example (cr, "out");
    example (cr, "atop");

    example (cr, "dest");
    example (cr, "dest_over");
    example (cr, "dest_in");
    example (cr, "dest_out");
    example (cr, "dest_atop");

    example (cr, "xor");
    example (cr, "add");
    example (cr, "saturate");

    example (cr, "multiply");
    example (cr, "screen");
    example (cr, "overlay");
    example (cr, "darken");
    example (cr, "lighten");
    example (cr, "color_dodge");
    example (cr, "color_burn");
    example (cr, "hard_light");
    example (cr, "soft_light");
    example (cr, "difference");
    example (cr, "exclusion");
    example (cr, "hsl_hue");
    example (cr, "hsl_saturation");
    example (cr, "hsl_color");
    example (cr, "hsl_luminosity");

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (operator_www,
	    "Operator samples from https://comacgraphics.org/operators/",
	    "operator", /* keywords */
	    NULL, /* requirements */
	    WIDTH * 2, HEIGHT * OPERATORS_COUNT,
	    NULL, draw)
