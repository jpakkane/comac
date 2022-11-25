/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2008 Chris Wilson
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
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <comac.h>

#if COMAC_HAS_PDF_SURFACE
#include <comac-pdf.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#include <errno.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "comac-test.h"
#include "buffer-diff.h"

/* This test exists to test comac_surface_set_fallback_resolution
 *
 * <behdad> one more thing.
 *          if you can somehow incorporate comac_show_page stuff in the
 *          test suite.  such that fallback-resolution can actually be
 *          automated..
 *          if we could get a callback on surface when that function is
 *          called, we could do cool stuff like making other backends
 *          draw a long strip of images, one for each page...
 */

#define INCHES_TO_POINTS(in) ((in) * 72.0)
#define SIZE INCHES_TO_POINTS(2)

/* comac_set_tolerance() is not respected by the PS/PDF backends currently */
#define SET_TOLERANCE 0

#define GENERATE_REFERENCE 0

static void
draw (comac_t *cr, double width, double height)
{
    const char *text = "comac";
    comac_text_extents_t extents;
    const double dash[2] = { 8, 16 };
    comac_pattern_t *pattern;

    comac_save (cr);

    comac_new_path (cr);

    comac_set_line_width (cr, .05 * SIZE / 2.0);

    comac_arc (cr, SIZE / 2.0, SIZE / 2.0,
	       0.875 * SIZE / 2.0,
	       0, 2.0 * M_PI);
    comac_stroke (cr);

    /* use dashes to demonstrate bugs:
     *  https://bugs.freedesktop.org/show_bug.cgi?id=9189
     *  https://bugs.freedesktop.org/show_bug.cgi?id=17223
     */
    comac_save (cr);
    comac_set_dash (cr, dash, 2, 0);
    comac_arc (cr, SIZE / 2.0, SIZE / 2.0,
	       0.75 * SIZE / 2.0,
	       0, 2.0 * M_PI);
    comac_stroke (cr);
    comac_restore (cr);

    comac_save (cr);
    comac_rectangle (cr, 0, 0, SIZE/2, SIZE);
    comac_clip (cr);
    comac_arc (cr, SIZE / 2.0, SIZE / 2.0,
	       0.6 * SIZE / 2.0,
	       0, 2.0 * M_PI);
    comac_fill (cr);
    comac_restore (cr);

    /* use a pattern to exercise bug:
     *   https://bugs.launchpad.net/inkscape/+bug/234546
     */
    comac_save (cr);
    comac_rectangle (cr, SIZE/2, 0, SIZE/2, SIZE);
    comac_clip (cr);
    pattern = comac_pattern_create_linear (SIZE/2, 0, SIZE, 0);
    comac_pattern_add_color_stop_rgba (pattern, 0, 0, 0, 0, 1.);
    comac_pattern_add_color_stop_rgba (pattern, 1, 0, 0, 0, 0.);
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);
    comac_arc (cr, SIZE / 2.0, SIZE / 2.0,
	       0.6 * SIZE / 2.0,
	       0, 2.0 * M_PI);
    comac_fill (cr);
    comac_restore (cr);

    comac_set_source_rgb (cr, 1, 1, 1); /* white */
    comac_set_font_size (cr, .25 * SIZE / 2.0);
    comac_text_extents (cr, text, &extents);
    comac_move_to (cr, (SIZE-extents.width)/2.0-extents.x_bearing,
		       (SIZE-extents.height)/2.0-extents.y_bearing);
    comac_show_text (cr, text);

    comac_restore (cr);
}

static void
_xunlink (const comac_test_context_t *ctx, const char *pathname)
{
    if (unlink (pathname) < 0 && errno != ENOENT) {
	comac_test_log (ctx, "Error: Cannot remove %s: %s\n",
			pathname, strerror (errno));
	exit (1);
    }
}

