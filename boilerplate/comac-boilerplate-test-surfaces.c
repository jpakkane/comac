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

#include <comac-types-private.h>

#include <test-compositor-surface.h>
#include <test-null-compositor-surface.h>
#if COMAC_HAS_TEST_PAGINATED_SURFACE
#include <test-paginated-surface.h>
#endif

static comac_surface_t *
_comac_boilerplate_test_base_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    *closure = NULL;
    return _comac_test_base_compositor_surface_create (content,
						       ceil (width),
						       ceil (height));
}

static comac_surface_t *
_comac_boilerplate_test_fallback_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    *closure = NULL;
    return _comac_test_fallback_compositor_surface_create (content,
							   ceil (width),
							   ceil (height));
}

static comac_surface_t *
_comac_boilerplate_test_mask_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    *closure = NULL;
    return _comac_test_mask_compositor_surface_create (content,
						       ceil (width),
						       ceil (height));
}

static comac_surface_t *
_comac_boilerplate_test_traps_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    *closure = NULL;
    return _comac_test_traps_compositor_surface_create (content,
							ceil (width),
							ceil (height));
}

static comac_surface_t *
_comac_boilerplate_test_spans_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    *closure = NULL;
    return _comac_test_spans_compositor_surface_create (content,
							ceil (width),
							ceil (height));
}

static comac_surface_t *
_comac_boilerplate_test_no_fallback_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    if (mode == COMAC_BOILERPLATE_MODE_TEST)
	return NULL;

    *closure = NULL;
    return _comac_test_no_fallback_compositor_surface_create (content,
							      ceil (width),
							      ceil (height));
}

static comac_surface_t *
_comac_boilerplate_test_no_traps_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    if (mode == COMAC_BOILERPLATE_MODE_TEST)
	return NULL;

    *closure = NULL;
    return _comac_test_no_traps_compositor_surface_create (content,
							   ceil (width),
							   ceil (height));
}

static comac_surface_t *
_comac_boilerplate_test_no_spans_compositor_create_surface (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure)
{
    if (mode == COMAC_BOILERPLATE_MODE_TEST)
	return NULL;

    *closure = NULL;
    return _comac_test_no_spans_compositor_surface_create (content,
							   ceil (width),
							   ceil (height));
}

#if COMAC_HAS_TEST_PAGINATED_SURFACE
static const comac_user_data_key_t test_paginated_closure_key;

typedef struct {
    comac_surface_t *target;
} test_paginated_closure_t;

static comac_surface_t *
_comac_boilerplate_test_paginated_create_surface (const char *name,
						  comac_content_t content,
						  double width,
						  double height,
						  double max_width,
						  double max_height,
						  comac_boilerplate_mode_t mode,
						  void **closure)
{
    test_paginated_closure_t *tpc;
    comac_format_t format;
    comac_surface_t *surface;
    comac_status_t status;

    *closure = tpc = xmalloc (sizeof (test_paginated_closure_t));

    format = comac_boilerplate_format_from_content (content);
    tpc->target =
	comac_image_surface_create (format, ceil (width), ceil (height));

    surface = _comac_test_paginated_surface_create (tpc->target);
    if (comac_surface_status (surface))
	goto CLEANUP;

    status = comac_surface_set_user_data (surface,
					  &test_paginated_closure_key,
					  tpc,
					  NULL);
    if (status == COMAC_STATUS_SUCCESS)
	return surface;

    comac_surface_destroy (surface);
    surface = comac_boilerplate_surface_create_in_error (status);

    comac_surface_destroy (tpc->target);

CLEANUP:
    free (tpc);
    return surface;
}

/* The only reason we go through all these machinations to write a PNG
 * image is to _really ensure_ that the data actually landed in our
 * buffer through the paginated surface to the test_paginated_surface.
 *
 * If we didn't implement this function then the default
 * comac_surface_write_to_png would result in the paginated_surface's
 * acquire_source_image function replaying the recording-surface to an
 * intermediate image surface. And in that case the
 * test_paginated_surface would not be involved and wouldn't be
 * tested.
 */
static comac_status_t
_comac_boilerplate_test_paginated_surface_write_to_png (
    comac_surface_t *surface, const char *filename)
{
    test_paginated_closure_t *tpc;
    comac_status_t status;

    /* show page first.  the automatic show_page is too late for us */
    comac_surface_show_page (surface);
    status = comac_surface_status (surface);
    if (status)
	return status;

    tpc = comac_surface_get_user_data (surface, &test_paginated_closure_key);
    return comac_surface_write_to_png (tpc->target, filename);
}

