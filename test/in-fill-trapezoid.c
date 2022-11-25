/*
 * Copyright © 2008 Chris Wilson
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
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_test_status_t ret = COMAC_TEST_SUCCESS;
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 0, 0);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    /* simple rectangle */
    comac_new_path (cr);
    comac_rectangle (cr, -10, -10, 20, 20);
    if (! comac_in_fill (cr, 0, 0)) {
	comac_test_log (ctx, "Error: Failed to find point inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* rectangular boundary tests */
    if (! comac_in_fill (cr, -10, -10)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find top-left vertex inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, -10, 10)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find bottom-left vertex inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 10, -10)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find top-right vertex inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 10, 10)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find bottom-right vertex inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, -10, 0)) {
	comac_test_log (ctx,
			"Error: Failed to find left edge inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 0, -10)) {
	comac_test_log (ctx,
			"Error: Failed to find top edge inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 10, 0)) {
	comac_test_log (ctx,
			"Error: Failed to find right edge inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 0, 10)) {
	comac_test_log (ctx,
			"Error: Failed to find bottom edge inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* simple circle */
    comac_new_path (cr);
    comac_arc (cr, 0, 0, 10, 0, 2 * M_PI);
    if (! comac_in_fill (cr, 0, 0)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find point inside circle [even-odd]\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* holey rectangle */
    comac_new_path (cr);
    comac_rectangle (cr, -10, -10, 20, 20);
    comac_rectangle (cr, -5, -5, 10, 10);
    if (comac_in_fill (cr, 0, 0)) {
	comac_test_log (
	    ctx,
	    "Error: Found an unexpected point inside rectangular eo-hole\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* holey circle */
    comac_new_path (cr);
    comac_arc (cr, 0, 0, 10, 0, 2 * M_PI);
    comac_arc (cr, 0, 0, 5, 0, 2 * M_PI);
    if (comac_in_fill (cr, 0, 0)) {
	comac_test_log (
	    ctx,
	    "Error: Found an unexpected point inside circular eo-hole\n");
	ret = COMAC_TEST_FAILURE;
    }

    comac_set_fill_rule (cr, COMAC_FILL_RULE_WINDING);

    /* simple rectangle */
    comac_new_path (cr);
    comac_rectangle (cr, -10, -10, 20, 20);
    if (! comac_in_fill (cr, 0, 0)) {
	comac_test_log (ctx, "Error: Failed to find point inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* simple circle */
    comac_new_path (cr);
    comac_arc (cr, 0, 0, 10, 0, 2 * M_PI);
    if (! comac_in_fill (cr, 0, 0)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find point inside circle [nonzero]\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* overlapping circle/rectangle */
    comac_new_path (cr);
    comac_rectangle (cr, -10, -10, 20, 20);
    comac_new_sub_path (cr);
    comac_arc (cr, 0, 0, 10, 0, 2 * M_PI);
    if (! comac_in_fill (cr, 0, 0)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find point inside circle+rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* holey rectangle */
    comac_new_path (cr);
    comac_rectangle (cr, -10, -10, 20, 20);
    comac_rectangle (cr, 5, -5, -10, 10);
    if (comac_in_fill (cr, 0, 0)) {
	comac_test_log (ctx,
			"Error: Found an unexpected point inside rectangular "
			"non-zero-hole\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* holey circle */
    comac_new_path (cr);
    comac_arc (cr, 0, 0, 10, 0, 2 * M_PI);
    comac_arc_negative (cr, 0, 0, 5, 0, -2 * M_PI);
    if (comac_in_fill (cr, 0, 0)) {
	comac_test_log (
	    ctx,
	    "Error: Found an unexpected point inside circular non-zero-hole\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* not a holey circle */
    comac_new_path (cr);
    comac_arc (cr, 0, 0, 10, 0, 2 * M_PI);
    comac_arc (cr, 0, 0, 5, 0, 2 * M_PI);
    if (! comac_in_fill (cr, 0, 0)) {
	comac_test_log (ctx,
			"Error: Failed to find point inside two circles\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* check off-centre */
    comac_new_path (cr);
    comac_arc (cr, 7.5, 0, 10, 0, 2 * M_PI);
    comac_arc_negative (cr, 7.5, 0, 5, 0, -2 * M_PI);
    if (comac_in_fill (cr, 7.5, 0)) {
	comac_test_log (ctx,
			"Error: Found an unexpected point inside off-centre-x "
			"circular non-zero-hole\n");
	ret = COMAC_TEST_FAILURE;
    }
    comac_new_path (cr);
    comac_arc (cr, 0, 7.5, 10, 0, 2 * M_PI);
    comac_arc_negative (cr, 0, 7.5, 5, 0, -2 * M_PI);
    if (comac_in_fill (cr, 0, 7.5)) {
	comac_test_log (ctx,
			"Error: Found an unexpected point inside off-centre-y "
			"circular non-zero-hole\n");
	ret = COMAC_TEST_FAILURE;
    }
    comac_new_path (cr);
    comac_arc (cr, 15, 0, 10, 0, 2 * M_PI);
    if (! comac_in_fill (cr, 15, 0)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find point inside off-centre-x circle\n");
	ret = COMAC_TEST_FAILURE;
    }
    comac_new_path (cr);
    comac_arc (cr, 0, 15, 10, 0, 2 * M_PI);
    if (! comac_in_fill (cr, 0, 15)) {
	comac_test_log (
	    ctx,
	    "Error: Failed to find point inside off-centre-y circle\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* simple rectangle */
    comac_new_path (cr);
    comac_rectangle (cr, 10, 0, 5, 5);
    if (comac_in_fill (cr, 0, 0)) {
	comac_test_log (ctx,
			"Error: Found an unexpected point outside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (comac_in_fill (cr, 20, 20)) {
	comac_test_log (ctx,
			"Error: Found an unexpected point outside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 12.5, 2.5)) {
	comac_test_log (ctx, "Error: Failed to find point inside rectangle\n");
	ret = COMAC_TEST_FAILURE;
    }

    /* off-centre triangle */
    comac_new_path (cr);
    comac_move_to (cr, 10, 0);
    comac_line_to (cr, 15, 5);
    comac_line_to (cr, 5, 5);
    comac_close_path (cr);
    if (comac_in_fill (cr, 0, 0) || comac_in_fill (cr, 5, 0) ||
	comac_in_fill (cr, 15, 0) || comac_in_fill (cr, 20, 0) ||
	comac_in_fill (cr, 0, 10) || comac_in_fill (cr, 10, 10) ||
	comac_in_fill (cr, 20, 10) || comac_in_fill (cr, 7, 2.5) ||
	comac_in_fill (cr, 13, 2.5)) {
	comac_test_log (ctx,
			"Error: Found an unexpected point outside triangle\n"
			"\t(0, 0) -> %s\n"
			"\t(5, 0) -> %s\n"
			"\t(15, 0) -> %s\n"
			"\t(20, 0) -> %s\n"
			"\t(0, 10) -> %s\n"
			"\t(10, 10) -> %s\n"
			"\t(20, 10) -> %s\n"
			"\t(7, 2.5) -> %s\n"
			"\t(13, 2.5) -> %s\n",
			comac_in_fill (cr, 0, 0) ? "inside" : "outside",
			comac_in_fill (cr, 5, 0) ? "inside" : "outside",
			comac_in_fill (cr, 15, 0) ? "inside" : "outside",
			comac_in_fill (cr, 20, 0) ? "inside" : "outside",
			comac_in_fill (cr, 0, 10) ? "inside" : "outside",
			comac_in_fill (cr, 10, 10) ? "inside" : "outside",
			comac_in_fill (cr, 20, 10) ? "inside" : "outside",
			comac_in_fill (cr, 7, 2.5) ? "inside" : "outside",
			comac_in_fill (cr, 13, 2.5) ? "inside" : "outside");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 7.5, 2.5) || ! comac_in_fill (cr, 12.5, 2.5) ||
	! comac_in_fill (cr, 10, 5)) {
	comac_test_log (ctx,
			"Error: Failed to find point on triangle edge\n"
			"\t(7.5, 2.5) -> %s\n"
			"\t(12.5, 2.5) -> %s\n"
			"\t(10, 5) -> %s\n",
			comac_in_fill (cr, 7.5, 2.5) ? "inside" : "outside",
			comac_in_fill (cr, 12.5, 2.5) ? "inside" : "outside",
			comac_in_fill (cr, 10, 5) ? "inside" : "outside");
	ret = COMAC_TEST_FAILURE;
    }
    if (! comac_in_fill (cr, 8, 2.5) || ! comac_in_fill (cr, 12, 2.5)) {
	comac_test_log (ctx, "Error: Failed to find point inside triangle\n");
	ret = COMAC_TEST_FAILURE;
    }

    comac_destroy (cr);

    return ret;
}

COMAC_TEST (in_fill_trapezoid,
	    "Test comac_in_fill",
	    "in, trap", /* keywords */
	    NULL,	/* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
