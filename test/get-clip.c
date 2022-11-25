/*
 * Copyright © 2006 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Novell, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Robert O'Callahan <rocallahan@novell.com>
 */

#include "comac-test.h"
#include <stddef.h>

static comac_bool_t
check_count (const comac_test_context_t *ctx,
	     const char *message,
             comac_rectangle_list_t *list, int expected)
{
    if (list->status != COMAC_STATUS_SUCCESS) {
        comac_test_log (ctx, "Error: %s; comac_copy_clip_rectangle_list failed with \"%s\"\n",
                        message, comac_status_to_string(list->status));
        return 0;
    }

    if (list->num_rectangles == expected)
        return 1;
    comac_test_log (ctx, "Error: %s; expected %d rectangles, got %d\n", message,
                    expected, list->num_rectangles);
    return 0;
}

static comac_bool_t
check_unrepresentable (const comac_test_context_t *ctx, const char *message, comac_rectangle_list_t *list)
{
    if (list->status != COMAC_STATUS_CLIP_NOT_REPRESENTABLE) {
        comac_test_log (ctx, "Error: %s; comac_copy_clip_rectangle_list got unexpected result \"%s\"\n"
                        " (we expected COMAC_STATUS_CLIP_NOT_REPRESENTABLE)",
                        message, comac_status_to_string(list->status));
        return 0;
    }
    return 1;
}

static comac_bool_t
check_rectangles_contain (const comac_test_context_t *ctx,
			  const char *message,
                          comac_rectangle_list_t *list,
                          double x, double y, double width, double height)
{
    int i;

    for (i = 0; i < list->num_rectangles; ++i) {
        if (list->rectangles[i].x == x && list->rectangles[i].y == y &&
            list->rectangles[i].width == width && list->rectangles[i].height == height)
            return 1;
    }
    comac_test_log (ctx, "Error: %s; rectangle list does not contain rectangle %f,%f,%f,%f\n",
                    message, x, y, width, height);
    return 0;
}

static comac_bool_t
check_clip_extents (const comac_test_context_t *ctx,
		    const char *message, comac_t *cr,
                    double x, double y, double width, double height)
{
    double ext_x1, ext_y1, ext_x2, ext_y2;
    comac_clip_extents (cr, &ext_x1, &ext_y1, &ext_x2, &ext_y2);
    if (ext_x1 == x && ext_y1 == y && ext_x2 == x + width && ext_y2 == y + height)
        return 1;
    if (width == 0.0 && height == 0.0 && ext_x1 == ext_x2 && ext_y1 == ext_y2)
        return 1;
    comac_test_log (ctx, "Error: %s; clip extents %f,%f,%f,%f should be %f,%f,%f,%f\n",
                    message, ext_x1, ext_y1, ext_x2 - ext_x1, ext_y2 - ext_y1,
                    x, y, width, height);
    return 0;
}

