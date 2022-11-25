/*
 * Copyright 2010 Igor Nikitin
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
 * Author: Igor Nikitin <igor_nikitin@valentina-db.com>
 */

#include "comac-test.h"

#define HEIGHT 15
#define WIDTH 40

static void background (comac_t *cr)
{
     comac_set_source_rgb( cr, 0, 0, 0 );
     comac_paint (cr);
}

static void text (comac_t *cr)
{
     comac_move_to (cr, 0, 12);
     comac_set_source_rgb (cr, 1, 1, 1);
     comac_show_text (cr, "COMAC");
}

static comac_test_status_t
top (comac_t *cr, int width, int height)
{
     background (cr);

     comac_rectangle (cr, 0, 0, WIDTH, 5);
     comac_clip (cr);

     text (cr);

     return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
bottom (comac_t *cr, int width, int height)
{
     background (cr);

     comac_rectangle (cr, 0, HEIGHT-5, WIDTH, 5);
     comac_clip (cr);

     text (cr);

     return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
left (comac_t *cr, int width, int height)
{
     background (cr);

     comac_rectangle (cr, 0, 0, 10, HEIGHT);
     comac_clip (cr);

     text (cr);

     return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
right (comac_t *cr, int width, int height)
{
     background (cr);

     comac_rectangle (cr, WIDTH-10, 0, 10, HEIGHT);
     comac_clip (cr);

     text (cr);

     return COMAC_TEST_SUCCESS;
}

COMAC_TEST (partial_clip_text_top,
	    "Tests drawing text through a single, partial clip.",
	    "clip, text", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, top)
COMAC_TEST (partial_clip_text_bottom,
	    "Tests drawing text through a single, partial clip.",
	    "clip, text", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, bottom)
COMAC_TEST (partial_clip_text_left,
	    "Tests drawing text through a single, partial clip.",
	    "clip, text", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, left)
COMAC_TEST (partial_clip_text_right,
	    "Tests drawing text through a single, partial clip.",
	    "clip, text", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, right)
