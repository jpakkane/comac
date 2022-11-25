/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2007 Chris Wilson
 * Copyright © 2010 Andrea Canciani
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
 *	Chris Wilson <chris@chris-wilson.co.uk>
 *	Andrea Canciani <ranma42@gmail.com>
 */

#ifndef COMAC_ATOMIC_PRIVATE_H
#define COMAC_ATOMIC_PRIVATE_H

#include "comac-compiler-private.h"

#include "config.h"

#include <assert.h>

/* The autoconf on OpenBSD 4.5 produces the malformed constant name
 * SIZEOF_VOID__ rather than SIZEOF_VOID_P.  Work around that here. */
#if !defined(SIZEOF_VOID_P) && defined(SIZEOF_VOID__)
# define SIZEOF_VOID_P SIZEOF_VOID__
#endif

COMAC_BEGIN_DECLS

/* C++11 atomic primitives were designed to be more flexible than the
 * __sync_* family of primitives.  Despite the name, they are available
 * in C as well as C++.  The motivating reason for using them is that
 * for _comac_atomic_{int,ptr}_get, the compiler is able to see that
 * the load is intended to be atomic, as opposed to the __sync_*
 * version, below, where the load looks like a plain load.  Having
 * the load appear atomic to the compiler is particular important for
 * tools like ThreadSanitizer so they don't report false positives on
 * memory operations that we intend to be atomic.
 */
#if HAVE_CXX11_ATOMIC_PRIMITIVES

#define HAS_ATOMIC_OPS 1

typedef int comac_atomic_int_t;

static comac_always_inline comac_atomic_int_t
_comac_atomic_int_get (comac_atomic_int_t *x)
{
    return __atomic_load_n(x, __ATOMIC_SEQ_CST);
}

static comac_always_inline comac_atomic_int_t
_comac_atomic_int_get_relaxed (comac_atomic_int_t *x)
{
    return __atomic_load_n(x, __ATOMIC_RELAXED);
}

static comac_always_inline void
_comac_atomic_int_set_relaxed (comac_atomic_int_t *x, comac_atomic_int_t val)
{
    __atomic_store_n(x, val, __ATOMIC_RELAXED);
}

static comac_always_inline void *
_comac_atomic_ptr_get (void **x)
{
    return __atomic_load_n(x, __ATOMIC_SEQ_CST);
}

# define _comac_atomic_int_inc(x) ((void) __atomic_fetch_add(x, 1, __ATOMIC_SEQ_CST))
# define _comac_atomic_int_dec(x) ((void) __atomic_fetch_sub(x, 1, __ATOMIC_SEQ_CST))
# define _comac_atomic_int_dec_and_test(x) (__atomic_fetch_sub(x, 1, __ATOMIC_SEQ_CST) == 1)

#if SIZEOF_VOID_P==SIZEOF_INT
typedef int comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG
typedef long comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
typedef long long comac_atomic_intptr_t;
#else
#error No matching integer pointer type
#endif

