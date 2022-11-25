/* imagediff - Compare two images
 *
 * Copyright © 2004 Richard D. Worth
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Richard Worth
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * Richard Worth makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * RICHARD WORTH DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL RICHARD WORTH BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Richard D. Worth <richard@theworths.org> */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include "buffer-diff.h"

static void
_xunlink (const char *pathname)
{
    if (unlink (pathname) < 0 && errno != ENOENT) {
	fprintf (stderr, "  Error: Cannot remove %s: %s\n",
			pathname, strerror (errno));
	exit (1);
    }
}

void
comac_test_logv (const comac_test_context_t *ctx,
		 const char *fmt, va_list va)
{
    vfprintf (stderr, fmt, va);
}

void
comac_test_log (const comac_test_context_t *ctx, const char *fmt, ...)
{
    va_list va;

    va_start (va, fmt);
    vfprintf (stderr, fmt, va);
    va_end (va);
}

/* Flatten an ARGB surface by blending it over white. The resulting
 * surface, (still in ARGB32 format, but with only alpha==1.0
 * everywhere) is returned in the same surface pointer.
 *
 * The original surface will be destroyed.
 *
 * The (x,y) value specify an origin of interest for the original
 * image. The flattened image will be generated only from the box
 * extending from (x,y) to (width,height).
 */
static void
flatten_surface (comac_surface_t **surface, int x, int y)
{
    comac_surface_t *flat;
    comac_t *cr;

    flat = comac_image_surface_create (COMAC_FORMAT_ARGB32,
				       comac_image_surface_get_width (*surface) - x,
				       comac_image_surface_get_height (*surface) - y);
    comac_surface_set_device_offset (flat, -x, -y);

    cr = comac_create (flat);
    comac_surface_destroy (flat);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_surface (cr, *surface, 0, 0);
    comac_surface_destroy (*surface);
    comac_paint (cr);

    *surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);
}

/* Given an image surface, create a new surface that has the same
 * contents as the sub-surface with its origin at x,y.
 *
 * The original surface will be destroyed.
 */
static void
extract_sub_surface (comac_surface_t **surface, int x, int y)
{
    comac_surface_t *sub;
    comac_t *cr;

    sub = comac_image_surface_create (COMAC_FORMAT_ARGB32,
				      comac_image_surface_get_width (*surface) - x,
				      comac_image_surface_get_height (*surface) - y);

    /* We don't use a device offset like flatten_surface. That's not
     * for any important reason, (the results should be
     * identical). This style just seemed more natural to me this
     * time, so I'm leaving both here so I can look at both to see
     * which I like better. */
    cr = comac_create (sub);
    comac_surface_destroy (sub);

    comac_set_source_surface (cr, *surface, -x, -y);
    comac_surface_destroy (*surface);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_paint (cr);

    *surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);
}

static comac_status_t
stdio_write_func (void *closure, const unsigned char *data, unsigned int length)
{
    FILE *file = closure;

    if (fwrite (data, 1, length, file) != length)
	return COMAC_STATUS_WRITE_ERROR;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
write_png (comac_surface_t *surface, const char *filename)
{
    comac_status_t status;
    FILE *png_file;

    if (filename != NULL) {
	png_file = fopen (filename, "wb");
	if (png_file == NULL) {
	    switch (errno) {
	    case ENOMEM:
		return COMAC_STATUS_NO_MEMORY;
	    default:
		return COMAC_STATUS_WRITE_ERROR;
	    }
	}
    } else
	png_file = stdout;

    status = comac_surface_write_to_png_stream (surface,
						stdio_write_func,
						png_file);

    if (png_file != stdout)
	fclose (png_file);

    return status;
}

static comac_status_t
png_diff (const char *filename_a,
	  const char *filename_b,
	  const char *filename_diff,
	  int		ax,
	  int		ay,
	  int		bx,
	  int		by,
	  buffer_diff_result_t *result)
{
    comac_surface_t *surface_a;
    comac_surface_t *surface_b;
    comac_surface_t *surface_diff;
    comac_status_t status;

    surface_a = comac_image_surface_create_from_png (filename_a);
    status = comac_surface_status (surface_a);
    if (status) {
	fprintf (stderr, "Error: Failed to create surface from %s: %s\n",
		 filename_a, comac_status_to_string (status));
	return status;
    }

    surface_b = comac_image_surface_create_from_png (filename_b);
    status = comac_surface_status (surface_b);
    if (status) {
	fprintf (stderr, "Error: Failed to create surface from %s: %s\n",
		 filename_b, comac_status_to_string (status));
	comac_surface_destroy (surface_a);
	return status;
    }

    if (ax || ay) {
	extract_sub_surface (&surface_a, ax, ay);
	ax = ay = 0;
    }

    if (bx || by) {
	extract_sub_surface (&surface_b, bx, by);
	bx = by = 0;
    }

    status = comac_surface_status (surface_a);
    if (status) {
	fprintf (stderr, "Error: Failed to extract surface from %s: %s\n",
		 filename_a, comac_status_to_string (status));
	comac_surface_destroy (surface_a);
	comac_surface_destroy (surface_b);
	return status;
    }
    status = comac_surface_status (surface_b);
    if (status) {
	fprintf (stderr, "Error: Failed to extract surface from %s: %s\n",
		 filename_b, comac_status_to_string (status));
	comac_surface_destroy (surface_a);
	comac_surface_destroy (surface_b);
	return status;
    }

    surface_diff = comac_image_surface_create (COMAC_FORMAT_ARGB32,
					       comac_image_surface_get_width (surface_a),
					       comac_image_surface_get_height (surface_a));
    status = comac_surface_status (surface_diff);
    if (status) {
	fprintf (stderr,
		 "Error: Failed to allocate surface to hold differences\n");
	comac_surface_destroy (surface_a);
	comac_surface_destroy (surface_b);
	return COMAC_STATUS_NO_MEMORY;
    }

    status = image_diff (NULL,
			 surface_a, surface_b, surface_diff,
			 result);

    if (filename_diff)
	_xunlink (filename_diff);

    if (status == COMAC_STATUS_SUCCESS &&
	result->pixels_changed)
    {
	status = write_png (surface_diff, filename_diff);
    }

    comac_surface_destroy (surface_a);
    comac_surface_destroy (surface_b);
    comac_surface_destroy (surface_diff);

    return status;
}

int
main (int argc, char *argv[])
{
    buffer_diff_result_t result;
    comac_status_t status;

    unsigned int ax, ay, bx, by;

    if (argc != 3 && argc != 7) {
	fprintf (stderr, "Usage: %s image1.png image2.png [ax ay bx by]\n", argv[0]);
	fprintf (stderr, "Computes an output image designed to present a \"visual diff\" such that even\n");
	fprintf (stderr, "small errors in single pixels are readily apparent in the output.\n");
	fprintf (stderr, "The output image is written on stdout.\n");
	exit (1);
    }

    if (argc == 7) {
	ax = strtoul (argv[3], NULL, 0);
	ay = strtoul (argv[4], NULL, 0);
	bx = strtoul (argv[5], NULL, 0);
	by = strtoul (argv[6], NULL, 0);
    } else {
	ax = ay = bx = by = 0;
    }

    status = png_diff (argv[1], argv[2], NULL, ax, ay, bx, by, &result);

    if (status) {
	fprintf (stderr, "Error comparing images: %s\n",
		 comac_status_to_string (status));
	return 1;
    }

    if (result.pixels_changed)
	fprintf (stderr, "Total pixels changed: %d with a maximum channel difference of %d.\n",
		 result.pixels_changed,
		 result.max_diff);

    return (result.pixels_changed != 0);
}