#define SIZE 100

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t        *surface;
    comac_t                *cr;
    comac_rectangle_list_t *rectangle_list;
    const char             *phase;
    comac_bool_t            completed = 0;
    comac_status_t          status;

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, SIZE, SIZE);
    cr = comac_create (surface);
    comac_surface_destroy (surface);


    /* first, test basic stuff. This should not be clipped, it should
       return the surface rectangle. */
    phase = "No clip set";
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_count (ctx, phase, rectangle_list, 1) ||
        ! check_clip_extents (ctx, phase, cr, 0, 0, SIZE, SIZE) ||
        ! check_rectangles_contain (ctx, phase, rectangle_list, 0, 0, SIZE, SIZE))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);

    /* We should get the same results after applying a clip that contains the
       existing clip. */
    phase = "Clip beyond surface extents";
    comac_save (cr);
    comac_rectangle (cr, -10, -10, SIZE + 20 , SIZE + 20);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_count (ctx, phase, rectangle_list, 1) ||
        ! check_clip_extents (ctx, phase, cr, 0, 0, SIZE, SIZE) ||
        ! check_rectangles_contain (ctx, phase, rectangle_list, 0, 0, SIZE, SIZE))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);
    comac_restore (cr);

    /* Test simple clip rect. */
    phase = "Simple clip rect";
    comac_save (cr);
    comac_rectangle (cr, 10, 10, 80, 80);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_count (ctx, phase, rectangle_list, 1) ||
        ! check_clip_extents (ctx, phase, cr, 10, 10, 80, 80) ||
        ! check_rectangles_contain (ctx, phase, rectangle_list, 10, 10, 80, 80))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);
    comac_restore (cr);

    /* Test everything clipped out. */
    phase = "All clipped out";
    comac_save (cr);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_count (ctx, phase, rectangle_list, 0) ||
        ! check_clip_extents (ctx, phase, cr, 0, 0, 0, 0))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);
    comac_restore (cr);

    /* test two clip rects */
    phase = "Two clip rects";
    comac_save (cr);
    comac_rectangle (cr, 10, 10, 10, 10);
    comac_rectangle (cr, 20, 20, 10, 10);
    comac_clip (cr);
    comac_rectangle (cr, 15, 15, 10, 10);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_count (ctx, phase, rectangle_list, 2) ||
        ! check_clip_extents (ctx, phase, cr, 15, 15, 10, 10) ||
        ! check_rectangles_contain (ctx, phase, rectangle_list, 15, 15, 5, 5) ||
        ! check_rectangles_contain (ctx, phase, rectangle_list, 20, 20, 5, 5))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);
    comac_restore (cr);

    /* test non-rectangular clip */
    phase = "Nonrectangular clip";
    comac_save (cr);
    comac_move_to (cr, 0, 0);
    comac_line_to (cr, 100, 100);
    comac_line_to (cr, 100, 0);
    comac_close_path (cr);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
     /* can't get this in one tight user-space rectangle */
    if (! check_unrepresentable (ctx, phase, rectangle_list) ||
        ! check_clip_extents (ctx, phase, cr, 0, 0, 100, 100))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);
    comac_restore (cr);

    phase = "User space, simple scale, getting clip with same transform";
    comac_save (cr);
    comac_scale (cr, 2, 2);
    comac_rectangle (cr, 5, 5, 40, 40);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_count (ctx, phase, rectangle_list, 1) ||
        ! check_clip_extents (ctx, phase, cr, 5, 5, 40, 40) ||
        ! check_rectangles_contain (ctx, phase, rectangle_list, 5, 5, 40, 40))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);
    comac_restore (cr);

    phase = "User space, simple scale, getting clip with no transform";
    comac_save (cr);
    comac_save (cr);
    comac_scale (cr, 2, 2);
    comac_rectangle (cr, 5, 5, 40, 40);
    comac_restore (cr);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_count (ctx, phase, rectangle_list, 1) ||
        ! check_clip_extents (ctx, phase, cr, 10, 10, 80, 80) ||
        ! check_rectangles_contain (ctx, phase, rectangle_list, 10, 10, 80, 80))
    {
	goto FAIL;
    }
    comac_rectangle_list_destroy (rectangle_list);
    comac_restore (cr);

    phase = "User space, rotation, getting clip with no transform";
    comac_save (cr);
    comac_save (cr);
    comac_rotate (cr, 12);
    comac_rectangle (cr, 5, 5, 40, 40);
    comac_restore (cr);
    comac_clip (cr);
    rectangle_list = comac_copy_clip_rectangle_list (cr);
    if (! check_unrepresentable (ctx, phase, rectangle_list))
	goto FAIL;

    completed = 1;
FAIL:
    comac_rectangle_list_destroy (rectangle_list);
    status = comac_status (cr);
    comac_destroy (cr);

    if (!completed)
        return COMAC_TEST_FAILURE;

    return comac_test_status_from_status (ctx, status);
}

COMAC_TEST (get_clip,
	    "Test comac_copy_clip_rectangle_list and comac_clip_extents",
	    "clip, extents", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
