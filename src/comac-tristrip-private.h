/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2011 Intel Corporation
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
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_TRISTRIP_PRIVATE_H
#define COMAC_TRISTRIP_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-error-private.h"
#include "comac-types-private.h"

COMAC_BEGIN_DECLS

struct _comac_tristrip {
    comac_status_t status;

    /* XXX clipping */

    const comac_box_t *limits;
    int num_limits;

    int num_points;
    int size_points;
    comac_point_t *points;
    comac_point_t  points_embedded[64];
};

comac_private void
_comac_tristrip_init (comac_tristrip_t *strip);

comac_private void
_comac_tristrip_limit (comac_tristrip_t	*strip,
		       const comac_box_t	*limits,
		       int			 num_limits);

comac_private void
_comac_tristrip_init_with_clip (comac_tristrip_t *strip,
				const comac_clip_t *clip);

comac_private void
_comac_tristrip_translate (comac_tristrip_t *strip, int x, int y);

comac_private void
_comac_tristrip_move_to (comac_tristrip_t *strip,
			 const comac_point_t *point);

comac_private void
_comac_tristrip_add_point (comac_tristrip_t *strip,
			   const comac_point_t *point);

comac_private void
_comac_tristrip_extents (const comac_tristrip_t *strip,
			 comac_box_t         *extents);

comac_private void
_comac_tristrip_fini (comac_tristrip_t *strip);

#define _comac_tristrip_status(T) ((T)->status)

COMAC_END_DECLS

#endif /* COMAC_TRISTRIP_PRIVATE_H */
