/*
 * Copyright © 2020 Ben Pfaff & Uli Schlachter
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
 * Authors:
 *   Ben Pfaff <blp@cs.stanford.edu>
 *   Uli Schlachter <psychon@znc.in>
 */

#include "comac-test.h"

static comac_surface_t *
draw_recording ()
{
    comac_surface_t *recording;
    comac_rectangle_t extents;
    comac_t *cr;

    extents.x = 0;
    extents.y = 0;
    extents.width = 10;
    extents.height = 10;

    recording =
	comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, &extents);

    cr = comac_create (recording);
    comac_tag_begin (cr, COMAC_TAG_DEST, "name='dest'");
    comac_rectangle (cr, 3, 3, 4, 4);
    comac_stroke (cr);
    comac_tag_end (cr, COMAC_TAG_DEST);
    comac_destroy (cr);

    return recording;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *recording;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    recording = draw_recording ();
    comac_set_source_surface (cr, recording, 0, 0);
    comac_paint (cr);
    comac_surface_destroy (recording);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_448,
	    "Exercises a bug with the tag API",
	    "pdf", /* keywords */
	    NULL,  /* requirements */
	    10,
	    10,
	    NULL,
	    draw)
