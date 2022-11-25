/*
 * Copyright © 2012 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"

#define SIZE 60
#define WIDTH  SIZE
#define HEIGHT SIZE


/* PDF transparency groups can be isolated or non-isolated. This test
 * checks that the PDF output is using isolated groups. If the group
 * is non-isolated the bottom half of the inner rectangle will be
 * red. Note poppler-comac currently ignores the isolated flag and
 * treats the group as isolated.
 *
 * Refer to http://www.pdfvt.com/PDFVT_TransparencyGuide.html for an
 * explanation isolated vs non-isolated.
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1, 0.5, 0);
    comac_rectangle (cr, 0, SIZE/2, SIZE, SIZE/2);
    comac_fill (cr);

    comac_set_operator (cr, COMAC_OPERATOR_MULTIPLY);

    comac_push_group (cr);

    comac_set_source_rgb (cr, 0.7, 0.7, 0.7);
    comac_rectangle (cr, SIZE/4, SIZE/4, SIZE/2, SIZE/2);
    comac_fill (cr);

    comac_pop_group_to_source (cr);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (pdf_isolated_group,
	    "Check that transparency groups in PDF output are isolated",
	    "group, operator", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
