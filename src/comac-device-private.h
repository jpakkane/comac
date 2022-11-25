/* Comac - a vector graphics library with display and print output
 *
 * Copyright © 2009 Intel Corporation
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
 * The Initial Developer of the Original Code is Intel Corporation.
 *
 * Contributors(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef _COMAC_DEVICE_PRIVATE_H_
#define _COMAC_DEVICE_PRIVATE_H_

#include "comac-compiler-private.h"
#include "comac-mutex-private.h"
#include "comac-reference-count-private.h"
#include "comac-types-private.h"

struct _comac_device {
    comac_reference_count_t ref_count;
    comac_status_t status;
    comac_user_data_array_t user_data;

    const comac_device_backend_t *backend;

    comac_recursive_mutex_t mutex;
    unsigned mutex_depth;

    comac_bool_t finished;
};

struct _comac_device_backend {
    comac_device_type_t type;

    void (*lock) (void *device);
    void (*unlock) (void *device);

    comac_warn comac_status_t (*flush) (void *device);
    void (*finish) (void *device);
    void (*destroy) (void *device);
};

comac_private comac_device_t *
_comac_device_create_in_error (comac_status_t status);

comac_private void
_comac_device_init (comac_device_t *device,
		    const comac_device_backend_t *backend);

comac_private comac_status_t
_comac_device_set_error (comac_device_t *device, comac_status_t error);

#endif /* _COMAC_DEVICE_PRIVATE_H_ */
