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

static comac_surface_t *
create_source_surface (int size);

/* We use a relatively large source to exercise bug:
 *   Bug 7360 painting huge surfaces fails
 *   [https://bugs.freedesktop.org/show_bug.cgi?id=7360]
 * but still keep the resultant image small for reasonably quick checking.
 */
#define SOURCE_SIZE 2000
#define INTER_SIZE 512
#define SIZE 96

static void
draw_pattern (comac_surface_t **surface_inout, int surface_size)
{
    comac_t *cr;
    int mid = surface_size / 2;

    cr = comac_create (*surface_inout);
    comac_surface_destroy (*surface_inout);

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgba (cr, 0, 0, 0, 0);
    comac_paint (cr);

    comac_rectangle (cr, 0, 0, surface_size, surface_size);
    comac_rectangle (cr, mid - SIZE / 4, mid + SIZE / 4, SIZE / 2, -SIZE / 2);
    comac_clip (cr);

    /* outside squares -> opaque */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_rectangle (cr, 0, 0, surface_size / 2, surface_size / 2);
    comac_fill (cr);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_rectangle (cr,
		     surface_size / 2,
		     0,
		     surface_size / 2,
		     surface_size / 2);
    comac_fill (cr);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_rectangle (cr,
		     0,
		     surface_size / 2,
		     surface_size / 2,
		     surface_size / 2);
    comac_fill (cr);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_rectangle (cr,
		     surface_size / 2,
		     surface_size / 2,
		     surface_size / 2,
		     surface_size / 2);
    comac_fill (cr);

    comac_reset_clip (cr);
    comac_rectangle (cr, mid - SIZE / 4, mid - SIZE / 4, SIZE / 2, SIZE / 2);
    comac_clip (cr);

    /* inside squares -> translucent */
    comac_set_source_rgba (cr, 0, 0, 1, .5);
    comac_rectangle (cr, 0, 0, surface_size / 2, surface_size / 2);
    comac_fill (cr);
    comac_set_source_rgba (cr, 0, 1, 0, .5);
    comac_rectangle (cr,
		     surface_size / 2,
		     0,
		     surface_size / 2,
		     surface_size / 2);
    comac_fill (cr);
    comac_set_source_rgba (cr, 1, 0, 0, .5);
    comac_rectangle (cr,
		     0,
		     surface_size / 2,
		     surface_size / 2,
		     surface_size / 2);
    comac_fill (cr);
    comac_set_source_rgba (cr, 1, 1, 1, .5);
    comac_rectangle (cr,
		     surface_size / 2,
		     surface_size / 2,
		     surface_size / 2,
		     surface_size / 2);
    comac_fill (cr);

    *surface_inout = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface;
    comac_surface_t *similar;
    comac_status_t status;
    comac_t *cr2;

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    surface = create_source_surface (SOURCE_SIZE);
    if (surface == NULL) /* can't create the source so skip the test */
	return COMAC_TEST_UNTESTED;

    draw_pattern (&surface, SOURCE_SIZE);

    /* copy a subregion to a smaller intermediate surface */
    similar = comac_surface_create_similar (surface,
					    COMAC_CONTENT_COLOR_ALPHA,
					    INTER_SIZE,
					    INTER_SIZE);
    cr2 = comac_create (similar);
    comac_surface_destroy (similar);
    comac_set_source_surface (cr2,
			      surface,
			      (INTER_SIZE - SOURCE_SIZE) / 2,
			      (INTER_SIZE - SOURCE_SIZE) / 2);
    comac_paint (cr2);

    /* and then paint onto a small surface for checking */
    comac_set_source_surface (cr,
			      comac_get_target (cr2),
			      (width - INTER_SIZE) / 2,
			      (height - INTER_SIZE) / 2);
    comac_destroy (cr2);
    comac_rectangle (cr, 16, 16, 64, 64);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_fill (cr);

    /* destroy the surface last, as this triggers XCloseDisplay */
    comac_surface_finish (surface);
    status = comac_surface_status (surface);
    comac_surface_destroy (surface);

    return comac_test_status_from_status (comac_test_get_context (cr), status);
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface;
    comac_status_t status;

    surface = create_source_surface (SOURCE_SIZE);
    if (surface == NULL) /* can't create the source so skip the test */
	return COMAC_TEST_UNTESTED;

    comac_surface_finish (surface);
    status = comac_surface_status (surface);
    comac_surface_destroy (surface);

    return comac_test_status_from_status (ctx, status);
}
