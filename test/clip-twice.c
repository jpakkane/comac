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
 * Author: Kristian Høgsberg <krh@redhat.com>
 */

#include "comac-test.h"

#define WIDTH 64
#define HEIGHT 64

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_new_path (cr);
    comac_arc (cr, WIDTH / 2, HEIGHT / 2, WIDTH / 3, 0, 2 * M_PI);
    comac_clip (cr);

    comac_new_path (cr);
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, WIDTH / 4, HEIGHT / 2);
    comac_line_to (cr, 0, HEIGHT);
    comac_line_to (cr, WIDTH, HEIGHT);
    comac_line_to (cr, 3 * WIDTH / 4, HEIGHT / 2);
    comac_line_to (cr, WIDTH, 0);
    comac_close_path (cr);
    comac_clip (cr);

    comac_set_source_rgb (cr, 0, 0, 0.6);

    comac_new_path (cr);
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, 0, HEIGHT);
    comac_line_to (cr, WIDTH / 2, 3 * HEIGHT / 4);
    comac_line_to (cr, WIDTH, HEIGHT);
    comac_line_to (cr, WIDTH, 0);
    comac_line_to (cr, WIDTH / 2, HEIGHT / 4);
    comac_close_path (cr);
    comac_fill (cr);

    comac_new_path (cr);
    comac_arc (cr, WIDTH / 2, HEIGHT / 2, WIDTH / 5, 0, 2 * M_PI);
    comac_clip (cr);
    comac_set_source_rgb (cr, 1, 1, 0);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_twice,
	    "Verifies that the clip mask is updated correctly when it constructed by setting the clip path twice.",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
