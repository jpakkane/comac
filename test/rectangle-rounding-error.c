/*
 * Copyright © 2005 Bertram Felgenhauer
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * the author not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The author makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHOR. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Bertram Felgenhauer <int-e@gmx.de>
 */

#include "comac-test.h"

/* Test case for:
 *
 *      https://bugs.freedesktop.org/show_bug.cgi?id=4137
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_translate (cr, -300, -300);
    comac_scale (cr, 677.0 / 26, 677.0 / 26);
    comac_translate (cr, 1, 1);

    /* this should draw a seamless 2x2 rectangle */
    comac_rectangle (cr, 11, 11, 1, 1);
    comac_rectangle (cr, 11, 12, 1, 1);
    comac_rectangle (cr, 12, 11, 1, 1);
    comac_rectangle (cr, 12, 12, 1, 1);

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    rectangle_rounding_error,
    "This demonstrates (or not) a rounding error that causes a gap between "
    "two neighbouring rectangles.",
    "trap",	     /* keywords */
    "target=raster", /* requirements */
    76,
    76,
    NULL,
    draw)
