/*
 * Copyright © 2005,2008 Red Hat, Inc.
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
 *         Behdad Esfahbod <behdad@behdad.org>
 */

#include "config.h"

#include "comac-test.h"

#include <comac.h>
#include <assert.h>
#include <string.h>

#if COMAC_HAS_WIN32_FONT
#define COMAC_FONT_FAMILY_DEFAULT "Arial"
#elif COMAC_HAS_QUARTZ_FONT
#define COMAC_FONT_FAMILY_DEFAULT "Helvetica"
#elif COMAC_HAS_FT_FONT
#define COMAC_FONT_FAMILY_DEFAULT ""
#else
#define COMAC_FONT_FAMILY_DEFAULT "@comac:"
#endif

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_t *cr;
    comac_surface_t *surface;
    comac_font_face_t *font_face;
    comac_status_t status;

    surface = comac_image_surface_create (COMAC_FORMAT_RGB24, 0, 0);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    font_face = comac_font_face_reference (comac_get_font_face (cr));
    assert (comac_font_face_get_type (font_face) == COMAC_FONT_TYPE_TOY);
    assert (comac_toy_font_face_get_family (font_face) != NULL);
    assert (comac_toy_font_face_get_slant (font_face) ==
	    COMAC_FONT_SLANT_NORMAL);
    assert (comac_toy_font_face_get_weight (font_face) ==
	    COMAC_FONT_WEIGHT_NORMAL);
    status = comac_font_face_status (font_face);
    comac_font_face_destroy (font_face);

    if (status)
	return comac_test_status_from_status (ctx, status);

    comac_select_font_face (cr,
			    "bizarre",
			    COMAC_FONT_SLANT_OBLIQUE,
			    COMAC_FONT_WEIGHT_BOLD);
    font_face = comac_font_face_reference (comac_get_font_face (cr));
    assert (comac_font_face_get_type (font_face) == COMAC_FONT_TYPE_TOY);
    assert (0 ==
	    (strcmp) (comac_toy_font_face_get_family (font_face), "bizarre"));
    assert (comac_toy_font_face_get_slant (font_face) ==
	    COMAC_FONT_SLANT_OBLIQUE);
    assert (comac_toy_font_face_get_weight (font_face) ==
	    COMAC_FONT_WEIGHT_BOLD);
    status = comac_font_face_status (font_face);
    comac_font_face_destroy (font_face);

    if (status)
	return comac_test_status_from_status (ctx, status);

    font_face = comac_toy_font_face_create ("bozarre",
					    COMAC_FONT_SLANT_OBLIQUE,
					    COMAC_FONT_WEIGHT_BOLD);
    assert (comac_font_face_get_type (font_face) == COMAC_FONT_TYPE_TOY);
    assert (0 ==
	    (strcmp) (comac_toy_font_face_get_family (font_face), "bozarre"));
    assert (comac_toy_font_face_get_slant (font_face) ==
	    COMAC_FONT_SLANT_OBLIQUE);
    assert (comac_toy_font_face_get_weight (font_face) ==
	    COMAC_FONT_WEIGHT_BOLD);
    status = comac_font_face_status (font_face);
    comac_font_face_destroy (font_face);

    if (status)
	return comac_test_status_from_status (ctx, status);

    font_face = comac_toy_font_face_create (NULL,
					    COMAC_FONT_SLANT_OBLIQUE,
					    COMAC_FONT_WEIGHT_BOLD);
    assert (comac_font_face_get_type (font_face) == COMAC_FONT_TYPE_TOY);
    assert (0 == (strcmp) (comac_toy_font_face_get_family (font_face),
			   COMAC_FONT_FAMILY_DEFAULT));
    assert (comac_toy_font_face_get_slant (font_face) ==
	    COMAC_FONT_SLANT_NORMAL);
    assert (comac_toy_font_face_get_weight (font_face) ==
	    COMAC_FONT_WEIGHT_NORMAL);
    assert (comac_font_face_status (font_face) == COMAC_STATUS_NULL_POINTER);
    comac_font_face_destroy (font_face);

    font_face = comac_toy_font_face_create ("\xff",
					    COMAC_FONT_SLANT_OBLIQUE,
					    COMAC_FONT_WEIGHT_BOLD);
    assert (comac_font_face_get_type (font_face) == COMAC_FONT_TYPE_TOY);
    assert (0 == (strcmp) (comac_toy_font_face_get_family (font_face),
			   COMAC_FONT_FAMILY_DEFAULT));
    assert (comac_toy_font_face_get_slant (font_face) ==
	    COMAC_FONT_SLANT_NORMAL);
    assert (comac_toy_font_face_get_weight (font_face) ==
	    COMAC_FONT_WEIGHT_NORMAL);
    assert (comac_font_face_status (font_face) == COMAC_STATUS_INVALID_STRING);
    comac_font_face_destroy (font_face);

    font_face = comac_toy_font_face_create ("sans", -1, COMAC_FONT_WEIGHT_BOLD);
    assert (comac_font_face_get_type (font_face) == COMAC_FONT_TYPE_TOY);
    assert (0 == (strcmp) (comac_toy_font_face_get_family (font_face),
			   COMAC_FONT_FAMILY_DEFAULT));
    assert (comac_toy_font_face_get_slant (font_face) ==
	    COMAC_FONT_SLANT_NORMAL);
    assert (comac_toy_font_face_get_weight (font_face) ==
	    COMAC_FONT_WEIGHT_NORMAL);
    assert (comac_font_face_status (font_face) == COMAC_STATUS_INVALID_SLANT);
    comac_font_face_destroy (font_face);

    font_face =
	comac_toy_font_face_create ("sans", COMAC_FONT_SLANT_OBLIQUE, -1);
    assert (comac_font_face_get_type (font_face) == COMAC_FONT_TYPE_TOY);
    assert (0 == (strcmp) (comac_toy_font_face_get_family (font_face),
			   COMAC_FONT_FAMILY_DEFAULT));
    assert (comac_toy_font_face_get_slant (font_face) ==
	    COMAC_FONT_SLANT_NORMAL);
    assert (comac_toy_font_face_get_weight (font_face) ==
	    COMAC_FONT_WEIGHT_NORMAL);
    assert (comac_font_face_status (font_face) == COMAC_STATUS_INVALID_WEIGHT);
    comac_font_face_destroy (font_face);

    comac_destroy (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (toy_font_face,
	    "Check the construction of 'toy' font faces",
	    "font, api", /* keywords */
	    NULL,	 /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
