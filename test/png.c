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

#include <assert.h>

/* Test the idempotency of write_png->read_png */

#define RGB_MASK 0x00ffffff
#define BASENAME "png.out"

static comac_bool_t
image_surface_equals (comac_surface_t *A, comac_surface_t *B)
{
    if (comac_image_surface_get_format (A) !=
	comac_image_surface_get_format (B))
	return 0;

    if (comac_image_surface_get_width (A) !=
	comac_image_surface_get_width (B))
	return 0;

    if (comac_image_surface_get_height (A) !=
	comac_image_surface_get_height (B))
	return 0;

    return 1;
}

static const char *
format_to_string (comac_format_t format)
{
    switch (format) {
    case COMAC_FORMAT_A1:     return "a1";
    case COMAC_FORMAT_A8:     return "a8";
    case COMAC_FORMAT_RGB16_565:  return "rgb16";
    case COMAC_FORMAT_RGB24:  return "rgb24";
    case COMAC_FORMAT_RGB30:  return "rgb30";
    case COMAC_FORMAT_ARGB32: return "argb32";
    case COMAC_FORMAT_RGB96F: return "rgb96f";
    case COMAC_FORMAT_RGBA128F: return "rgba128f";
    case COMAC_FORMAT_INVALID:
    default: return "???";
    }
}

static void
print_surface (const comac_test_context_t *ctx, comac_surface_t *surface)
{
    comac_test_log (ctx,
		    "%s (%dx%d)\n",
		    format_to_string (comac_image_surface_get_format (surface)),
		    comac_image_surface_get_width (surface),
		    comac_image_surface_get_height (surface));
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface0, *surface1;
    comac_status_t status;
    uint32_t argb32 = 0xdeadbede;
    char *filename;
    const char *path = comac_test_mkdir (COMAC_TEST_OUTPUT_DIR) ? COMAC_TEST_OUTPUT_DIR : ".";

    xasprintf (&filename, "%s/%s.png", path, BASENAME);
    surface0 = comac_image_surface_create_for_data ((unsigned char *) &argb32,
						    COMAC_FORMAT_ARGB32,
						    1, 1, 4);
    status = comac_surface_write_to_png (surface0, filename);
    if (status) {
	comac_test_log (ctx, "Error writing '%s': %s\n",
			filename, comac_status_to_string (status));

	comac_surface_destroy (surface0);
	free (filename);
	return comac_test_status_from_status (ctx, status);
    }
    surface1 = comac_image_surface_create_from_png (filename);
    status = comac_surface_status (surface1);
    if (status) {
	comac_test_log (ctx, "Error reading '%s': %s\n",
			filename, comac_status_to_string (status));

	comac_surface_destroy (surface1);
	comac_surface_destroy (surface0);
	free (filename);
	return comac_test_status_from_status (ctx, status);
    }

    if (! image_surface_equals (surface0, surface1)) {
	comac_test_log (ctx, "Error surface mismatch.\n");
	comac_test_log (ctx, "to png: "); print_surface (ctx, surface0);
	comac_test_log (ctx, "from png: "); print_surface (ctx, surface1);

	comac_surface_destroy (surface0);
	comac_surface_destroy (surface1);
	free (filename);
	return COMAC_TEST_FAILURE;
    }
    assert (*(uint32_t *) comac_image_surface_get_data (surface1) == argb32);

    comac_surface_destroy (surface0);
    comac_surface_destroy (surface1);

    surface0 = comac_image_surface_create_for_data ((unsigned char *) &argb32,
						    COMAC_FORMAT_RGB24,
						    1, 1, 4);
    status = comac_surface_write_to_png (surface0, filename);
    if (status) {
	comac_test_log (ctx, "Error writing '%s': %s\n",
			filename, comac_status_to_string (status));
	comac_surface_destroy (surface0);
	return comac_test_status_from_status (ctx, status);
    }
    surface1 = comac_image_surface_create_from_png (filename);
    status = comac_surface_status (surface1);
    if (status) {
	comac_test_log (ctx, "Error reading '%s': %s\n",
			filename, comac_status_to_string (status));
	free (filename);

	comac_surface_destroy (surface1);
	comac_surface_destroy (surface0);
	return comac_test_status_from_status (ctx, status);
    }
    free (filename);

    if (! image_surface_equals (surface0, surface1)) {
	comac_test_log (ctx, "Error surface mismatch.\n");
	comac_test_log (ctx, "to png: "); print_surface (ctx, surface0);
	comac_test_log (ctx, "from png: "); print_surface (ctx, surface1);

	comac_surface_destroy (surface0);
	comac_surface_destroy (surface1);
	return COMAC_TEST_FAILURE;
    }
    assert ((*(uint32_t *) comac_image_surface_get_data (surface1) & RGB_MASK)
	    == (argb32 & RGB_MASK));

    comac_surface_destroy (surface0);
    comac_surface_destroy (surface1);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (png,
	    "Check that the png export/import is idempotent.",
	    "png, api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
