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

#ifndef _COMAC_ERROR_PRIVATE_H_
#define _COMAC_ERROR_PRIVATE_H_

#include "comac.h"
#include "comac-compiler-private.h"
#include "comac-types-private.h"

#include <assert.h>

COMAC_BEGIN_DECLS

/* _comac_int_status: internal status
 *
 * Sure wish C had a real enum type so that this would be distinct
 * from #comac_status_t. Oh well, without that, I'll use this bogus 100
 * offset.  We want to keep it fit in int8_t as the compiler may choose
 * that for #comac_status_t
 */
enum _comac_int_status {
    COMAC_INT_STATUS_SUCCESS = 0,

    COMAC_INT_STATUS_NO_MEMORY,
    COMAC_INT_STATUS_INVALID_RESTORE,
    COMAC_INT_STATUS_INVALID_POP_GROUP,
    COMAC_INT_STATUS_NO_CURRENT_POINT,
    COMAC_INT_STATUS_INVALID_MATRIX,
    COMAC_INT_STATUS_INVALID_STATUS,
    COMAC_INT_STATUS_NULL_POINTER,
    COMAC_INT_STATUS_INVALID_STRING,
    COMAC_INT_STATUS_INVALID_PATH_DATA,
    COMAC_INT_STATUS_READ_ERROR,
    COMAC_INT_STATUS_WRITE_ERROR,
    COMAC_INT_STATUS_SURFACE_FINISHED,
    COMAC_INT_STATUS_SURFACE_TYPE_MISMATCH,
    COMAC_INT_STATUS_PATTERN_TYPE_MISMATCH,
    COMAC_INT_STATUS_INVALID_CONTENT,
    COMAC_INT_STATUS_INVALID_FORMAT,
    COMAC_INT_STATUS_INVALID_VISUAL,
    COMAC_INT_STATUS_FILE_NOT_FOUND,
    COMAC_INT_STATUS_INVALID_DASH,
    COMAC_INT_STATUS_INVALID_DSC_COMMENT,
    COMAC_INT_STATUS_INVALID_INDEX,
    COMAC_INT_STATUS_CLIP_NOT_REPRESENTABLE,
    COMAC_INT_STATUS_TEMP_FILE_ERROR,
    COMAC_INT_STATUS_INVALID_STRIDE,
    COMAC_INT_STATUS_FONT_TYPE_MISMATCH,
    COMAC_INT_STATUS_USER_FONT_IMMUTABLE,
    COMAC_INT_STATUS_USER_FONT_ERROR,
    COMAC_INT_STATUS_NEGATIVE_COUNT,
    COMAC_INT_STATUS_INVALID_CLUSTERS,
    COMAC_INT_STATUS_INVALID_SLANT,
    COMAC_INT_STATUS_INVALID_WEIGHT,
    COMAC_INT_STATUS_INVALID_SIZE,
    COMAC_INT_STATUS_USER_FONT_NOT_IMPLEMENTED,
    COMAC_INT_STATUS_DEVICE_TYPE_MISMATCH,
    COMAC_INT_STATUS_DEVICE_ERROR,
    COMAC_INT_STATUS_INVALID_MESH_CONSTRUCTION,
    COMAC_INT_STATUS_DEVICE_FINISHED,
    COMAC_INT_STATUS_JBIG2_GLOBAL_MISSING,
    COMAC_INT_STATUS_PNG_ERROR,
    COMAC_INT_STATUS_FREETYPE_ERROR,
    COMAC_INT_STATUS_WIN32_GDI_ERROR,
    COMAC_INT_STATUS_TAG_ERROR,
    COMAC_INT_STATUS_DWRITE_ERROR,

    COMAC_INT_STATUS_LAST_STATUS,

    COMAC_INT_STATUS_UNSUPPORTED = 100,
    COMAC_INT_STATUS_DEGENERATE,
    COMAC_INT_STATUS_NOTHING_TO_DO,
    COMAC_INT_STATUS_FLATTEN_TRANSPARENCY,
    COMAC_INT_STATUS_IMAGE_FALLBACK,
    COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN,
};

typedef enum _comac_int_status comac_int_status_t;

#define _comac_status_is_error(status)                                         \
    ((status) != COMAC_STATUS_SUCCESS && (status) < COMAC_STATUS_LAST_STATUS)

#define _comac_int_status_is_error(status)                                     \
    ((status) != COMAC_INT_STATUS_SUCCESS &&                                   \
     (status) < COMAC_INT_STATUS_LAST_STATUS)

comac_private comac_status_t
_comac_error (comac_status_t status);

/* hide compiler warnings when discarding the return value */
#define _comac_error_throw(status)                                             \
    do {                                                                       \
	comac_status_t status__ = _comac_error (status);                       \
	(void) status__;                                                       \
    } while (0)

COMAC_END_DECLS

#endif /* _COMAC_ERROR_PRIVATE_H_ */
