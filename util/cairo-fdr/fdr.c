/* comac-fdr - a 'flight data recorder', a black box, for comac
 *
 * Copyright © 2009 Chris Wilson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <comac.h>
#include <comac-script.h>
#include <comac-tee.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include <dlfcn.h>

static void *_dlhandle = RTLD_NEXT;
#define DLCALL(name, args...) ({ \
    static typeof (&name) name##_real; \
    if (name##_real == NULL) { \
	name##_real = dlsym (_dlhandle, #name); \
	if (name##_real == NULL && _dlhandle == RTLD_NEXT) { \
	    _dlhandle = dlopen ("libcomac.so", RTLD_LAZY); \
	    name##_real = dlsym (_dlhandle, #name); \
	    assert (name##_real != NULL); \
	} \
    } \
    (*name##_real) (args);  \
})

#define RINGBUFFER_SIZE 16
static comac_surface_t *fdr_ringbuffer[RINGBUFFER_SIZE];
static int fdr_position;
static int fdr_dump;

static const comac_user_data_key_t fdr_key;

static void
fdr_replay_to_script (comac_surface_t *recording, comac_device_t *ctx)
{
    if (recording != NULL) {
	DLCALL (comac_script_write_comment, ctx, "--- fdr ---", -1);
	DLCALL (comac_script_from_recording_surface, ctx, recording);
    }
}

static void
fdr_dump_ringbuffer (void)
{
    comac_device_t *ctx;
    int n;

    ctx = DLCALL (comac_script_create, "/tmp/fdr.trace");

    for (n = fdr_position; n < RINGBUFFER_SIZE; n++)
	fdr_replay_to_script (fdr_ringbuffer[n], ctx);

    for (n = 0; n < fdr_position; n++)
	fdr_replay_to_script (fdr_ringbuffer[n], ctx);

    DLCALL (comac_device_destroy, ctx);
}

static void
fdr_sighandler (int sig)
{
    fdr_dump = 1;
}

static void
fdr_urgent_sighandler (int sig)
{
    fdr_dump_ringbuffer ();
}

static void
fdr_atexit (void)
{
    if (fdr_dump)
	fdr_dump_ringbuffer ();
}

static void
fdr_pending_signals (void)
{
    static int initialized;

    if (! initialized) {
	initialized = 1;

	signal (SIGUSR1, fdr_sighandler);

	signal (SIGSEGV, fdr_urgent_sighandler);
	signal (SIGABRT, fdr_urgent_sighandler);
	atexit (fdr_atexit);
    }

    if (fdr_dump) {
	fdr_dump_ringbuffer ();
	fdr_dump = 0;
    }
}

static void
fdr_get_extents (comac_surface_t *surface,
		 comac_rectangle_t *extents)
{
    comac_t *cr;

    cr = DLCALL (comac_create, surface);
    DLCALL (comac_clip_extents, cr,
	    &extents->x, &extents->y, &extents->width, &extents->height);
    DLCALL (comac_destroy, cr);

    extents->width -= extents->x;
    extents->height -= extents->y;
}

static void
fdr_surface_destroy (void *surface)
{
    DLCALL (comac_surface_destroy, surface);
}

static void
fdr_surface_reference (void *surface)
{
    DLCALL (comac_surface_reference, surface);
}

static comac_surface_t *
fdr_surface_get_tee (comac_surface_t *surface)
{
    return DLCALL (comac_surface_get_user_data, surface, &fdr_key);
}

static comac_surface_t *
fdr_tee_surface_index (comac_surface_t *surface, int index)
{
    return DLCALL (comac_tee_surface_index, surface, index);
}

comac_t *
comac_create (comac_surface_t *surface)
{
    comac_surface_t *record, *tee;

    fdr_pending_signals ();

    tee = fdr_surface_get_tee (surface);
    if (tee == NULL) {
	comac_rectangle_t extents;
	comac_content_t content;

	fdr_get_extents (surface, &extents);
	content = DLCALL (comac_surface_get_content, surface);

	tee = DLCALL (comac_tee_surface_create, surface);
	record = DLCALL (comac_recording_surface_create, content, &extents);
	DLCALL (comac_tee_surface_add, tee, record);

	DLCALL (comac_surface_set_user_data, surface,
		&fdr_key, tee, fdr_surface_destroy);
    } else {
	int n;

	record = fdr_tee_surface_index (tee, 1);

	/* update the position of the recording surface in the ringbuffer */
	for (n = 0; n < RINGBUFFER_SIZE; n++) {
	    if (record == fdr_ringbuffer[n]) {
		fdr_ringbuffer[n] = NULL;
		break;
	    }
	}
    }

    fdr_surface_destroy (fdr_ringbuffer[fdr_position]);
    fdr_ringbuffer[fdr_position] = record;
    fdr_position = (fdr_position + 1) % RINGBUFFER_SIZE;

    return DLCALL (comac_create, tee);
}

