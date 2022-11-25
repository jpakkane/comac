/*
 * Copyright © 2006 Red Hat, Inc.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#if COMAC_HAS_PS_SURFACE
#include <comac-ps.h>
#endif

#if COMAC_HAS_PDF_SURFACE
#include <comac-pdf.h>
#endif

#if COMAC_HAS_SVG_SURFACE
#include <comac-svg.h>
#endif

#include "comac-test.h"

/* The main test suite doesn't test the *_create_for_stream
 * constructors for the PDF, PS and SVG surface, so we do that here.
 * We draw to an in-memory buffer using the stream constructor and
 * compare the output to the contents of a file written using the
 * file constructor.
 */

#define MAX_OUTPUT_SIZE 4096

#define WIDTH_IN_INCHES 3
#define HEIGHT_IN_INCHES 3
#define WIDTH_IN_POINTS (WIDTH_IN_INCHES * 72.0)
#define HEIGHT_IN_POINTS (HEIGHT_IN_INCHES * 72.0)

#define BASENAME "create-for-stream.out"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    /* Just draw a rectangle. */

    comac_rectangle (cr,
		     width / 10.,
		     height / 10.,
		     width - 2 * width / 10.,
		     height - 2 * height / 10.);
    comac_fill (cr);

    comac_show_page (cr);

    return COMAC_TEST_SUCCESS;
}

static void
draw_to (comac_surface_t *surface)
{
    comac_t *cr;

    cr = comac_create (surface);

    draw (cr, WIDTH_IN_POINTS, HEIGHT_IN_POINTS);

    comac_destroy (cr);
}

typedef struct _write_closure {
    const comac_test_context_t *ctx;
    char buffer[MAX_OUTPUT_SIZE];
    size_t index;
    comac_test_status_t status;
} write_closure_t;

static comac_status_t
bad_write (void *closure, const unsigned char *data, unsigned int length)
{
    return COMAC_STATUS_WRITE_ERROR;
}

static comac_status_t
test_write (void *closure, const unsigned char *data, unsigned int length)
{
    write_closure_t *wc = closure;

    if (wc->index + length >= sizeof wc->buffer) {
	comac_test_log (wc->ctx, "Error: out of bounds in write callback\n");
	wc->status = COMAC_TEST_FAILURE;
	return COMAC_STATUS_SUCCESS;
    }

    memcpy (&wc->buffer[wc->index], data, length);
    wc->index += length;

    return COMAC_STATUS_SUCCESS;
}

typedef comac_surface_t *(*file_constructor_t) (const char *filename,
						double width_in_points,
						double height_in_points);

typedef comac_surface_t *(*stream_constructor_t) (comac_write_func_t write_func,
						  void *closure,
						  double width_in_points,
						  double height_in_points);

