/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2007 Chris Wilson
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
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-atomic-private.h"
#include "comac-mutex-private.h"

#ifdef HAS_ATOMIC_OPS
COMPILE_TIME_ASSERT (sizeof (void *) == sizeof (int) ||
		     sizeof (void *) == sizeof (long) ||
		     sizeof (void *) == sizeof (long long));
#else
void
_comac_atomic_int_inc (comac_atomic_intptr_t *x)
{
    COMAC_MUTEX_LOCK (_comac_atomic_mutex);
    *x += 1;
    COMAC_MUTEX_UNLOCK (_comac_atomic_mutex);
}

comac_bool_t
_comac_atomic_int_dec_and_test (comac_atomic_intptr_t *x)
{
    comac_bool_t ret;

    COMAC_MUTEX_LOCK (_comac_atomic_mutex);
    ret = --*x == 0;
    COMAC_MUTEX_UNLOCK (_comac_atomic_mutex);

    return ret;
}

comac_atomic_intptr_t
_comac_atomic_int_cmpxchg_return_old_impl (comac_atomic_intptr_t *x,
					   comac_atomic_intptr_t oldv,
					   comac_atomic_intptr_t newv)
{
    comac_atomic_intptr_t ret;

    COMAC_MUTEX_LOCK (_comac_atomic_mutex);
    ret = *x;
    if (ret == oldv)
	*x = newv;
    COMAC_MUTEX_UNLOCK (_comac_atomic_mutex);

    return ret;
}

void *
_comac_atomic_ptr_cmpxchg_return_old_impl (void **x, void *oldv, void *newv)
{
    void *ret;

    COMAC_MUTEX_LOCK (_comac_atomic_mutex);
    ret = *x;
    if (ret == oldv)
	*x = newv;
    COMAC_MUTEX_UNLOCK (_comac_atomic_mutex);

    return ret;
}

#ifdef ATOMIC_OP_NEEDS_MEMORY_BARRIER
comac_atomic_intptr_t
_comac_atomic_int_get (comac_atomic_intptr_t *x)
{
    comac_atomic_intptr_t ret;

    COMAC_MUTEX_LOCK (_comac_atomic_mutex);
    ret = *x;
    COMAC_MUTEX_UNLOCK (_comac_atomic_mutex);

    return ret;
}

comac_atomic_intptr_t
_comac_atomic_int_get_relaxed (comac_atomic_intptr_t *x)
{
    return _comac_atomic_int_get (x);
}

void
_comac_atomic_int_set_relaxed (comac_atomic_intptr_t *x,
			       comac_atomic_intptr_t val)
{
    COMAC_MUTEX_LOCK (_comac_atomic_mutex);
    *x = val;
    COMAC_MUTEX_UNLOCK (_comac_atomic_mutex);
}
#endif

#endif
