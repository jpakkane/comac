/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2004 Keith Packard
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
 *	Keith R. Packard <keithp@keithp.com>
 *
 */

#ifndef COMAC_WIDEINT_H
#define COMAC_WIDEINT_H

#include "comac-wideint-type-private.h"

#include "comac-compiler-private.h"

/*
 * 64-bit datatypes.  Two separate implementations, one using
 * built-in 64-bit signed/unsigned types another implemented
 * as a pair of 32-bit ints
 */

#define I comac_private comac_const

#if ! HAVE_UINT64_T

comac_uquorem64_t I
_comac_uint64_divrem (comac_uint64_t num, comac_uint64_t den);

comac_uint64_t I
_comac_double_to_uint64 (double i);
double I
_comac_uint64_to_double (comac_uint64_t i);
comac_int64_t I
_comac_double_to_int64 (double i);
double I
_comac_int64_to_double (comac_uint64_t i);

comac_uint64_t I
_comac_uint32_to_uint64 (uint32_t i);
#define _comac_uint64_to_uint32(a) ((a).lo)
comac_uint64_t I
_comac_uint64_add (comac_uint64_t a, comac_uint64_t b);
comac_uint64_t I
_comac_uint64_sub (comac_uint64_t a, comac_uint64_t b);
comac_uint64_t I
_comac_uint64_mul (comac_uint64_t a, comac_uint64_t b);
comac_uint64_t I
_comac_uint32x32_64_mul (uint32_t a, uint32_t b);
comac_uint64_t I
_comac_uint64_lsl (comac_uint64_t a, int shift);
comac_uint64_t I
_comac_uint64_rsl (comac_uint64_t a, int shift);
comac_uint64_t I
_comac_uint64_rsa (comac_uint64_t a, int shift);
int I
_comac_uint64_lt (comac_uint64_t a, comac_uint64_t b);
int I
_comac_uint64_cmp (comac_uint64_t a, comac_uint64_t b);
int I
_comac_uint64_eq (comac_uint64_t a, comac_uint64_t b);
comac_uint64_t I
_comac_uint64_negate (comac_uint64_t a);
#define _comac_uint64_is_zero(a) ((a).hi == 0 && (a).lo == 0)
#define _comac_uint64_negative(a) (((int32_t) ((a).hi)) < 0)
comac_uint64_t I
_comac_uint64_not (comac_uint64_t a);

#define _comac_uint64_to_int64(i) (i)
#define _comac_int64_to_uint64(i) (i)

comac_int64_t I
_comac_int32_to_int64 (int32_t i);
#define _comac_int64_to_int32(a) ((int32_t) _comac_uint64_to_uint32 (a))
#define _comac_int64_add(a, b) _comac_uint64_add (a, b)
#define _comac_int64_sub(a, b) _comac_uint64_sub (a, b)
#define _comac_int64_mul(a, b) _comac_uint64_mul (a, b)
comac_int64_t I
_comac_int32x32_64_mul (int32_t a, int32_t b);
int I
_comac_int64_lt (comac_int64_t a, comac_int64_t b);
int I
_comac_int64_cmp (comac_int64_t a, comac_int64_t b);
#define _comac_int64_is_zero(a) _comac_uint64_is_zero (a)
#define _comac_int64_eq(a, b) _comac_uint64_eq (a, b)
#define _comac_int64_lsl(a, b) _comac_uint64_lsl (a, b)
#define _comac_int64_rsl(a, b) _comac_uint64_rsl (a, b)
#define _comac_int64_rsa(a, b) _comac_uint64_rsa (a, b)
#define _comac_int64_negate(a) _comac_uint64_negate (a)
#define _comac_int64_negative(a) (((int32_t) ((a).hi)) < 0)
#define _comac_int64_not(a) _comac_uint64_not (a)

#else

static inline comac_uquorem64_t
_comac_uint64_divrem (comac_uint64_t num, comac_uint64_t den)
{
    comac_uquorem64_t qr;

    qr.quo = num / den;
    qr.rem = num % den;
    return qr;
}

/*
 * These need to be functions or gcc will complain when used on the
 * result of a function:
 *
 * warning: cast from function call of type ‘#comac_uint64_t’ to
 * non-matching type ‘double’
 */
