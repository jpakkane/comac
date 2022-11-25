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

#if COMAC_CAN_TEST_PDF_SURFACE

#include <comac-pdf.h>
#include <comac-pdf-surface-private.h>
#include <comac-paginated-surface-private.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if ! COMAC_HAS_RECORDING_SURFACE
#define COMAC_SURFACE_TYPE_RECORDING COMAC_INTERNAL_SURFACE_TYPE_RECORDING
#endif

static const comac_user_data_key_t pdf_closure_key;

typedef struct _pdf_target_closure {
    char *filename;
    int width;
    int height;
    comac_surface_t *target;
} pdf_target_closure_t;

static comac_surface_t *
_comac_boilerplate_pdf_create_surface (const char *name,
				       comac_content_t content,
				       double width,
				       double height,
				       double max_width,
				       double max_height,
				       comac_boilerplate_mode_t mode,
				       void **closure)
{
    pdf_target_closure_t *ptc;
    comac_surface_t *surface;
    comac_status_t status;

    /* Sanitize back to a real comac_content_t value. */
    if (content == COMAC_TEST_CONTENT_COLOR_ALPHA_FLATTENED)
	content = COMAC_CONTENT_COLOR_ALPHA;

    *closure = ptc = xmalloc (sizeof (pdf_target_closure_t));

    ptc->width = ceil (width);
    ptc->height = ceil (height);

    xasprintf (&ptc->filename, "%s.out.pdf", name);
    xunlink (ptc->filename);

    surface = comac_pdf_surface_create (ptc->filename, width, height);
    if (comac_surface_status (surface))
	goto CLEANUP_FILENAME;

    comac_pdf_surface_set_metadata (surface,
				    COMAC_PDF_METADATA_CREATE_DATE,
				    NULL);
    comac_surface_set_fallback_resolution (surface, 72., 72.);

    if (content == COMAC_CONTENT_COLOR) {
	ptc->target = surface;
	surface = comac_surface_create_similar (ptc->target,
						COMAC_CONTENT_COLOR,
						ptc->width,
						ptc->height);
	if (comac_surface_status (surface))
	    goto CLEANUP_TARGET;
    } else {
	ptc->target = NULL;
    }

    status = comac_surface_set_user_data (surface, &pdf_closure_key, ptc, NULL);
    if (status == COMAC_STATUS_SUCCESS)
	return surface;

    comac_surface_destroy (surface);
    surface = comac_boilerplate_surface_create_in_error (status);

CLEANUP_TARGET:
    comac_surface_destroy (ptc->target);
CLEANUP_FILENAME:
    free (ptc->filename);
    free (ptc);
    return surface;
}