static comac_surface_t *
_comac_boilerplate_test_paginated_get_image_surface (comac_surface_t *surface,
						     int page,
						     int width,
						     int height)
{
    test_paginated_closure_t *tpc;
    comac_status_t status;

    /* XXX separate finish as per PDF */
    if (page != 0)
	return comac_boilerplate_surface_create_in_error (
	    COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    /* show page first.  the automatic show_page is too late for us */
    comac_surface_show_page (surface);
    status = comac_surface_status (surface);
    if (status)
	return comac_boilerplate_surface_create_in_error (status);

    tpc = comac_surface_get_user_data (surface, &test_paginated_closure_key);
    return _comac_boilerplate_get_image_surface (tpc->target, 0, width, height);
}

static void
_comac_boilerplate_test_paginated_cleanup (void *closure)
{
    test_paginated_closure_t *tpc = closure;

    comac_surface_destroy (tpc->target);
    free (tpc);
}
#endif

static const comac_boilerplate_target_t targets[] = {
    {"test-base",
     "base",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_base_compositor_surface_create",
     _comac_boilerplate_test_base_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     TRUE,
     FALSE,
     FALSE},
    {"test-base",
     "base",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR,
     0,
     "_comac_test_base_compositor_surface_create",
     _comac_boilerplate_test_base_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE},

    {"test-fallback",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_fallback_compositor_surface_create",
     _comac_boilerplate_test_fallback_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE},
    {"test-fallback",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR,
     0,
     "_comac_test_fallback_compositor_surface_create",
     _comac_boilerplate_test_fallback_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE},

    {"test-mask",
     "mask",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_mask_compositor_surface_create",
     _comac_boilerplate_test_mask_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     TRUE,
     FALSE,
     FALSE},
    {"test-mask",
     "mask",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR,
     0,
     "_comac_test_mask_compositor_surface_create",
     _comac_boilerplate_test_mask_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE},

    {"test-traps",
     "traps",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_traps_compositor_surface_create",
     _comac_boilerplate_test_traps_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     TRUE,
     FALSE,
     FALSE},
    {"test-traps",
     "traps",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR,
     0,
     "_comac_test_traps_compositor_surface_create",
     _comac_boilerplate_test_traps_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE},

    {"test-spans",
     "spans",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_spans_compositor_surface_create",
     _comac_boilerplate_test_spans_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     TRUE,
     FALSE,
     FALSE},
    {"test-spans",
     "spans",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR,
     0,
     "_comac_test_spans_compositor_surface_create",
     _comac_boilerplate_test_spans_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE},

    {"no-fallback",
     "image",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_no_fallback_compositor_surface_create",
     _comac_boilerplate_test_no_fallback_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE},
    {"no-traps",
     "traps",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_no_traps_compositor_surface_create",
     _comac_boilerplate_test_no_traps_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     TRUE,
     FALSE,
     FALSE},
    {"no-spans",
     "spans",
     NULL,
     NULL,
     COMAC_SURFACE_TYPE_IMAGE,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_no_spans_compositor_surface_create",
     _comac_boilerplate_test_no_spans_compositor_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_get_image_surface,
     comac_surface_write_to_png,
     NULL,
     NULL,
     NULL,
     TRUE,
     FALSE,
     FALSE},
#if COMAC_HAS_TEST_PAGINATED_SURFACE
    {"test-paginated",
     "image",
     NULL,
     NULL,
     COMAC_INTERNAL_SURFACE_TYPE_TEST_PAGINATED,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "_comac_test_paginated_surface_create",
     _comac_boilerplate_test_paginated_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_test_paginated_get_image_surface,
     _comac_boilerplate_test_paginated_surface_write_to_png,
     _comac_boilerplate_test_paginated_cleanup,
     NULL,
     NULL,
     FALSE,
     TRUE,
     FALSE},
    {"test-paginated",
     "image",
     NULL,
     NULL,
     COMAC_INTERNAL_SURFACE_TYPE_TEST_PAGINATED,
     COMAC_CONTENT_COLOR,
     0,
     "_comac_test_paginated_surface_create",
     _comac_boilerplate_test_paginated_create_surface,
     comac_surface_create_similar,
     NULL,
     NULL,
     _comac_boilerplate_test_paginated_get_image_surface,
     _comac_boilerplate_test_paginated_surface_write_to_png,
     _comac_boilerplate_test_paginated_cleanup,
     NULL,
     NULL,
     FALSE,
     TRUE,
     FALSE},
#endif
};
COMAC_BOILERPLATE (test, targets)
