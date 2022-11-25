/*
 * Copyright © 2005 Red Hat, Inc.
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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#define PAD 2
#define SIZE 10

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_rectangle (cr, PAD, PAD, SIZE, SIZE);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_fill_preserve (cr);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_stroke (cr);

    comac_translate (cr, SIZE + 2 * PAD, 0);

    comac_arc (cr, PAD + SIZE / 2, PAD + SIZE / 2, SIZE / 2, 0, 2 * M_PI);
    comac_fill_preserve (cr);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    fill_and_stroke,
    "Tests using comac_fill_preserve/comac_stroke to fill/stroke the same path",
    "fill-and-stroke, fill, stroke", /* keywords */
    NULL,			     /* requirements */
    2 * SIZE + 4 * PAD,
    SIZE + 2 * PAD,
    NULL,
    draw)
