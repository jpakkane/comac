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

#include <stdio.h>

#include <comac.h>

int
main (int argc, char *argv[])
{
    comac_t *cr;
    comac_surface_t *argb, *rgb24;
    comac_status_t status;
    const char *input, *output;

    if (argc != 3) {
	fprintf (stderr, "usage: %s input.png output.png", argv[0]);
	fprintf (stderr, "Loads a PNG image (potentially with alpha) and writes out a flattened (no alpha)\nPNG image by first blending over white.\n");
	return 1;
    }

    input = argv[1];
    output = argv[2];

    argb = comac_image_surface_create_from_png (input);
    status = comac_surface_status (argb);
    if (status) {
	fprintf (stderr, "%s: Error: Failed to load %s: %s\n",
		 argv[0], input, comac_status_to_string (status));
	return 1;
    }

    rgb24 = comac_image_surface_create (COMAC_FORMAT_RGB24,
					comac_image_surface_get_width (argb),
					comac_image_surface_get_height (argb));

    cr = comac_create (rgb24);

    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);

    comac_set_source_surface (cr, argb, 0, 0);
    comac_paint (cr);

    comac_destroy (cr);

    status = comac_surface_write_to_png (rgb24, output);
    if (status) {
	fprintf (stderr, "%s: Error: Failed to write %s: %s\n",
		 argv[0], output, comac_status_to_string (status));
	return 1;
    }

    return 0;
}
