/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2008 Adrian Johnson
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
 * The Initial Developer of the Original Code is Adrian Johnson.
 *
 * Contributor(s):
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#ifndef COMAC_IMAGE_INFO_PRIVATE_H
#define COMAC_IMAGE_INFO_PRIVATE_H

#include "comacint.h"

typedef struct _comac_image_info {
    int width;
    int height;
    int num_components;
    int bits_per_component;
} comac_image_info_t;

comac_private comac_int_status_t
_comac_image_info_get_jpeg_info (comac_image_info_t *info,
				 const unsigned char *data,
				 unsigned long length);

comac_private comac_int_status_t
_comac_image_info_get_jpx_info (comac_image_info_t *info,
				const unsigned char *data,
				unsigned long length);

comac_private comac_int_status_t
_comac_image_info_get_png_info (comac_image_info_t *info,
				const unsigned char *data,
				unsigned long length);

comac_private comac_int_status_t
_comac_image_info_get_jbig2_info (comac_image_info_t *info,
				  const unsigned char *data,
				  unsigned long length);

#endif /* COMAC_IMAGE_INFO_PRIVATE_H */
