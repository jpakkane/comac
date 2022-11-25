/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright © 2004,2006 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-boilerplate-private.h"
#include "comac-boilerplate-scaled-font.h"
#include "comac-malloc-private.h"

#include <pixman.h>

#include <comac-ctype-inline.h>
#include <comac-types-private.h>
#include <comac-scaled-font-private.h>

#if COMAC_HAS_SCRIPT_SURFACE
#include <comac-script.h>
#endif

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#if HAVE_UNISTD_H && HAVE_FCNTL_H && HAVE_SIGNAL_H && HAVE_SYS_STAT_H &&       \
    HAVE_SYS_SOCKET_H && HAVE_SYS_UN_H
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#define HAS_DAEMON 1
#define SOCKET_PATH "./.any2ppm"
#endif

comac_content_t
comac_boilerplate_content (comac_content_t content)
{
    if (content == COMAC_TEST_CONTENT_COLOR_ALPHA_FLATTENED)
	content = COMAC_CONTENT_COLOR_ALPHA;

    return content;
}

const char *
comac_boilerplate_content_name (comac_content_t content)
{
    /* For the purpose of the content name, we don't distinguish the
     * flattened content value.
     */
    switch (comac_boilerplate_content (content)) {
    case COMAC_CONTENT_COLOR:
	return "rgb24";
    case COMAC_CONTENT_COLOR_ALPHA:
	return "argb32";
    case COMAC_CONTENT_ALPHA:
    default:
	assert (0); /* not reached */
	return "---";
    }
}

static const char *
_comac_boilerplate_content_visible_name (comac_content_t content)
{
    switch (comac_boilerplate_content (content)) {
    case COMAC_CONTENT_COLOR:
	return "rgb";
    case COMAC_CONTENT_COLOR_ALPHA:
	return "rgba";
    case COMAC_CONTENT_ALPHA:
	return "a";
    default:
	assert (0); /* not reached */
	return "---";
    }
}

comac_format_t
comac_boilerplate_format_from_content (comac_content_t content)
{
    comac_format_t format;

    switch (content) {
    case COMAC_CONTENT_COLOR:
	format = COMAC_FORMAT_RGB24;
	break;
    case COMAC_CONTENT_COLOR_ALPHA:
	format = COMAC_FORMAT_ARGB32;
	break;
    case COMAC_CONTENT_ALPHA:
	format = COMAC_FORMAT_A8;
	break;
    default:
	assert (0); /* not reached */
	format = COMAC_FORMAT_INVALID;
	break;
    }

    return format;
}

static comac_surface_t *
_comac_boilerplate_image_create_surface (const char *name,
					 comac_content_t content,
					 double width,
					 double height,
					 double max_width,
					 double max_height,
					 comac_boilerplate_mode_t mode,
					 void **closure)
{
    comac_format_t format;

    *closure = NULL;

    if (content == COMAC_CONTENT_COLOR_ALPHA) {
	format = COMAC_FORMAT_ARGB32;
    } else if (content == COMAC_CONTENT_COLOR) {
	format = COMAC_FORMAT_RGB24;
    } else {
	assert (0); /* not reached */
	return NULL;
    }

    return comac_image_surface_create (format, ceil (width), ceil (height));
}

static const comac_user_data_key_t key;

static comac_surface_t *
_comac_boilerplate_image_create_similar (comac_surface_t *other,
					 comac_content_t content,
					 int width,
					 int height)
{
    comac_format_t format;
    comac_surface_t *surface;
    int stride;
    void *ptr;

    switch (content) {
    case COMAC_CONTENT_ALPHA:
	format = COMAC_FORMAT_A8;
	break;
    case COMAC_CONTENT_COLOR:
	format = COMAC_FORMAT_RGB24;
	break;
    case COMAC_CONTENT_COLOR_ALPHA:
    default:
	format = COMAC_FORMAT_ARGB32;
	break;
    }

    stride = comac_format_stride_for_width (format, width);
    ptr = _comac_malloc (stride * height);

    surface = comac_image_surface_create_for_data (ptr,
						   format,
						   width,
						   height,
						   stride);
    comac_surface_set_user_data (surface, &key, ptr, free);

    return surface;
}

