/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2012 Intel Corporation
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
 * The Initial Developer of the Original Code is Chris Wilson
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_DAMAGE_PRIVATE_H
#define COMAC_DAMAGE_PRIVATE_H

#include "comac-types-private.h"

#include <pixman.h>

COMAC_BEGIN_DECLS

struct _comac_damage {
    comac_status_t status;
    comac_region_t *region;

    int dirty, remain;
    struct _comac_damage_chunk {
	struct _comac_damage_chunk *next;
	comac_box_t *base;
	int count;
	int size;
    } chunks, *tail;
    comac_box_t boxes[32];
};

comac_private comac_damage_t *
_comac_damage_create (void);

comac_private comac_damage_t *
_comac_damage_create_in_error (comac_status_t status);

comac_private comac_damage_t *
_comac_damage_add_box (comac_damage_t *damage, const comac_box_t *box);

comac_private comac_damage_t *
_comac_damage_add_rectangle (comac_damage_t *damage,
			     const comac_rectangle_int_t *rect);

comac_private comac_damage_t *
_comac_damage_add_region (comac_damage_t *damage, const comac_region_t *region);

comac_private comac_damage_t *
_comac_damage_reduce (comac_damage_t *damage);

comac_private void
_comac_damage_destroy (comac_damage_t *damage);

COMAC_END_DECLS

#endif /* COMAC_DAMAGE_PRIVATE_H */