static comac_bool_t
check_result (comac_test_context_t *ctx,
	      const comac_boilerplate_target_t *target,
	      const char *test_name,
	      const char *base_name,
	      comac_surface_t *surface)
{
    const char *format;
    char *ref_name;
    char *png_name;
    char *diff_name;
    comac_surface_t *test_image, *ref_image, *diff_image;
    buffer_diff_result_t result;
    comac_status_t status;
    comac_bool_t ret;

    /* XXX log target, OUTPUT, REFERENCE, DIFFERENCE for index.html */

    if (target->finish_surface != NULL) {
	status = target->finish_surface (surface);
	if (status) {
	    comac_test_log (ctx, "Error: Failed to finish surface: %s\n",
		    comac_status_to_string (status));
	    comac_surface_destroy (surface);
	    return FALSE;
	}
    }

    xasprintf (&png_name,  "%s.out.png", base_name);
    xasprintf (&diff_name, "%s.diff.png", base_name);

    test_image = target->get_image_surface (surface, 0, SIZE, SIZE);
    if (comac_surface_status (test_image)) {
	comac_test_log (ctx, "Error: Failed to extract page: %s\n",
		        comac_status_to_string (comac_surface_status (test_image)));
	comac_surface_destroy (test_image);
	free (png_name);
	free (diff_name);
	return FALSE;
    }

    _xunlink (ctx, png_name);
    status = comac_surface_write_to_png (test_image, png_name);
    if (status) {
	comac_test_log (ctx, "Error: Failed to write output image: %s\n",
		comac_status_to_string (status));
	comac_surface_destroy (test_image);
	free (png_name);
	free (diff_name);
	return FALSE;
    }

    format = comac_boilerplate_content_name (target->content);
    ref_name = comac_test_reference_filename (ctx,
					      base_name,
					      test_name,
					      target->name,
					      target->basename,
					      format,
					      COMAC_TEST_REF_SUFFIX,
					      COMAC_TEST_PNG_EXTENSION);
    if (ref_name == NULL) {
	comac_test_log (ctx, "Error: Cannot find reference image for %s\n",
		        base_name);
	comac_surface_destroy (test_image);
	free (png_name);
	free (diff_name);
	return FALSE;
    }


    ref_image = comac_test_get_reference_image (ctx, ref_name,
	    target->content == COMAC_TEST_CONTENT_COLOR_ALPHA_FLATTENED);
    if (comac_surface_status (ref_image)) {
	comac_test_log (ctx, "Error: Cannot open reference image for %s: %s\n",
		        ref_name,
		comac_status_to_string (comac_surface_status (ref_image)));
	comac_surface_destroy (ref_image);
	comac_surface_destroy (test_image);
	free (png_name);
	free (diff_name);
	free (ref_name);
	return FALSE;
    }

    diff_image = comac_image_surface_create (COMAC_FORMAT_ARGB32,
	    SIZE, SIZE);

    ret = TRUE;
    status = image_diff (ctx,
	    test_image, ref_image, diff_image,
	    &result);
    _xunlink (ctx, diff_name);
    if (status) {
	comac_test_log (ctx, "Error: Failed to compare images: %s\n",
			comac_status_to_string (status));
	ret = FALSE;
    } else if (image_diff_is_failure (&result, target->error_tolerance))
    {
	ret = FALSE;

	status = comac_surface_write_to_png (diff_image, diff_name);
	if (status) {
	    comac_test_log (ctx, "Error: Failed to write differences image: %s\n",
		    comac_status_to_string (status));
	}
    }

    comac_surface_destroy (test_image);
    comac_surface_destroy (diff_image);
    free (png_name);
    free (diff_name);
    free (ref_name);

    return ret;
}

#if GENERATE_REFERENCE
static void
generate_reference (double ppi_x, double ppi_y, const char *filename)
{
    comac_surface_t *surface, *target;
    comac_t *cr;
    comac_status_t status;

    surface = comac_image_surface_create (COMAC_FORMAT_RGB24,
	                                  SIZE*ppi_x/72, SIZE*ppi_y/72);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    /* As we wish to mimic a PDF surface, copy across the default font options
     * from the PDF backend.
     */
    {
	comac_surface_t *pdf;
	comac_font_options_t *options;

	options = comac_font_options_create ();

#if COMAC_HAS_PDF_SURFACE
	pdf = comac_pdf_surface_create ("tmp.pdf", 1, 1);
	comac_surface_get_font_options (pdf, options);
	comac_surface_destroy (pdf);
#endif

	comac_set_font_options (cr, options);
	comac_font_options_destroy (options);
    }

#if SET_TOLERANCE
    comac_set_tolerance (cr, 3.0);
#endif

    comac_save (cr); {
	comac_set_source_rgb (cr, 1, 1, 1);
	comac_paint (cr);
    } comac_restore (cr);

    comac_scale (cr, ppi_x/72., ppi_y/72.);
    draw (cr, SIZE, SIZE);

    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    target = comac_image_surface_create (COMAC_FORMAT_RGB24, SIZE, SIZE);
    cr = comac_create (target);
    comac_scale (cr, 72./ppi_x, 72./ppi_y);
    comac_set_source_surface (cr, surface, 0, 0);
    comac_paint (cr);

    status = comac_surface_write_to_png (comac_get_target (cr), filename);
    comac_destroy (cr);

    if (status) {
	fprintf (stderr, "Failed to generate reference image '%s': %s\n",
		 filename, comac_status_to_string (status));
	exit (1);
    }
}
#endif

/* TODO: Split each ppi case out to its own COMAC_TEST() test case */
static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_t *cr;
    comac_test_status_t ret = COMAC_TEST_UNTESTED;
    struct {
	double x, y;
    } ppi[] = {
	{ 576, 576 },
	{ 576, 72 },

	{ 288, 288 },
	{ 288, 72 },

	{ 144, 144 },
	{ 144, 72 },

	{ 72, 576 },
	{ 72, 288 },
	{ 72, 144 },
	{ 72, 72 },
    };
    unsigned int i;
    int n, num_ppi;
    const char *path = comac_test_mkdir (COMAC_TEST_OUTPUT_DIR) ? COMAC_TEST_OUTPUT_DIR : ".";

    num_ppi = ARRAY_LENGTH (ppi);

