/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@redhat.com>
 */

#ifndef COMAC_PATH_PRIVATE_H
#define COMAC_PATH_PRIVATE_H

#include "comacint.h"

comac_private comac_path_t *
_comac_path_create (comac_path_fixed_t *path, comac_t *cr);

comac_private comac_path_t *
_comac_path_create_flat (comac_path_fixed_t *path, comac_t *cr);

comac_private comac_path_t *
_comac_path_create_in_error (comac_status_t status);

comac_private comac_status_t
_comac_path_append_to_context (const comac_path_t *path, comac_t *cr);

#endif /* COMAC_PATH_DATA_PRIVATE_H */
