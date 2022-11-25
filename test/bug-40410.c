/*
 * Copyright © 2011 Krzysztof Kosiński
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
 * Author: Krzysztof Kosiński <tweenk.pl@gmail.com>
 */

#include "comac-test.h"

#define WIDTH	300
#define HEIGHT	100

/*
 * The bug appears to be triggered if:
 * 1. There is more than one subpath
 * 2. All subpaths are axis-aligned rectangles
 * 3. Only one of the subpaths is within surface bounds
 *
 * Tweaking any of the coordinates so that there is at least one
 * non-axis-aligned segment or removing the second subpath (the one that is
 * outside the surface bounds) causes the bug to disappear.
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1,1,1);
    comac_paint (cr);

    comac_move_to (cr, 10.3, 10.6);
    comac_line_to (cr, 10.3, 150.2);
    comac_line_to (cr, 290.1, 150.2);
    comac_line_to (cr, 290.1, 10.6);
    comac_close_path (cr);

    comac_move_to (cr, 10.3, 180.7);
    comac_line_to (cr, 10.3, 230.2);
    comac_line_to (cr, 290.1, 230.2);
    comac_line_to (cr, 290.1, 180.7);
    comac_close_path (cr);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_40410,
	    "Exercises a bug found in 1.10.2 (and never again!)",
	    "fill", /* keywords */
	    "target=raster", /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
