/*
 * Copyright © 2013 Adrian Johnson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"
#include <math.h>

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_save (cr);
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);
    comac_restore (cr);

    comac_rectangle (cr, 5, 5, 20, 20);
    comac_rotate (cr, M_PI/4);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (rotate_stroke_box,
	    "Ensure rectangle path optimization works when the stroking transform is non rectilinear",
	    "path", /* keywords */
	    NULL, /* requirements */
	    30, 30,
	    NULL, draw)