static comac_surface_t *
_comac_boilerplate_image16_create_surface (const char *name,
					   comac_content_t content,
					   double width,
					   double height,
					   double max_width,
					   double max_height,
					   comac_boilerplate_mode_t mode,
					   void **closure)
{
    *closure = NULL;

    /* XXX force COMAC_CONTENT_COLOR */
    return comac_image_surface_create (COMAC_FORMAT_RGB16_565,
				       ceil (width),
				       ceil (height));
}

static comac_surface_t *
_comac_boilerplate_image16_create_similar (comac_surface_t *other,
					   comac_content_t content,
					   int width,
					   int height)
{
    comac_format_t format;
    comac_surface_t *surface;
    int stride;
    void *ptr;

    switch (content) {
    case COMAC_CONTENT_ALPHA:
	format = COMAC_FORMAT_A8;
	break;
    case COMAC_CONTENT_COLOR:
	format = COMAC_FORMAT_RGB16_565;
	break;
    case COMAC_CONTENT_COLOR_ALPHA:
    default:
	format = COMAC_FORMAT_ARGB32;
	break;
    }

    stride = comac_format_stride_for_width (format, width);
    ptr = _comac_malloc (stride * height);

    surface = comac_image_surface_create_for_data (ptr,
						   format,
						   width,
						   height,
						   stride);
    comac_surface_set_user_data (surface, &key, ptr, free);

    return surface;
}

static char *
_comac_boilerplate_image_describe (void *closure)
{
    char *s;

    xasprintf (&s, "pixman %s", pixman_version_string ());

    return s;
}

#if COMAC_HAS_RECORDING_SURFACE
static comac_surface_t *
_comac_boilerplate_recording_create_surface (const char *name,
					     comac_content_t content,
					     double width,
					     double height,
					     double max_width,
					     double max_height,
					     comac_boilerplate_mode_t mode,
					     void **closure)
{
    comac_rectangle_t extents;

    extents.x = 0;
    extents.y = 0;
    extents.width = width;
    extents.height = height;
    return *closure = comac_surface_reference (
	       comac_recording_surface_create (content, &extents));
}

static void
_comac_boilerplate_recording_surface_cleanup (void *closure)
{
    comac_surface_finish (closure);
    comac_surface_destroy (closure);
}
#endif

const comac_user_data_key_t comac_boilerplate_output_basename_key;

