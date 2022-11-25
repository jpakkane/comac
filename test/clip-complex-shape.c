/*
 * Copyright 2011 SCore Corporation
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
 * Author: Taekyun Kim <podain77@gmail.com>
 */

#include "comac-test.h"

static void
rounded_rectangle (
    comac_t *cr, double x, double y, double width, double height, double radius)
{
    comac_move_to (cr, x, y + radius);
    comac_line_to (cr, x, y + height - radius);
    comac_curve_to (cr,
		    x,
		    y + height - radius / 2.0,
		    x + radius / 2.0,
		    y + height,
		    x + radius,
		    y + height);
    comac_line_to (cr, x + width - radius, y + height);
    comac_curve_to (cr,
		    x + width - radius / 2.0,
		    y + height,
		    x + width,
		    y + height - radius / 2.0,
		    x + width,
		    y + height - radius);
    comac_line_to (cr, x + width, y + radius);
    comac_curve_to (cr,
		    x + width,
		    y + radius / 2.0,
		    x + width - radius / 2.0,
		    y,
		    x + width - radius,
		    y);
    comac_line_to (cr, x + radius, y);
    comac_curve_to (cr,
		    x + radius / 2.0,
		    y,
		    x,
		    y + radius / 2.0,
		    x,
		    y + radius);
    comac_close_path (cr);
}

static void
background (comac_t *cr)
{
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
    comac_paint (cr);
}

static void
foreground (comac_t *cr)
{
    comac_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_rectangle (cr, 20, 20, 60, 60);
    comac_fill (cr);
}

static comac_test_status_t
clip_eo_mono (comac_t *cr, int width, int height)
{

    background (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    rounded_rectangle (cr, 0, 0, 40, 100, 10);
    rounded_rectangle (cr, 60, 0, 40, 100, 10);
    comac_clip (cr);

    foreground (cr);

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
clip_eo_aa (comac_t *cr, int width, int height)
{
    background (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_DEFAULT);
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);
    rounded_rectangle (cr, 0, 0, 40, 100, 10);
    rounded_rectangle (cr, 60, 0, 40, 100, 10);
    comac_clip (cr);

    foreground (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_complex_shape_eo_mono,
	    "Test clipping against a complex shape",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    100,
	    100,
	    NULL,
	    clip_eo_mono)
COMAC_TEST (clip_complex_shape_eo_aa,
	    "Test clipping against a complex shape",
	    "clip", /* keywords */
	    NULL,   /* requirements */
	    100,
	    100,
	    NULL,
	    clip_eo_aa)
