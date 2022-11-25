/*
 * Copyright © 2014 Samsung Electronics
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
 * Author: Ravi Nanjundappa <nravi.n@samsung.com>
 */

/*
 * This case tests using a EGL surface as the source
 *
 */

#include "comac-test.h"
#include <comac-gl.h>

#include "surface-source.c"

struct closure {
    EGLDisplay dpy;
    EGLContext ctx;
};

static void
cleanup (void *data)
{
    struct closure *arg = data;

    eglDestroyContext (arg->dpy, arg->ctx);
    eglMakeCurrent (arg->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate (arg->dpy);

    free (arg);
}

static comac_surface_t *
create_source_surface (int size)
{
    EGLint config_attribs[] = {
	EGL_RED_SIZE,
	8,
	EGL_GREEN_SIZE,
	8,
	EGL_BLUE_SIZE,
	8,
	EGL_ALPHA_SIZE,
	8,
	EGL_SURFACE_TYPE,
	EGL_PBUFFER_BIT,
#if COMAC_HAS_GL_SURFACE
	EGL_RENDERABLE_TYPE,
	EGL_OPENGL_BIT,
#elif COMAC_HAS_GLESV2_SURFACE
	EGL_RENDERABLE_TYPE,
	EGL_OPENGL_ES2_BIT,
#endif
	EGL_NONE
    };
    const EGLint ctx_attribs[] = {
#if COMAC_HAS_GLESV2_SURFACE
	EGL_CONTEXT_CLIENT_VERSION,
	2,
#endif
	EGL_NONE
    };

    struct closure *arg;
    comac_device_t *device;
    comac_surface_t *surface;
    EGLConfig config;
    EGLint numConfigs;
    EGLDisplay dpy;
    EGLContext ctx;
    int major, minor;

    dpy = eglGetDisplay (EGL_DEFAULT_DISPLAY);
    if (! eglInitialize (dpy, &major, &minor)) {
	return NULL;
    }

    eglChooseConfig (dpy, config_attribs, &config, 1, &numConfigs);
    if (numConfigs == 0) {
	return NULL;
    }

#if COMAC_HAS_GL_SURFACE
    eglBindAPI (EGL_OPENGL_API);
#elif COMAC_HAS_GLESV2_SURFACE
    eglBindAPI (EGL_OPENGL_ES_API);
#endif

    ctx = eglCreateContext (dpy, config, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT) {
	eglTerminate (dpy);
	return NULL;
    }

    arg = xmalloc (sizeof (struct closure));
    arg->dpy = dpy;
    arg->ctx = ctx;
    device = comac_egl_device_create (dpy, ctx);
    if (comac_device_set_user_data (device,
				    (comac_user_data_key_t *) cleanup,
				    arg,
				    cleanup)) {
	cleanup (arg);
	return NULL;
    }

    surface =
	comac_gl_surface_create (device, COMAC_CONTENT_COLOR_ALPHA, size, size);
    comac_device_destroy (device);

    return surface;
}

COMAC_TEST (egl_surface_source,
	    "Test using a EGL surface as the source",
	    "source", /* keywords */
	    NULL,     /* requirements */
	    SIZE,
	    SIZE,
	    preamble,
	    draw)
