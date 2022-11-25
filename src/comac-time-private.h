/* comac - a vector graphics library with display and print output
 *
 * Copyright (C) 2011 Andrea Canciani
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission. The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Authors: Andrea Canciani <ranma42@gmail.com>
 *
 */

#ifndef COMAC_TIME_PRIVATE_H
#define COMAC_TIME_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-wideint-private.h"

/* Make the base type signed for easier arithmetic */
typedef comac_int64_t comac_time_t;

#define _comac_time_add _comac_int64_add
#define _comac_time_sub _comac_int64_sub
#define _comac_time_gt  _comac_int64_gt
#define _comac_time_lt  _comac_int64_lt

#define _comac_time_to_double   _comac_int64_to_double
#define _comac_time_from_double _comac_double_to_int64

comac_private int
_comac_time_cmp (const void *a,
		 const void *b);

comac_private double
_comac_time_to_s (comac_time_t t);

comac_private comac_time_t
_comac_time_from_s (double t);

comac_private comac_time_t
_comac_time_get (void);

static comac_always_inline comac_time_t
_comac_time_get_delta (comac_time_t t)
{
    comac_time_t now;

    now = _comac_time_get ();

    return _comac_time_sub (now, t);
}

static comac_always_inline double
_comac_time_to_ns (comac_time_t t)
{
    return 1.e9 * _comac_time_to_s (t);
}

static comac_always_inline comac_time_t
_comac_time_max (comac_time_t a, comac_time_t b)
{
    if (_comac_int64_gt (a, b))
	return a;
    else
	return b;
}

static comac_always_inline comac_time_t
_comac_time_min (comac_time_t a, comac_time_t b)
{
    if (_comac_int64_lt (a, b))
	return a;
    else
	return b;
}

#endif
