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

#ifndef _COMAC_SLOPE_PRIVATE_H
#define _COMAC_SLOPE_PRIVATE_H

#include "comac-types-private.h"
#include "comac-fixed-private.h"

static inline void
_comac_slope_init (comac_slope_t *slope,
		   const comac_point_t *a,
		   const comac_point_t *b)
{
    slope->dx = b->x - a->x;
    slope->dy = b->y - a->y;
}

static inline comac_bool_t
_comac_slope_equal (const comac_slope_t *a, const comac_slope_t *b)
{
    return _comac_int64_eq (_comac_int32x32_64_mul (a->dy, b->dx),
			    _comac_int32x32_64_mul (b->dy, a->dx));
}

static inline comac_bool_t
_comac_slope_backwards (const comac_slope_t *a, const comac_slope_t *b)
{
    return _comac_int64_negative (_comac_int64_add (_comac_int32x32_64_mul (a->dx, b->dx),
						    _comac_int32x32_64_mul (a->dy, b->dy)));
}

comac_private int
_comac_slope_compare (const comac_slope_t *a,
	              const comac_slope_t *b) comac_pure;


#endif /* _COMAC_SLOPE_PRIVATE_H */
