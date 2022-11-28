/*
 * Copyright © 2022 Jussi Pakkanen
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
 * The Initial Developer of the Original Code is Jussi Pakkanen
 *
 * Contributor(s):
 *	Jussi Pakkanen <jpakkane@gmail.com>
 */

#include "comacint.h"

static void
rgb2cmyk (const double *rgb, double *cmyk)
{
    const int red = 0;
    const int green = 1;
    const int blue = 2;
    const int c = 0;
    const int m = 1;
    const int y = 2;
    const int k = 3;
    cmyk[k] = 1.0 - MAX (rgb[red], MAX (rgb[green], rgb[blue]));
    if (cmyk[k] > 0.99) {
	cmyk[m] = cmyk[y] = cmyk[c] = 0.0;
    } else {
	cmyk[c] = (1.0 - rgb[red] - cmyk[k]) / (1.0 - cmyk[k]);
	cmyk[m] = (1.0 - rgb[green] - cmyk[k]) / (1.0 - cmyk[k]);
	cmyk[y] = (1.0 - rgb[blue] - cmyk[k]) / (1.0 - cmyk[k]);
    }
    cmyk[4] = rgb[3];
}

static void
cmyk2rbg (const double *cmyk, double *rgb)
{
    const int red = 0;
    const int green = 1;
    const int blue = 2;
    const int c = 0;
    const int m = 1;
    const int y = 2;
    const int k = 3;
    rgb[red] = (1.0 - cmyk[c]) * (1.0 - cmyk[k]);
    rgb[green] = (1.0 - cmyk[m]) * (1.0 - cmyk[k]);
    rgb[blue] = (1.0 - cmyk[y]) * (1.0 - cmyk[k]);
    rgb[3] = cmyk[4];
}

void
comac_default_color_convert_func (comac_colorspace_t from_colorspace,
				  const double *from_data,
				  comac_colorspace_t to_colorspace,
				  double *to_data,
				  comac_rendering_intent_t intent,
				  void *ctx)
{
    (void) intent;
    (void) ctx;

    switch (to_colorspace) {
    case COMAC_COLORSPACE_RGB:
	switch (from_colorspace) {
	case COMAC_COLORSPACE_RGB:
	    memcpy (to_data, from_data, 4 * sizeof (double));
	    break;
	case COMAC_COLORSPACE_GRAY:
	    to_data[0] = to_data[1] = to_data[2] = from_data[0];
	    to_data[3] = from_data[1];
	    break;
	case COMAC_COLORSPACE_CMYK:
	    cmyk2rbg (from_data, to_data);
	    break;
	case COMAC_COLORSPACE_NUM_COLORSPACES:
	    abort ();
	}
	break;

    case COMAC_COLORSPACE_GRAY:
	switch (from_colorspace) {
	case COMAC_COLORSPACE_RGB:
	    to_data[0] = 0.2126 * from_data[0] + 0.7152 * from_data[1] +
			 0.0722 * from_data[2];
	    to_data[1] = from_data[3];
	    break;
	case COMAC_COLORSPACE_GRAY:
	    memcpy (to_data, from_data, 2 * sizeof (double));
	    break;
	case COMAC_COLORSPACE_CMYK:
	    to_data[0] = from_data[3];
	    to_data[1] = from_data[4];
	    break;
	case COMAC_COLORSPACE_NUM_COLORSPACES:
	    abort ();
	}
	break;

    case COMAC_COLORSPACE_CMYK:
	switch (from_colorspace) {
	case COMAC_COLORSPACE_RGB:
	    rgb2cmyk (from_data, to_data);
	    break;
	case COMAC_COLORSPACE_GRAY:
	    to_data[0] = to_data[1] = to_data[2] = 0;
	    to_data[3] = from_data[0];
	    to_data[4] = from_data[1];
	    break;
	case COMAC_COLORSPACE_CMYK:
	    memcpy (to_data, from_data, 5 * sizeof (double));
	    break;
	case COMAC_COLORSPACE_NUM_COLORSPACES:
	    abort ();
	}
	break;

    case COMAC_COLORSPACE_NUM_COLORSPACES:
	abort ();
    }
}
