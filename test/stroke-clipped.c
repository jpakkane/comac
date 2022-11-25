/*
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

#define SIZE 200

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int row;

    comac_set_source_rgb(cr, 1, 1, 1);
    comac_paint(cr);

    comac_set_source_rgb(cr, 1, 0, 0);
    for(row = 0; row < SIZE; row++) {
	comac_rectangle(cr, 0, row, SIZE, 1);
	comac_clip(cr);

	comac_arc(cr, SIZE/2, SIZE/2, SIZE/2-8, 0, 2*M_PI);
	comac_stroke(cr);

	comac_reset_clip(cr);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (stroke_clipped,
	    "Check that the stroke is accurately drawn through smaller clips",
	    "stroke", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)
