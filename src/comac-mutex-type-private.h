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

#ifndef COMAC_MUTEX_TYPE_PRIVATE_H
#define COMAC_MUTEX_TYPE_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-mutex-impl-private.h"

/* Only the following four are mandatory at this point */
#ifndef COMAC_MUTEX_IMPL_LOCK
# error "COMAC_MUTEX_IMPL_LOCK not defined.  Check comac-mutex-impl-private.h."
#endif
#ifndef COMAC_MUTEX_IMPL_TRY_LOCK
# error "COMAC_MUTEX_IMPL_TRY_LOCK not defined.  Check comac-mutex-impl-private.h."
#endif
#ifndef COMAC_MUTEX_IMPL_UNLOCK
# error "COMAC_MUTEX_IMPL_UNLOCK not defined.  Check comac-mutex-impl-private.h."
#endif
#ifndef COMAC_MUTEX_IMPL_NIL_INITIALIZER
# error "COMAC_MUTEX_IMPL_NIL_INITIALIZER not defined.  Check comac-mutex-impl-private.h."
#endif
#ifndef COMAC_RECURSIVE_MUTEX_IMPL_INIT
# error "COMAC_RECURSIVE_MUTEX_IMPL_INIT not defined.  Check comac-mutex-impl-private.h."
#endif


/* make sure implementations don't fool us: we decide these ourself */
#undef _COMAC_MUTEX_IMPL_USE_STATIC_INITIALIZER
#undef _COMAC_MUTEX_IMPL_USE_STATIC_FINALIZER


#ifdef COMAC_MUTEX_IMPL_INIT

/* If %COMAC_MUTEX_IMPL_INIT is defined, we may need to initialize all
 * static mutex'es. */
# ifndef COMAC_MUTEX_IMPL_INITIALIZE
#  define COMAC_MUTEX_IMPL_INITIALIZE() do {	\
       if (!_comac_mutex_initialized)	\
           _comac_mutex_initialize ();	\
    } while(0)

/* and make sure we implement the above */
#  define _COMAC_MUTEX_IMPL_USE_STATIC_INITIALIZER 1
# endif /* COMAC_MUTEX_IMPL_INITIALIZE */

#else /* no COMAC_MUTEX_IMPL_INIT */

/* Otherwise we probably don't need to initialize static mutex'es, */
# ifndef COMAC_MUTEX_IMPL_INITIALIZE
#  define COMAC_MUTEX_IMPL_INITIALIZE() COMAC_MUTEX_IMPL_NOOP
# endif /* COMAC_MUTEX_IMPL_INITIALIZE */

/* and dynamic ones can be initialized using the static initializer. */
# define COMAC_MUTEX_IMPL_INIT(mutex) do {				\
      comac_mutex_t _tmp_mutex = COMAC_MUTEX_IMPL_NIL_INITIALIZER;	\
      memcpy (&(mutex), &_tmp_mutex, sizeof (_tmp_mutex));	\
  } while (0)

#endif /* COMAC_MUTEX_IMPL_INIT */

#ifdef COMAC_MUTEX_IMPL_FINI

/* If %COMAC_MUTEX_IMPL_FINI is defined, we may need to finalize all
 * static mutex'es. */
# ifndef COMAC_MUTEX_IMPL_FINALIZE
#  define COMAC_MUTEX_IMPL_FINALIZE() do {	\
       if (_comac_mutex_initialized)	\
           _comac_mutex_finalize ();	\
    } while(0)

/* and make sure we implement the above */
#  define _COMAC_MUTEX_IMPL_USE_STATIC_FINALIZER 1
# endif /* COMAC_MUTEX_IMPL_FINALIZE */

#else /* no COMAC_MUTEX_IMPL_FINI */

/* Otherwise we probably don't need to finalize static mutex'es, */
# ifndef COMAC_MUTEX_IMPL_FINALIZE
#  define COMAC_MUTEX_IMPL_FINALIZE() COMAC_MUTEX_IMPL_NOOP
# endif /* COMAC_MUTEX_IMPL_FINALIZE */

