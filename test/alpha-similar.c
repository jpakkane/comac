/*
 * Copyright 2008 Chris Wilson
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

static comac_surface_t *
create_source (comac_surface_t *target, int width, int height)
{
    comac_surface_t *similar;
    comac_t *cr;

    similar = comac_surface_create_similar (target,
					    COMAC_CONTENT_ALPHA,
					    width, height);
    cr = comac_create (similar);
    comac_surface_destroy (similar);

    comac_set_source_rgba (cr, 1, 0, 0, .5);
    comac_paint (cr);

    similar = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return similar;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *source;

    source = create_source (comac_get_target (cr), width, height);
    comac_set_source_surface (cr, source, 0, 0);
    comac_surface_destroy (source);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

/*
 * XFAIL: discrepancy between backends in applying color components of a pure
 * alpha surface
 */
COMAC_TEST (alpha_similar,
	    "Tests creation of similar alpha surfaces"
	    "\nApplication of a pure-alpha similar source is inconsistent across backends.",
	    "alpha, similar", /* keywords */
	    NULL, /* requirements */
	    10, 10,
	    NULL, draw)

