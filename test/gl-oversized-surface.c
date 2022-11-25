/*
 * Copyright © 2012 Igalia S.L.
 * Copyright © 2009 Eric Anholt
 * Copyright © 2009 Chris Wilson
 * Copyright © 2005 Red Hat, Inc
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
 * IGALIA S.L. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Martin Robinson <mrobinson@igalia.com>
 */

#include "comac-test.h"
#include <comac-gl.h>
#include <assert.h>
#include <limits.h>

static comac_test_status_t
preamble (comac_test_context_t *test_ctx)
{
    int rgba_attribs[] = {GLX_RGBA,
			  GLX_RED_SIZE,
			  1,
			  GLX_GREEN_SIZE,
			  1,
			  GLX_BLUE_SIZE,
			  1,
			  GLX_ALPHA_SIZE,
			  1,
			  GLX_DOUBLEBUFFER,
			  None};

    Display *display;
    XVisualInfo *visual_info;
    GLXContext glx_context;
    comac_device_t *device;
    comac_surface_t *oversized_surface;
    comac_test_status_t test_status = COMAC_TEST_SUCCESS;

    display = XOpenDisplay (NULL);
    if (display == NULL)
	return COMAC_TEST_UNTESTED;

    visual_info =
	glXChooseVisual (display, DefaultScreen (display), rgba_attribs);
    if (visual_info == NULL) {
	XCloseDisplay (display);
	return COMAC_TEST_UNTESTED;
    }

    glx_context = glXCreateContext (display, visual_info, NULL, True);
    if (glx_context == NULL) {
	XCloseDisplay (display);
	return COMAC_TEST_UNTESTED;
    }

    device = comac_glx_device_create (display, glx_context);

    oversized_surface = comac_gl_surface_create (device,
						 COMAC_CONTENT_COLOR_ALPHA,
						 INT_MAX,
						 INT_MAX);
    if (comac_surface_status (oversized_surface) != COMAC_STATUS_INVALID_SIZE)
	test_status = COMAC_TEST_FAILURE;

    comac_device_destroy (device);
    glXDestroyContext (display, glx_context);
    XCloseDisplay (display);

    return test_status;
}

COMAC_TEST (gl_oversized_surface,
	    "Test that creating a surface beyond texture limits results in an "
	    "error surface",
	    "gl", /* keywords */
	    NULL, /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
