/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the comac graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

#include "comacint.h"

static comac_color_t const comac_color_white = {
    COMAC_COLORSPACE_RGB,
    {{1.0, 1.0, 1.0, 1.0, 0xffff, 0xffff, 0xffff, 0xffff}}};

static comac_color_t const comac_color_black = {
    COMAC_COLORSPACE_RGB, {{0.0, 0.0, 0.0, 1.0, 0x0, 0x0, 0x0, 0xffff}}};

static comac_color_t const comac_color_transparent = {
    COMAC_COLORSPACE_RGB, {{0.0, 0.0, 0.0, 0.0, 0x0, 0x0, 0x0, 0x0}}};

static comac_color_t const comac_color_magenta = {
    COMAC_COLORSPACE_RGB, {{1.0, 0.0, 1.0, 1.0, 0xffff, 0x0, 0xffff, 0xffff}}};

const comac_color_t *
_comac_stock_color (comac_stock_t stock)
{
    switch (stock) {
    case COMAC_STOCK_WHITE:
	return &comac_color_white;
    case COMAC_STOCK_BLACK:
	return &comac_color_black;
    case COMAC_STOCK_TRANSPARENT:
	return &comac_color_transparent;

    case COMAC_STOCK_NUM_COLORS:
    default:
	ASSERT_NOT_REACHED;
	/* If the user can get here somehow, give a color that indicates a
	 * problem. */
	return &comac_color_magenta;
    }
}

/* Convert a double in [0.0, 1.0] to an integer in [0, 65535]
 * The conversion is designed to choose the integer i such that
 * i / 65535.0 is as close as possible to the input value.
 */
uint16_t
_comac_color_double_to_short (double d)
{
    return d * 65535.0 + 0.5;
}

static void
_comac_color_compute_shorts (comac_color_t *color)
{
    assert (color->colorspace == COMAC_COLORSPACE_RGB);
    color->c.rgb.red_short =
	_comac_color_double_to_short (color->c.rgb.red * color->c.rgb.alpha);
    color->c.rgb.green_short =
	_comac_color_double_to_short (color->c.rgb.green * color->c.rgb.alpha);
    color->c.rgb.blue_short =
	_comac_color_double_to_short (color->c.rgb.blue * color->c.rgb.alpha);
    color->c.rgb.alpha_short =
	_comac_color_double_to_short (color->c.rgb.alpha);
}

void
_comac_color_init_rgba (
    comac_color_t *color, double red, double green, double blue, double alpha)
{
    color->colorspace = COMAC_COLORSPACE_RGB;
    color->c.rgb.red = red;
    color->c.rgb.green = green;
    color->c.rgb.blue = blue;
    color->c.rgb.alpha = alpha;

    _comac_color_compute_shorts (color);
}

void
_comac_color_multiply_alpha (comac_color_t *color, double alpha)
{
    assert (color->colorspace == COMAC_COLORSPACE_RGB);
    color->c.rgb.alpha *= alpha;

    _comac_color_compute_shorts (color);
}

void
_comac_color_get_rgba (comac_color_t *color,
		       double *red,
		       double *green,
		       double *blue,
		       double *alpha)
{
    assert (color->colorspace == COMAC_COLORSPACE_RGB);
    *red = color->c.rgb.red;
    *green = color->c.rgb.green;
    *blue = color->c.rgb.blue;
    *alpha = color->c.rgb.alpha;
}

void
_comac_color_get_rgba_premultiplied (comac_color_t *color,
				     double *red,
				     double *green,
				     double *blue,
				     double *alpha)
{
    assert (color->colorspace == COMAC_COLORSPACE_RGB);
    *red = color->c.rgb.red * color->c.rgb.alpha;
    *green = color->c.rgb.green * color->c.rgb.alpha;
    *blue = color->c.rgb.blue * color->c.rgb.alpha;
    *alpha = color->c.rgb.alpha;
}

/* NB: This function works both for unmultiplied and premultiplied colors */
comac_bool_t
_comac_color_equal (const comac_color_t *color_a, const comac_color_t *color_b)
{
    assert (color_a->colorspace == COMAC_COLORSPACE_RGB);
    assert (color_b->colorspace == COMAC_COLORSPACE_RGB);
    if (color_a == color_b)
	return TRUE;

    if (color_a->c.rgb.alpha_short != color_b->c.rgb.alpha_short)
	return FALSE;

    if (color_a->c.rgb.alpha_short == 0)
	return TRUE;

    return color_a->c.rgb.red_short == color_b->c.rgb.red_short &&
	   color_a->c.rgb.green_short == color_b->c.rgb.green_short &&
	   color_a->c.rgb.blue_short == color_b->c.rgb.blue_short;
}

comac_bool_t
_comac_color_stop_equal (const comac_color_stop_t *color_a,
			 const comac_color_stop_t *color_b)
{
    if (color_a == color_b)
	return TRUE;

    return color_a->alpha_short == color_b->alpha_short &&
	   color_a->red_short == color_b->red_short &&
	   color_a->green_short == color_b->green_short &&
	   color_a->blue_short == color_b->blue_short;
}

comac_content_t
_comac_color_get_content (const comac_color_t *color)
{
    assert (color->colorspace == COMAC_COLORSPACE_RGB);
    if (COMAC_COLOR_IS_OPAQUE (&(color->c.rgb)))
	return COMAC_CONTENT_COLOR;

    if (color->c.rgb.red_short == 0 && color->c.rgb.green_short == 0 &&
	color->c.rgb.blue_short == 0) {
	return COMAC_CONTENT_ALPHA;
    }

    return COMAC_CONTENT_COLOR_ALPHA;
}