comac_surface_t *
_comac_boilerplate_get_image_surface (comac_surface_t *src,
				      int page,
				      int width,
				      int height)
{
    comac_surface_t *surface, *image;
    comac_t *cr;
    comac_status_t status;
    comac_format_t format;

    if (comac_surface_status (src))
	return comac_surface_reference (src);

    if (page != 0)
	return comac_boilerplate_surface_create_in_error (
	    COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    /* extract sub-surface */
    switch (comac_surface_get_content (src)) {
    case COMAC_CONTENT_ALPHA:
	format = COMAC_FORMAT_A8;
	break;
    case COMAC_CONTENT_COLOR:
	format = COMAC_FORMAT_RGB24;
	break;
    default:
    case COMAC_CONTENT_COLOR_ALPHA:
	format = COMAC_FORMAT_ARGB32;
	break;
    }
    surface = comac_image_surface_create (format, width, height);
    assert (comac_surface_get_content (surface) ==
	    comac_surface_get_content (src));
    image = comac_surface_reference (surface);

    /* open a logging channel (only interesting for recording surfaces) */
#if COMAC_HAS_SCRIPT_SURFACE && COMAC_HAS_RECORDING_SURFACE
    if (comac_surface_get_type (src) == COMAC_SURFACE_TYPE_RECORDING) {
	const char *test_name;

	test_name = comac_surface_get_user_data (
	    src,
	    &comac_boilerplate_output_basename_key);
	if (test_name != NULL) {
	    comac_device_t *ctx;
	    char *filename;

	    comac_surface_destroy (surface);

	    xasprintf (&filename, "%s.out.trace", test_name);
	    ctx = comac_script_create (filename);
	    surface = comac_script_surface_create_for_target (ctx, image);
	    comac_device_destroy (ctx);
	    free (filename);
	}
    }
#endif

    cr = comac_create (surface);
    comac_surface_destroy (surface);
    comac_set_source_surface (cr, src, 0, 0);
    comac_paint (cr);

    status = comac_status (cr);
    if (status) {
	comac_surface_destroy (image);
	image = comac_surface_reference (comac_get_target (cr));
    }
    comac_destroy (cr);

    return image;
}

comac_surface_t *
comac_boilerplate_get_image_surface_from_png (const char *filename,
					      int width,
					      int height,
					      comac_bool_t flatten)
{
    comac_surface_t *surface;

    surface = comac_image_surface_create_from_png (filename);
    if (comac_surface_status (surface))
	return surface;

    if (flatten) {
	comac_t *cr;
	comac_surface_t *flattened;

	flattened = comac_image_surface_create (
	    comac_image_surface_get_format (surface),
	    width,
	    height);
	cr = comac_create (flattened);
	comac_surface_destroy (flattened);

	comac_set_source_rgb (cr, 1, 1, 1);
	comac_paint (cr);

	comac_set_source_surface (
	    cr,
	    surface,
	    width - comac_image_surface_get_width (surface),
	    height - comac_image_surface_get_height (surface));
	comac_paint (cr);

	comac_surface_destroy (surface);
	surface = comac_surface_reference (comac_get_target (cr));
	comac_destroy (cr);
    } else if (comac_image_surface_get_width (surface) != width ||
	       comac_image_surface_get_height (surface) != height) {
	comac_t *cr;
	comac_surface_t *sub;

	sub = comac_image_surface_create (
	    comac_image_surface_get_format (surface),
	    width,
	    height);
	cr = comac_create (sub);
	comac_surface_destroy (sub);

	comac_set_source_surface (
	    cr,
	    surface,
	    width - comac_image_surface_get_width (surface),
	    height - comac_image_surface_get_height (surface));
	comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
	comac_paint (cr);

	comac_surface_destroy (surface);
	surface = comac_surface_reference (comac_get_target (cr));
	comac_destroy (cr);
    }

    return surface;
}

static const comac_boilerplate_target_t builtin_targets[] = {
    /* I'm uncompromising about leaving the image backend as 0
     * for tolerance. There shouldn't ever be anything that is out of
     * our control here. */
    {"image",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     NULL,
     _comac_boilerplate_image_create_surface,
     _comac_boilerplate_image_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     _comac_boilerplate_image_describe,
     TRUE,
     FALSE,
     FALSE},
    {"image",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR,
     0,
     NULL,
     _comac_boilerplate_image_create_surface,
     _comac_boilerplate_image_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     _comac_boilerplate_image_describe,
     FALSE,
     FALSE,
     FALSE},
    {"image16",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR,
     0,
     NULL,
     _comac_boilerplate_image16_create_surface,
     _comac_boilerplate_image16_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     _comac_boilerplate_image_describe,
     TRUE,
     FALSE,
     FALSE},
#if COMAC_HAS_RECORDING_SURFACE
    {"recording",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_RECORDING,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "comac_recording_surface_create",
     _comac_boilerplate_recording_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     _comac_boilerplate_recording_surface_cleanup,
     NULL,
     NULL,
     FALSE,
     FALSE,
     TRUE},
    {"recording",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_RECORDING,
     COMAC_CONTENT_COLOR,
     0,
     "comac_recording_surface_create",
     _comac_boilerplate_recording_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     _comac_boilerplate_recording_surface_cleanup,
     NULL,
     NULL,
     FALSE,
     FALSE,
     TRUE},
#endif
};
COMAC_BOILERPLATE (builtin, builtin_targets)

static struct comac_boilerplate_target_list {
    struct comac_boilerplate_target_list *next;
    const comac_boilerplate_target_t *target;
} *comac_boilerplate_targets;

static comac_bool_t
probe_target (const comac_boilerplate_target_t *target)
{
    if (target->probe == NULL)
	return TRUE;

#if HAVE_DLSYM
    return dlsym (NULL, target->probe) != NULL;
#else
    return TRUE;
#endif
}

void
_comac_boilerplate_register_backend (const comac_boilerplate_target_t *targets,
				     unsigned int count)
{
    targets += count;
    while (count--) {
	struct comac_boilerplate_target_list *list;

	--targets;
	if (! probe_target (targets))
	    continue;

	list = xmalloc (sizeof (*list));
	list->next = comac_boilerplate_targets;
	list->target = targets;
	comac_boilerplate_targets = list;
    }
}

static comac_bool_t
_comac_boilerplate_target_format_matches_name (
    const comac_boilerplate_target_t *target,
    const char *tcontent_name,
    const char *tcontent_end)
{
    char const *content_name;
    const char *content_end = tcontent_end;
    size_t content_len;

    content_name = _comac_boilerplate_content_visible_name (target->content);
    if (tcontent_end)
	content_len = content_end - tcontent_name;
    else
	content_len = strlen (tcontent_name);
    if (strlen (content_name) != content_len)
	return FALSE;
    if (0 == strncmp (content_name, tcontent_name, content_len))
	return TRUE;

    return FALSE;
}

static comac_bool_t
_comac_boilerplate_target_matches_name (
    const comac_boilerplate_target_t *target,
    const char *tname,
    const char *end)
{
    char const *content_name;
    const char *content_start = strpbrk (tname, ".");
    const char *content_end = end;
    size_t name_len;
    size_t content_len;

    if (content_start >= end)
	content_start = NULL;
    if (content_start != NULL)
	end = content_start++;

    name_len = end - tname;

    /* Check name. */
    if (! (name_len == 1 && 0 == strncmp (tname, "?", 1))) { /* wildcard? */
	if (0 != strncmp (target->name, tname, name_len))    /* exact match? */
	    return FALSE;
	if (_comac_isalnum (target->name[name_len]))
	    return FALSE;
    }

    /* Check optional content. */
    if (content_start == NULL) /* none given? */
	return TRUE;

    /* Exact content match? */
    content_name = _comac_boilerplate_content_visible_name (target->content);
    content_len = content_end - content_start;
    if (strlen (content_name) != content_len)
	return FALSE;
    if (0 == strncmp (content_name, content_start, content_len))
	return TRUE;

    return FALSE;
}

const comac_boilerplate_target_t **
comac_boilerplate_get_targets (int *pnum_targets,
			       comac_bool_t *plimited_targets)
{
    size_t i, num_targets;
    comac_bool_t limited_targets = FALSE;
    const char *tname;
    const comac_boilerplate_target_t **targets_to_test;
    struct comac_boilerplate_target_list *list;

    if (comac_boilerplate_targets == NULL)
	_comac_boilerplate_register_all ();

    if ((tname = getenv ("COMAC_TEST_TARGET")) != NULL && *tname) {
	/* check the list of targets specified by the user */
	limited_targets = TRUE;

	num_targets = 0;
	targets_to_test = NULL;

	while (*tname) {
	    int found = 0;
	    const char *end = strpbrk (tname, " \t\r\n;:,");
	    if (! end)
		end = tname + strlen (tname);

	    if (end == tname) {
		tname = end + 1;
		continue;
	    }

	    for (list = comac_boilerplate_targets; list != NULL;
		 list = list->next) {
		const comac_boilerplate_target_t *target = list->target;
		const char *tcontent_name;
		const char *tcontent_end;
		if (_comac_boilerplate_target_matches_name (target,
							    tname,
							    end)) {
		    if ((tcontent_name = getenv ("COMAC_TEST_TARGET_FORMAT")) !=
			    NULL &&
			*tcontent_name) {
			while (tcontent_name) {
			    tcontent_end =
				strpbrk (tcontent_name, " \t\r\n;:,");
			    if (tcontent_end == tcontent_name) {
				tcontent_name = tcontent_end + 1;
				continue;
			    }
			    if (_comac_boilerplate_target_format_matches_name (
				    target,
				    tcontent_name,
				    tcontent_end)) {
				/* realloc isn't exactly the best thing here, but meh. */
				targets_to_test = xrealloc (
				    targets_to_test,
				    sizeof (comac_boilerplate_target_t *) *
					(num_targets + 1));
				targets_to_test[num_targets++] = target;
				found = 1;
			    }

			    if (tcontent_end)
				tcontent_end++;
			    tcontent_name = tcontent_end;
			}
		    } else {
			/* realloc isn't exactly the best thing here, but meh. */
			targets_to_test =
			    xrealloc (targets_to_test,
				      sizeof (comac_boilerplate_target_t *) *
					  (num_targets + 1));
			targets_to_test[num_targets++] = target;
			found = 1;
		    }
		}
	    }

	    if (! found) {
		const char *last_name = NULL;

		fprintf (stderr,
			 "Cannot find target '%.*s'.\n",
			 (int) (end - tname),
			 tname);
		fprintf (stderr, "Known targets:");
		for (list = comac_boilerplate_targets; list != NULL;
		     list = list->next) {
		    const comac_boilerplate_target_t *target = list->target;
		    if (last_name != NULL) {
			if (strcmp (target->name, last_name) == 0) {
			    /* filter out repeats that differ in content */
			    continue;
			}
			fprintf (stderr, ",");
		    }
		    fprintf (stderr, " %s", target->name);
		    last_name = target->name;
		}
		fprintf (stderr, "\n");
		exit (-1);
	    }

	    if (*end)
		end++;
	    tname = end;
	}
    } else {
	int found = 0;
	int not_found_targets = 0;
	num_targets = 0;
	targets_to_test =
	    xmalloc (sizeof (comac_boilerplate_target_t *) * num_targets);
	for (list = comac_boilerplate_targets; list != NULL;
	     list = list->next) {
	    const comac_boilerplate_target_t *target = list->target;
	    const char *tcontent_name;
	    const char *tcontent_end;
	    if ((tcontent_name = getenv ("COMAC_TEST_TARGET_FORMAT")) != NULL &&
		*tcontent_name) {
		while (tcontent_name) {
		    tcontent_end = strpbrk (tcontent_name, " \t\r\n;:,");
		    if (tcontent_end == tcontent_name) {
			tcontent_name = tcontent_end + 1;
			continue;
		    }
		    if (_comac_boilerplate_target_format_matches_name (
			    target,
			    tcontent_name,
			    tcontent_end)) {
			/* realloc isn't exactly the best thing here, but meh. */
			targets_to_test =
			    xrealloc (targets_to_test,
				      sizeof (comac_boilerplate_target_t *) *
					  (num_targets + 1));
			targets_to_test[num_targets++] = target;
			found = 1;
		    } else {
			not_found_targets++;
		    }

		    if (tcontent_end)
			tcontent_end++;

		    tcontent_name = tcontent_end;
		}
	    } else {
		num_targets++;
	    }
	}
	if (! found) {
	    /* check all compiled in targets */
	    num_targets = num_targets + not_found_targets;
	    targets_to_test =
		xrealloc (targets_to_test,
			  sizeof (comac_boilerplate_target_t *) * num_targets);
	    num_targets = 0;
	    for (list = comac_boilerplate_targets; list != NULL;
		 list = list->next) {
		const comac_boilerplate_target_t *target = list->target;
		targets_to_test[num_targets++] = target;
	    }
	}
    }

    /* exclude targets as specified by the user */
    if ((tname = getenv ("COMAC_TEST_TARGET_EXCLUDE")) != NULL && *tname) {
	limited_targets = TRUE;

	while (*tname) {
	    int j;
	    const char *end = strpbrk (tname, " \t\r\n;:,");
	    if (! end)
		end = tname + strlen (tname);

	    if (end == tname) {
		tname = end + 1;
		continue;
	    }

	    for (i = j = 0; i < num_targets; i++) {
		const comac_boilerplate_target_t *target = targets_to_test[i];
		if (! _comac_boilerplate_target_matches_name (target,
							      tname,
							      end)) {
		    targets_to_test[j++] = targets_to_test[i];
		}
	    }
	    num_targets = j;

	    if (*end)
		end++;
	    tname = end;
	}
    }

    if (pnum_targets)
	*pnum_targets = num_targets;

    if (plimited_targets)
	*plimited_targets = limited_targets;

    return targets_to_test;
}

const comac_boilerplate_target_t *
comac_boilerplate_get_image_target (comac_content_t content)
{
    if (comac_boilerplate_targets == NULL)
	_comac_boilerplate_register_all ();

    switch (content) {
    case COMAC_CONTENT_COLOR:
	return &builtin_targets[1];
    case COMAC_CONTENT_COLOR_ALPHA:
	return &builtin_targets[0];
    case COMAC_CONTENT_ALPHA:
    default:
	return NULL;
    }
}

const comac_boilerplate_target_t *
comac_boilerplate_get_target_by_name (const char *name, comac_content_t content)
{
    struct comac_boilerplate_target_list *list;

    if (comac_boilerplate_targets == NULL)
	_comac_boilerplate_register_all ();

    /* first return an exact match */
    for (list = comac_boilerplate_targets; list != NULL; list = list->next) {
	const comac_boilerplate_target_t *target = list->target;
	if (strcmp (target->name, name) == 0 && target->content == content) {
	    return target;
	}
    }

    /* otherwise just return a match that may differ in content */
    for (list = comac_boilerplate_targets; list != NULL; list = list->next) {
	const comac_boilerplate_target_t *target = list->target;
	if (strcmp (target->name, name) == 0)
	    return target;
    }

    return NULL;
}

void
comac_boilerplate_free_targets (const comac_boilerplate_target_t **targets)
{
    free (targets);
}

comac_surface_t *
comac_boilerplate_surface_create_in_error (comac_status_t status)
{
    comac_surface_t *surface = NULL;
    int loop = 5;

    do {
	comac_surface_t *intermediate;
	comac_t *cr;
	comac_path_t path;

	intermediate = comac_image_surface_create (COMAC_FORMAT_A8, 0, 0);
	cr = comac_create (intermediate);
	comac_surface_destroy (intermediate);

	path.status = status;
	comac_append_path (cr, &path);

	comac_surface_destroy (surface);
	surface = comac_surface_reference (comac_get_target (cr));
	comac_destroy (cr);
    } while (comac_surface_status (surface) != status && --loop);

    return surface;
}

void
comac_boilerplate_scaled_font_set_max_glyphs_cached (
    comac_scaled_font_t *scaled_font, int max_glyphs)
{
    /* XXX COMAC_DEBUG */
}

#if HAS_DAEMON
static int
any2ppm_daemon_exists (void)
{
    struct stat st;
    int fd;
    char buf[80];
    int pid;
    int ret;

    if (stat (SOCKET_PATH, &st) < 0)
	return 0;

    fd = open (SOCKET_PATH ".pid", O_RDONLY);
    if (fd < 0)
	return 0;

    pid = 0;
    ret = read (fd, buf, sizeof (buf) - 1);
    if (ret > 0) {
	buf[ret] = '\0';
	pid = atoi (buf);
    }
    close (fd);

    return pid > 0 && kill (pid, 0) != -1;
}
#endif

FILE *
comac_boilerplate_open_any2ppm (const char *filename,
				int page,
				unsigned int flags,
				int (**close_cb) (FILE *))
{
    char command[4096];
    const char *any2ppm;
#if HAS_DAEMON
    int sk;
    struct sockaddr_un addr;
    int len;
#endif

    any2ppm = getenv ("ANY2PPM");
    if (any2ppm == NULL)
	any2ppm = "./any2ppm";

#if HAS_DAEMON
    if (flags & COMAC_BOILERPLATE_OPEN_NO_DAEMON)
	goto POPEN;

    if (! any2ppm_daemon_exists ()) {
	if (system (any2ppm) != 0)
	    goto POPEN;
    }

    sk = socket (PF_UNIX, SOCK_STREAM, 0);
    if (sk == -1)
	goto POPEN;

    memset (&addr, 0, sizeof (addr));
    addr.sun_family = AF_UNIX;
    strcpy (addr.sun_path, SOCKET_PATH);

    if (connect (sk, (struct sockaddr *) &addr, sizeof (addr)) == -1) {
	close (sk);
	goto POPEN;
    }

    len = sprintf (command, "%s %d\n", filename, page);
    if (write (sk, command, len) != len) {
	close (sk);
	goto POPEN;
    }

    *close_cb = fclose;
    return fdopen (sk, "rb");

POPEN:
#endif

    *close_cb = pclose;
    sprintf (command, "%s %s %d", any2ppm, filename, page);
    return popen (command, "r");
}

static comac_bool_t
freadn (unsigned char *buf, int len, FILE *file)
{
    int ret;

    while (len) {
	ret = fread (buf, 1, len, file);
	if (ret != len) {
	    if (ferror (file) || feof (file))
		return FALSE;
	}
	len -= ret;
	buf += len;
    }

    return TRUE;
}

comac_surface_t *
comac_boilerplate_image_surface_create_from_ppm_stream (FILE *file)
{
    char format;
    int width, height;
    ptrdiff_t stride;
    int x, y;
    unsigned char *data;
    comac_surface_t *image = NULL;

    if (fscanf (file, "P%c %d %d 255\n", &format, &width, &height) != 3)
	goto FAIL;

    switch (format) {
    case '7': /* XXX */
	image = comac_image_surface_create (COMAC_FORMAT_ARGB32, width, height);
	break;
    case '6':
	image = comac_image_surface_create (COMAC_FORMAT_RGB24, width, height);
	break;
    case '5':
	image = comac_image_surface_create (COMAC_FORMAT_A8, width, height);
	break;
    default:
	goto FAIL;
    }
    if (comac_surface_status (image))
	return image;

    data = comac_image_surface_get_data (image);
    stride = comac_image_surface_get_stride (image);
    for (y = 0; y < height; y++) {
	unsigned char *buf = data + y * stride;
	switch (format) {
	case '7':
	    if (! freadn (buf, 4 * width, file))
		goto FAIL;
	    break;
	case '6':
	    if (! freadn (buf, 3 * width, file))
		goto FAIL;
	    buf += 3 * width;
	    for (x = width; x--;) {
		buf -= 3;
		((uint32_t *) (data + y * stride))[x] =
		    (buf[0] << 16) | (buf[1] << 8) | (buf[2] << 0);
	    }
	    break;
	case '5':
	    if (! freadn (buf, width, file))
		goto FAIL;
	    break;
	}
    }
    comac_surface_mark_dirty (image);

    return image;

FAIL:
    comac_surface_destroy (image);
    return comac_boilerplate_surface_create_in_error (COMAC_STATUS_READ_ERROR);
}

comac_surface_t *
comac_boilerplate_convert_to_image (const char *filename, int page)
{
    FILE *file;
    unsigned int flags = 0;
    comac_surface_t *image;
    int (*close_cb) (FILE *);
    int ret;

    if (getenv ("COMAC_BOILERPLATE_OPEN_NO_DAEMON") != NULL) {
	flags |= COMAC_BOILERPLATE_OPEN_NO_DAEMON;
    }

RETRY:
    file = comac_boilerplate_open_any2ppm (filename, page, flags, &close_cb);
    if (file == NULL) {
	switch (errno) {
	case ENOMEM:
	    return comac_boilerplate_surface_create_in_error (
		COMAC_STATUS_NO_MEMORY);
	default:
	    return comac_boilerplate_surface_create_in_error (
		COMAC_STATUS_READ_ERROR);
	}
    }

    image = comac_boilerplate_image_surface_create_from_ppm_stream (file);
    ret = close_cb (file);
    /* check for fatal errors from the interpreter */
    if (ret) { /* any2pmm should never die... */
	comac_surface_destroy (image);
	if (getenv ("COMAC_BOILERPLATE_DO_NOT_CRASH_ON_ANY2PPM_ERROR") !=
	    NULL) {
	    return comac_boilerplate_surface_create_in_error (
		COMAC_STATUS_WRITE_ERROR);
	} else {
	    return comac_boilerplate_surface_create_in_error (
		COMAC_STATUS_INVALID_STATUS);
	}
    }

    if (ret == 0 && comac_surface_status (image) == COMAC_STATUS_READ_ERROR) {
	if (flags == 0) {
	    /* Try again in a standalone process. */
	    comac_surface_destroy (image);
	    flags = COMAC_BOILERPLATE_OPEN_NO_DAEMON;
	    goto RETRY;
	}
    }

    return image;
}

int
comac_boilerplate_version (void)
{
    return COMAC_VERSION;
}

const char *
comac_boilerplate_version_string (void)
{
    return COMAC_VERSION_STRING;
}

void
comac_boilerplate_fini (void)
{
    while (comac_boilerplate_targets != NULL) {
	struct comac_boilerplate_target_list *next;

	next = comac_boilerplate_targets->next;

	free (comac_boilerplate_targets);
	comac_boilerplate_targets = next;
    }
}
