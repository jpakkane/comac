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
 * The Initial Developer of the Original Code is Intel Corporation
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef TEST_COMPOSITOR_SURFACE_H
#define TEST_COMPOSITOR_SURFACE_H

#include "comac.h"

COMAC_BEGIN_DECLS

comac_surface_t *
_comac_test_fallback_compositor_surface_create (comac_content_t content,
						int width,
						int height);

comac_surface_t *
_comac_test_mask_compositor_surface_create (comac_content_t content,
					    int width,
					    int height);

comac_surface_t *
_comac_test_traps_compositor_surface_create (comac_content_t content,
					     int width,
					     int height);

comac_surface_t *
_comac_test_spans_compositor_surface_create (comac_content_t content,
					     int width,
					     int height);

comac_surface_t *
_comac_test_base_compositor_surface_create (comac_content_t content,
					    int width,
					    int height);

COMAC_END_DECLS

#endif /* TEST_COMPOSITOR_SURFACE_H */
