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

#define WIDTH 50
#define HEIGHT 50

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* Neutral gray background */
    comac_set_source_rgb (cr, 0.5, 0.5, 0.5);
    comac_paint (cr);

    /* remove this clip operation and everything works */
    comac_rectangle (cr, 10, 10, 30, 30);
    comac_clip (cr);

    /* remove this no-op and everything works */
    comac_stroke (cr);

    /* make the y coordinates integers and everything works */
    comac_move_to (cr, 20, 20.101562);
    comac_line_to (cr, 30, 20.101562);

    /* This clip operation should fail to work. But with comac 1.9, if all the 
     * 3 cases above happen, the clip will not work and the paint will happen.
     */
    comac_save (cr);
    {
	comac_set_source_rgba (cr, 1, 0.5, 0.5, 1);
	comac_clip_preserve (cr);
	comac_paint (cr);
    }
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_stroke_no_op,
	    "Exercises a bug found by Benjamin Otte whereby a no-op clip is "
	    "nullified by a stroke",
	    "clip, stroke", /* keywords */
	    NULL,	    /* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
