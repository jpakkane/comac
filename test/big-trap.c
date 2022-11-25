/*
 * Copyright © 2006 Mozilla Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Mozilla Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Mozilla Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * MOZILLA CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MOZILLA CORPORATION BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Vladimir Vukicevic <vladimir@pobox.com>
 */

#include "comac-test.h"

/* This test was originally written to exercise a bug in pixman in
 * which it would scribble all over memory when given a particular
 * (and bogus) trapezoid. However, a recent change to
 * _comac_fixed_from_double changed the details of the bogus trapezoid
 * (it overflows in a different way now), so the bug is being masked.
 *
 * According to Vladimir, (https://lists.freedesktop.org/archives/comac/2006-November/008482.html):
 *
 *	Before the change, the two trapezoids that were generated were:
 *
 *	Trap[0]: T: 0x80000000 B: 0x80000003
 *	   L: [(0x000a0000, 0x80000000) (0x00080000, 0x00080000)]
 *	   R: [(0x01360000, 0x80000000) (0x01380000, 0x00080000)]
 *	Trap[1]: T: 0x80000003 B: 0x00080000
 *	   L: [(0x000a0000, 0x80000000) (0x00080000, 0x00080000)]
 *	   R: [(0x01360000, 0x80000000) (0x01380000, 0x00080000)]
 *
 *	After the change, the L/R coordinates are identical for both traps, but
 *	the top and bottom change:
 *
 *	Trap[0]: t: 0x80000000 b: 0xfda80003
 *	   l: [(0x000a0000, 0x80000000) (0x00080000, 0x00080000)]
 *	   r: [(0x01360000, 0x80000000) (0x01380000, 0x00080000)]
 *	Trap[1]: t: 0xfda80003 b: 0x00080000
 *	   l: [(0x000a0000, 0x80000000) (0x00080000, 0x00080000)]
 *	   r: [(0x01360000, 0x80000000) (0x01380000, 0x00080000)]
 *
 * I think the fix we want here is to rewrite this test to call
 * directly into pixman with the trapezoid of interest, (which will
 * require adding a new way to configure comac for "testing" which
 * will prevent the hiding of internal library symbols.
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 1,1,1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0,0,0);

    /* Note that without the clip, this doesn't crash... */
    comac_new_path (cr);
    comac_rectangle (cr, 0, 0, width, height);
    comac_clip (cr);

    comac_new_path (cr);
    comac_line_to (cr, 8.0, 8.0);
    comac_line_to (cr, 312.0, 8.0);
    comac_line_to (cr, 310.0, 31378756.2666666666);
    comac_line_to (cr, 10.0, 31378756.2666666666);
    comac_line_to (cr, 8.0, 8.0);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

/* XFAIL: range overflow of fixed-point */
COMAC_TEST (big_trap,
	    "Test oversize trapezoid with a clip region"
	    "\nTest needs to be adjusted to trigger the original bug",
	    "trap", /* keywords */
	    NULL, /* requirements */
	    100, 100,
	    NULL, draw)
