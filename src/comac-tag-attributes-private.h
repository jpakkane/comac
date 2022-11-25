/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2016 Adrian Johnson
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

#ifndef COMAC_TAG_ATTRIBUTES_PRIVATE_H
#define COMAC_TAG_ATTRIBUTES_PRIVATE_H

#include "comac-array-private.h"
#include "comac-error-private.h"
#include "comac-types-private.h"

typedef enum {
    TAG_LINK_INVALID = 0,
    TAG_LINK_EMPTY,
    TAG_LINK_DEST,
    TAG_LINK_URI,
    TAG_LINK_FILE,
} comac_tag_link_type_t;

typedef struct _comac_link_attrs {
    comac_tag_link_type_t link_type;
    comac_array_t rects;
    char *dest;
    char *uri;
    char *file;
    int page;
    comac_bool_t has_pos;
    comac_point_double_t pos;
} comac_link_attrs_t;

typedef struct _comac_dest_attrs {
    char *name;
    double x;
    double y;
    comac_bool_t x_valid;
    comac_bool_t y_valid;
    comac_bool_t internal;
} comac_dest_attrs_t;

typedef struct _comac_ccitt_params {
    int columns;
    int rows;
    int k;
    comac_bool_t end_of_line;
    comac_bool_t encoded_byte_align;
    comac_bool_t end_of_block;
    comac_bool_t black_is_1;
    int damaged_rows_before_error;
} comac_ccitt_params_t;

typedef struct _comac_eps_params {
    comac_box_double_t bbox;
} comac_eps_params_t;

comac_private comac_int_status_t
_comac_tag_parse_link_attributes (const char *attributes,
				  comac_link_attrs_t *link_attrs);

comac_private comac_int_status_t
_comac_tag_parse_dest_attributes (const char *attributes,
				  comac_dest_attrs_t *dest_attrs);

comac_private comac_int_status_t
_comac_tag_parse_ccitt_params (const char *attributes,
			       comac_ccitt_params_t *dest_attrs);

comac_private comac_int_status_t
_comac_tag_parse_eps_params (const char *attributes,
			     comac_eps_params_t *dest_attrs);

#endif /* COMAC_TAG_ATTRIBUTES_PRIVATE_H */
