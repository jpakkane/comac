/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright © Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-boilerplate-private.h"

#include "comac-script.h"

static comac_user_data_key_t script_closure_key;

typedef struct _script_target_closure {
    char *filename;
    double width;
    double height;
} script_target_closure_t;

static comac_surface_t *
_comac_boilerplate_script_create_surface (const char *name,
					  comac_content_t content,
					  double width,
					  double height,
					  double max_width,
					  double max_height,
					  comac_boilerplate_mode_t mode,
					  void **closure)
{
    script_target_closure_t *ptc;
    comac_device_t *ctx;
    comac_surface_t *surface;
    comac_status_t status;

    *closure = ptc = xmalloc (sizeof (script_target_closure_t));

    ptc->width = width;
    ptc->height = height;

    xasprintf (&ptc->filename, "%s.out.cs", name);
    xunlink (ptc->filename);

    ctx = comac_script_create (ptc->filename);
    surface = comac_script_surface_create (ctx, content, width, height);
    comac_device_destroy (ctx);

    status =
	comac_surface_set_user_data (surface, &script_closure_key, ptc, NULL);
    if (status == COMAC_STATUS_SUCCESS)
	return surface;

    comac_surface_destroy (surface);
    surface = comac_boilerplate_surface_create_in_error (status);

    free (ptc->filename);
    free (ptc);
    return surface;
}

static comac_status_t
_comac_boilerplate_script_finish_surface (comac_surface_t *surface)
{
    comac_surface_finish (surface);
    return comac_surface_status (surface);
}

static comac_status_t
_comac_boilerplate_script_surface_write_to_png (comac_surface_t *surface,
						const char *filename)
{
    return COMAC_STATUS_WRITE_ERROR;
}

static comac_surface_t *
_comac_boilerplate_script_convert_to_image (comac_surface_t *surface, int page)
{
    script_target_closure_t *ptc =
	comac_surface_get_user_data (surface, &script_closure_key);
    return comac_boilerplate_convert_to_image (ptc->filename, page);
}

static comac_surface_t *
_comac_boilerplate_script_get_image_surface (comac_surface_t *surface,
					     int page,
					     int width,
					     int height)
{
    comac_surface_t *image;

    image = _comac_boilerplate_script_convert_to_image (surface, page);
    comac_surface_set_device_offset (
	image,
	comac_image_surface_get_width (image) - width,
	comac_image_surface_get_height (image) - height);
    surface = _comac_boilerplate_get_image_surface (image, 0, width, height);
    comac_surface_destroy (image);

    return surface;
}

static void
_comac_boilerplate_script_cleanup (void *closure)
{
    script_target_closure_t *ptc = closure;
    free (ptc->filename);
    free (ptc);
}

static const comac_boilerplate_target_t target[] = {
    {"script",
     "script",
     ".cs",
     NULL,
     COMAC_SURFACE_TYPE_SCRIPT,
     COMAC_CONTENT_COLOR_ALPHA,
     0,
     "comac_script_surface_create",
     _comac_boilerplate_script_create_surface,
     comac_surface_create_similar,
     NULL,
     _comac_boilerplate_script_finish_surface,
     _comac_boilerplate_script_get_image_surface,
     _comac_boilerplate_script_surface_write_to_png,
     _comac_boilerplate_script_cleanup,
     NULL,
     NULL,
     FALSE,
     FALSE,
     FALSE}};
COMAC_BOILERPLATE (script, target)
