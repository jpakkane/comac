/*
 * Copyright © 2007 Red Hat, Inc.
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
 * Author: Carl Worth <cworth@cworth.org>
 */

#define _ISOC99_SOURCE	/* for INFINITY */

#include "config.h"
#include "comac-test.h"

#if !defined(INFINITY)
#define INFINITY HUGE_VAL
#endif

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_status_t status;
    comac_surface_t *target;
    comac_font_face_t *font_face;
    comac_font_options_t *font_options;
    comac_scaled_font_t *scaled_font;
    comac_pattern_t *pattern;
    comac_t *cr2;
    comac_matrix_t identity, bogus, inf, invalid = {
	4.0, 4.0,
	4.0, 4.0,
	4.0, 4.0
    };

#define CHECK_STATUS(status, function_name)						\
if ((status) == COMAC_STATUS_SUCCESS) {							\
    comac_test_log (ctx, "Error: %s with invalid matrix passed\n",				\
		    (function_name));							\
    return COMAC_TEST_FAILURE;								\
} else if ((status) != COMAC_STATUS_INVALID_MATRIX) {					\
    comac_test_log (ctx, "Error: %s with invalid matrix returned unexpected status "	\
		    "(%d): %s\n",							\
		    (function_name),							\
		    status,								\
		    comac_status_to_string (status));					\
    return COMAC_TEST_FAILURE;								\
}

    /* clear floating point exceptions (added by comac_test_init()) */
#if HAVE_FEDISABLEEXCEPT
    fedisableexcept (FE_INVALID);
