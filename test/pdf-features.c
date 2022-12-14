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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#include <stdio.h>
#include <comac.h>
#include <comac-pdf.h>

/* This test exists to test the various features of comac-pdf.h.
 *
 * Currently, this test exercises the following function calls:
 *
 *	comac_pdf_surface_set_size
 */

#define INCHES_TO_POINTS(in) ((in) *72.0)
#define MM_TO_POINTS(mm) ((mm) / 25.4 * 72.0)
#define TEXT_SIZE 12
#define BASENAME "pdf-features.out"

static struct {
    const char *page_size;
    const char *page_size_alias;
    const char *orientation;
    double width_in_points;
    double height_in_points;
} pages[] = {
    {"na_letter_8.5x11in",
     "letter",
     "portrait",
     INCHES_TO_POINTS (8.5),
     INCHES_TO_POINTS (11)},
    {"na_letter_8.5x11in",
     "letter",
     "landscape",
     INCHES_TO_POINTS (11),
     INCHES_TO_POINTS (8.5)},
    {"iso_a4_210x297mm",
     "a4",
     "portrait",
     MM_TO_POINTS (210),
     MM_TO_POINTS (297)},
    {"iso_a4_210x297mm",
     "a4",
     "landscape",
     MM_TO_POINTS (297),
     MM_TO_POINTS (210)},
    {"iso_a5_148x210mm",
     "a5",
     "portrait",
     MM_TO_POINTS (148),
     MM_TO_POINTS (210)},
    {"iso_a5_148x210mm",
     "a5",
     "landscape",
     MM_TO_POINTS (210),
     MM_TO_POINTS (148)},
    {"iso_a6_105x148mm",
     "a6",
     "portrait",
     MM_TO_POINTS (105),
     MM_TO_POINTS (148)},
    {"iso_a6_105x148mm",
     "a6",
     "landscape",
     MM_TO_POINTS (148),
     MM_TO_POINTS (105)},
    {"iso_a7_74x105mm",
     "a7",
     "portrait",
     MM_TO_POINTS (74),
     MM_TO_POINTS (105)},
    {"iso_a7_74x105mm",
     "a7",
     "landscape",
     MM_TO_POINTS (105),
     MM_TO_POINTS (74)},
    {"iso_a8_52x74mm", "a8", "portrait", MM_TO_POINTS (52), MM_TO_POINTS (74)},
    {"iso_a8_52x74mm", "a8", "landscape", MM_TO_POINTS (74), MM_TO_POINTS (52)},
    {"iso_a9_37x52mm", "a9", "portrait", MM_TO_POINTS (37), MM_TO_POINTS (52)},
    {"iso_a9_37x52mm", "a9", "landscape", MM_TO_POINTS (52), MM_TO_POINTS (37)},
    {"iso_a10_26x37mm",
     "a10",
     "portrait",
     MM_TO_POINTS (26),
     MM_TO_POINTS (37)},
    {"iso_a10_26x37mm",
     "a10",
     "landscape",
     MM_TO_POINTS (37),
     MM_TO_POINTS (26)}};

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface;
    comac_t *cr;
    comac_status_t status;
    size_t i;
    char *filename;
    const char *path =
	comac_test_mkdir (COMAC_TEST_OUTPUT_DIR) ? COMAC_TEST_OUTPUT_DIR : ".";

    if (! comac_test_is_target_enabled (ctx, "pdf"))
	return COMAC_TEST_UNTESTED;

    xasprintf (&filename, "%s/%s.pdf", path, BASENAME);

    /* The initial size passed here is the default size that will be
     * inheritable by each page. That is, any page for which this
     * initial size applies will not have its own /MediaBox entry in
     * its dictionary. */
    surface = comac_pdf_surface_create (filename,
					INCHES_TO_POINTS (8.5),
					INCHES_TO_POINTS (11));

    cr = comac_create (surface);

    comac_select_font_face (cr,
			    COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);

    for (i = 0; i < ARRAY_LENGTH (pages); i++) {
	comac_pdf_surface_set_size (surface,
				    pages[i].width_in_points,
				    pages[i].height_in_points);

	comac_move_to (cr, TEXT_SIZE, TEXT_SIZE);
	comac_show_text (cr, pages[i].page_size);
	comac_show_text (cr, " - ");
	comac_show_text (cr, pages[i].orientation);
	comac_show_page (cr);
    }

    status = comac_status (cr);

    comac_destroy (cr);
    comac_surface_destroy (surface);

    if (status) {
	comac_test_log (ctx,
			"Failed to create pdf surface for file %s: %s\n",
			filename,
			comac_status_to_string (status));
	free (filename);
	return COMAC_TEST_FAILURE;
    }

    printf (
	"pdf-features: Please check %s to ensure it looks/prints correctly.\n",
	filename);
    free (filename);
    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (pdf_features,
	    "Check PDF specific API",
	    "pdf", /* keywords */
	    NULL,  /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
