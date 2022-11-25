/*
 * Copyright © 2017 Red Hat, Inc.
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
 * Author: MAtthias Clasen <mclasen@redhat.com>
 */

#include "comac-test.h"

#include <assert.h>

/* This test requires freetype2 */
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

#if COMAC_HAS_FC_FONT
#include <fontconfig/fontconfig.h>
#endif

#include "comac-ft.h"

#define FloatToFixed(f) ((FT_Fixed) ((f) *65536))

static comac_test_status_t
test_variation (comac_test_context_t *ctx,
		const char *input,
		const char *tag,
		int def,
		float expected_value)
{
    comac_font_face_t *font_face;
    comac_scaled_font_t *scaled_font;
    comac_matrix_t matrix;
    comac_font_options_t *options;
    comac_status_t status;

    FT_Face ft_face;
    FT_MM_Var *ft_mm_var;
    FT_Error ret;
    FT_Fixed coords[20];
    unsigned int i;

#if COMAC_HAS_FC_FONT
    FcPattern *pattern;

    /* we need a font that has variations */
    pattern = FcPatternBuild (NULL,
			      FC_FAMILY,
			      FcTypeString,
			      (FcChar8 *) "Adobe Variable Font Prototype",
			      NULL);
    font_face = comac_ft_font_face_create_for_pattern (pattern);
    status = comac_font_face_status (font_face);
    FcPatternDestroy (pattern);

    if (status != COMAC_STATUS_SUCCESS) {
	comac_test_log (ctx, "Failed to create font face");
	return COMAC_TEST_FAILURE;
    }

    comac_matrix_init_identity (&matrix);
    options = comac_font_options_create ();
    if (comac_font_options_status (options) != COMAC_STATUS_SUCCESS) {
	comac_test_log (ctx, "Failed to create font options");
	return COMAC_TEST_FAILURE;
    }

    comac_font_options_set_variations (options, input);
    if (comac_font_options_status (options) != COMAC_STATUS_SUCCESS) {
	comac_test_log (ctx, "Failed to set variations");
	return COMAC_TEST_FAILURE;
    }

    if (strcmp (comac_font_options_get_variations (options), input) != 0) {
	comac_test_log (ctx, "Failed to verify variations");
	return COMAC_TEST_FAILURE;
    }

    scaled_font =
	comac_scaled_font_create (font_face, &matrix, &matrix, options);
    status = comac_scaled_font_status (scaled_font);

    if (status != COMAC_STATUS_SUCCESS) {
	comac_test_log (ctx, "Failed to create scaled font");
	return COMAC_TEST_FAILURE;
    }

    ft_face = comac_ft_scaled_font_lock_face (scaled_font);
    if (comac_scaled_font_status (scaled_font) != COMAC_STATUS_SUCCESS) {
	comac_test_log (ctx, "Failed to get FT_Face");
	return COMAC_TEST_FAILURE;
    }
    if (strcmp (ft_face->family_name, "Adobe Variable Font Prototype") != 0) {
	comac_test_log (
	    ctx,
	    "This test requires the font \"Adobe Variable Font Prototype\" "
	    "(https://github.com/adobe-fonts/adobe-variable-font-prototype/"
	    "releases)");
	return COMAC_TEST_UNTESTED;
    }

    ret = FT_Get_MM_Var (ft_face, &ft_mm_var);
    if (ret != 0) {
	comac_test_log (ctx, "Failed to get MM");
	return COMAC_TEST_FAILURE;
    }

#ifdef HAVE_FT_GET_VAR_DESIGN_COORDINATES
    ret = FT_Get_Var_Design_Coordinates (ft_face, 20, coords);
    if (ret != 0) {
	comac_test_log (ctx, "Failed to get coords");
	return COMAC_TEST_FAILURE;
    }
#else
    return COMAC_TEST_UNTESTED;
#endif

    for (i = 0; i < ft_mm_var->num_axis; i++) {
	FT_Var_Axis *axis = &ft_mm_var->axis[i];
	comac_test_log (ctx,
			"axis %s, value %g\n",
			axis->name,
			coords[i] / 65536.);
    }
    for (i = 0; i < ft_mm_var->num_axis; i++) {
	FT_Var_Axis *axis = &ft_mm_var->axis[i];
	if (axis->tag == FT_MAKE_TAG (tag[0], tag[1], tag[2], tag[3])) {
	    if (def) {
		if (coords[i] != axis->def) {
		    comac_test_log (ctx,
				    "Axis %s: not default value (%g != %g)",
				    axis->name,
				    coords[i] / 65536.,
				    axis->def / 65536.);
		    return COMAC_TEST_FAILURE;
		}
	    } else {
		if (coords[i] != FloatToFixed (expected_value)) {
		    comac_test_log (ctx,
				    "Axis %s: not expected value (%g != %g)",
				    axis->name,
				    coords[i] / 65536.,
				    expected_value);
		    return COMAC_TEST_FAILURE;
		}
	    }
	} else {
	}
    }

    comac_ft_scaled_font_unlock_face (scaled_font);

    comac_scaled_font_destroy (scaled_font);
    comac_font_options_destroy (options);
    comac_font_face_destroy (font_face);

    return COMAC_TEST_SUCCESS;
#else
    return COMAC_TEST_UNTESTED;
#endif
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_test_status_t status = COMAC_TEST_SUCCESS;
    struct {
	const char *input;
	const char *tag;
	int expected_default;
	float expected_value;
    } tests[] = {
	{"wdth=200,wght=300", "wght", 0, 300.0},     // valid
	{"wdth=200.5,wght=300.5", "wght", 0, 300.5}, // valid, using decimal dot
	{"wdth 200 , wght 300", "wght", 0, 300.0},   // valid, without =
	{"wght = 200", "wght", 0, 200.0},	     // valid, whitespace and =
	{"CNTR=20", "wght", 1, 0.0},		     // valid, not setting wght
	{"weight=100", "wght", 1, 0.0},		     // not a valid tag
	{NULL, 0}};
    int i;

    for (i = 0; tests[i].input; i++) {
	status = test_variation (ctx,
				 tests[i].input,
				 tests[i].tag,
				 tests[i].expected_default,
				 tests[i].expected_value);
	if (status != COMAC_TEST_SUCCESS)
	    return status;
    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (font_variations,
	    "Test font variations",
	    "fonts", /* keywords */
	    NULL,    /* requirements */
	    9,
	    11,
	    preamble,
	    NULL)
