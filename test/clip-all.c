/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Novell, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Radek Doulík <rodo@novell.com>
 */

#include "comac-test.h"

#define SIZE 10
#define CLIP_SIZE 2

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_fill (cr);

    comac_reset_clip (cr);
    comac_rectangle (cr, CLIP_SIZE, CLIP_SIZE, CLIP_SIZE, CLIP_SIZE);
    comac_clip (cr);
    comac_rectangle (cr, 3*CLIP_SIZE, 3*CLIP_SIZE, CLIP_SIZE, CLIP_SIZE);
    comac_clip (cr);

    comac_translate (cr, .5, .5);

    comac_reset_clip (cr);
    comac_rectangle (cr, CLIP_SIZE, CLIP_SIZE, CLIP_SIZE, CLIP_SIZE);
    comac_clip (cr);
    comac_rectangle (cr, 3*CLIP_SIZE, 3*CLIP_SIZE, CLIP_SIZE, CLIP_SIZE);
    comac_clip (cr);

    comac_rectangle (cr, 0, 0, SIZE, SIZE);
    comac_set_source_rgb (cr, 1, 1, 0);
    comac_fill (cr);

    /* https://bugs.freedesktop.org/show_bug.cgi?id=13084 */
    comac_select_font_face (cr,
	                    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    comac_move_to (cr, 0., SIZE);
    comac_show_text (cr, "comac");


    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_all,
	    "Test clipping with everything clipped out",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    SIZE, SIZE,
	    NULL, draw)

