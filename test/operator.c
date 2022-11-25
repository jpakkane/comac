/*
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define N_OPERATORS (COMAC_OPERATOR_SATURATE + 1)
#define SIZE 10
#define PAD 3

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    unsigned int n;

    comac_translate (cr, PAD, PAD);

    for (n = 0; n < N_OPERATORS; n++) {
	comac_reset_clip (cr);
	comac_rectangle (cr, 0, 0, SIZE, SIZE);
	comac_clip (cr);

	comac_set_source_rgb (cr, 1, 0, 0);
	comac_set_operator (cr, COMAC_OPERATOR_OVER);
	comac_rectangle (cr, 0, 0, SIZE-PAD, SIZE-PAD);
	comac_fill (cr);

	comac_set_source_rgb (cr, 0, 0, 1);
	comac_set_operator (cr, n);
	comac_rectangle (cr, PAD, PAD, SIZE-PAD, SIZE-PAD);
	comac_fill (cr);

	comac_translate (cr, SIZE+PAD, 0);
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (operator,
	    "Tests using set_operator()",
	    "operator", /* keywords */
	    NULL, /* requirements */
	    (SIZE+PAD) * N_OPERATORS + PAD, SIZE + 2*PAD,
	    NULL, draw)
