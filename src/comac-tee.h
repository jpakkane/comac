/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2009 Chris Wilson
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
 * The Initial Developer of the Original Code is Chris Wilson
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_TEE_H
#define COMAC_TEE_H

#include "comac.h"

#if COMAC_HAS_TEE_SURFACE

COMAC_BEGIN_DECLS

comac_public comac_surface_t *
comac_tee_surface_create (comac_surface_t *master);

comac_public void
comac_tee_surface_add (comac_surface_t *surface, comac_surface_t *target);

comac_public void
comac_tee_surface_remove (comac_surface_t *surface, comac_surface_t *target);

comac_public comac_surface_t *
comac_tee_surface_index (comac_surface_t *surface, unsigned int index);

COMAC_END_DECLS

#else /*COMAC_HAS_TEE_SURFACE*/
#error Comac was not compiled with support for the TEE backend
#endif /*COMAC_HAS_TEE_SURFACE*/

#endif /*COMAC_TEE_H*/