static comac_always_inline comac_const comac_uint64_t
_comac_double_to_uint64 (double i)
{
    return i;
}
static comac_always_inline comac_const double
_comac_uint64_to_double (comac_uint64_t i)
{
    return i;
}

static comac_always_inline comac_int64_t I
_comac_double_to_int64 (double i)
{
    return i;
}
static comac_always_inline double I
_comac_int64_to_double (comac_int64_t i)
{
    return i;
}

#define _comac_uint32_to_uint64(i) ((uint64_t) (i))
#define _comac_uint64_to_uint32(i) ((uint32_t) (i))
#define _comac_uint64_add(a, b) ((a) + (b))
#define _comac_uint64_sub(a, b) ((a) - (b))
#define _comac_uint64_mul(a, b) ((a) * (b))
#define _comac_uint32x32_64_mul(a, b) ((uint64_t) (a) * (b))
#define _comac_uint64_lsl(a, b) ((a) << (b))
#define _comac_uint64_rsl(a, b) ((uint64_t) (a) >> (b))
#define _comac_uint64_rsa(a, b) ((uint64_t) ((int64_t) (a) >> (b)))
#define _comac_uint64_lt(a, b) ((a) < (b))
#define _comac_uint64_cmp(a, b) ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define _comac_uint64_is_zero(a) ((a) == 0)
#define _comac_uint64_eq(a, b) ((a) == (b))
#define _comac_uint64_negate(a) ((uint64_t) - ((int64_t) (a)))
#define _comac_uint64_negative(a) ((int64_t) (a) < 0)
#define _comac_uint64_not(a) (~(a))

#define _comac_uint64_to_int64(i) ((int64_t) (i))
#define _comac_int64_to_uint64(i) ((uint64_t) (i))

#define _comac_int32_to_int64(i) ((int64_t) (i))
#define _comac_int64_to_int32(i) ((int32_t) (i))
#define _comac_int64_add(a, b) ((a) + (b))
#define _comac_int64_sub(a, b) ((a) - (b))
#define _comac_int64_mul(a, b) ((a) * (b))
#define _comac_int32x32_64_mul(a, b) ((int64_t) (a) * (b))
#define _comac_int64_lt(a, b) ((a) < (b))
#define _comac_int64_cmp(a, b) ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define _comac_int64_is_zero(a) ((a) == 0)
#define _comac_int64_eq(a, b) ((a) == (b))
#define _comac_int64_lsl(a, b) ((a) << (b))
#define _comac_int64_rsl(a, b) ((int64_t) ((uint64_t) (a) >> (b)))
#define _comac_int64_rsa(a, b) ((int64_t) (a) >> (b))
#define _comac_int64_negate(a) (-(a))
#define _comac_int64_negative(a) ((a) < 0)
#define _comac_int64_not(a) (~(a))

#endif

/*
 * 64-bit comparisons derived from lt or eq
 */
#define _comac_uint64_le(a, b) (! _comac_uint64_gt (a, b))
#define _comac_uint64_ne(a, b) (! _comac_uint64_eq (a, b))
#define _comac_uint64_ge(a, b) (! _comac_uint64_lt (a, b))
#define _comac_uint64_gt(a, b) _comac_uint64_lt (b, a)

#define _comac_int64_le(a, b) (! _comac_int64_gt (a, b))
#define _comac_int64_ne(a, b) (! _comac_int64_eq (a, b))
#define _comac_int64_ge(a, b) (! _comac_int64_lt (a, b))
#define _comac_int64_gt(a, b) _comac_int64_lt (b, a)

/*
 * As the C implementation always computes both, create
 * a function which returns both for the 'native' type as well
 */

static inline comac_quorem64_t
_comac_int64_divrem (comac_int64_t num, comac_int64_t den)
{
    int num_neg = _comac_int64_negative (num);
    int den_neg = _comac_int64_negative (den);
    comac_uquorem64_t uqr;
    comac_quorem64_t qr;

    if (num_neg)
	num = _comac_int64_negate (num);
    if (den_neg)
	den = _comac_int64_negate (den);
    uqr = _comac_uint64_divrem (num, den);
    if (num_neg)
	qr.rem = _comac_int64_negate (uqr.rem);
    else
	qr.rem = uqr.rem;
    if (num_neg != den_neg)
	qr.quo = (comac_int64_t) _comac_int64_negate (uqr.quo);
    else
	qr.quo = (comac_int64_t) uqr.quo;
    return qr;
}

