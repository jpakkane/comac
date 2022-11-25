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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

#ifndef COMAC_IMAGE_SURFACE_INLINE_H
#define COMAC_IMAGE_SURFACE_INLINE_H

#include "comac-surface-private.h"
#include "comac-image-surface-private.h"

COMAC_BEGIN_DECLS

static inline comac_image_surface_t *
_comac_image_surface_create_in_error (comac_status_t status)
{
    return (comac_image_surface_t *) _comac_surface_create_in_error (status);
}

static inline void
_comac_image_surface_set_parent (comac_image_surface_t *image,
				 comac_surface_t *parent)
{
    image->parent = parent;
}

static inline comac_bool_t
_comac_image_surface_is_clone (comac_image_surface_t *image)
{
    return image->parent != NULL;
}

/**
 * _comac_surface_is_image:
 * @surface: a #comac_surface_t
 *
 * Checks if a surface is an #comac_image_surface_t
 *
 * Return value: %TRUE if the surface is an image surface
 **/
static inline comac_bool_t
_comac_surface_is_image (const comac_surface_t *surface)
{
    /* _comac_surface_nil sets a NULL backend so be safe */
    return surface->backend && surface->backend->type == COMAC_SURFACE_TYPE_IMAGE;
}

/**
 * _comac_surface_is_image_source:
 * @surface: a #comac_surface_t
 *
 * Checks if a surface is an #comac_image_source_t
 *
 * Return value: %TRUE if the surface is an image source
 **/
static inline comac_bool_t
_comac_surface_is_image_source (const comac_surface_t *surface)
{
    return surface->backend == &_comac_image_source_backend;
}

COMAC_END_DECLS

#endif /* COMAC_IMAGE_SURFACE_INLINE_H */