static comac_test_status_t
test_surface (const comac_test_context_t *ctx,
	      const char *backend,
	      const char *filename,
	      file_constructor_t file_constructor,
	      stream_constructor_t stream_constructor)
{
    comac_surface_t *surface;
    write_closure_t wc;
    char file_contents[MAX_OUTPUT_SIZE];
    comac_status_t status;
    FILE *fp;

    /* test propagation of user errors */
    surface =
	stream_constructor (bad_write, &wc, WIDTH_IN_POINTS, HEIGHT_IN_POINTS);

    status = comac_surface_status (surface);
    if (status) {
	comac_test_log (ctx,
			"%s: Failed to create surface for stream.\n",
			backend);
	return COMAC_TEST_FAILURE;
    }

    draw_to (surface);

    comac_surface_finish (surface);
    status = comac_surface_status (surface);
    comac_surface_destroy (surface);

    if (status != COMAC_STATUS_WRITE_ERROR) {
	comac_test_log (ctx,
			"%s: Error: expected \"write error\" from bad_write(), "
			"but received \"%s\".\n",
			backend,
			comac_status_to_string (status));
	return COMAC_TEST_FAILURE;
    }

    /* test propagation of file errors - for now this is unix-only */
#ifndef _WIN32
    if (access ("/dev/full", W_OK) == 0) {
	surface =
	    file_constructor ("/dev/full", WIDTH_IN_POINTS, HEIGHT_IN_POINTS);
	comac_surface_finish (surface);
	status = comac_surface_status (surface);
	comac_surface_destroy (surface);

	if (status != COMAC_STATUS_WRITE_ERROR) {
	    comac_test_log (ctx,
			    "%s: Error: expected \"write error\" from "
			    "/dev/full, but received \"%s\".\n",
			    backend,
			    comac_status_to_string (status));
	    return COMAC_TEST_FAILURE;
	}
    } else {
	comac_test_log (ctx,
			"/dev/full does not exist; skipping write test.\n");
    }
#endif

    /* construct the real surface */
    wc.ctx = ctx;
    wc.status = COMAC_TEST_SUCCESS;
    wc.index = 0;

    surface =
	stream_constructor (test_write, &wc, WIDTH_IN_POINTS, HEIGHT_IN_POINTS);

    status = comac_surface_status (surface);
    if (status) {
	comac_test_log (ctx,
			"%s: Failed to create surface for stream.\n",
			backend);
	return COMAC_TEST_FAILURE;
    }

    draw_to (surface);

    comac_surface_destroy (surface);

    if (wc.status != COMAC_TEST_SUCCESS) {
	/* Error already reported. */
	return wc.status;
    }

    surface = file_constructor (filename, WIDTH_IN_POINTS, HEIGHT_IN_POINTS);

    status = comac_surface_status (surface);
    if (status) {
	comac_test_log (ctx,
			"%s: Failed to create surface for file %s: %s.\n",
			backend,
			filename,
			comac_status_to_string (status));
	return COMAC_TEST_FAILURE;
    }

    draw_to (surface);

    comac_surface_destroy (surface);

    fp = fopen (filename, "r");
    if (fp == NULL) {
	comac_test_log (ctx,
			"%s: Failed to open %s for reading: %s.\n",
			backend,
			filename,
			strerror (errno));
	return COMAC_TEST_FAILURE;
    }

    if (fread (file_contents, 1, wc.index, fp) != wc.index) {
	comac_test_log (ctx,
			"%s: Failed to read %s: %s.\n",
			backend,
			filename,
			strerror (errno));
	fclose (fp);
	return COMAC_TEST_FAILURE;
    }

    if (memcmp (file_contents, wc.buffer, wc.index) != 0) {
	comac_test_log (
	    ctx,
	    "%s: Stream based output differ from file output for %s.\n",
	    backend,
	    filename);
	fclose (fp);
	return COMAC_TEST_FAILURE;
    }

    fclose (fp);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_test_status_t status = COMAC_TEST_UNTESTED;
    comac_test_status_t test_status;
    char *filename;
    const char *path =
	comac_test_mkdir (COMAC_TEST_OUTPUT_DIR) ? COMAC_TEST_OUTPUT_DIR : ".";

#if COMAC_HAS_PS_SURFACE
    if (comac_test_is_target_enabled (ctx, "ps2") ||
	comac_test_is_target_enabled (ctx, "ps3")) {
	if (status == COMAC_TEST_UNTESTED)
	    status = COMAC_TEST_SUCCESS;

	xasprintf (&filename, "%s/%s", path, BASENAME ".ps");
	test_status = test_surface (ctx,
				    "ps",
				    filename,
				    comac_ps_surface_create,
				    comac_ps_surface_create_for_stream);
	comac_test_log (ctx,
			"TEST: %s TARGET: %s RESULT: %s\n",
			ctx->test->name,
			"ps",
			test_status ? "FAIL" : "PASS");
	if (status == COMAC_TEST_SUCCESS)
	    status = test_status;
	free (filename);
    }
#endif

#if COMAC_HAS_PDF_SURFACE
    if (comac_test_is_target_enabled (ctx, "pdf")) {
	if (status == COMAC_TEST_UNTESTED)
	    status = COMAC_TEST_SUCCESS;

	xasprintf (&filename, "%s/%s", path, BASENAME ".pdf");
	test_status = test_surface (ctx,
				    "pdf",
				    filename,
				    comac_pdf_surface_create,
				    comac_pdf_surface_create_for_stream);
	comac_test_log (ctx,
			"TEST: %s TARGET: %s RESULT: %s\n",
			ctx->test->name,
			"pdf",
			test_status ? "FAIL" : "PASS");
	if (status == COMAC_TEST_SUCCESS)
	    status = test_status;
	free (filename);
    }
#endif

#if COMAC_HAS_SVG_SURFACE
    if (comac_test_is_target_enabled (ctx, "svg11") ||
	comac_test_is_target_enabled (ctx, "svg12")) {
	if (status == COMAC_TEST_UNTESTED)
	    status = COMAC_TEST_SUCCESS;

	xasprintf (&filename, "%s/%s", path, BASENAME ".svg");
	test_status = test_surface (ctx,
				    "svg",
				    filename,
				    comac_svg_surface_create,
				    comac_svg_surface_create_for_stream);
	comac_test_log (ctx,
			"TEST: %s TARGET: %s RESULT: %s\n",
			ctx->test->name,
			"svg",
			test_status ? "FAIL" : "PASS");
	if (status == COMAC_TEST_SUCCESS)
	    status = test_status;
	free (filename);
    }
#endif

    return status;
}

COMAC_TEST (create_for_stream,
	    "Checks creating vector surfaces with user defined I/O\n",
	    "stream",	     /* keywords */
	    "target=vector", /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