static comac_status_t
_comac_boilerplate_pdf_finish_surface (comac_surface_t *surface)
{
    pdf_target_closure_t *ptc =
	comac_surface_get_user_data (surface, &pdf_closure_key);
    comac_status_t status;

    if (ptc->target) {
	/* Both surface and ptc->target were originally created at the
	 * same dimensions. We want a 1:1 copy here, so we first clear any
	 * device offset and scale on surface.
	 *
	 * In a more realistic use case of device offsets, the target of
	 * this copying would be of a different size than the source, and
	 * the offset would be desirable during the copy operation. */
	double x_offset, y_offset;
	double x_scale, y_scale;
	comac_surface_get_device_offset (surface, &x_offset, &y_offset);
	comac_surface_get_device_scale (surface, &x_scale, &y_scale);
	comac_surface_set_device_offset (surface, 0, 0);
	comac_surface_set_device_scale (surface, 1, 1);
	comac_t *cr;
	cr = comac_create (ptc->target);
	comac_set_source_surface (cr, surface, 0, 0);
	comac_paint (cr);
	comac_show_page (cr);
	status = comac_status (cr);
	comac_destroy (cr);
	comac_surface_set_device_offset (surface, x_offset, y_offset);
	comac_surface_set_device_scale (surface, x_scale, y_scale);

	if (status)
	    return status;

	comac_surface_finish (surface);
	status = comac_surface_status (surface);
	if (status)
	    return status;

	surface = ptc->target;
    }

    comac_surface_finish (surface);
    status = comac_surface_status (surface);
    if (status)
	return status;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_boilerplate_pdf_surface_write_to_png (comac_surface_t *surface,
					     const char *filename)
{
    pdf_target_closure_t *ptc =
	comac_surface_get_user_data (surface, &pdf_closure_key);
    char command[4096];
    int exitstatus;

    sprintf (command, "./pdf2png %s %s 1", ptc->filename, filename);

    exitstatus = system (command);
#if _XOPEN_SOURCE && HAVE_SIGNAL_H
    if (WIFSIGNALED (exitstatus))
	raise (WTERMSIG (exitstatus));
#endif
    if (exitstatus)
	return COMAC_STATUS_WRITE_ERROR;

    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
_comac_boilerplate_pdf_convert_to_image (comac_surface_t *surface, int page)
{
    pdf_target_closure_t *ptc =
	comac_surface_get_user_data (surface, &pdf_closure_key);

    return comac_boilerplate_convert_to_image (ptc->filename, page + 1);
}

static comac_surface_t *
_comac_boilerplate_pdf_get_image_surface (comac_surface_t *surface,
					  int page,
					  int width,
					  int height)
{
    comac_surface_t *image;
    double x_offset, y_offset;
    double x_scale, y_scale;

    image = _comac_boilerplate_pdf_convert_to_image (surface, page);
    comac_surface_get_device_offset (surface, &x_offset, &y_offset);
    comac_surface_get_device_scale (surface, &x_scale, &y_scale);
    comac_surface_set_device_offset (image, x_offset, y_offset);
    comac_surface_set_device_scale (image, x_scale, y_scale);
    surface = _comac_boilerplate_get_image_surface (image, 0, width, height);
    comac_surface_destroy (image);

    return surface;
}

static void
_comac_boilerplate_pdf_cleanup (void *closure)
{
    pdf_target_closure_t *ptc = closure;
    if (ptc->target) {
	comac_surface_finish (ptc->target);
	comac_surface_destroy (ptc->target);
    }
    free (ptc->filename);
    free (ptc);
}

static void
_comac_boilerplate_pdf_force_fallbacks (comac_surface_t *abstract_surface,
					double x_pixels_per_inch,
					double y_pixels_per_inch)
{
    pdf_target_closure_t *ptc =
	comac_surface_get_user_data (abstract_surface, &pdf_closure_key);

    comac_paginated_surface_t *paginated;
    comac_pdf_surface_t *surface;

    if (ptc->target)
	abstract_surface = ptc->target;

    paginated = (comac_paginated_surface_t *) abstract_surface;
    surface = (comac_pdf_surface_t *) paginated->target;
    surface->force_fallbacks = TRUE;
    comac_surface_set_fallback_resolution (&paginated->base,
					   x_pixels_per_inch,
					   y_pixels_per_inch);
}

static const comac_boilerplate_target_t targets[] = {
    {"pdf",
     "pdf",
     ".pdf",
     NULL,
     COMAC_SURFACE_TYPE_PDF,
     COMAC_TEST_CONTENT_COLOR_ALPHA_FLATTENED,
     0,
     "comac_pdf_surface_create",
     _comac_boilerplate_pdf_create_surface,
     comac_surface_create_similar,
     _comac_boilerplate_pdf_force_fallbacks,
     _comac_boilerplate_pdf_finish_surface,
     _comac_boilerplate_pdf_get_image_surface,
     _comac_boilerplate_pdf_surface_write_to_png,
     _comac_boilerplate_pdf_cleanup,
     NULL,
     NULL,
     FALSE,
     TRUE,
     TRUE},
    {"pdf",
     "pdf",
     ".pdf",
     NULL,
     COMAC_SURFACE_TYPE_RECORDING,
     COMAC_CONTENT_COLOR,
     0,
     "comac_pdf_surface_create",
     _comac_boilerplate_pdf_create_surface,
     comac_surface_create_similar,
     _comac_boilerplate_pdf_force_fallbacks,
     _comac_boilerplate_pdf_finish_surface,
     _comac_boilerplate_pdf_get_image_surface,
     _comac_boilerplate_pdf_surface_write_to_png,
     _comac_boilerplate_pdf_cleanup,
     NULL,
     NULL,
     FALSE,
     TRUE,
     TRUE},
};
COMAC_BOILERPLATE (pdf, targets)

#else

COMAC_NO_BOILERPLATE (pdf)

#endif
