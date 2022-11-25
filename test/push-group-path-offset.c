/*
 * Copyright 2010 Red Hat Inc.
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

#define CLIP_OFFSET 15
#define CLIP_SIZE 20

#define WIDTH 50
#define HEIGHT 50

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* Neutral gray background */
    comac_set_source_rgb (cr, 0.51613, 0.55555, 0.51613);
    comac_paint (cr);

    /* the rest uses COMAC_OPERATOR_SOURCE so we see better when something goes wrong */
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);

    /* add a rectangle */
    comac_rectangle (cr, CLIP_OFFSET, CLIP_OFFSET, CLIP_SIZE, CLIP_SIZE);

    /* clip to the rectangle */
    comac_clip_preserve (cr);

    /* push a group. We now have a device offset. */
    comac_push_group (cr);

    /* push a group again. This is where the bug used to happen. */
    comac_push_group (cr);

    /* draw something */
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill_preserve (cr);

    /* make sure the stuff we drew ends up on the output */
    comac_pop_group_to_source (cr);
    comac_fill_preserve (cr);

    comac_pop_group_to_source (cr);
    comac_fill_preserve (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (push_group_path_offset,
	    "Exercises a bug in Comac 1.9 where existing paths applied the target's"
            " device offset twice when comac_push_group() was called.",
	    "group, path", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