static comac_always_inline comac_bool_t
_comac_atomic_int_cmpxchg_impl(comac_atomic_int_t *x,
			       comac_atomic_int_t oldv,
			       comac_atomic_int_t newv)
{
    comac_atomic_int_t expected = oldv;
    return __atomic_compare_exchange_n(x, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#define _comac_atomic_int_cmpxchg(x, oldv, newv) \
  _comac_atomic_int_cmpxchg_impl(x, oldv, newv)

static comac_always_inline comac_atomic_int_t
_comac_atomic_int_cmpxchg_return_old_impl(comac_atomic_int_t *x,
					  comac_atomic_int_t oldv,
					  comac_atomic_int_t newv)
{
    comac_atomic_int_t expected = oldv;
    (void) __atomic_compare_exchange_n(x, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}

#define _comac_atomic_int_cmpxchg_return_old(x, oldv, newv) \
  _comac_atomic_int_cmpxchg_return_old_impl(x, oldv, newv)

static comac_always_inline comac_bool_t
_comac_atomic_ptr_cmpxchg_impl(void **x, void *oldv, void *newv)
{
    void *expected = oldv;
    return __atomic_compare_exchange_n(x, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#define _comac_atomic_ptr_cmpxchg(x, oldv, newv) \
  _comac_atomic_ptr_cmpxchg_impl(x, oldv, newv)

static comac_always_inline void *
_comac_atomic_ptr_cmpxchg_return_old_impl(void **x, void *oldv, void *newv)
{
    void *expected = oldv;
    (void) __atomic_compare_exchange_n(x, &expected, newv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}

#define _comac_atomic_ptr_cmpxchg_return_old(x, oldv, newv) \
  _comac_atomic_ptr_cmpxchg_return_old_impl(x, oldv, newv)

#endif

#if HAVE_GCC_LEGACY_ATOMICS

#define HAS_ATOMIC_OPS 1

typedef int comac_atomic_int_t;

static comac_always_inline comac_atomic_int_t
_comac_atomic_int_get (comac_atomic_int_t *x)
{
    __sync_synchronize ();
    return *x;
}

static comac_always_inline comac_atomic_int_t
_comac_atomic_int_get_relaxed (comac_atomic_int_t *x)
{
    return *x;
}

static comac_always_inline void
_comac_atomic_int_set_relaxed (comac_atomic_int_t *x, comac_atomic_int_t val)
{
    *x = val;
}

static comac_always_inline void *
_comac_atomic_ptr_get (void **x)
{
    __sync_synchronize ();
    return *x;
}

# define _comac_atomic_int_inc(x) ((void) __sync_fetch_and_add(x, 1))
# define _comac_atomic_int_dec(x) ((void) __sync_fetch_and_add(x, -1))
# define _comac_atomic_int_dec_and_test(x) (__sync_fetch_and_add(x, -1) == 1)
# define _comac_atomic_int_cmpxchg(x, oldv, newv) __sync_bool_compare_and_swap (x, oldv, newv)
# define _comac_atomic_int_cmpxchg_return_old(x, oldv, newv) __sync_val_compare_and_swap (x, oldv, newv)

#if SIZEOF_VOID_P==SIZEOF_INT
typedef int comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG
typedef long comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
typedef long long comac_atomic_intptr_t;
#else
#error No matching integer pointer type
#endif

# define _comac_atomic_ptr_cmpxchg(x, oldv, newv) \
    __sync_bool_compare_and_swap ((comac_atomic_intptr_t*)x, (comac_atomic_intptr_t)oldv, (comac_atomic_intptr_t)newv)

# define _comac_atomic_ptr_cmpxchg_return_old(x, oldv, newv) \
    _comac_atomic_intptr_to_voidptr (__sync_val_compare_and_swap ((comac_atomic_intptr_t*)x, (comac_atomic_intptr_t)oldv, (comac_atomic_intptr_t)newv))

#endif

#if HAVE_LIB_ATOMIC_OPS
#include <atomic_ops.h>

#define HAS_ATOMIC_OPS 1

typedef  AO_t comac_atomic_int_t;

# define _comac_atomic_int_get(x) (AO_load_full (x))
# define _comac_atomic_int_get_relaxed(x) (AO_load_full (x))
# define _comac_atomic_int_set_relaxed(x, val) (AO_store_full ((x), (val)))

# define _comac_atomic_int_inc(x) ((void) AO_fetch_and_add1_full(x))
# define _comac_atomic_int_dec(x) ((void) AO_fetch_and_sub1_full(x))
# define _comac_atomic_int_dec_and_test(x) (AO_fetch_and_sub1_full(x) == 1)
# define _comac_atomic_int_cmpxchg(x, oldv, newv) AO_compare_and_swap_full(x, oldv, newv)

#if SIZEOF_VOID_P==SIZEOF_INT
typedef unsigned int comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG
typedef unsigned long comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
typedef unsigned long long comac_atomic_intptr_t;
#else
#error No matching integer pointer type
#endif

# define _comac_atomic_ptr_get(x) _comac_atomic_intptr_to_voidptr (AO_load_full (x))
# define _comac_atomic_ptr_cmpxchg(x, oldv, newv) \
    _comac_atomic_int_cmpxchg ((comac_atomic_intptr_t*)(x), (comac_atomic_intptr_t)oldv, (comac_atomic_intptr_t)newv)

#endif

#if HAVE_OS_ATOMIC_OPS
#include <libkern/OSAtomic.h>

#define HAS_ATOMIC_OPS 1

typedef int32_t comac_atomic_int_t;

# define _comac_atomic_int_get(x) (OSMemoryBarrier(), *(x))
# define _comac_atomic_int_get_relaxed(x) *(x)
# define _comac_atomic_int_set_relaxed(x, val) *(x) = (val)

# define _comac_atomic_int_inc(x) ((void) OSAtomicIncrement32Barrier (x))
# define _comac_atomic_int_dec(x) ((void) OSAtomicDecrement32Barrier (x))
# define _comac_atomic_int_dec_and_test(x) (OSAtomicDecrement32Barrier (x) == 0)
# define _comac_atomic_int_cmpxchg(x, oldv, newv) OSAtomicCompareAndSwap32Barrier(oldv, newv, x)

#if SIZEOF_VOID_P==4
typedef int32_t comac_atomic_intptr_t;
# define _comac_atomic_ptr_cmpxchg(x, oldv, newv) \
    OSAtomicCompareAndSwap32Barrier((comac_atomic_intptr_t)oldv, (comac_atomic_intptr_t)newv, (comac_atomic_intptr_t *)x)

#elif SIZEOF_VOID_P==8
typedef int64_t comac_atomic_intptr_t;
# define _comac_atomic_ptr_cmpxchg(x, oldv, newv) \
    OSAtomicCompareAndSwap64Barrier((comac_atomic_intptr_t)oldv, (comac_atomic_intptr_t)newv, (comac_atomic_intptr_t *)x)

#else
#error No matching integer pointer type
#endif

# define _comac_atomic_ptr_get(x) (OSMemoryBarrier(), *(x))

#endif

#if !defined(HAS_ATOMIC_OPS) && defined(_WIN32)
#include <windows.h>

#define HAS_ATOMIC_OPS 1

typedef int32_t comac_atomic_int_t;

#if SIZEOF_VOID_P==SIZEOF_INT
typedef unsigned int comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG
typedef unsigned long comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
typedef unsigned long long comac_atomic_intptr_t;
#else
#error No matching integer pointer type
#endif

static comac_always_inline comac_atomic_int_t
_comac_atomic_int_get (comac_atomic_int_t *x)
{
    MemoryBarrier ();
    return *x;
}

# define _comac_atomic_int_get_relaxed(x) *(x)
# define _comac_atomic_int_set_relaxed(x, val) *(x) = (val)

# define _comac_atomic_int_inc(x) ((void) InterlockedIncrement (x))
# define _comac_atomic_int_dec(x) ((void) InterlockedDecrement (x))
# define _comac_atomic_int_dec_and_test(x) (InterlockedDecrement (x) == 0)

static comac_always_inline comac_bool_t
_comac_atomic_int_cmpxchg (comac_atomic_int_t *x,
                           comac_atomic_int_t oldv,
                           comac_atomic_int_t newv)
{
    return InterlockedCompareExchange ((unsigned int*)x, (unsigned int)newv, (unsigned int)oldv) == oldv;
}

static comac_always_inline void *
_comac_atomic_ptr_get (void **x)
{
    MemoryBarrier ();
    return *x;
}

static comac_always_inline comac_bool_t
_comac_atomic_ptr_cmpxchg (void **x, void *oldv, void *newv)
{
    return InterlockedCompareExchangePointer (x, newv, oldv) == oldv;
}

static comac_always_inline void *
_comac_atomic_ptr_cmpxchg_return_old (void **x, void *oldv, void *newv)
{
    return InterlockedCompareExchangePointer (x, newv, oldv);
}

#endif /* !defined(HAS_ATOMIC_OPS) && defined(_WIN32) */


#ifndef HAS_ATOMIC_OPS

#if SIZEOF_VOID_P==SIZEOF_INT
typedef unsigned int comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG
typedef unsigned long comac_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
typedef unsigned long long comac_atomic_intptr_t;
#else
#error No matching integer pointer type
#endif

typedef comac_atomic_intptr_t comac_atomic_int_t;

comac_private void
_comac_atomic_int_inc (comac_atomic_int_t *x);

#define _comac_atomic_int_dec(x) _comac_atomic_int_dec_and_test(x)

comac_private comac_bool_t
_comac_atomic_int_dec_and_test (comac_atomic_int_t *x);

comac_private comac_atomic_int_t
_comac_atomic_int_cmpxchg_return_old_impl (comac_atomic_int_t *x, comac_atomic_int_t oldv, comac_atomic_int_t newv);

comac_private void *
_comac_atomic_ptr_cmpxchg_return_old_impl (void **x, void *oldv, void *newv);

#define _comac_atomic_int_cmpxchg_return_old(x, oldv, newv) _comac_atomic_int_cmpxchg_return_old_impl (x, oldv, newv)
#define _comac_atomic_ptr_cmpxchg_return_old(x, oldv, newv) _comac_atomic_ptr_cmpxchg_return_old_impl (x, oldv, newv)

#ifdef ATOMIC_OP_NEEDS_MEMORY_BARRIER
comac_private comac_atomic_int_t
_comac_atomic_int_get (comac_atomic_int_t *x);
comac_private comac_atomic_int_t
_comac_atomic_int_get_relaxed (comac_atomic_int_t *x);
void
_comac_atomic_int_set_relaxed (comac_atomic_int_t *x, comac_atomic_int_t val);
# define _comac_atomic_ptr_get(x) (void *) _comac_atomic_int_get((comac_atomic_int_t *) x)
#else
# define _comac_atomic_int_get(x) (*x)
# define _comac_atomic_int_get_relaxed(x) (*x)
# define _comac_atomic_int_set_relaxed(x, val) (*x) = (val)
# define _comac_atomic_ptr_get(x) (*x)
#endif

#else

/* Workaround GCC complaining about casts */
static comac_always_inline void *
_comac_atomic_intptr_to_voidptr (comac_atomic_intptr_t x)
{
  return (void *) x;
}

static comac_always_inline comac_atomic_int_t
_comac_atomic_int_cmpxchg_return_old_fallback(comac_atomic_int_t *x, comac_atomic_int_t oldv, comac_atomic_int_t newv)
{
    comac_atomic_int_t curr;

    do {
        curr = _comac_atomic_int_get (x);
    } while (curr == oldv && !_comac_atomic_int_cmpxchg (x, oldv, newv));

    return curr;
}

static comac_always_inline void *
_comac_atomic_ptr_cmpxchg_return_old_fallback(void **x, void *oldv, void *newv)
{
    void *curr;

    do {
        curr = _comac_atomic_ptr_get (x);
    } while (curr == oldv && !_comac_atomic_ptr_cmpxchg (x, oldv, newv));

    return curr;
}
#endif

#ifndef _comac_atomic_int_cmpxchg_return_old
#define _comac_atomic_int_cmpxchg_return_old(x, oldv, newv) _comac_atomic_int_cmpxchg_return_old_fallback (x, oldv, newv)
#endif

#ifndef _comac_atomic_ptr_cmpxchg_return_old
#define _comac_atomic_ptr_cmpxchg_return_old(x, oldv, newv) _comac_atomic_ptr_cmpxchg_return_old_fallback (x, oldv, newv)
#endif

#ifndef _comac_atomic_int_cmpxchg
#define _comac_atomic_int_cmpxchg(x, oldv, newv) (_comac_atomic_int_cmpxchg_return_old (x, oldv, newv) == oldv)
#endif

#ifndef _comac_atomic_ptr_cmpxchg
#define _comac_atomic_ptr_cmpxchg(x, oldv, newv) (_comac_atomic_ptr_cmpxchg_return_old (x, oldv, newv) == oldv)
#endif

#define _comac_atomic_uint_get(x) _comac_atomic_int_get(x)
#define _comac_atomic_uint_cmpxchg(x, oldv, newv) \
    _comac_atomic_int_cmpxchg((comac_atomic_int_t *)x, oldv, newv)

#define _comac_status_set_error(status, err) do { \
    int ret__; \
    assert (err < COMAC_STATUS_LAST_STATUS); \
    /* hide compiler warnings about comac_status_t != int (gcc treats its as \
     * an unsigned integer instead, and about ignoring the return value. */  \
    ret__ = _comac_atomic_int_cmpxchg ((comac_atomic_int_t *) status, COMAC_STATUS_SUCCESS, err); \
    (void) ret__; \
} while (0)

typedef comac_atomic_int_t comac_atomic_once_t;

#define COMAC_ATOMIC_ONCE_UNINITIALIZED (0)
#define COMAC_ATOMIC_ONCE_INITIALIZING  (1)
#define COMAC_ATOMIC_ONCE_INITIALIZED   (2)
#define COMAC_ATOMIC_ONCE_INIT          COMAC_ATOMIC_ONCE_UNINITIALIZED

static comac_always_inline comac_bool_t
_comac_atomic_init_once_enter(comac_atomic_once_t *once)
{
    if (likely(_comac_atomic_int_get(once) == COMAC_ATOMIC_ONCE_INITIALIZED))
	return 0;

    if (_comac_atomic_int_cmpxchg(once,
				  COMAC_ATOMIC_ONCE_UNINITIALIZED,
				  COMAC_ATOMIC_ONCE_INITIALIZING))
	return 1;

    while (_comac_atomic_int_get(once) != COMAC_ATOMIC_ONCE_INITIALIZED) {}
    return 0;
}

static comac_always_inline void
_comac_atomic_init_once_leave(comac_atomic_once_t *once)
{
    if (unlikely(!_comac_atomic_int_cmpxchg(once,
					    COMAC_ATOMIC_ONCE_INITIALIZING,
					    COMAC_ATOMIC_ONCE_INITIALIZED)))
	assert (0 && "incorrect use of _comac_atomic_init_once API (once != COMAC_ATOMIC_ONCE_INITIALIZING)");
}

COMAC_END_DECLS

#endif
