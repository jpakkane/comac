/*
 * Copyright © 2004 Red Hat, Inc.
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

/* Bug history
 *
 * 2004-11-04 Ned Konz <ned@squeakland.org>
 *
 *   Reported bug on mailing list:
 *
 *	From: Ned Konz <ned@squeakland.org>
 *	To: comac@comacgraphics.org
 *	Date: Thu, 4 Nov 2004 09:49:38 -0800
 *	Subject: [comac] getting assertions [comac_cache.c:143: _entry_destroy:
 *	        Assertion `cache->used_memory > entry->memory' failed]
 *
 *	The attached program dies on me with the assert
 *
 *	$ ./testComac
 *	testComac: comac_cache.c:143: _entry_destroy: Assertion `cache->used_memory > entry->memory' failed.
 *
 * 2004-11-04 Carl Worth <cworth@cworth.org>
 *
 *   I trimmed down Ned's example to the following test while still
 *   maintaining the assertion.
 *
 *   Oh, actually, it looks like I may have triggered something
 *   slightly different:
 *
 *	text_cache_crash: comac_cache.c:422: _comac_cache_lookup: Assertion `cache->max_memory >= (cache->used_memory + new_entry->memory)' failed.
 *
 *   I'll have to go back and try the original test after I fix this.
 *
 * 2004-11-13 Carl Worth <cworth@cworth.org>
 *
 *   Found the bug. comac_gstate_select_font was noticing when the
 *   same font was selected twice in a row and was erroneously failing
 *   to free the old reference. Committed a fix and verified it also
 *   fixed the original test case.
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* Once there was a bug that choked when selecting the same font twice. */
    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_BOLD);
    comac_set_font_size (cr, 40.0);

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_BOLD);
    comac_set_font_size (cr, 40.0);
    comac_move_to (cr, 10, 50);
    comac_show_text (cr, "hello");

    /* Then there was a bug that choked when selecting a font too big
     * for the cache. */

    comac_set_font_size (cr, 500);
    comac_show_text (cr, "hello");

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    text_cache_crash,
    "Test case for bug causing an assertion failure in _comac_cache_lookup",
    "text, stress", /* keywords */
    NULL,	    /* requirements */
    0,
    0,
    NULL,
    draw)
