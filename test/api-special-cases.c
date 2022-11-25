/*
 * Copyright © 2010 Red Hat Inc.
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
 * Author: Benjamin Otte <otte@redhat.com>
 */

/*
 * WHAT THIS TEST DOES
 *
 * This test tests that for all public APIs Comac behaves correct, consistent
 * and most of all doesn't crash. It does this by calling all APIs that take
 * surfaces or contexts and calling them on specially prepared arguments that
 * should fail when called on this function.
 *
 * ADDING NEW FUNCTIONS
 *
 * You need (for adding the function comac_surface_foo):
 * 1) A surface_test_func_t named test_comac_surface_foo that gets passed the
 *    prepared surface and has the job of calling the function and checking
 *    the return value (if one exists) for correctness. The top of this file
 *    contains all these shim functions.
 * 2) Knowledge if the function behaves like a setter or like a getter. A 
 *    setter should set an error status on the surface, a getter does not
 *    modify the function.
 * 3) Knowledge if the function only works for a specific surface type and for
 *    which one.
 * 4) An entry in the tests array using the TEST() macro. It takes as arguments:
 *    - The function name
 *    - TRUE if the function modifies the surface, FALSE otherwise
 *    - the surface type for which the function is valid or -1 if it is valid
 *      for all surface types.
 *
 * FIXING FAILURES
 *
 * The test will dump failures notices into the api-special-cases.log file (when 
 * it doesn't crash). These should be pretty self-explanatory. Usually it is 
 * enough to just add a new check to the function it complained about.
 */

#include "config.h"

#include <assert.h>
#include <limits.h>

#include "comac-test.h"

#if COMAC_HAS_GL_SURFACE
#include <comac-gl.h>
#endif
#if COMAC_HAS_PDF_SURFACE
#include <comac-pdf.h>
#endif
#if COMAC_HAS_PS_SURFACE
#include <comac-ps.h>
#endif
#if COMAC_HAS_QUARTZ_SURFACE
#define Cursor QuartzCursor
#include <comac-quartz.h>
#undef Cursor
#endif
#if COMAC_HAS_SVG_SURFACE
#include <comac-svg.h>
#endif
#if COMAC_HAS_TEE_SURFACE
#include <comac-tee.h>
#endif
#if COMAC_HAS_XCB_SURFACE
#include <comac-xcb.h>
#endif
#if COMAC_HAS_XLIB_SURFACE
#define Cursor XCursor
#include <comac-xlib.h>
#undef Cursor
#endif

#define surface_has_type(surface,type) (comac_surface_get_type (surface) == (type))

typedef comac_test_status_t (* surface_test_func_t) (comac_surface_t *surface);
typedef comac_test_status_t (* context_test_func_t) (comac_t *cr);

