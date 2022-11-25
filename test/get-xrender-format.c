/*
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include "comac-test.h"

#include "comac-xlib.h"
#include "comac-xlib-xrender.h"

#include "comac-boilerplate-xlib.h"

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    Display *dpy;
    XRenderPictFormat *orig_format, *format;
    comac_surface_t *surface;
    Pixmap pixmap;
    int screen;
    comac_test_status_t result;

    result = COMAC_TEST_UNTESTED;

    if (! comac_test_is_target_enabled (ctx, "xlib"))
	goto CLEANUP_TEST;

    dpy = XOpenDisplay (NULL);
    if (! dpy) {
	comac_test_log (ctx, "Error: Cannot open display: %s, skipping.\n",
			XDisplayName (NULL));
	goto CLEANUP_TEST;
    }

    result = COMAC_TEST_FAILURE;

    screen = DefaultScreen (dpy);

    comac_test_log (ctx, "Testing with image surface.\n");

    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1);

    format = comac_xlib_surface_get_xrender_format (surface);
    if (format != NULL) {
	comac_test_log (ctx, "Error: expected NULL for image surface\n");
	goto CLEANUP_SURFACE;
    }

    comac_surface_destroy (surface);

    comac_test_log (ctx, "Testing with non-xrender xlib surface.\n");

    pixmap = XCreatePixmap (dpy, DefaultRootWindow (dpy),
			    1, 1, DefaultDepth (dpy, screen));
    surface = comac_xlib_surface_create (dpy, pixmap,
					 DefaultVisual (dpy, screen),
					 1, 1);
    orig_format = XRenderFindVisualFormat (dpy, DefaultVisual (dpy, screen));
    format = comac_xlib_surface_get_xrender_format (surface);
    if (format != orig_format) {
	comac_test_log (ctx, "Error: did not receive the same format as XRenderFindVisualFormat\n");
	goto CLEANUP_PIXMAP;
    }
    comac_surface_destroy (surface);
    XFreePixmap (dpy, pixmap);

    comac_test_log (ctx, "Testing with xlib xrender surface.\n");

    orig_format = XRenderFindStandardFormat (dpy, PictStandardARGB32);
    pixmap = XCreatePixmap (dpy, DefaultRootWindow (dpy),
			    1, 1, 32);
    surface = comac_xlib_surface_create_with_xrender_format (dpy,
							     pixmap,
							     DefaultScreenOfDisplay (dpy),
							     orig_format,
							     1, 1);
    format = comac_xlib_surface_get_xrender_format (surface);
    if (format != orig_format) {
	comac_test_log (ctx, "Error: did not receive the same format originally set\n");
	goto CLEANUP_PIXMAP;
    }

    result = COMAC_TEST_SUCCESS;

  CLEANUP_PIXMAP:
    XFreePixmap (dpy, pixmap);
  CLEANUP_SURFACE:
    comac_surface_destroy (surface);

    XCloseDisplay (dpy);

  CLEANUP_TEST:
    return result;
}

COMAC_TEST (get_xrender_format,
	    "Check XRender specific API",
	    "xrender, api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