static void
fdr_remove_tee (comac_surface_t *surface)
{
    fdr_surface_reference (surface);
    DLCALL (comac_surface_set_user_data, surface, &fdr_key, NULL, NULL);
    fdr_surface_destroy (surface);
}

void
comac_destroy (comac_t *cr)
{
    comac_surface_t *tee;

    tee = DLCALL (comac_get_target, cr);
    DLCALL (comac_destroy, cr);

    if (DLCALL (comac_surface_get_reference_count, tee) == 1)
	fdr_remove_tee (fdr_tee_surface_index (tee, 0));
}

void
comac_pattern_destroy (comac_pattern_t *pattern)
{
    if (DLCALL (comac_pattern_get_type, pattern) == COMAC_PATTERN_TYPE_SURFACE) {
	comac_surface_t *surface;

	if (DLCALL (comac_pattern_get_surface, pattern, &surface) == COMAC_STATUS_SUCCESS &&
	    DLCALL (comac_surface_get_type, surface) == COMAC_SURFACE_TYPE_TEE &&
	    DLCALL (comac_surface_get_reference_count, surface) == 2)
	{
	    fdr_remove_tee (fdr_tee_surface_index (surface, 0));
	}
    }

    DLCALL (comac_pattern_destroy, pattern);
}

comac_surface_t *
comac_get_target (comac_t *cr)
{
    comac_surface_t *tee;

    tee = DLCALL (comac_get_target, cr);
    return fdr_tee_surface_index (tee, 0);
}

comac_surface_t *
comac_get_group_target (comac_t *cr)
{
    comac_surface_t *tee;

    tee = DLCALL (comac_get_group_target, cr);
    return fdr_tee_surface_index (tee, 0);
}

comac_pattern_t *
comac_pattern_create_for_surface (comac_surface_t *surface)
{
    comac_surface_t *tee;

    tee = fdr_surface_get_tee (surface);
    if (tee != NULL)
	surface = tee;

    return DLCALL (comac_pattern_create_for_surface, surface);
}

comac_status_t
comac_pattern_get_surface (comac_pattern_t *pattern,
			   comac_surface_t **surface)
{
    comac_status_t status;
    comac_surface_t *tee;

    status = DLCALL (comac_pattern_get_surface, pattern, surface);
    if (status != COMAC_STATUS_SUCCESS)
	return status;

    tee = fdr_surface_get_tee (*surface);
    if (tee != NULL)
	*surface = tee;

    return COMAC_STATUS_SUCCESS;
}

void
comac_set_source_surface (comac_t *cr,
			  comac_surface_t *surface,
			  double x, double y)
{
    comac_surface_t *tee;

    tee = fdr_surface_get_tee (surface);
    if (tee != NULL)
	surface = tee;

    DLCALL (comac_set_source_surface, cr, surface, x, y);
}

comac_surface_t *
comac_surface_create_similar (comac_surface_t *surface,
			      comac_content_t content,
			      int width, int height)
{
    comac_surface_t *tee;

    tee = fdr_surface_get_tee (surface);
    if (tee != NULL)
	surface = tee;

    return DLCALL (comac_surface_create_similar,
		   surface, content, width, height);
}

comac_surface_t *
comac_surface_create_for_rectangle (comac_surface_t *surface,
                                    double		 x,
                                    double		 y,
                                    double		 width,
                                    double		 height)
{
    comac_surface_t *tee;

    tee = fdr_surface_get_tee (surface);
    if (tee != NULL)
	surface = tee;

    return DLCALL (comac_surface_create_for_rectangle,
		   surface, x, y, width, height);
}
