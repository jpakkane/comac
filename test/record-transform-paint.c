/*
 * Copyright © 2021 Anton Danilkin
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
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *surface2 =
	comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, NULL);
    {
	comac_t *cr2 = comac_create (surface2);
	comac_surface_t *surface3 =
	    comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, NULL);
	{
	    comac_t *cr3 = comac_create (surface3);
	    comac_pattern_t *pattern4 =
		comac_pattern_create_linear (0.0, 0.0, width, height);
	    comac_pattern_add_color_stop_rgb (pattern4, 0, 0, 1, 0);
	    comac_pattern_add_color_stop_rgb (pattern4, 1, 0, 0, 1);
	    comac_set_source (cr3, pattern4);
	    comac_paint (cr3);
	}
	comac_pattern_t *pattern3 = comac_pattern_create_for_surface (surface3);
	comac_matrix_t matrix3;
	comac_matrix_init_scale (&matrix3, 2, 2);
	comac_pattern_set_matrix (pattern3, &matrix3);
	comac_set_source (cr2, pattern3);
	comac_paint (cr2);
    }
    comac_pattern_t *pattern2 = comac_pattern_create_for_surface (surface2);
    comac_matrix_t matrix2;
    comac_matrix_init_scale (&matrix2, 5, 5);
    comac_pattern_set_matrix (pattern2, &matrix2);
    comac_set_source (cr, pattern2);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (record_transform_paint,
	    "Tests paint in nested transformed recording patterns",
	    "record, paint", /* keywords */
	    NULL,	     /* requirements */
	    512,
	    512,
	    NULL,
	    draw)
