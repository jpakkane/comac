/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Kristian Høgsberg <krh@redhat.com>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#ifndef COMAC_RECORDING_SURFACE_INLINE_H
#define COMAC_RECORDING_SURFACE_INLINE_H

#include "comac-recording-surface-private.h"

static inline comac_bool_t
_comac_recording_surface_get_bounds (comac_surface_t *surface,
				     comac_rectangle_t *extents)
{
    comac_recording_surface_t *recording = (comac_recording_surface_t *)surface;
    if (recording->unbounded)
	return FALSE;

    *extents = recording->extents_pixels;
    return TRUE;
}

/**
 * _comac_surface_is_recording:
 * @surface: a #comac_surface_t
 *
 * Checks if a surface is a #comac_recording_surface_t
 *
 * Return value: %TRUE if the surface is a recording surface
 **/
static inline comac_bool_t
_comac_surface_is_recording (const comac_surface_t *surface)
{
    return surface->backend->type == COMAC_SURFACE_TYPE_RECORDING;
}

#endif /* COMAC_RECORDING_SURFACE_INLINE_H */
