/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Owen Taylor <otaylor@redhat.com>
 *      Vladimir Vukicevic <vladimir@pobox.com>
 *      Søren Sandmann <sandmann@daimi.au.dk>
 */

#ifndef COMAC_REGION_PRIVATE_H
#define COMAC_REGION_PRIVATE_H

#include "comac-types-private.h"
#include "comac-reference-count-private.h"

#include <pixman.h>

COMAC_BEGIN_DECLS

struct _comac_region {
    comac_reference_count_t ref_count;
    comac_status_t status;

    pixman_region32_t rgn;
};

comac_private comac_region_t *
_comac_region_create_in_error (comac_status_t status);

comac_private void
_comac_region_init (comac_region_t *region);

comac_private void
_comac_region_init_rectangle (comac_region_t *region,
			      const comac_rectangle_int_t *rectangle);

comac_private void
_comac_region_fini (comac_region_t *region);

comac_private comac_region_t *
_comac_region_create_from_boxes (const comac_box_t *boxes, int count);

comac_private comac_box_t *
_comac_region_get_boxes (const comac_region_t *region, int *nbox);

COMAC_END_DECLS

#endif /* COMAC_REGION_PRIVATE_H */