/* neither do the dynamic ones. */
# define COMAC_MUTEX_IMPL_FINI(mutex)	COMAC_MUTEX_IMPL_NOOP1(mutex)

#endif /* COMAC_MUTEX_IMPL_FINI */


#ifndef _COMAC_MUTEX_IMPL_USE_STATIC_INITIALIZER
#define _COMAC_MUTEX_IMPL_USE_STATIC_INITIALIZER 0
#endif
#ifndef _COMAC_MUTEX_IMPL_USE_STATIC_FINALIZER
#define _COMAC_MUTEX_IMPL_USE_STATIC_FINALIZER 0
#endif


/* Make sure everything we want is defined */
#ifndef COMAC_MUTEX_IMPL_INITIALIZE
# error "COMAC_MUTEX_IMPL_INITIALIZE not defined"
#endif
#ifndef COMAC_MUTEX_IMPL_FINALIZE
# error "COMAC_MUTEX_IMPL_FINALIZE not defined"
#endif
#ifndef COMAC_MUTEX_IMPL_LOCK
# error "COMAC_MUTEX_IMPL_LOCK not defined"
#endif
#ifndef COMAC_MUTEX_IMPL_TRY_LOCK
# error "COMAC_MUTEX_IMPL_TRY_LOCK not defined"
#endif
#ifndef COMAC_MUTEX_IMPL_UNLOCK
# error "COMAC_MUTEX_IMPL_UNLOCK not defined"
#endif
#ifndef COMAC_MUTEX_IMPL_INIT
# error "COMAC_MUTEX_IMPL_INIT not defined"
#endif
#ifndef COMAC_MUTEX_IMPL_FINI
# error "COMAC_MUTEX_IMPL_FINI not defined"
#endif
#ifndef COMAC_MUTEX_IMPL_NIL_INITIALIZER
# error "COMAC_MUTEX_IMPL_NIL_INITIALIZER not defined"
#endif


/* Public interface. */

/* By default it simply uses the implementation provided.
 * But we can provide for debugging features by overriding them */

#ifndef COMAC_MUTEX_DEBUG
typedef comac_mutex_impl_t comac_mutex_t;
typedef comac_recursive_mutex_impl_t comac_recursive_mutex_t;
#else
# define comac_mutex_t			comac_mutex_impl_t
#endif

#define COMAC_MUTEX_INITIALIZE		COMAC_MUTEX_IMPL_INITIALIZE
#define COMAC_MUTEX_FINALIZE		COMAC_MUTEX_IMPL_FINALIZE
#define COMAC_MUTEX_LOCK		COMAC_MUTEX_IMPL_LOCK
#define COMAC_MUTEX_TRY_LOCK		COMAC_MUTEX_IMPL_TRY_LOCK
#define COMAC_MUTEX_UNLOCK		COMAC_MUTEX_IMPL_UNLOCK
#define COMAC_MUTEX_INIT		COMAC_MUTEX_IMPL_INIT
#define COMAC_MUTEX_FINI		COMAC_MUTEX_IMPL_FINI
#define COMAC_MUTEX_NIL_INITIALIZER	COMAC_MUTEX_IMPL_NIL_INITIALIZER

#define COMAC_RECURSIVE_MUTEX_INIT		COMAC_RECURSIVE_MUTEX_IMPL_INIT
#define COMAC_RECURSIVE_MUTEX_NIL_INITIALIZER	COMAC_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER

#ifndef COMAC_MUTEX_IS_LOCKED
# define COMAC_MUTEX_IS_LOCKED(name) 1
#endif
#ifndef COMAC_MUTEX_IS_UNLOCKED
# define COMAC_MUTEX_IS_UNLOCKED(name) 1
#endif


/* Debugging support */

#ifdef COMAC_MUTEX_DEBUG

/* TODO add mutex debugging facilities here (eg deadlock detection) */

#endif /* COMAC_MUTEX_DEBUG */

#endif
