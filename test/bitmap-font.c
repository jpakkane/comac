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
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <comac-ft.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#define FONT "6x13.pcf"
#define TEXT_SIZE 13

static comac_bool_t
font_extents_equal (const comac_font_extents_t *A,
	            const comac_font_extents_t *B)
{
    return
	COMAC_TEST_DOUBLE_EQUALS (A->ascent,  B->ascent)  &&
	COMAC_TEST_DOUBLE_EQUALS (A->descent, B->descent) &&
	COMAC_TEST_DOUBLE_EQUALS (A->height,  B->height)  &&
	COMAC_TEST_DOUBLE_EQUALS (A->max_x_advance, B->max_x_advance) &&
	COMAC_TEST_DOUBLE_EQUALS (A->max_y_advance, B->max_y_advance);
}

static comac_test_status_t
check_font_extents (const comac_test_context_t *ctx, comac_t *cr, const char *comment)
{
    comac_font_extents_t font_extents, ref_font_extents = {11, 2, 13, 6, 0};
    comac_status_t status;

    memset (&font_extents, 0xff, sizeof (comac_font_extents_t));
    comac_font_extents (cr, &font_extents);

    status = comac_status (cr);
    if (status)
	return comac_test_status_from_status (ctx, status);

    if (! font_extents_equal (&font_extents, &ref_font_extents)) {
	comac_test_log (ctx, "Error: %s: comac_font_extents(); extents (%g, %g, %g, %g, %g)\n",
			comment,
		        font_extents.ascent, font_extents.descent,
			font_extents.height,
			font_extents.max_x_advance, font_extents.max_y_advance);
	return COMAC_TEST_FAILURE;
    }

    return COMAC_TEST_SUCCESS;
}

#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    FcPattern *pattern;
    comac_font_face_t *font_face;
    comac_font_extents_t font_extents;
    comac_font_options_t *font_options;
    comac_status_t status;
    char *filename;
    int face_count;
    struct stat stat_buf;

    xasprintf (&filename, "%s/%s", ctx->srcdir, FONT);

    if (stat (filename, &stat_buf) || ! S_ISREG (stat_buf.st_mode)) {
	comac_test_log (ctx, "Error finding font: %s: file not found?\n", filename);
	return COMAC_TEST_FAILURE;
    }

    pattern = FcFreeTypeQuery ((unsigned char *)filename, 0, NULL, &face_count);
    if (! pattern) {
	comac_test_log (ctx, "FcFreeTypeQuery failed.\n");
	free (filename);
	return comac_test_status_from_status (ctx, COMAC_STATUS_NO_MEMORY);
    }

    font_face = comac_ft_font_face_create_for_pattern (pattern);
    FcPatternDestroy (pattern);

    status = comac_font_face_status (font_face);
    if (status) {
	comac_test_log (ctx, "Error creating font face for %s: %s\n",
			filename,
			comac_status_to_string (status));
	free (filename);
	return comac_test_status_from_status (ctx, status);
    }

    free (filename);
    if (comac_font_face_get_type (font_face) != COMAC_FONT_TYPE_FT) {
	comac_test_log (ctx, "Unexpected value from comac_font_face_get_type: %d (expected %d)\n",
			comac_font_face_get_type (font_face), COMAC_FONT_TYPE_FT);
	comac_font_face_destroy (font_face);
	return COMAC_TEST_FAILURE;
    }

    comac_set_font_face (cr, font_face);
    comac_font_face_destroy (font_face);
    comac_set_font_size (cr, 13);

    font_options = comac_font_options_create ();

#define CHECK_FONT_EXTENTS(comment) do {\
    comac_test_status_t test_status; \
    test_status = check_font_extents (ctx, cr, (comment)); \
    if (test_status != COMAC_TEST_SUCCESS) { \
	comac_font_options_destroy (font_options); \
	return test_status; \
    } \
} while (0)

    comac_font_extents (cr, &font_extents);
    CHECK_FONT_EXTENTS ("default");

    comac_font_options_set_hint_metrics (font_options, COMAC_HINT_METRICS_ON);
    comac_set_font_options (cr, font_options);

    CHECK_FONT_EXTENTS ("HINT_METRICS_ON");

    comac_move_to (cr, 1, font_extents.ascent - 1);
    comac_set_source_rgb (cr, 0.0, 0.0, 1.0); /* blue */

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_NONE);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_ON HINT_STYLE_NONE");
    comac_show_text (cr, "the ");

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_SLIGHT);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_ON HINT_STYLE_SLIGHT");
    comac_show_text (cr, "quick ");

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_MEDIUM);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_ON HINT_STYLE_MEDIUM");
    comac_show_text (cr, "brown");

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_FULL);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_ON HINT_STYLE_FULL");
    comac_show_text (cr, " fox");

    /* Switch from show_text to text_path/fill to exercise bug #7889 */
    comac_text_path (cr, " jumps over a lazy dog");
    comac_fill (cr);

    /* And test it rotated as well for the sake of bug #7888 */

    comac_translate (cr, width, height);
    comac_rotate (cr, M_PI);

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_DEFAULT);
    comac_font_options_set_hint_metrics (font_options, COMAC_HINT_METRICS_OFF);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_OFF");

    comac_move_to (cr, 1, font_extents.height - font_extents.descent - 1);

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_NONE);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_OFF HINT_STYLE_NONE");
    comac_show_text (cr, "the ");

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_SLIGHT);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_OFF HINT_STYLE_SLIGHT");
    comac_show_text (cr, "quick");

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_MEDIUM);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_OFF HINT_STYLE_MEDIUM");
    comac_show_text (cr, " brown");

    comac_font_options_set_hint_style (font_options, COMAC_HINT_STYLE_FULL);
    comac_set_font_options (cr, font_options);
    CHECK_FONT_EXTENTS ("HINT_METRICS_OFF HINT_STYLE_FULL");
    comac_show_text (cr, " fox");

    comac_text_path (cr, " jumps over");
    comac_text_path (cr, " a lazy dog");
    comac_fill (cr);

    comac_font_options_destroy (font_options);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bitmap_font,
	    "Test drawing with a font consisting only of bitmaps"
	    "\nThe PDF and PS backends embed a slightly distorted font for the rotated case.",
	    "text", /* keywords */
	    "ft", /* requirements */
	    246 + 1, 2 * TEXT_SIZE,
	    NULL, draw)
