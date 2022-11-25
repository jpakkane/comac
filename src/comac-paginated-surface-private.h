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
 *	Carl Worth <cworth@cworth.org>
 */

#ifndef COMAC_PAGINATED_SURFACE_H
#define COMAC_PAGINATED_SURFACE_H

#include "comac.h"

#include "comac-surface-private.h"

typedef struct _comac_paginated_surface {
    comac_surface_t base;

    /* The target surface to hold the final result. */
    comac_surface_t *target;

    comac_content_t content;

    /* Paginated-surface specific functions for the target */
    const comac_paginated_surface_backend_t *backend;

    /* A comac_recording_surface to record all operations. To be replayed
     * against target, and also against image surface as necessary for
     * fallbacks. */
    comac_surface_t *recording_surface;

    int page_num;
} comac_paginated_surface_t;

#endif /* COMAC_PAGINATED_SURFACE_H */