#if GENERATE_REFERENCE
    for (n = 0; n < num_ppi; n++) {
	char *ref_name;
	xasprintf (&ref_name, "reference/fallback-resolution.ppi%gx%g.ref.png",
		   ppi[n].x, ppi[n].y);
	generate_reference (ppi[n].x, ppi[n].y, ref_name);
	free (ref_name);
    }
#endif

    for (i = 0; i < ctx->num_targets; i++) {
	const comac_boilerplate_target_t *target = ctx->targets_to_test[i];
	comac_surface_t *surface = NULL;
	char *base_name;
	void *closure;
	const char *format;
	comac_status_t status;

	if (! target->is_vector)
	    continue;

	if (! comac_test_is_target_enabled (ctx, target->name))
	    continue;

	format = comac_boilerplate_content_name (target->content);
	xasprintf (&base_name, "%s/fallback-resolution.%s.%s",
		   path, target->name,
		   format);

	surface = (target->create_surface) (base_name,
					    target->content,
					    SIZE, SIZE,
					    SIZE, SIZE,
					    COMAC_BOILERPLATE_MODE_TEST,
					    &closure);

	if (surface == NULL) {
	    free (base_name);
	    continue;
	}

	if (ret == COMAC_TEST_UNTESTED)
	    ret = COMAC_TEST_SUCCESS;

	comac_surface_destroy (surface);
	if (target->cleanup)
	    target->cleanup (closure);
	free (base_name);

	/* we need to recreate the surface for each resolution as we include
	 * SVG in testing which does not support the paginated interface.
	 */
	for (n = 0; n < num_ppi; n++) {
	    char *test_name;
	    comac_bool_t pass;

	    xasprintf (&test_name, "fallback-resolution.ppi%gx%g",
		       ppi[n].x, ppi[n].y);
	    xasprintf (&base_name, "%s/%s.%s.%s",
		       path, test_name,
		       target->name,
		       format);

	    surface = (target->create_surface) (base_name,
						target->content,
						SIZE + 25, SIZE + 25,
						SIZE + 25, SIZE + 25,
						COMAC_BOILERPLATE_MODE_TEST,
						&closure);
	    if (surface == NULL || comac_surface_status (surface)) {
		comac_test_log (ctx, "Failed to generate surface: %s.%s\n",
				target->name,
				format);
		free (base_name);
		free (test_name);
		ret = COMAC_TEST_FAILURE;
		continue;
	    }

	    comac_test_log (ctx,
			    "Testing fallback-resolution %gx%g with %s target\n",
			    ppi[n].x, ppi[n].y, target->name);
	    printf ("%s:\t", base_name);
	    fflush (stdout);

	    if (target->force_fallbacks != NULL)
		target->force_fallbacks (surface, ppi[n].x, ppi[n].y);
	    cr = comac_create (surface);
#if SET_TOLERANCE
	    comac_set_tolerance (cr, 3.0);
#endif

	    comac_surface_set_device_offset (surface, 25, 25);

	    comac_save (cr); {
		comac_set_source_rgb (cr, 1, 1, 1);
		comac_paint (cr);
	    } comac_restore (cr);

	    /* First draw the top half in a conventional way. */
	    comac_save (cr); {
		comac_rectangle (cr, 0, 0, SIZE, SIZE / 2.0);
		comac_clip (cr);

		draw (cr, SIZE, SIZE);
	    } comac_restore (cr);

	    /* Then draw the bottom half in a separate group,
	     * (exposing a bug in 1.6.4 with the group not being
	     * rendered with the correct fallback resolution). */
	    comac_save (cr); {
		comac_rectangle (cr, 0, SIZE / 2.0, SIZE, SIZE / 2.0);
		comac_clip (cr);

		comac_push_group (cr); {
		    draw (cr, SIZE, SIZE);
		} comac_pop_group_to_source (cr);

		comac_paint (cr);
	    } comac_restore (cr);

	    status = comac_status (cr);
	    comac_destroy (cr);

	    pass = FALSE;
	    if (status) {
		comac_test_log (ctx, "Error: Failed to create target surface: %s\n",
				comac_status_to_string (status));
		ret = COMAC_TEST_FAILURE;
	    } else {
		/* extract the image and compare it to our reference */
		if (! check_result (ctx, target, test_name, base_name, surface))
		    ret = COMAC_TEST_FAILURE;
		else
		    pass = TRUE;
	    }
	    comac_surface_destroy (surface);
	    if (target->cleanup)
		target->cleanup (closure);

	    free (base_name);
	    free (test_name);

	    if (pass) {
		printf ("PASS\n");
	    } else {
		printf ("FAIL\n");
	    }
	    fflush (stdout);
	}
    }

    return ret;
}

COMAC_TEST (fallback_resolution,
	    "Check handling of fallback resolutions",
	    "fallback", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
