/* comac - a vector graphics library with display and print output
 *
 * Copyright (c) 2007 Netlabs
 * Copyright (c) 2006 Mozilla Corporation
 * Copyright (c) 2006 Red Hat, Inc.
 * Copyright (c) 2011 Andrea Canciani
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * the authors not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The authors make no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Peter Weilbacher <mozilla@weilbacher.org>
 *	    Vladimir Vukicevic <vladimir@pobox.com>
 *	    Carl Worth <cworth@cworth.org>
 *          Andrea Canciani <ranma42@gmail.com>
 */

#include "comacint.h"

#include "comac-time-private.h"

#if HAVE_CLOCK_GETTIME
#if defined(CLOCK_MONOTONIC_RAW)
#define COMAC_CLOCK CLOCK_MONOTONIC_RAW
#elif defined(CLOCK_MONOTONIC)
#define COMAC_CLOCK CLOCK_MONOTONIC
#endif
#endif

#if defined(__APPLE__)
#include <mach/mach_time.h>

static comac_always_inline double
_comac_time_1s (void)
{
    mach_timebase_info_data_t freq;

    mach_timebase_info (&freq);

    return 1000000000. * freq.denom / freq.numer;
}

comac_time_t
_comac_time_get (void)
{
    return mach_absolute_time ();
}

#elif _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static comac_always_inline double
_comac_time_1s (void)
{
    LARGE_INTEGER freq;

    QueryPerformanceFrequency (&freq);

    return freq.QuadPart;
}

#ifndef HAVE_UINT64_T
static comac_always_inline comac_time_t
_comac_time_from_large_integer (LARGE_INTEGER t)
{
    comac_int64_t r;

    r = _comac_int64_lsl (_comac_int32_to_int64 (t.HighPart), 32);
    r = _comac_int64_add (r, _comac_int32_to_int64 (t.LowPart));

    return r;
}
#else
static comac_always_inline comac_time_t
_comac_time_from_large_integer (LARGE_INTEGER t)
{
    return t.QuadPart;
}
#endif

comac_time_t
_comac_time_get (void)
{
    LARGE_INTEGER t;

    QueryPerformanceCounter (&t);

    return _comac_time_from_large_integer (t);
}

#elif defined(COMAC_CLOCK)
#include <time.h>

static comac_always_inline double
_comac_time_1s (void)
{
    return 1000000000;
}

comac_time_t
_comac_time_get (void)
{
    struct timespec t;
    comac_time_t r;

    clock_gettime (COMAC_CLOCK, &t);

    r = _comac_double_to_int64 (_comac_time_1s ());
    r = _comac_int64_mul (r, _comac_int32_to_int64 (t.tv_sec));
    r = _comac_int64_add (r, _comac_int32_to_int64 (t.tv_nsec));

    return r;
}

#else
#include <sys/time.h>

static comac_always_inline double
_comac_time_1s (void)
{
    return 1000000;
}

comac_time_t
_comac_time_get (void)
{
    struct timeval t;
    comac_time_t r;

    gettimeofday (&t, NULL);

    r = _comac_double_to_int64 (_comac_time_1s ());
    r = _comac_int64_mul (r, _comac_int32_to_int64 (t.tv_sec));
    r = _comac_int64_add (r, _comac_int32_to_int64 (t.tv_usec));

    return r;
}

#endif

int
_comac_time_cmp (const void *a, const void *b)
{
    const comac_time_t *ta = a, *tb = b;
    return _comac_int64_cmp (*ta, *tb);
}

static double
_comac_time_ticks_per_sec (void)
{
    static double ticks = 0;

    if (unlikely (ticks == 0))
	ticks = _comac_time_1s ();

    return ticks;
}

static double
_comac_time_s_per_tick (void)
{
    static double s = 0;

    if (unlikely (s == 0))
	s = 1. / _comac_time_ticks_per_sec ();

    return s;
}

double
_comac_time_to_s (comac_time_t t)
{
    return _comac_int64_to_double (t) * _comac_time_s_per_tick ();
}

comac_time_t
_comac_time_from_s (double t)
{
    return _comac_double_to_int64 (t * _comac_time_ticks_per_sec ());
}
