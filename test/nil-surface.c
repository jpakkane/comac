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
#include <stddef.h>

/* Test to verify fixes for the following similar bugs:
 *
 *	https://bugs.freedesktop.org/show_bug.cgi?id=4088
 *	https://bugs.freedesktop.org/show_bug.cgi?id=3915
 *	https://bugs.freedesktop.org/show_bug.cgi?id=9906
 */

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    comac_t *cr2;

    /*
     * 1. Test file-not-found from surface->pattern->comac_t
     */

    /* Make a custom context to not interfere with the one passed in. */
    cr2 = comac_create (comac_get_target (cr));

    /* First, let's make a nil surface. */
    surface = comac_image_surface_create_from_png ("___THIS_FILE_DOES_NOT_EXIST___");

    /* Let the error propagate into a nil pattern. */
    pattern = comac_pattern_create_for_surface (surface);

    /* Then let it propagate into the comac_t. */
    comac_set_source (cr2, pattern);
    comac_paint (cr2);

    comac_pattern_destroy (pattern);
    comac_surface_destroy (surface);

    /* Check that the error made it all that way. */
    if (comac_status (cr2) != COMAC_STATUS_FILE_NOT_FOUND) {
	comac_test_log (ctx, "Error: Received status of \"%s\" rather than expected \"%s\"\n",
			comac_status_to_string (comac_status (cr2)),
			comac_status_to_string (COMAC_STATUS_FILE_NOT_FOUND));
	comac_destroy (cr2);
	return COMAC_TEST_FAILURE;
    }

    comac_destroy (cr2);

    /*
     * 2. Test NULL pointer pattern->comac_t
     */
    cr2 = comac_create (comac_get_target (cr));

    /* First, trigger the NULL pointer status. */
    pattern = comac_pattern_create_for_surface (NULL);

    /* Then let it propagate into the comac_t. */
    comac_set_source (cr2, pattern);
    comac_paint (cr2);

    comac_pattern_destroy (pattern);

    /* Check that the error made it all that way. */
    if (comac_status (cr2) != COMAC_STATUS_NULL_POINTER) {
	comac_test_log (ctx, "Error: Received status of \"%s\" rather than expected \"%s\"\n",
			comac_status_to_string (comac_status (cr2)),
			comac_status_to_string (COMAC_STATUS_NULL_POINTER));
	comac_destroy (cr2);
	return COMAC_TEST_FAILURE;
    }

    comac_destroy (cr2);

    /*
     * 3. Test that comac_surface_finish can accept NULL or a nil
     *    surface without crashing.
     */

    comac_surface_finish (NULL);

    surface = comac_image_surface_create_from_png ("___THIS_FILE_DOES_NOT_EXIST___");
    comac_surface_finish (surface);
    comac_surface_destroy (surface);

    /*
     * 4. OK, we're straying from the original name, but it's still a
     * similar kind of testing of error paths. Here we're making sure
     * we can still call a comac_get_* function after triggering an
     * INVALID_RESTORE error.
     */
    cr2 = comac_create (comac_get_target (cr));

    /* Trigger invalid restore. */
    comac_restore (cr2);
    if (comac_status (cr2) != COMAC_STATUS_INVALID_RESTORE) {
	comac_test_log (ctx, "Error: Received status of \"%s\" rather than expected \"%s\"\n",
			comac_status_to_string (comac_status (cr2)),
			comac_status_to_string (COMAC_STATUS_INVALID_RESTORE));
	comac_destroy (cr2);
	return COMAC_TEST_FAILURE;
    }

    /* Test that we can still call comac_get_fill_rule without crashing. */
    comac_get_fill_rule (cr2);

    comac_destroy (cr2);

    /*
     * 5. Create a comac_t for the NULL surface.
     */
    cr2 = comac_create (NULL);

    if (comac_status (cr2) != COMAC_STATUS_NULL_POINTER) {
	comac_test_log (ctx, "Error: Received status of \"%s\" rather than expected \"%s\"\n",
			comac_status_to_string (comac_status (cr2)),
			comac_status_to_string (COMAC_STATUS_NULL_POINTER));
	comac_destroy (cr2);
	return COMAC_TEST_FAILURE;
    }

    /* Test that get_target returns something valid */
    if (comac_get_target (cr2) == NULL) {
	comac_test_log (ctx, "Error: comac_get_target() returned NULL\n");
	comac_destroy (cr2);
	return COMAC_TEST_FAILURE;
    }

    /* Test that push_group doesn't crash */
    comac_push_group (cr2);
    comac_stroke (cr2);
    pattern = comac_pop_group (cr2);
    comac_pattern_destroy (pattern);

    comac_destroy (cr2);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (nil_surface,
	    "Test that nil surfaces do not make comac crash.",
	    "api", /* keywords */
	    NULL, /* requirements */
	    1, 1,
	    NULL, draw)
