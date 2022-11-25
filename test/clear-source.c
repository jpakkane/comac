/*
 * Copyright 2009 Benjamin Otte
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

typedef enum {
  CLEAR,
  CLEARED,
  PAINTED
} surface_type_t;

#define SIZE 10
#define SPACE 5

static comac_surface_t *
create_surface (comac_t *target, comac_content_t content, surface_type_t type)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_surface_create_similar (comac_get_target (target),
					    content,
					    SIZE, SIZE);

    if (type == CLEAR)
	return surface;

    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 0.75, 0, 0);
    comac_paint (cr);

    if (type == PAINTED)
	goto DONE;

    comac_set_operator (cr, COMAC_OPERATOR_CLEAR);
    comac_paint (cr);

DONE:
    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return surface;
}

static void
paint (comac_t *cr, comac_surface_t *surface)
{
    comac_set_source_surface (cr, surface, 0, 0);
    comac_paint (cr);
}

static void
fill (comac_t *cr, comac_surface_t *surface)
{
    comac_set_source_surface (cr, surface, 0, 0);
    comac_rectangle (cr, -SPACE, -SPACE, SIZE + 2 * SPACE, SIZE + 2 * SPACE);
    comac_fill (cr);
}

static void
stroke (comac_t *cr, comac_surface_t *surface)
{
    comac_set_source_surface (cr, surface, 0, 0);
    comac_set_line_width (cr, 2.0);
    comac_rectangle (cr, 1, 1, SIZE - 2, SIZE - 2);
    comac_stroke (cr);
}

static void
mask (comac_t *cr, comac_surface_t *surface)
{
    comac_set_source_rgb (cr, 0, 0, 0.75);
    comac_mask_surface (cr, surface, 0, 0);
}

static void
mask_self (comac_t *cr, comac_surface_t *surface)
{
    comac_set_source_surface (cr, surface, 0, 0);
    comac_mask_surface (cr, surface, 0, 0);
}

static void
glyphs (comac_t *cr, comac_surface_t *surface)
{
    comac_set_source_surface (cr, surface, 0, 0);
    comac_select_font_face (cr,
			    "@comac:",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, 16);
    comac_translate (cr, 0, SIZE);
    comac_show_text (cr, "C");
}

typedef void (* operation_t) (comac_t *cr, comac_surface_t *surface);
static operation_t operations[] = {
    paint,
    fill,
    stroke,
    mask,
    mask_self,
    glyphs
};

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_content_t contents[] = { COMAC_CONTENT_COLOR_ALPHA, COMAC_CONTENT_COLOR, COMAC_CONTENT_ALPHA };
    unsigned int content, type, ops;

    comac_set_source_rgb (cr, 0.5, 0.5, 0.5);
    comac_paint (cr);
    comac_translate (cr, SPACE, SPACE);

    for (type = 0; type <= PAINTED; type++) {
	for (content = 0; content < ARRAY_LENGTH (contents); content++) {
	    comac_surface_t *surface;

	    surface = create_surface (cr, contents[content], type);

            comac_save (cr);
            for (ops = 0; ops < ARRAY_LENGTH (operations); ops++) {
                comac_save (cr);
                operations[ops] (cr, surface);
                comac_restore (cr);
                comac_translate (cr, 0, SIZE + SPACE);
            }
            comac_restore (cr);
            comac_translate (cr, SIZE + SPACE, 0);

	    comac_surface_destroy (surface);
        }
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clear_source,
	    "Check painting with cleared surfaces works as expected",
	    NULL, /* keywords */
	    NULL, /* requirements */
	    (SIZE + SPACE) * 9 + SPACE, ARRAY_LENGTH (operations) * (SIZE + SPACE) + SPACE,
	    NULL, draw)
