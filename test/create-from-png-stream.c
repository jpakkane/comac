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
 * Author: Carl Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define WIDTH 2
#define HEIGHT 2

static comac_status_t
read_png_from_file (void *closure, unsigned char *data, unsigned int length)
{
    FILE *file = closure;
    size_t bytes_read;

    bytes_read = fread (data, 1, length, file);
    if (bytes_read != length)
	return COMAC_STATUS_READ_ERROR;

    return COMAC_STATUS_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    char *filename;
    FILE *file;
    comac_surface_t *surface;
    comac_status_t status;

    xasprintf (&filename, "%s/reference/%s", ctx->srcdir,
	       "create-from-png-stream.ref.png");

    file = fopen (filename, "rb");
    if (file == NULL) {
	comac_test_status_t ret;

	ret = COMAC_TEST_FAILURE;
	if (errno == ENOMEM)
	    ret = comac_test_status_from_status (ctx, COMAC_STATUS_NO_MEMORY);

	if (ret != COMAC_TEST_NO_MEMORY)
	    comac_test_log (ctx, "Error: failed to open file: %s\n", filename);

	free (filename);
	return ret;
    }

    surface = comac_image_surface_create_from_png_stream (read_png_from_file,
							  file);

    fclose (file);

    status = comac_surface_status (surface);
    if (status) {
	comac_test_status_t ret;

	comac_surface_destroy (surface);

	ret = comac_test_status_from_status (ctx, status);
	if (ret != COMAC_TEST_NO_MEMORY) {
	    comac_test_log (ctx,
			    "Error: failed to create surface from PNG: %s - %s\n",
			    filename,
			    comac_status_to_string (status));
	}

	free (filename);

	return ret;
    }

    free (filename);

    /* Pretend we modify the surface data (which detaches the PNG mime data) */
    comac_surface_flush (surface);
    comac_surface_mark_dirty (surface);

    comac_set_source_surface (cr, surface, 0, 0);
    comac_pattern_set_filter (comac_get_source (cr), COMAC_FILTER_NEAREST);
    comac_paint (cr);

    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (create_from_png_stream,
	    "Tests the creation of an image surface from a PNG using a FILE *",
	    "png", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
