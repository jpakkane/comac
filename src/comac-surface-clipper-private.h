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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.u>
 */

#ifndef COMAC_SURFACE_CLIPPER_PRIVATE_H
#define COMAC_SURFACE_CLIPPER_PRIVATE_H

#include "comac-types-private.h"
#include "comac-clip-private.h"

COMAC_BEGIN_DECLS

typedef struct _comac_surface_clipper comac_surface_clipper_t;

typedef comac_status_t
(*comac_surface_clipper_intersect_clip_path_func_t) (comac_surface_clipper_t *,
						     comac_path_fixed_t *,
						     comac_fill_rule_t,
						     double,
						     comac_antialias_t);
struct _comac_surface_clipper {
    comac_clip_t *clip;
    comac_surface_clipper_intersect_clip_path_func_t intersect_clip_path;
};

comac_private comac_status_t
_comac_surface_clipper_set_clip (comac_surface_clipper_t *clipper,
				 const comac_clip_t *clip);

comac_private void
_comac_surface_clipper_init (comac_surface_clipper_t *clipper,
			     comac_surface_clipper_intersect_clip_path_func_t intersect);

comac_private void
_comac_surface_clipper_reset (comac_surface_clipper_t *clipper);

COMAC_END_DECLS

#endif /* COMAC_SURFACE_CLIPPER_PRIVATE_H */
