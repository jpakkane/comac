/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2009 Intel Corporation
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
 * The Initial Developer of the Original Code is Intel Corporation.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_SURFACE_SNAPSHOT_INLINE_H
#define COMAC_SURFACE_SNAPSHOT_INLINE_H

#include "comac-surface-snapshot-private.h"
#include "comac-surface-inline.h"

static inline comac_bool_t
_comac_surface_snapshot_is_reused (comac_surface_t *surface)
{
    return COMAC_REFERENCE_COUNT_GET_VALUE (&surface->ref_count) > 2;
}

static inline comac_surface_t *
_comac_surface_snapshot_get_target (comac_surface_t *surface)
{
    comac_surface_snapshot_t *snapshot = (comac_surface_snapshot_t *) surface;
    comac_surface_t *target;

    COMAC_MUTEX_LOCK (snapshot->mutex);
    target = _comac_surface_reference (snapshot->target);
    COMAC_MUTEX_UNLOCK (snapshot->mutex);

    return target;
}

static inline comac_bool_t
_comac_surface_is_snapshot (comac_surface_t *surface)
{
    return surface->backend->type == (comac_surface_type_t)COMAC_INTERNAL_SURFACE_TYPE_SNAPSHOT;
}

#endif /* COMAC_SURFACE_SNAPSHOT_INLINE_H */