static inline int32_t
_comac_int64_32_div (comac_int64_t num, int32_t den)
{
#if ! HAVE_UINT64_T
    return _comac_int64_to_int32 (
	_comac_int64_divrem (num, _comac_int32_to_int64 (den)).quo);
#else
    return num / den;
#endif
}

/*
 * 128-bit datatypes.  Again, provide two implementations in
 * case the machine has a native 128-bit datatype.  GCC supports int128_t
 * on ia64
 */

#if ! HAVE_UINT128_T

comac_uint128_t I
_comac_uint32_to_uint128 (uint32_t i);
comac_uint128_t I
_comac_uint64_to_uint128 (comac_uint64_t i);
#define _comac_uint128_to_uint64(a) ((a).lo)
#define _comac_uint128_to_uint32(a)                                            \
    _comac_uint64_to_uint32 (_comac_uint128_to_uint64 (a))
comac_uint128_t I
_comac_uint128_add (comac_uint128_t a, comac_uint128_t b);
comac_uint128_t I
_comac_uint128_sub (comac_uint128_t a, comac_uint128_t b);
comac_uint128_t I
_comac_uint128_mul (comac_uint128_t a, comac_uint128_t b);
comac_uint128_t I
_comac_uint64x64_128_mul (comac_uint64_t a, comac_uint64_t b);
comac_uint128_t I
_comac_uint128_lsl (comac_uint128_t a, int shift);
comac_uint128_t I
_comac_uint128_rsl (comac_uint128_t a, int shift);
comac_uint128_t I
_comac_uint128_rsa (comac_uint128_t a, int shift);
int I
_comac_uint128_lt (comac_uint128_t a, comac_uint128_t b);
int I
_comac_uint128_cmp (comac_uint128_t a, comac_uint128_t b);
int I
_comac_uint128_eq (comac_uint128_t a, comac_uint128_t b);
#define _comac_uint128_is_zero(a)                                              \
    (_comac_uint64_is_zero ((a).hi) && _comac_uint64_is_zero ((a).lo))
comac_uint128_t I
_comac_uint128_negate (comac_uint128_t a);
#define _comac_uint128_negative(a) (_comac_uint64_negative (a.hi))
comac_uint128_t I
_comac_uint128_not (comac_uint128_t a);

#define _comac_uint128_to_int128(i) (i)
#define _comac_int128_to_uint128(i) (i)

comac_int128_t I
_comac_int32_to_int128 (int32_t i);
comac_int128_t I
_comac_int64_to_int128 (comac_int64_t i);
#define _comac_int128_to_int64(a) ((comac_int64_t) (a).lo)
#define _comac_int128_to_int32(a)                                              \
    _comac_int64_to_int32 (_comac_int128_to_int64 (a))
#define _comac_int128_add(a, b) _comac_uint128_add (a, b)
#define _comac_int128_sub(a, b) _comac_uint128_sub (a, b)
#define _comac_int128_mul(a, b) _comac_uint128_mul (a, b)
comac_int128_t I
_comac_int64x64_128_mul (comac_int64_t a, comac_int64_t b);
#define _comac_int64x32_128_mul(a, b)                                          \
    _comac_int64x64_128_mul (a, _comac_int32_to_int64 (b))
#define _comac_int128_lsl(a, b) _comac_uint128_lsl (a, b)
#define _comac_int128_rsl(a, b) _comac_uint128_rsl (a, b)
#define _comac_int128_rsa(a, b) _comac_uint128_rsa (a, b)
int I
_comac_int128_lt (comac_int128_t a, comac_int128_t b);
int I
_comac_int128_cmp (comac_int128_t a, comac_int128_t b);
#define _comac_int128_is_zero(a) _comac_uint128_is_zero (a)
#define _comac_int128_eq(a, b) _comac_uint128_eq (a, b)
#define _comac_int128_negate(a) _comac_uint128_negate (a)
#define _comac_int128_negative(a) (_comac_uint128_negative (a))
#define _comac_int128_not(a) _comac_uint128_not (a)

