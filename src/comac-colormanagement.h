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
 * The Original Code is the Comac graphics library.
 *
 * The Initial Developer of the Original Code is Jussi Pakkanen
 *
 * Contributor(s):
 *	Jussi Pakkanen <jpakkane@gmail.com>
 */

#ifndef COMAC_COLORMANAGEMENT_H
#define COMAC_COLORMANAGEMENT_H

#include "comac.h"

typedef enum {
    COMAC_COLORSPACE_RGB,  // Uncalibrated, maybe this should be sRGB.
    COMAC_COLORSPACE_GRAY, // As above.
    COMAC_COLORSPACE_CMYK,
    COMAC_COLORSPACE_NUM_COLORSPACES,
} comac_colorspace_t;

typedef enum {
    COMAC_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
    COMAC_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC,
    COMAC_RENDERING_INTENT_SATURATION,
    COMAC_RENDERING_INTENT_PERCEPTUAL,
} comac_rendering_intent_t;

/*
 * FIXME, add typedefs for this function, like:
 *
 * typedef struct {
 *   double r;
 *   double g;
 *   double b;
 *   double a;
 * } callback_rgb;
 *
 * So that end users can do
 *
 * const callback_rgb *d = (const callback_rgb) in_data;
 */

// FIXME, maybe this should take arrays instead of individual values
// for more efficient batch processing?

typedef void (*comac_color_convert_cb) (comac_colorspace_t,
					const double *,
					comac_colorspace_t,
					double *,
					comac_rendering_intent_t,
					void *);

void
comac_default_color_convert_func (comac_colorspace_t from_colorspace,
				  const double *from_data,
				  comac_colorspace_t to_colorspace,
				  double *to_data,
				  comac_rendering_intent_t intent,
				  void *ctx);

#endif // COMAC_COLORMANAGEMENT_H
