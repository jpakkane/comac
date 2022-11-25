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

#ifndef COMAC_SURFACE_INLINE_H
#define COMAC_SURFACE_INLINE_H

#include "comac-surface-private.h"

static inline comac_status_t
__comac_surface_flush (comac_surface_t *surface, unsigned flags)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    if (surface->backend->flush)
	status = surface->backend->flush (surface, flags);
    return status;
}

static inline comac_surface_t *
_comac_surface_reference (comac_surface_t *surface)
{
    if (!COMAC_REFERENCE_COUNT_IS_INVALID (&surface->ref_count))
	_comac_reference_count_inc (&surface->ref_count);
    return surface;
}

#endif /* COMAC_SURFACE_INLINE_H */
