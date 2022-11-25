/*
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
 *	    Vladimir Vukicevic <vladimir@pobox.com> (win32/linux code)
 *	    Carl Worth <cworth@cworth.org> (win32/linux code)
 *          Andrea Canciani <ranma42@gmail.com>
 */

#include "comac-perf.h"
#include "../src/comac-time-private.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(_POSIX_PRIORITY_SCHEDULING)
#include <sched.h>
#endif

/* timers */
static comac_time_t timer;
static comac_perf_timer_synchronize_t comac_perf_timer_synchronize = NULL;
static void *comac_perf_timer_synchronize_closure = NULL;

void
comac_perf_timer_set_synchronize (comac_perf_timer_synchronize_t synchronize,
				  void *closure)
{
    comac_perf_timer_synchronize = synchronize;
    comac_perf_timer_synchronize_closure = closure;
}

void
comac_perf_timer_start (void)
{
    timer = _comac_time_get ();
}

void
comac_perf_timer_stop (void)
{
    if (comac_perf_timer_synchronize)
	comac_perf_timer_synchronize (comac_perf_timer_synchronize_closure);

    timer = _comac_time_get_delta (timer);
}

comac_time_t
comac_perf_timer_elapsed (void)
{
    return timer;
}

void
comac_perf_yield (void)
{
    /* try to deactivate this thread until the scheduler calls it again */

#if defined(_WIN32)
    SleepEx (0, TRUE);
#elif defined(_POSIX_PRIORITY_SCHEDULING)
    sched_yield ();
#else
    sleep (0);
#endif
}
