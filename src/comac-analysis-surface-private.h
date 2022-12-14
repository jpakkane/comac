/*
 * Copyright © 2005 Keith Packard
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
 * The Initial Developer of the Original Code is Keith Packard
 *
 * Contributor(s):
 *      Keith Packard <keithp@keithp.com>
 */

#ifndef COMAC_ANALYSIS_SURFACE_H
#define COMAC_ANALYSIS_SURFACE_H

#include "comacint.h"

comac_private comac_surface_t *
_comac_analysis_surface_create (comac_surface_t *target);

comac_private void
_comac_analysis_surface_set_ctm (comac_surface_t *surface,
				 const comac_matrix_t *ctm);

comac_private void
_comac_analysis_surface_get_ctm (comac_surface_t *surface, comac_matrix_t *ctm);

comac_private comac_region_t *
_comac_analysis_surface_get_supported (comac_surface_t *surface);

comac_private comac_region_t *
_comac_analysis_surface_get_unsupported (comac_surface_t *surface);

comac_private comac_bool_t
_comac_analysis_surface_has_supported (comac_surface_t *surface);

comac_private comac_bool_t
_comac_analysis_surface_has_unsupported (comac_surface_t *surface);

comac_private void
_comac_analysis_surface_get_bounding_box (comac_surface_t *surface,
					  comac_box_t *bbox);

comac_private comac_int_status_t
_comac_analysis_surface_merge_status (comac_int_status_t status_a,
				      comac_int_status_t status_b);

comac_private comac_surface_t *
_comac_null_surface_create (comac_content_t content);

#endif /* COMAC_ANALYSIS_SURFACE_H */