static comac_test_status_t
test_comac_reference (comac_t *cr)
{
    comac_destroy (comac_reference (cr));

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_reference_count (comac_t *cr)
{
    unsigned int refcount = comac_get_reference_count (cr);
    if (refcount > 0)
        return COMAC_TEST_SUCCESS;
    /* inert error context have a refcount of 0 */
    return comac_status (cr) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_set_user_data (comac_t *cr)
{
    static comac_user_data_key_t key;
    comac_status_t status;

    status = comac_set_user_data (cr, &key, &key, NULL);
    if (status == COMAC_STATUS_NO_MEMORY)
        return COMAC_TEST_NO_MEMORY;
    else if (status)
        return COMAC_TEST_SUCCESS;

    if (comac_get_user_data (cr, &key) != &key)
        return COMAC_TEST_ERROR;

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_save (comac_t *cr)
{
    comac_save (cr);
    comac_restore (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_push_group (comac_t *cr)
{
    comac_pattern_t *pattern;
    comac_status_t status;

    comac_push_group (cr);
    pattern = comac_pop_group (cr);
    status = comac_pattern_status (pattern);
    comac_pattern_destroy (pattern);

    return status == COMAC_STATUS_SUCCESS || status == comac_status (cr) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_push_group_with_content (comac_t *cr)
{
    comac_push_group_with_content (cr, COMAC_CONTENT_COLOR_ALPHA);
    comac_pop_group_to_source (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_operator (comac_t *cr)
{
    comac_set_operator (cr, COMAC_OPERATOR_OVER);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_source (comac_t *cr)
{
    comac_pattern_t *source = comac_pattern_create_rgb (0, 0, 0);
    comac_set_source (cr, source);
    comac_pattern_destroy (source);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_source_rgb (comac_t *cr)
{
    comac_set_source_rgb (cr, 0, 0, 0);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_source_rgba (comac_t *cr)
{
    comac_set_source_rgba (cr, 0, 0, 0, 1);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_source_surface (comac_t *cr)
{
    comac_surface_t *surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1);
    comac_set_source_surface (cr, surface, 0, 0);
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_tolerance (comac_t *cr)
{
    comac_set_tolerance (cr, 42);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_antialias (comac_t *cr)
{
    comac_set_antialias (cr, COMAC_ANTIALIAS_BEST);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_fill_rule (comac_t *cr)
{
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_line_width (comac_t *cr)
{
    comac_set_line_width (cr, 42);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_line_cap (comac_t *cr)
{
    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_line_join (comac_t *cr)
{
    comac_set_line_join (cr, COMAC_LINE_JOIN_BEVEL);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_dash (comac_t *cr)
{
    comac_set_dash (cr, NULL, 0, 0);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_miter_limit (comac_t *cr)
{
    comac_set_miter_limit (cr, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_translate (comac_t *cr)
{
    comac_translate (cr, 2, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_scale (comac_t *cr)
{
    comac_scale (cr, 2, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_rotate (comac_t *cr)
{
    comac_rotate (cr, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_transform (comac_t *cr)
{
    comac_matrix_t matrix;

    comac_matrix_init_translate (&matrix, 1, 1);
    comac_transform (cr, &matrix);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_matrix (comac_t *cr)
{
    comac_matrix_t matrix;

    comac_matrix_init_translate (&matrix, 1, 1);
    comac_set_matrix (cr, &matrix);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_identity_matrix (comac_t *cr)
{
    comac_identity_matrix (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_user_to_device (comac_t *cr)
{
    double x = 42, y = 42;

    comac_user_to_device (cr, &x, &y);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_user_to_device_distance (comac_t *cr)
{
    double x = 42, y = 42;

    comac_user_to_device_distance (cr, &x, &y);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_device_to_user (comac_t *cr)
{
    double x = 42, y = 42;

    comac_device_to_user (cr, &x, &y);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_device_to_user_distance (comac_t *cr)
{
    double x = 42, y = 42;

    comac_device_to_user_distance (cr, &x, &y);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_new_path (comac_t *cr)
{
    comac_new_path (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_move_to (comac_t *cr)
{
    comac_move_to (cr, 2, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_new_sub_path (comac_t *cr)
{
    comac_new_sub_path (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_line_to (comac_t *cr)
{
    comac_line_to (cr, 2, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_curve_to (comac_t *cr)
{
    comac_curve_to (cr, 2, 2, 3, 3, 4, 4);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_arc (comac_t *cr)
{
    comac_arc (cr, 2, 2, 3, 0, 2 * M_PI);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_arc_negative (comac_t *cr)
{
    comac_arc_negative (cr, 2, 2, 3, 0, 2 * M_PI);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_rel_move_to (comac_t *cr)
{
    comac_rel_move_to (cr, 2, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_rel_line_to (comac_t *cr)
{
    comac_rel_line_to (cr, 2, 2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_rel_curve_to (comac_t *cr)
{
    comac_rel_curve_to (cr, 2, 2, 3, 3, 4, 4);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_rectangle (comac_t *cr)
{
    comac_rectangle (cr, 2, 2, 3, 3);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_close_path (comac_t *cr)
{
    comac_close_path (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_path_extents (comac_t *cr)
{
    double x1, y1, x2, y2;
    comac_path_extents (cr, &x1, &y1, &x2, &y2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_paint (comac_t *cr)
{
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_paint_with_alpha (comac_t *cr)
{
    comac_paint_with_alpha (cr, 0.5);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_mask (comac_t *cr)
{
    comac_pattern_t *pattern;

    pattern = comac_pattern_create_rgb (0.5, 0.5, 0.5);
    comac_mask (cr, pattern);

    comac_pattern_destroy (pattern);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_mask_surface (comac_t *cr)
{
    comac_surface_t *surface;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1);
    comac_mask_surface (cr, surface, 0, 0);

    comac_surface_destroy (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_stroke (comac_t *cr)
{
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_stroke_preserve (comac_t *cr)
{
    comac_stroke_preserve (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_fill (comac_t *cr)
{
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_fill_preserve (comac_t *cr)
{
    comac_fill_preserve (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_copy_page (comac_t *cr)
{
    comac_copy_page (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_show_page (comac_t *cr)
{
    comac_show_page (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_in_stroke (comac_t *cr)
{
    comac_in_stroke (cr, 1, 1);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_in_fill (comac_t *cr)
{
    comac_in_fill (cr, 1, 1);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_in_clip (comac_t *cr)
{
    comac_in_clip (cr, 1, 1);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_stroke_extents (comac_t *cr)
{
    double x1, y1, x2, y2;
    comac_stroke_extents (cr, &x1, &y1, &x2, &y2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_fill_extents (comac_t *cr)
{
    double x1, y1, x2, y2;
    comac_fill_extents (cr, &x1, &y1, &x2, &y2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_reset_clip (comac_t *cr)
{
    comac_reset_clip (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_clip (comac_t *cr)
{
    comac_clip (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_clip_preserve (comac_t *cr)
{
    comac_clip_preserve (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_clip_extents (comac_t *cr)
{
    double x1, y1, x2, y2;
    comac_clip_extents (cr, &x1, &y1, &x2, &y2);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_copy_clip_rectangle_list (comac_t *cr)
{
    comac_rectangle_list_destroy (comac_copy_clip_rectangle_list (cr));

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_select_font_face (comac_t *cr)
{
    comac_select_font_face (cr, "Arial", COMAC_FONT_SLANT_ITALIC, COMAC_FONT_WEIGHT_BOLD);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_font_size (comac_t *cr)
{
    comac_set_font_size (cr, 42);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_font_matrix (comac_t *cr)
{
    comac_matrix_t matrix;

    comac_matrix_init_translate (&matrix, 1, 1);
    comac_set_font_matrix (cr, &matrix);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_font_matrix (comac_t *cr)
{
    comac_matrix_t matrix;

    comac_get_font_matrix (cr, &matrix);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_font_options (comac_t *cr)
{
    comac_font_options_t *opt = comac_font_options_create ();
    comac_set_font_options (cr, opt);
    comac_font_options_destroy (opt);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_font_options (comac_t *cr)
{
    comac_font_options_t *opt = comac_font_options_create ();
    comac_get_font_options (cr, opt);
    comac_font_options_destroy (opt);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_font_face (comac_t *cr)
{
    comac_set_font_face (cr, comac_get_font_face (cr));

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_set_scaled_font (comac_t *cr)
{
    comac_set_scaled_font (cr, comac_get_scaled_font (cr));

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_show_text (comac_t *cr)
{
    comac_show_text (cr, "Comac");

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_show_glyphs (comac_t *cr)
{
    comac_glyph_t glyph;

    glyph.index = 65;
    glyph.x = 0;
    glyph.y = 0;

    comac_show_glyphs (cr, &glyph, 1);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_show_text_glyphs (comac_t *cr)
{
    comac_glyph_t glyph;
    comac_text_cluster_t cluster;

    glyph.index = 65;
    glyph.x = 0;
    glyph.y = 0;

    cluster.num_bytes = 1;
    cluster.num_glyphs = 1;

    comac_show_text_glyphs (cr, "a", -1, &glyph, 1, &cluster, 1, 0);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_text_path (comac_t *cr)
{
    comac_text_path (cr, "Comac");

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_glyph_path (comac_t *cr)
{
    comac_glyph_t glyph;

    glyph.index = 65;
    glyph.x = 0;
    glyph.y = 0;

    comac_glyph_path (cr, &glyph, 1);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_text_extents (comac_t *cr)
{
    comac_text_extents_t extents;

    comac_text_extents (cr, "Comac", &extents);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_glyph_extents (comac_t *cr)
{
    comac_glyph_t glyph;
    comac_text_extents_t extents;

    glyph.index = 65;
    glyph.x = 0;
    glyph.y = 0;

    comac_glyph_extents (cr, &glyph, 1, &extents);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_font_extents (comac_t *cr)
{
    comac_font_extents_t extents;

    comac_font_extents (cr, &extents);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_operator (comac_t *cr)
{
    comac_get_operator (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_source (comac_t *cr)
{
    comac_get_source (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_tolerance (comac_t *cr)
{
    comac_get_tolerance (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_antialias (comac_t *cr)
{
    comac_get_antialias (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_has_current_point (comac_t *cr)
{
    comac_has_current_point (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_current_point (comac_t *cr)
{
    double x, y;

    comac_get_current_point (cr, &x, &y);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_fill_rule (comac_t *cr)
{
    comac_get_fill_rule (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_line_width (comac_t *cr)
{
    comac_get_line_width (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_line_cap (comac_t *cr)
{
    comac_get_line_cap (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_line_join (comac_t *cr)
{
    comac_get_line_join (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_miter_limit (comac_t *cr)
{
    comac_get_miter_limit (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_dash_count (comac_t *cr)
{
    comac_get_dash_count (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_dash (comac_t *cr)
{
    double dashes[42];
    double offset;

    comac_get_dash (cr, &dashes[0], &offset);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_matrix (comac_t *cr)
{
    comac_matrix_t matrix;

    comac_get_matrix (cr, &matrix);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_target (comac_t *cr)
{
    comac_get_target (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_get_group_target (comac_t *cr)
{
    comac_get_group_target (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_copy_path (comac_t *cr)
{
    comac_path_destroy (comac_copy_path (cr));

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_copy_path_flat (comac_t *cr)
{
    comac_path_destroy (comac_copy_path_flat (cr));

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_append_path (comac_t *cr)
{
    comac_path_data_t data[3];
    comac_path_t path;

    path.status = COMAC_STATUS_SUCCESS;
    path.data = &data[0];
    path.num_data = ARRAY_LENGTH(data);

    data[0].header.type = COMAC_PATH_MOVE_TO;
    data[0].header.length = 2;
    data[1].point.x = 1;
    data[1].point.y = 2;
    data[2].header.type = COMAC_PATH_CLOSE_PATH;
    data[2].header.length = 1;

    comac_append_path (cr, &path);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_create_similar (comac_surface_t *surface)
{
    comac_surface_t *similar;
    
    similar = comac_surface_create_similar (surface, COMAC_CONTENT_ALPHA, 100, 100);
    
    comac_surface_destroy (similar);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_create_for_rectangle (comac_surface_t *surface)
{
    comac_surface_t *similar;
    
    similar = comac_surface_create_for_rectangle (surface, 1, 1, 8, 8);
    
    comac_surface_destroy (similar);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_reference (comac_surface_t *surface)
{
    comac_surface_destroy (comac_surface_reference (surface));
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_finish (comac_surface_t *surface)
{
    comac_surface_finish (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_get_device (comac_surface_t *surface)
{
    /* comac_device_t *device = */comac_surface_get_device (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_get_reference_count (comac_surface_t *surface)
{
    unsigned int refcount = comac_surface_get_reference_count (surface);
    if (refcount > 0)
        return COMAC_TEST_SUCCESS;
    /* inert error surfaces have a refcount of 0 */
    return comac_surface_status (surface) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_surface_status (comac_surface_t *surface)
{
    comac_status_t status = comac_surface_status (surface);
    return status < COMAC_STATUS_LAST_STATUS ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_surface_get_type (comac_surface_t *surface)
{
    /* comac_surface_type_t type = */comac_surface_get_type (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_get_content (comac_surface_t *surface)
{
    comac_content_t content = comac_surface_get_content (surface);

    switch (content) {
    case COMAC_CONTENT_COLOR:
    case COMAC_CONTENT_ALPHA:
    case COMAC_CONTENT_COLOR_ALPHA:
        return COMAC_TEST_SUCCESS;
    default:
        return COMAC_TEST_ERROR;
    }
}

static comac_test_status_t
test_comac_surface_set_user_data (comac_surface_t *surface)
{
    static comac_user_data_key_t key;
    comac_status_t status;

    status = comac_surface_set_user_data (surface, &key, &key, NULL);
    if (status == COMAC_STATUS_NO_MEMORY)
        return COMAC_TEST_NO_MEMORY;
    else if (status)
        return COMAC_TEST_SUCCESS;

    if (comac_surface_get_user_data (surface, &key) != &key)
        return COMAC_TEST_ERROR;

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_set_mime_data (comac_surface_t *surface)
{
    const char *mimetype = "text/x-uri";
    const char *data = "https://www.comacgraphics.org";
    comac_status_t status;

    status = comac_surface_set_mime_data (surface,
                                          mimetype,
                                          (const unsigned char *) data,
					  strlen (data),
                                          NULL, NULL);
    return status ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_surface_get_mime_data (comac_surface_t *surface)
{
    const char *mimetype = "text/x-uri";
    const unsigned char *data;
    unsigned long length;

    comac_surface_get_mime_data (surface, mimetype, &data, &length);
    return data == NULL && length == 0 ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_surface_get_font_options (comac_surface_t *surface)
{
    comac_font_options_t *options;
    comac_status_t status;

    options = comac_font_options_create ();
    if (likely (!comac_font_options_status (options)))
        comac_surface_get_font_options (surface, options);
    status = comac_font_options_status (options);
    comac_font_options_destroy (options);
    return status ? COMAC_TEST_ERROR : COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_flush (comac_surface_t *surface)
{
    comac_surface_flush (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_mark_dirty (comac_surface_t *surface)
{
    comac_surface_mark_dirty (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_mark_dirty_rectangle (comac_surface_t *surface)
{
    comac_surface_mark_dirty_rectangle (surface, 1, 1, 8, 8);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_set_device_offset (comac_surface_t *surface)
{
    comac_surface_set_device_offset (surface, 5, 5);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_get_device_offset (comac_surface_t *surface)
{
    double x, y;

    comac_surface_get_device_offset (surface, &x, &y);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_set_fallback_resolution (comac_surface_t *surface)
{
    comac_surface_set_fallback_resolution (surface, 42, 42);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_get_fallback_resolution (comac_surface_t *surface)
{
    double x, y;

    comac_surface_get_fallback_resolution (surface, &x, &y);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_copy_page (comac_surface_t *surface)
{
    comac_surface_copy_page (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_show_page (comac_surface_t *surface)
{
    comac_surface_show_page (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_surface_has_show_text_glyphs (comac_surface_t *surface)
{
    comac_surface_has_show_text_glyphs (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_image_surface_get_data (comac_surface_t *surface)
{
    unsigned char *data = comac_image_surface_get_data (surface);
    return data == NULL || surface_has_type (surface, COMAC_SURFACE_TYPE_IMAGE) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_image_surface_get_format (comac_surface_t *surface)
{
    comac_format_t format = comac_image_surface_get_format (surface);
    return format == COMAC_FORMAT_INVALID || surface_has_type (surface, COMAC_SURFACE_TYPE_IMAGE) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_image_surface_get_width (comac_surface_t *surface)
{
    unsigned int width = comac_image_surface_get_width (surface);
    return width == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_IMAGE) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_image_surface_get_height (comac_surface_t *surface)
{
    unsigned int height = comac_image_surface_get_height (surface);
    return height == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_IMAGE) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_image_surface_get_stride (comac_surface_t *surface)
{
    unsigned int stride = comac_image_surface_get_stride (surface);
    return stride == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_IMAGE) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

#if COMAC_HAS_PNG_FUNCTIONS

static comac_test_status_t
test_comac_surface_write_to_png (comac_surface_t *surface)
{
    comac_status_t status;

    status = comac_surface_write_to_png (surface, "/this/file/will/definitely/not/exist.png");
    
    return status ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_status_t
write_func_that_always_fails (void *closure, const unsigned char *data, unsigned int length)
{
    return COMAC_STATUS_WRITE_ERROR;
}

static comac_test_status_t
test_comac_surface_write_to_png_stream (comac_surface_t *surface)
{
    comac_status_t status;

    status = comac_surface_write_to_png_stream (surface,
                                                write_func_that_always_fails,
                                                NULL);
    
    return status && status != COMAC_STATUS_WRITE_ERROR ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

#endif /* COMAC_HAS_PNG_FUNCTIONS */

static comac_test_status_t
test_comac_recording_surface_ink_extents (comac_surface_t *surface)
{
    double x, y, w, h;

    comac_recording_surface_ink_extents (surface, &x, &y, &w, &h);
    return x == 0 && y == 0 && w == 0 && h == 0 ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

#if COMAC_HAS_TEE_SURFACE

static comac_test_status_t
test_comac_tee_surface_add (comac_surface_t *surface)
{
    comac_surface_t *image = comac_image_surface_create (COMAC_FORMAT_A8, 10, 10);

    comac_tee_surface_add (surface, image);
    comac_surface_destroy (image);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_tee_surface_remove (comac_surface_t *surface)
{
    comac_surface_t *image = comac_image_surface_create (COMAC_FORMAT_A8, 10, 10);

    comac_tee_surface_remove (surface, image);
    comac_surface_destroy (image);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_tee_surface_index (comac_surface_t *surface)
{
    comac_surface_t *master;
    comac_status_t status;

    master = comac_tee_surface_index (surface, 0);
    status = comac_surface_status (master);
    comac_surface_destroy (master);
    return status ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

#endif /* COMAC_HAS_TEE_SURFACE */

#if COMAC_HAS_GL_SURFACE

static comac_test_status_t
test_comac_gl_surface_set_size (comac_surface_t *surface)
{
    comac_gl_surface_set_size (surface, 5, 5);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_gl_surface_get_width (comac_surface_t *surface)
{
    unsigned int width = comac_gl_surface_get_width (surface);
    return width == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_GL) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_gl_surface_get_height (comac_surface_t *surface)
{
    unsigned int height = comac_gl_surface_get_height (surface);
    return height == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_GL) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_gl_surface_swapbuffers (comac_surface_t *surface)
{
    comac_gl_surface_swapbuffers (surface);
    return COMAC_TEST_SUCCESS;
}

#endif /* COMAC_HAS_GL_SURFACE */

#if COMAC_HAS_PDF_SURFACE

static comac_test_status_t
test_comac_pdf_surface_restrict_to_version (comac_surface_t *surface)
{
    comac_pdf_surface_restrict_to_version (surface, COMAC_PDF_VERSION_1_4);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_pdf_surface_set_size (comac_surface_t *surface)
{
    comac_pdf_surface_set_size (surface, 5, 5);
    return COMAC_TEST_SUCCESS;
}

#endif /* COMAC_HAS_PDF_SURFACE */

#if COMAC_HAS_PS_SURFACE

static comac_test_status_t
test_comac_ps_surface_restrict_to_level (comac_surface_t *surface)
{
    comac_ps_surface_restrict_to_level (surface, COMAC_PS_LEVEL_2);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_ps_surface_set_eps (comac_surface_t *surface)
{
    comac_ps_surface_set_eps (surface, TRUE);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_ps_surface_get_eps (comac_surface_t *surface)
{
    comac_bool_t eps = comac_ps_surface_get_eps (surface);
    return eps ? COMAC_TEST_ERROR : COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_ps_surface_set_size (comac_surface_t *surface)
{
    comac_ps_surface_set_size (surface, 5, 5);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_ps_surface_dsc_comment (comac_surface_t *surface)
{
    comac_ps_surface_dsc_comment (surface, "54, 74, 90, 2010");
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_ps_surface_dsc_begin_setup (comac_surface_t *surface)
{
    comac_ps_surface_dsc_begin_setup (surface);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_ps_surface_dsc_begin_page_setup (comac_surface_t *surface)
{
    comac_ps_surface_dsc_begin_page_setup (surface);
    return COMAC_TEST_SUCCESS;
}

#endif /* COMAC_HAS_PS_SURFACE */

#if COMAC_HAS_QUARTZ_SURFACE

static comac_test_status_t
test_comac_quartz_surface_get_cg_context (comac_surface_t *surface)
{
    CGContextRef context = comac_quartz_surface_get_cg_context (surface);
    return context == NULL || surface_has_type (surface, COMAC_SURFACE_TYPE_QUARTZ) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

#endif /* COMAC_HAS_QUARTZ_SURFACE */

#if COMAC_HAS_SVG_SURFACE

static comac_test_status_t
test_comac_svg_surface_restrict_to_version (comac_surface_t *surface)
{
    comac_svg_surface_restrict_to_version (surface, COMAC_SVG_VERSION_1_1);
    return COMAC_TEST_SUCCESS;
}

#endif /* COMAC_HAS_SVG_SURFACE */

#if COMAC_HAS_XCB_SURFACE

static comac_test_status_t
test_comac_xcb_surface_set_size (comac_surface_t *surface)
{
    comac_xcb_surface_set_size (surface, 5, 5);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_xcb_surface_set_drawable (comac_surface_t *surface)
{
    comac_xcb_surface_set_drawable (surface, 0, 5, 5);
    return COMAC_TEST_SUCCESS;
}

#endif

#if COMAC_HAS_XLIB_SURFACE

static comac_test_status_t
test_comac_xlib_surface_set_size (comac_surface_t *surface)
{
    comac_xlib_surface_set_size (surface, 5, 5);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_xlib_surface_set_drawable (comac_surface_t *surface)
{
    comac_xlib_surface_set_drawable (surface, 0, 5, 5);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_comac_xlib_surface_get_display (comac_surface_t *surface)
{
    Display *display = comac_xlib_surface_get_display (surface);
    return display == NULL || surface_has_type (surface, COMAC_SURFACE_TYPE_XLIB) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_xlib_surface_get_screen (comac_surface_t *surface)
{
    Screen *screen = comac_xlib_surface_get_screen (surface);
    return screen == NULL || surface_has_type (surface, COMAC_SURFACE_TYPE_XLIB) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_xlib_surface_get_visual (comac_surface_t *surface)
{
    Visual *visual = comac_xlib_surface_get_visual (surface);
    return visual == NULL || surface_has_type (surface, COMAC_SURFACE_TYPE_XLIB) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_xlib_surface_get_drawable (comac_surface_t *surface)
{
    Drawable drawable = comac_xlib_surface_get_drawable (surface);
    return drawable == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_XLIB) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_xlib_surface_get_depth (comac_surface_t *surface)
{
    int depth = comac_xlib_surface_get_depth (surface);
    return depth == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_XLIB) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_xlib_surface_get_width (comac_surface_t *surface)
{
    int width = comac_xlib_surface_get_width (surface);
    return width == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_XLIB) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

static comac_test_status_t
test_comac_xlib_surface_get_height (comac_surface_t *surface)
{
    int height = comac_xlib_surface_get_height (surface);
    return height == 0 || surface_has_type (surface, COMAC_SURFACE_TYPE_XLIB) ? COMAC_TEST_SUCCESS : COMAC_TEST_ERROR;
}

#endif

#define TEST(name) { #name, test_ ## name }

struct {
    const char *name;
    context_test_func_t func;
} context_tests[] = {
    TEST (comac_reference),
    TEST (comac_get_reference_count),
    TEST (comac_set_user_data),
    TEST (comac_save),
    TEST (comac_push_group),
    TEST (comac_push_group_with_content),
    TEST (comac_set_operator),
    TEST (comac_set_source),
    TEST (comac_set_source_rgb),
    TEST (comac_set_source_rgba),
    TEST (comac_set_source_surface),
    TEST (comac_set_tolerance),
    TEST (comac_set_antialias),
    TEST (comac_set_fill_rule),
    TEST (comac_set_line_width),
    TEST (comac_set_line_cap),
    TEST (comac_set_line_join),
    TEST (comac_set_dash),
    TEST (comac_set_miter_limit),
    TEST (comac_translate),
    TEST (comac_scale),
    TEST (comac_rotate),
    TEST (comac_transform),
    TEST (comac_set_matrix),
    TEST (comac_identity_matrix),
    TEST (comac_user_to_device),
    TEST (comac_user_to_device_distance),
    TEST (comac_device_to_user),
    TEST (comac_device_to_user_distance),
    TEST (comac_new_path),
    TEST (comac_move_to),
    TEST (comac_new_sub_path),
    TEST (comac_line_to),
    TEST (comac_curve_to),
    TEST (comac_arc),
    TEST (comac_arc_negative),
    TEST (comac_rel_move_to),
    TEST (comac_rel_line_to),
    TEST (comac_rel_curve_to),
    TEST (comac_rectangle),
    TEST (comac_close_path),
    TEST (comac_path_extents),
    TEST (comac_paint),
    TEST (comac_paint_with_alpha),
    TEST (comac_mask),
    TEST (comac_mask_surface),
    TEST (comac_stroke),
    TEST (comac_stroke_preserve),
    TEST (comac_fill),
    TEST (comac_fill_preserve),
    TEST (comac_copy_page),
    TEST (comac_show_page),
    TEST (comac_in_stroke),
    TEST (comac_in_fill),
    TEST (comac_in_clip),
    TEST (comac_stroke_extents),
    TEST (comac_fill_extents),
    TEST (comac_reset_clip),
    TEST (comac_clip),
    TEST (comac_clip_preserve),
    TEST (comac_clip_extents),
    TEST (comac_copy_clip_rectangle_list),
    TEST (comac_select_font_face),
    TEST (comac_set_font_size),
    TEST (comac_set_font_matrix),
    TEST (comac_get_font_matrix),
    TEST (comac_set_font_options),
    TEST (comac_get_font_options),
    TEST (comac_set_font_face),
    TEST (comac_set_scaled_font),
    TEST (comac_show_text),
    TEST (comac_show_glyphs),
    TEST (comac_show_text_glyphs),
    TEST (comac_text_path),
    TEST (comac_glyph_path),
    TEST (comac_text_extents),
    TEST (comac_glyph_extents),
    TEST (comac_font_extents),
    TEST (comac_get_operator),
    TEST (comac_get_source),
    TEST (comac_get_tolerance),
    TEST (comac_get_antialias),
    TEST (comac_has_current_point),
    TEST (comac_get_current_point),
    TEST (comac_get_fill_rule),
    TEST (comac_get_line_width),
    TEST (comac_get_line_cap),
    TEST (comac_get_line_join),
    TEST (comac_get_miter_limit),
    TEST (comac_get_dash_count),
    TEST (comac_get_dash),
    TEST (comac_get_matrix),
    TEST (comac_get_target),
    TEST (comac_get_group_target),
    TEST (comac_copy_path),
    TEST (comac_copy_path_flat),
    TEST (comac_append_path),
};

#undef TEST

#define TEST(name, surface_type, sets_status) { #name, test_ ## name, surface_type, sets_status }

struct {
    const char *name;
    surface_test_func_t func;
    int surface_type; /* comac_surface_type_t or -1 */
    comac_bool_t modifies_surface;
} surface_tests[] = {
    TEST (comac_surface_create_similar, -1, FALSE),
    TEST (comac_surface_create_for_rectangle, -1, FALSE),
    TEST (comac_surface_reference, -1, FALSE),
    TEST (comac_surface_finish, -1, TRUE),
    TEST (comac_surface_get_device, -1, FALSE),
    TEST (comac_surface_get_reference_count, -1, FALSE),
    TEST (comac_surface_status, -1, FALSE),
    TEST (comac_surface_get_type, -1, FALSE),
    TEST (comac_surface_get_content, -1, FALSE),
    TEST (comac_surface_set_user_data, -1, FALSE),
    TEST (comac_surface_set_mime_data, -1, TRUE),
    TEST (comac_surface_get_mime_data, -1, FALSE),
    TEST (comac_surface_get_font_options, -1, FALSE),
    TEST (comac_surface_flush, -1, TRUE),
    TEST (comac_surface_mark_dirty, -1, TRUE),
    TEST (comac_surface_mark_dirty_rectangle, -1, TRUE),
    TEST (comac_surface_set_device_offset, -1, TRUE),
    TEST (comac_surface_get_device_offset, -1, FALSE),
    TEST (comac_surface_set_fallback_resolution, -1, TRUE),
    TEST (comac_surface_get_fallback_resolution, -1, FALSE),
    TEST (comac_surface_copy_page, -1, TRUE),
    TEST (comac_surface_show_page, -1, TRUE),
    TEST (comac_surface_has_show_text_glyphs, -1, FALSE),
    TEST (comac_image_surface_get_data, COMAC_SURFACE_TYPE_IMAGE, FALSE),
    TEST (comac_image_surface_get_format, COMAC_SURFACE_TYPE_IMAGE, FALSE),
    TEST (comac_image_surface_get_width, COMAC_SURFACE_TYPE_IMAGE, FALSE),
    TEST (comac_image_surface_get_height, COMAC_SURFACE_TYPE_IMAGE, FALSE),
    TEST (comac_image_surface_get_stride, COMAC_SURFACE_TYPE_IMAGE, FALSE),
#if COMAC_HAS_PNG_FUNCTIONS
    TEST (comac_surface_write_to_png, -1, FALSE),
    TEST (comac_surface_write_to_png_stream, -1, FALSE),
#endif
    TEST (comac_recording_surface_ink_extents, COMAC_SURFACE_TYPE_RECORDING, FALSE),
#if COMAC_HAS_TEE_SURFACE
    TEST (comac_tee_surface_add, COMAC_SURFACE_TYPE_TEE, TRUE),
    TEST (comac_tee_surface_remove, COMAC_SURFACE_TYPE_TEE, TRUE),
    TEST (comac_tee_surface_index, COMAC_SURFACE_TYPE_TEE, FALSE),
#endif
#if COMAC_HAS_GL_SURFACE
    TEST (comac_gl_surface_set_size, COMAC_SURFACE_TYPE_GL, TRUE),
    TEST (comac_gl_surface_get_width, COMAC_SURFACE_TYPE_GL, FALSE),
    TEST (comac_gl_surface_get_height, COMAC_SURFACE_TYPE_GL, FALSE),
    TEST (comac_gl_surface_swapbuffers, COMAC_SURFACE_TYPE_GL, TRUE),
#endif
#if COMAC_HAS_PDF_SURFACE
    TEST (comac_pdf_surface_restrict_to_version, COMAC_SURFACE_TYPE_PDF, TRUE),
    TEST (comac_pdf_surface_set_size, COMAC_SURFACE_TYPE_PDF, TRUE),
#endif
#if COMAC_HAS_PS_SURFACE
    TEST (comac_ps_surface_restrict_to_level, COMAC_SURFACE_TYPE_PS, TRUE),
    TEST (comac_ps_surface_set_eps, COMAC_SURFACE_TYPE_PS, TRUE),
    TEST (comac_ps_surface_get_eps, COMAC_SURFACE_TYPE_PS, FALSE),
    TEST (comac_ps_surface_set_size, COMAC_SURFACE_TYPE_PS, TRUE),
    TEST (comac_ps_surface_dsc_comment, COMAC_SURFACE_TYPE_PS, TRUE),
    TEST (comac_ps_surface_dsc_begin_setup, COMAC_SURFACE_TYPE_PS, TRUE),
    TEST (comac_ps_surface_dsc_begin_page_setup, COMAC_SURFACE_TYPE_PS, TRUE),
#endif
#if COMAC_HAS_QUARTZ_SURFACE
    TEST (comac_quartz_surface_get_cg_context, COMAC_SURFACE_TYPE_QUARTZ, FALSE),
#endif
#if COMAC_HAS_SVG_SURFACE
    TEST (comac_svg_surface_restrict_to_version, COMAC_SURFACE_TYPE_SVG, TRUE),
#endif
#if COMAC_HAS_XCB_SURFACE
    TEST (comac_xcb_surface_set_size, COMAC_SURFACE_TYPE_XCB, TRUE),
    TEST (comac_xcb_surface_set_drawable, COMAC_SURFACE_TYPE_XCB, TRUE),
#endif
#if COMAC_HAS_XLIB_SURFACE
    TEST (comac_xlib_surface_set_size, COMAC_SURFACE_TYPE_XLIB, TRUE),
    TEST (comac_xlib_surface_set_drawable, COMAC_SURFACE_TYPE_XLIB, TRUE),
    TEST (comac_xlib_surface_get_display, COMAC_SURFACE_TYPE_XLIB, FALSE),
    TEST (comac_xlib_surface_get_drawable, COMAC_SURFACE_TYPE_XLIB, FALSE),
    TEST (comac_xlib_surface_get_screen, COMAC_SURFACE_TYPE_XLIB, FALSE),
    TEST (comac_xlib_surface_get_visual, COMAC_SURFACE_TYPE_XLIB, FALSE),
    TEST (comac_xlib_surface_get_depth, COMAC_SURFACE_TYPE_XLIB, FALSE),
    TEST (comac_xlib_surface_get_width, COMAC_SURFACE_TYPE_XLIB, FALSE),
    TEST (comac_xlib_surface_get_height, COMAC_SURFACE_TYPE_XLIB, FALSE),
#endif
};

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface;
    comac_t *cr;
    comac_test_status_t test_status;
    comac_status_t status_before, status_after;
    unsigned int i;

    /* Test an error surface */
    for (i = 0; i < ARRAY_LENGTH (surface_tests); i++) {
        surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, INT_MAX, INT_MAX);
        status_before = comac_surface_status (surface);
        assert (status_before);

        test_status = surface_tests[i].func (surface);

        status_after = comac_surface_status (surface);
        comac_surface_destroy (surface);

        if (test_status != COMAC_TEST_SUCCESS) {
            comac_test_log (ctx,
                            "Failed test %s with %d\n",
                            surface_tests[i].name, (int) test_status);
            return test_status;
        }

        if (status_before != status_after) {
            comac_test_log (ctx,
                            "Failed test %s: Modified surface status from %u (%s) to %u (%s)\n",
                            surface_tests[i].name,
                            status_before, comac_status_to_string (status_before),
                            status_after, comac_status_to_string (status_after));
            return COMAC_TEST_ERROR;
        }
    }

    /* Test an error context */
    for (i = 0; i < ARRAY_LENGTH (context_tests); i++) {
        cr = comac_create (NULL);
        status_before = comac_status (cr);
        assert (status_before);

        test_status = context_tests[i].func (cr);

        status_after = comac_status (cr);
        comac_destroy (cr);

        if (test_status != COMAC_TEST_SUCCESS) {
            comac_test_log (ctx,
                            "Failed test %s with %d\n",
                            context_tests[i].name, (int) test_status);
            return test_status;
        }

        if (status_before != status_after) {
            comac_test_log (ctx,
                            "Failed test %s: Modified context status from %u (%s) to %u (%s)\n",
                            context_tests[i].name,
                            status_before, comac_status_to_string (status_before),
                            status_after, comac_status_to_string (status_after));
            return COMAC_TEST_ERROR;
        }
    }

    /* Test a context for an error surface */
    for (i = 0; i < ARRAY_LENGTH (context_tests); i++) {
        surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, INT_MAX, INT_MAX);
        cr = comac_create (surface);
        comac_surface_destroy (surface);
        status_before = comac_status (cr);
        assert (status_before);

        test_status = context_tests[i].func (cr);

        status_after = comac_status (cr);
        comac_destroy (cr);

        if (test_status != COMAC_TEST_SUCCESS) {
            comac_test_log (ctx,
                            "Failed test %s with %d\n",
                            context_tests[i].name, (int) test_status);
            return test_status;
        }

        if (status_before != status_after) {
            comac_test_log (ctx,
                            "Failed test %s: Modified context status from %u (%s) to %u (%s)\n",
                            context_tests[i].name,
                            status_before, comac_status_to_string (status_before),
                            status_after, comac_status_to_string (status_after));
            return COMAC_TEST_ERROR;
        }
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
test_context (const comac_test_context_t *ctx, comac_t *cr, const char *name, unsigned int i)
{
    comac_test_status_t test_status;
    comac_status_t status_before, status_after;

    /* Make sure that there is a current point */
    comac_move_to (cr, 0, 0);

    status_before = comac_status (cr);
    test_status = context_tests[i].func (cr);
    status_after = comac_status (cr);

    if (test_status != COMAC_TEST_SUCCESS) {
        comac_test_log (ctx,
                        "Failed test %s on %s with %d\n",
                        context_tests[i].name, name, (int) test_status);
        return test_status;
    }

    if (status_after != COMAC_STATUS_SURFACE_FINISHED && status_before != status_after) {
        comac_test_log (ctx,
                        "Failed test %s on %s: Modified context status from %u (%s) to %u (%s)\n",
                        context_tests[i].name, name,
                        status_before, comac_status_to_string (status_before),
                        status_after, comac_status_to_string (status_after));
        return COMAC_TEST_ERROR;
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *similar, *target;
    comac_test_status_t test_status;
    comac_status_t status;
    comac_t *cr2;
    unsigned int i;

    target = comac_get_target (cr);

    /* Test a finished similar surface */
    for (i = 0; i < ARRAY_LENGTH (surface_tests); i++) {
        similar = comac_surface_create_similar (target,
                                                comac_surface_get_content (target),
                                                10, 10);
        comac_surface_finish (similar);
        test_status = surface_tests[i].func (similar);
        status = comac_surface_status (similar);
        comac_surface_destroy (similar);

        if (test_status != COMAC_TEST_SUCCESS) {
            comac_test_log (ctx,
                            "Failed test %s with %d\n",
                            surface_tests[i].name, (int) test_status);
            return test_status;
        }

        if (surface_tests[i].modifies_surface &&
            strcmp (surface_tests[i].name, "comac_surface_finish") &&
            strcmp (surface_tests[i].name, "comac_surface_flush") &&
            status != COMAC_STATUS_SURFACE_FINISHED) {
            comac_test_log (ctx,
                            "Failed test %s: Finished surface not set into error state\n",
                            surface_tests[i].name);
            return COMAC_TEST_ERROR;
        }
    }

    /* Test a context for a finished similar surface */
    for (i = 0; i < ARRAY_LENGTH (context_tests); i++) {
        similar = comac_surface_create_similar (target,
                                                comac_surface_get_content (target),
                                                10, 10);
        comac_surface_finish (similar);
        cr2 = comac_create (similar);
        test_status = test_context (ctx, cr2, "finished surface", i);
        comac_surface_destroy (similar);
        comac_destroy (cr2);

        if (test_status != COMAC_TEST_SUCCESS)
            return test_status;
    }

    /* Test a context for a similar surface finished later */
    for (i = 0; i < ARRAY_LENGTH (context_tests); i++) {
        similar = comac_surface_create_similar (target,
                                                comac_surface_get_content (target),
                                                10, 10);
        cr2 = comac_create (similar);
        comac_surface_finish (similar);
        test_status = test_context (ctx, cr2, "finished surface after create", i);
        comac_surface_destroy (similar);
        comac_destroy (cr2);

        if (test_status != COMAC_TEST_SUCCESS)
            return test_status;
    }

    /* Test a context for a similar surface finished later with a path */
    for (i = 0; i < ARRAY_LENGTH (context_tests); i++) {
        similar = comac_surface_create_similar (target,
                                                comac_surface_get_content (target),
                                                10, 10);
        cr2 = comac_create (similar);
        comac_rectangle (cr2, 2, 2, 4, 4);
        comac_surface_finish (similar);
        test_status = test_context (ctx, cr2, "finished surface with path", i);
        comac_surface_destroy (similar);
        comac_destroy (cr2);

        if (test_status != COMAC_TEST_SUCCESS)
            return test_status;
    }

    /* Test a normal surface for functions that have the wrong type */
    for (i = 0; i < ARRAY_LENGTH (surface_tests); i++) {
        comac_status_t desired_status;

        if (surface_tests[i].surface_type == -1)
            continue;
        similar = comac_surface_create_similar (target,
                                                comac_surface_get_content (target),
                                                10, 10);
        if (comac_surface_get_type (similar) == (comac_surface_type_t) surface_tests[i].surface_type) {
            comac_surface_destroy (similar);
            continue;
        }

        test_status = surface_tests[i].func (similar);
        status = comac_surface_status (similar);
        comac_surface_destroy (similar);

        if (test_status != COMAC_TEST_SUCCESS) {
            comac_test_log (ctx,
                            "Failed test %s with %d\n",
                            surface_tests[i].name, (int) test_status);
            return test_status;
        }

        desired_status = surface_tests[i].modifies_surface ? COMAC_STATUS_SURFACE_TYPE_MISMATCH : COMAC_STATUS_SUCCESS;
        if (status != desired_status) {
            comac_test_log (ctx,
                            "Failed test %s: Surface status should be %u (%s), but is %u (%s)\n",
                            surface_tests[i].name,
                            desired_status, comac_status_to_string (desired_status),
                            status, comac_status_to_string (status));
            return COMAC_TEST_ERROR;
        }
    }

    /* 565-compatible gray background */
    comac_set_source_rgb (cr, 0.51613, 0.55555, 0.51613);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (api_special_cases,
	    "Check surface functions properly handle wrong surface arguments",
	    "api", /* keywords */
	    NULL, /* requirements */
	    10, 10,
	    preamble, draw)
