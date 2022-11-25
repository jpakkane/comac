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
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include <dlfcn.h>

static void *_dlhandle = RTLD_NEXT;
#define DLCALL(name, args...)                                                  \
    ({                                                                         \
	static typeof (&name) name##_real;                                     \
	if (name##_real == NULL) {                                             \
	    name##_real = dlsym (_dlhandle, #name);                            \
	    if (name##_real == NULL && _dlhandle == RTLD_NEXT) {               \
		_dlhandle = dlopen ("libcomac.so", RTLD_LAZY);                 \
		name##_real = dlsym (_dlhandle, #name);                        \
		assert (name##_real != NULL);                                  \
	    }                                                                  \
	}                                                                      \
	(*name##_real) (args);                                                 \
    })

static comac_device_t *fdr_context;
static const comac_user_data_key_t fdr_key;

static void
fdr_get_extents (comac_surface_t *surface, comac_rectangle_t *extents)
{
    comac_t *cr;

    cr = DLCALL (comac_create, surface);
    DLCALL (comac_clip_extents,
	    cr,
	    &extents->x,
	    &extents->y,
	    &extents->width,
	    &extents->height);
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

static comac_status_t
fdr_write (void *closure, const unsigned char *data, unsigned int len)
{
    int fd = (int) (intptr_t) closure;
    while (len) {
	int ret = write (fd, data, len);
	if (ret < 0) {
	    switch (errno) {
	    case EAGAIN:
	    case EINTR:
		continue;
	    default:
		return COMAC_STATUS_WRITE_ERROR;
	    }
	} else if (ret == 0) {
	    return COMAC_STATUS_WRITE_ERROR;
	} else {
	    data += ret;
	    len -= ret;
	}
    }
    return COMAC_STATUS_SUCCESS;
}

comac_t *
comac_create (comac_surface_t *surface)
{
    comac_surface_t *tee;

    tee = fdr_surface_get_tee (surface);
    if (tee == NULL) {
	comac_surface_t *script;
	comac_rectangle_t extents;
	comac_content_t content;

	if (fdr_context == NULL) {
	    const char *env = getenv ("COMAC_SPHINX_FD");
	    int fd = env ? atoi (env) : 1;
	    fdr_context = DLCALL (comac_script_create_for_stream,
				  fdr_write,
				  (void *) (intptr_t) fd);
	}

	fdr_get_extents (surface, &extents);
	content = DLCALL (comac_surface_get_content, surface);

	tee = DLCALL (comac_tee_surface_create, surface);
	script = DLCALL (comac_script_surface_create,
			 fdr_context,
			 content,
			 extents.width,
			 extents.height);
	DLCALL (comac_tee_surface_add, tee, script);

	DLCALL (comac_surface_set_user_data,
		surface,
		&fdr_key,
		tee,
		fdr_surface_destroy);
    }

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
    if (DLCALL (comac_pattern_get_type, pattern) ==
	COMAC_PATTERN_TYPE_SURFACE) {
	comac_surface_t *surface;

	if (DLCALL (comac_pattern_get_surface, pattern, &surface) ==
		COMAC_STATUS_SUCCESS &&
	    DLCALL (comac_surface_get_type, surface) ==
		COMAC_SURFACE_TYPE_TEE &&
	    DLCALL (comac_surface_get_reference_count, surface) == 2) {
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
comac_pattern_get_surface (comac_pattern_t *pattern, comac_surface_t **surface)
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
			  double x,
			  double y)
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
			      int width,
			      int height)
{
    comac_surface_t *tee;

    tee = fdr_surface_get_tee (surface);
    if (tee != NULL)
	surface = tee;

    return DLCALL (comac_surface_create_similar,
		   surface,
		   content,
		   width,
		   height);
}