#endif

    /* create a bogus matrix and check results of attempted inversion */
    bogus.x0 = bogus.xy = bogus.xx = comac_test_NaN ();
    bogus.y0 = bogus.yx = bogus.yy = bogus.xx;
    status = comac_matrix_invert (&bogus);
    CHECK_STATUS (status, "comac_matrix_invert(NaN)");

    inf.x0 = inf.xy = inf.xx = INFINITY;
    inf.y0 = inf.yx = inf.yy = inf.xx;
    status = comac_matrix_invert (&inf);
    CHECK_STATUS (status, "comac_matrix_invert(infinity)");

    /* test comac_matrix_invert with invalid matrix */
    status = comac_matrix_invert (&invalid);
    CHECK_STATUS (status, "comac_matrix_invert(invalid)");


    comac_matrix_init_identity (&identity);

    target = comac_get_group_target (cr);

    /* test comac_transform with invalid matrix */
    cr2 = comac_create (target);
    comac_transform (cr2, &invalid);

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_transform(invalid)");

    /* test comac_transform with bogus matrix */
    cr2 = comac_create (target);
    comac_transform (cr2, &bogus);

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_transform(NaN)");

    /* test comac_transform with ∞ matrix */
    cr2 = comac_create (target);
    comac_transform (cr2, &inf);

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_transform(infinity)");


    /* test comac_set_matrix with invalid matrix */
    cr2 = comac_create (target);
    comac_set_matrix (cr2, &invalid);

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_set_matrix(invalid)");

    /* test comac_set_matrix with bogus matrix */
    cr2 = comac_create (target);
    comac_set_matrix (cr2, &bogus);

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_set_matrix(NaN)");

    /* test comac_set_matrix with ∞ matrix */
    cr2 = comac_create (target);
    comac_set_matrix (cr2, &inf);

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_set_matrix(infinity)");


    /* test comac_set_font_matrix with invalid matrix */
    cr2 = comac_create (target);
    comac_set_font_matrix (cr2, &invalid);

    /* draw some text to force the font to be resolved */
    comac_show_text (cr2, "hello");

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_set_font_matrix(invalid)");

    /* test comac_set_font_matrix with bogus matrix */
    cr2 = comac_create (target);
    comac_set_font_matrix (cr2, &bogus);

    /* draw some text to force the font to be resolved */
    comac_show_text (cr2, "hello");

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_set_font_matrix(NaN)");

    /* test comac_set_font_matrix with ∞ matrix */
    cr2 = comac_create (target);
    comac_set_font_matrix (cr2, &inf);

    /* draw some text to force the font to be resolved */
    comac_show_text (cr2, "hello");

    status = comac_status (cr2);
    comac_destroy (cr2);
    CHECK_STATUS (status, "comac_set_font_matrix(infinity)");


    /* test comac_scaled_font_create with invalid matrix */
    cr2 = comac_create (target);
    font_face = comac_get_font_face (cr2);
    font_options = comac_font_options_create ();
    comac_get_font_options (cr, font_options);
    scaled_font = comac_scaled_font_create (font_face,
					    &invalid,
					    &identity,
					    font_options);
    status = comac_scaled_font_status (scaled_font);
    CHECK_STATUS (status, "comac_scaled_font_create(invalid)");

    comac_scaled_font_destroy (scaled_font);

    scaled_font = comac_scaled_font_create (font_face,
					    &identity,
					    &invalid,
					    font_options);
    status = comac_scaled_font_status (scaled_font);
    CHECK_STATUS (status, "comac_scaled_font_create(invalid)");

    comac_scaled_font_destroy (scaled_font);
    comac_font_options_destroy (font_options);
    comac_destroy (cr2);

    /* test comac_scaled_font_create with bogus matrix */
    cr2 = comac_create (target);
    font_face = comac_get_font_face (cr2);
    font_options = comac_font_options_create ();
    comac_get_font_options (cr, font_options);
    scaled_font = comac_scaled_font_create (font_face,
					    &bogus,
					    &identity,
					    font_options);
    status = comac_scaled_font_status (scaled_font);
    CHECK_STATUS (status, "comac_scaled_font_create(NaN)");

    comac_scaled_font_destroy (scaled_font);

    scaled_font = comac_scaled_font_create (font_face,
					    &identity,
					    &bogus,
					    font_options);
    status = comac_scaled_font_status (scaled_font);
    CHECK_STATUS (status, "comac_scaled_font_create(NaN)");

    comac_scaled_font_destroy (scaled_font);
    comac_font_options_destroy (font_options);
    comac_destroy (cr2);

    /* test comac_scaled_font_create with ∞ matrix */
    cr2 = comac_create (target);
    font_face = comac_get_font_face (cr2);
    font_options = comac_font_options_create ();
    comac_get_font_options (cr, font_options);
    scaled_font = comac_scaled_font_create (font_face,
					    &inf,
					    &identity,
					    font_options);
    status = comac_scaled_font_status (scaled_font);
    CHECK_STATUS (status, "comac_scaled_font_create(infinity)");

    comac_scaled_font_destroy (scaled_font);

    scaled_font = comac_scaled_font_create (font_face,
					    &identity,
					    &inf,
					    font_options);
    status = comac_scaled_font_status (scaled_font);
    CHECK_STATUS (status, "comac_scaled_font_create(infinity)");

    comac_scaled_font_destroy (scaled_font);
    comac_font_options_destroy (font_options);
    comac_destroy (cr2);


    /* test comac_pattern_set_matrix with invalid matrix */
    pattern = comac_pattern_create_rgb (1.0, 1.0, 1.0);
    comac_pattern_set_matrix (pattern, &invalid);
    status = comac_pattern_status (pattern);
    CHECK_STATUS (status, "comac_pattern_set_matrix(invalid)");
    comac_pattern_destroy (pattern);

    /* test comac_pattern_set_matrix with bogus matrix */
    pattern = comac_pattern_create_rgb (1.0, 1.0, 1.0);
    comac_pattern_set_matrix (pattern, &bogus);
    status = comac_pattern_status (pattern);
    CHECK_STATUS (status, "comac_pattern_set_matrix(NaN)");
    comac_pattern_destroy (pattern);

    /* test comac_pattern_set_matrix with ∞ matrix */
    pattern = comac_pattern_create_rgb (1.0, 1.0, 1.0);
    comac_pattern_set_matrix (pattern, &inf);
    status = comac_pattern_status (pattern);
    CHECK_STATUS (status, "comac_pattern_set_matrix(infinity)");
    comac_pattern_destroy (pattern);


    /* test invalid transformations */
    cr2 = comac_create (target);
    comac_translate (cr2, bogus.xx, bogus.yy);
    CHECK_STATUS (status, "comac_translate(NaN, NaN)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_translate (cr2, 0, bogus.yy);
    CHECK_STATUS (status, "comac_translate(0, NaN)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_translate (cr2, bogus.xx, 0);
    CHECK_STATUS (status, "comac_translate(NaN, 0)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_translate (cr2, inf.xx, inf.yy);
    CHECK_STATUS (status, "comac_translate(∞, ∞)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_translate (cr2, 0, inf.yy);
    CHECK_STATUS (status, "comac_translate(0, ∞)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_translate (cr2, inf.xx, 0);
    CHECK_STATUS (status, "comac_translate(∞, 0)");
    comac_destroy (cr2);


    cr2 = comac_create (target);
    comac_scale (cr2, bogus.xx, bogus.yy);
    CHECK_STATUS (status, "comac_scale(NaN, NaN)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, 1, bogus.yy);
    CHECK_STATUS (status, "comac_scale(1, NaN)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, bogus.xx, 1);
    CHECK_STATUS (status, "comac_scale(NaN, 1)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, inf.xx, inf.yy);
    CHECK_STATUS (status, "comac_scale(∞, ∞)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, 1, inf.yy);
    CHECK_STATUS (status, "comac_scale(1, ∞)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, inf.xx, 1);
    CHECK_STATUS (status, "comac_scale(∞, 1)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, bogus.xx, bogus.yy);
    CHECK_STATUS (status, "comac_scale(0, 0)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, 1, bogus.yy);
    CHECK_STATUS (status, "comac_scale(1, 0)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_scale (cr2, bogus.xx, 1);
    CHECK_STATUS (status, "comac_scale(0, 1)");
    comac_destroy (cr2);


    cr2 = comac_create (target);
    comac_rotate (cr2, bogus.xx);
    CHECK_STATUS (status, "comac_rotate(NaN)");
    comac_destroy (cr2);

    cr2 = comac_create (target);
    comac_rotate (cr2, inf.xx);
    CHECK_STATUS (status, "comac_rotate(∞)");
    comac_destroy (cr2);

#if HAVE_FECLEAREXCEPT
    feclearexcept (FE_INVALID);
#endif

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (invalid_matrix,
	    "Test that all relevant public functions return COMAC_STATUS_INVALID_MATRIX as appropriate",
	    "api, matrix", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    NULL, draw)