#else /* !HAVE_UINT128_T */

#define _comac_uint32_to_uint128(i) ((uint128_t) (i))
#define _comac_uint64_to_uint128(i) ((uint128_t) (i))
#define _comac_uint128_to_uint64(i) ((uint64_t) (i))
#define _comac_uint128_to_uint32(i) ((uint32_t) (i))
#define _comac_uint128_add(a, b) ((a) + (b))
#define _comac_uint128_sub(a, b) ((a) - (b))
#define _comac_uint128_mul(a, b) ((a) * (b))
#define _comac_uint64x64_128_mul(a, b) ((uint128_t) (a) * (b))
#define _comac_uint128_lsl(a, b) ((a) << (b))
#define _comac_uint128_rsl(a, b) ((uint128_t) (a) >> (b))
#define _comac_uint128_rsa(a, b) ((uint128_t) ((int128_t) (a) >> (b)))
#define _comac_uint128_lt(a, b) ((a) < (b))
#define _comac_uint128_cmp(a, b) ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define _comac_uint128_is_zero(a) ((a) == 0)
#define _comac_uint128_eq(a, b) ((a) == (b))
#define _comac_uint128_negate(a) ((uint128_t) - ((int128_t) (a)))
#define _comac_uint128_negative(a) ((int128_t) (a) < 0)
#define _comac_uint128_not(a) (~(a))

#define _comac_uint128_to_int128(i) ((int128_t) (i))
#define _comac_int128_to_uint128(i) ((uint128_t) (i))

#define _comac_int32_to_int128(i) ((int128_t) (i))
#define _comac_int64_to_int128(i) ((int128_t) (i))
#define _comac_int128_to_int64(i) ((int64_t) (i))
#define _comac_int128_to_int32(i) ((int32_t) (i))
#define _comac_int128_add(a, b) ((a) + (b))
#define _comac_int128_sub(a, b) ((a) - (b))
#define _comac_int128_mul(a, b) ((a) * (b))
#define _comac_int64x64_128_mul(a, b) ((int128_t) (a) * (b))
#define _comac_int64x32_128_mul(a, b)                                          \
    _comac_int64x64_128_mul (a, _comac_int32_to_int64 (b))
#define _comac_int128_lt(a, b) ((a) < (b))
#define _comac_int128_cmp(a, b) ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define _comac_int128_is_zero(a) ((a) == 0)
#define _comac_int128_eq(a, b) ((a) == (b))
#define _comac_int128_lsl(a, b) ((a) << (b))
#define _comac_int128_rsl(a, b) ((int128_t) ((uint128_t) (a) >> (b)))
#define _comac_int128_rsa(a, b) ((int128_t) (a) >> (b))
#define _comac_int128_negate(a) (-(a))
#define _comac_int128_negative(a) ((a) < 0)
#define _comac_int128_not(a) (~(a))

#endif /* HAVE_UINT128_T */

comac_uquorem128_t I
_comac_uint128_divrem (comac_uint128_t num, comac_uint128_t den);

comac_quorem128_t I
_comac_int128_divrem (comac_int128_t num, comac_int128_t den);

comac_uquorem64_t I
_comac_uint_96by64_32x64_divrem (comac_uint128_t num, comac_uint64_t den);

comac_quorem64_t I
_comac_int_96by64_32x64_divrem (comac_int128_t num, comac_int64_t den);

#define _comac_uint128_le(a, b) (! _comac_uint128_gt (a, b))
#define _comac_uint128_ne(a, b) (! _comac_uint128_eq (a, b))
#define _comac_uint128_ge(a, b) (! _comac_uint128_lt (a, b))
#define _comac_uint128_gt(a, b) _comac_uint128_lt (b, a)

#define _comac_int128_le(a, b) (! _comac_int128_gt (a, b))
#define _comac_int128_ne(a, b) (! _comac_int128_eq (a, b))
#define _comac_int128_ge(a, b) (! _comac_int128_lt (a, b))
#define _comac_int128_gt(a, b) _comac_int128_lt (b, a)

#undef I

#endif /* COMAC_WIDEINT_H */
