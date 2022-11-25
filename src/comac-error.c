/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

#include "comacint.h"
#include "comac-private.h"

#include "comac-compiler-private.h"
#include "comac-error-private.h"

#include <assert.h>

/**
 * _comac_error:
 * @status: a status value indicating an error, (eg. not
 * %COMAC_STATUS_SUCCESS)
 *
 * Checks that status is an error status, but does nothing else.
 *
 * All assignments of an error status to any user-visible object
 * within the comac application should result in a call to
 * _comac_error().
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _comac_error() to generate a stack trace for when the
 * user causes comac to detect an error.
 *
 * Return value: the error status.
 **/
comac_status_t
_comac_error (comac_status_t status)
{
    COMAC_ENSURE_UNIQUE;
    assert (_comac_status_is_error (status));

    return status;
}

COMPILE_TIME_ASSERT ((int)COMAC_INT_STATUS_LAST_STATUS == (int)COMAC_STATUS_LAST_STATUS);
