/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005,2007 Red Hat, Inc.
 * Copyright © 2007 Mathias Hasselmann
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
 *	Mathias Hasselmann <mathias.hasselmann@gmx.de>
 *	Behdad Esfahbod <behdad@behdad.org>
 */

#ifndef COMAC_MUTEX_PRIVATE_H
#define COMAC_MUTEX_PRIVATE_H

#include "comac-mutex-type-private.h"

COMAC_BEGIN_DECLS

#if _COMAC_MUTEX_IMPL_USE_STATIC_INITIALIZER
comac_private void _comac_mutex_initialize (void);
#endif
#if _COMAC_MUTEX_IMPL_USE_STATIC_FINALIZER
comac_private void _comac_mutex_finalize (void);
#endif
/* only if using static initializer and/or finalizer define the boolean */
#if _COMAC_MUTEX_IMPL_USE_STATIC_INITIALIZER || _COMAC_MUTEX_IMPL_USE_STATIC_FINALIZER
  comac_private extern comac_bool_t _comac_mutex_initialized;
#endif

/* Finally, extern the static mutexes and undef */

#define COMAC_MUTEX_DECLARE(mutex) comac_private extern comac_mutex_t mutex;
#include "comac-mutex-list-private.h"
#undef COMAC_MUTEX_DECLARE

COMAC_END_DECLS

#endif
