/*
 * Copyright © 2008 Adrian Johnson
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"

#include <stdio.h>
#include <errno.h>
#include <comac.h>
#include <comac-pdf.h>

/* This test checks that the mime data is correctly used by the PDF
 * surface when embedding images..
 */

/* Both a *.png and *.jpg file with this name is required since we
 * are not using a jpeg library */
#define IMAGE_FILE "romedalen"

#define BASENAME "pdf-mime-data.out"

static comac_test_status_t
read_file (const comac_test_context_t *ctx,
	   const char *file,
	   unsigned char **data,
	   unsigned int *len)
{
    FILE *fp;

    fp = fopen (file, "rb");
    if (fp == NULL) {
	char filename[4096];

	/* try again with srcdir */
	snprintf (filename, sizeof (filename),
		  "%s/%s", ctx->srcdir, file);
	fp = fopen (filename, "rb");
    }
    if (fp == NULL) {
	switch (errno) {
	case ENOMEM:
	    comac_test_log (ctx, "Could not create file handle for %s due to \
				lack of memory\n", file);
	    return COMAC_TEST_NO_MEMORY;
	default:
	    comac_test_log (ctx, "Could not get the file handle for %s\n", file);
	    return COMAC_TEST_FAILURE;
	}
    }

    fseek (fp, 0, SEEK_END);
    *len = ftell(fp);
    fseek (fp, 0, SEEK_SET);
    *data = malloc (*len);
    if (*data == NULL) {
	fclose(fp);
	comac_test_log (ctx, "Could not allocate memory for buffer to read \
				from file %s\n", file);
	return COMAC_TEST_NO_MEMORY;
    }

    if (fread(*data, *len, 1, fp) != 1) {
	free (*data);
	fclose(fp);
	comac_test_log (ctx, "Could not read data from file %s\n", file);
	return COMAC_TEST_FAILURE;
    }

    fclose(fp);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *image;
    comac_surface_t *surface;
    comac_t *cr;
    comac_status_t status, status2;
    comac_test_status_t test_status;
    int width, height;
    unsigned char *data;
    unsigned char *out_data;
    unsigned int len, out_len;
    char command[4096];
    int exit_status;
    char *filename;
    const char *path = comac_test_mkdir (COMAC_TEST_OUTPUT_DIR) ? COMAC_TEST_OUTPUT_DIR : ".";

    if (! comac_test_is_target_enabled (ctx, "pdf"))
	return COMAC_TEST_UNTESTED;

    exit_status = system ("command -v pdfimages");
    if (exit_status) {
	comac_test_log (ctx, "pdfimages not available\n");
	return COMAC_TEST_UNTESTED;
    }

    image = comac_test_create_surface_from_png (ctx, IMAGE_FILE ".png");
    test_status = read_file (ctx, IMAGE_FILE ".jpg", &data, &len);
    if (test_status) {
	return test_status;
    }

    comac_surface_set_mime_data (image, COMAC_MIME_TYPE_JPEG,
				 data, len,
				 free, data);
    width = comac_image_surface_get_width (image);
    height = comac_image_surface_get_height (image);

    xasprintf (&filename, "%s/%s.pdf", path, BASENAME);
    surface = comac_pdf_surface_create (filename, width + 20, height + 20);
    cr = comac_create (surface);
    comac_translate (cr, 10, 10);
    comac_set_source_surface (cr, image, 0, 0);
    comac_paint (cr);
    status = comac_status (cr);
    comac_destroy (cr);
    comac_surface_finish (surface);
    status2 = comac_surface_status (surface);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;
    comac_surface_destroy (surface);
    comac_surface_destroy (image);

    if (status) {
	comac_test_log (ctx, "Failed to create pdf surface for file %s: %s\n",
			filename, comac_status_to_string (status));
        free (filename);
	return COMAC_TEST_FAILURE;
    }

    printf ("pdf-mime-data: Please check %s to ensure it looks/prints correctly.\n", filename);

    sprintf (command, "pdfimages -j %s %s", filename, COMAC_TEST_OUTPUT_DIR "/" BASENAME);
    exit_status = system (command);
    free (filename);
    if (exit_status) {
	comac_test_log (ctx, "pdfimages failed with exit status %d\n", exit_status);
	return COMAC_TEST_FAILURE;
    }

    test_status = read_file (ctx, IMAGE_FILE ".jpg", &data, &len);
    if (test_status) {
	return test_status;
    }

    test_status = read_file (ctx, COMAC_TEST_OUTPUT_DIR "/" BASENAME "-000.jpg", &out_data, &out_len);
    if (test_status) {
	return test_status;
    }

    if (len != out_len || memcmp(data, out_data, len) != 0) {
	free (data);
	free (out_data);
	comac_test_log (ctx, "output mime data does not match source mime data\n");
	return COMAC_TEST_FAILURE;
    }

    free (data);
    free (out_data);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (pdf_mime_data,
	    "Check mime data correctly used by PDF surface",
	    "pdf, mime-data", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
