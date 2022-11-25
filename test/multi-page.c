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

#include <stdio.h>

#include <comac.h>

#if COMAC_HAS_PS_SURFACE
#include <comac-ps.h>
#endif

#if COMAC_HAS_PDF_SURFACE
#include <comac-pdf.h>
#endif

/* The PostScript and PDF backends are now integrated into the main
 * test suite, so we are getting good verification of most things
 * there.
 *
 * One thing that isn't supported there yet is multi-page output. So,
 * for now we have this one-off test. There's no automatic
 * verififcation here yet, but you can manually view the output to
 * make sure it looks happy.
 */

#define WIDTH_IN_INCHES  3
#define HEIGHT_IN_INCHES 3
#define WIDTH_IN_POINTS  (WIDTH_IN_INCHES  * 72.0)
#define HEIGHT_IN_POINTS (HEIGHT_IN_INCHES * 72.0)
#define BASENAME         "multi-page.out"

static void
draw_smiley (comac_t *cr, double width, double height, double smile_ratio)
{
#define STROKE_WIDTH .04
    double size;

    double theta = M_PI / 4 * smile_ratio;
    double dx = sqrt (0.005) * cos (theta);
    double dy = sqrt (0.005) * sin (theta);

    comac_save (cr);

    if (width > height)
	size = height;
    else
	size = width;

    comac_translate (cr, (width - size) / 2.0, (height - size) / 2.0);
    comac_scale (cr, size, size);

    /* Fill face */
    comac_arc (cr, 0.5, 0.5, 0.5 - STROKE_WIDTH, 0, 2 * M_PI);
    comac_set_source_rgb (cr, 1, 1, 0);
    comac_fill_preserve (cr);

    comac_set_source_rgb (cr, 0, 0, 0);

    /* Stroke face */
    comac_set_line_width (cr, STROKE_WIDTH / 2.0);
    comac_stroke (cr);

    /* Eyes */
    comac_set_line_width (cr, STROKE_WIDTH);
    comac_arc (cr, 0.3, 0.4, STROKE_WIDTH, 0, 2 * M_PI);
    comac_fill (cr);
    comac_arc (cr, 0.7, 0.4, STROKE_WIDTH, 0, 2 * M_PI);
    comac_fill (cr);

    /* Mouth */
    comac_move_to (cr,
		   0.35 - dx, 0.75 - dy);
    comac_curve_to (cr,
		    0.35 + dx, 0.75 + dy,
		    0.65 - dx, 0.75 + dy,
		    0.65 + dx, 0.75 - dy);
    comac_stroke (cr);

    comac_restore (cr);
}

static void
draw_some_pages (comac_surface_t *surface)
{
    comac_t *cr;
    int i;

    cr = comac_create (surface);

#define NUM_FRAMES 5
    for (i=0; i < NUM_FRAMES; i++) {
	draw_smiley (cr, WIDTH_IN_POINTS, HEIGHT_IN_POINTS,
	             (double) i / (NUM_FRAMES - 1));

	/* Duplicate the last frame onto another page. (This is just a
	 * way to sneak comac_copy_page into the test).
	 */
	if (i == (NUM_FRAMES - 1))
	    comac_copy_page (cr);

	comac_show_page (cr);
    }

    comac_destroy (cr);
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface;
    comac_status_t status;
    char *filename;
    comac_test_status_t result = COMAC_TEST_UNTESTED;
    const char *path = comac_test_mkdir (COMAC_TEST_OUTPUT_DIR) ? COMAC_TEST_OUTPUT_DIR : ".";

#if COMAC_HAS_PS_SURFACE
    if (comac_test_is_target_enabled (ctx, "ps2") ||
        comac_test_is_target_enabled (ctx, "ps3"))
    {
	if (result == COMAC_TEST_UNTESTED)
	    result = COMAC_TEST_SUCCESS;

	xasprintf (&filename, "%s/%s", path, BASENAME ".ps");
	surface = comac_ps_surface_create (filename,
					   WIDTH_IN_POINTS, HEIGHT_IN_POINTS);
	status = comac_surface_status (surface);
	if (status) {
	    comac_test_log (ctx, "Failed to create ps surface for file %s: %s\n",
			    filename, comac_status_to_string (status));
	    result = COMAC_TEST_FAILURE;
	}

	draw_some_pages (surface);

	comac_surface_destroy (surface);

	printf ("multi-page: Please check %s to ensure it looks happy.\n", filename);
	free (filename);
    }
#endif

#if COMAC_HAS_PDF_SURFACE
    if (comac_test_is_target_enabled (ctx, "pdf")) {
	if (result == COMAC_TEST_UNTESTED)
	    result = COMAC_TEST_SUCCESS;

	xasprintf (&filename, "%s/%s", path, BASENAME ".pdf");
	surface = comac_pdf_surface_create (filename,
					    WIDTH_IN_POINTS, HEIGHT_IN_POINTS);
	status = comac_surface_status (surface);
	if (status) {
	    comac_test_log (ctx, "Failed to create pdf surface for file %s: %s\n",
			    filename, comac_status_to_string (status));
	    result = COMAC_TEST_FAILURE;
	}

	draw_some_pages (surface);

	comac_surface_destroy (surface);

	printf ("multi-page: Please check %s to ensure it looks happy.\n", filename);
	free (filename);
    }
#endif

    return result;
}

COMAC_TEST (multi_page,
	    "Check the paginated surfaces handle multiple pages.",
	    "paginated", /* keywords */
	    "target=vector", /* requirements */
	    0, 0,
	    preamble, NULL)
