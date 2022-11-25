/*
 * Copyright © 2008 Chris Wilson <chris@chris-wilson.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the comac graphics library.
 *
 * The Initial Developer of the Original Code is Chris Wilson.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "config.h"

#include "comac.h"
#include "comac-script-interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(COMAC_HAS_XLIB_SURFACE) || defined(COMAC_HAS_XLIB_XRENDER_SURFACE)
static const comac_user_data_key_t _key;
#endif

#define SINGLE_SURFACE 1

#if SINGLE_SURFACE
static comac_surface_t *
_similar_surface_create (void *closure,
			 comac_content_t content,
			 double width, double height,
			 long uid)
{
    return comac_surface_create_similar (closure, content, width, height);
}

static struct list {
    struct list *next;
    comac_t *context;
    comac_surface_t *surface;
} *list;

static comac_t *
_context_create (void *closure, comac_surface_t *surface)
{
    comac_t *cr = comac_create (surface);
    struct list *l = malloc (sizeof (*l));
    l->next = list;
    l->context = cr;
    l->surface = comac_surface_reference (surface);
    list = l;
    return cr;
}

static void
_context_destroy (void *closure, void *ptr)
{
    struct list *l, **prev = &list;
    while ((l = *prev) != NULL) {
	if (l->context == ptr) {
	    if (comac_surface_status (l->surface) == COMAC_STATUS_SUCCESS) {
		comac_t *cr = comac_create (closure);
		comac_set_source_surface (cr, l->surface, 0, 0);
		comac_paint (cr);
		comac_destroy (cr);
	    }

	    comac_surface_destroy (l->surface);
	    *prev = l->next;
	    free (l);
	    return;
	}
	prev = &l->next;
    }
}
#endif

#if COMAC_HAS_XLIB_SURFACE
#include <comac-xlib.h>
static Display *
_get_display (void)
{
    static Display *dpy;

    if (dpy != NULL)
	return dpy;

    dpy = XOpenDisplay (NULL);
    if (dpy == NULL) {
	fprintf (stderr, "Failed to open display.\n");
	exit (1);
    }

    return dpy;
}

static void
_destroy_window (void *closure)
{
    XFlush (_get_display ());
    XDestroyWindow (_get_display(), (Window) closure);
}

static comac_surface_t *
_xlib_surface_create (void *closure,
		      comac_content_t content,
		      double width, double height,
		      long uid)
{
    Display *dpy;
    XSetWindowAttributes attr;
    Visual *visual;
    int depth;
    Window w;
    comac_surface_t *surface;

    dpy = _get_display ();

    visual = DefaultVisual (dpy, DefaultScreen (dpy));
    depth = DefaultDepth (dpy, DefaultScreen (dpy));
    attr.override_redirect = True;
    w = XCreateWindow (dpy, DefaultRootWindow (dpy), 0, 0,
		       width <= 0 ? 1 : width,
		       height <= 0 ? 1 : height,
		       0, depth,
		       InputOutput, visual, CWOverrideRedirect, &attr);
    XMapWindow (dpy, w);

    surface = comac_xlib_surface_create (dpy, w, visual, width, height);
    comac_surface_set_user_data (surface, &_key, (void *) w, _destroy_window);

    return surface;
}

#if COMAC_HAS_XLIB_XRENDER_SURFACE
#include <comac-xlib-xrender.h>

static void
_destroy_pixmap (void *closure)
{
    XFreePixmap (_get_display(), (Pixmap) closure);
}

static comac_surface_t *
_xrender_surface_create (void *closure,
			 comac_content_t content,
			 double width, double height,
			 long uid)
{
    Display *dpy;
    Pixmap pixmap;
    XRenderPictFormat *xrender_format;
    comac_surface_t *surface;

    dpy = _get_display ();

    content = COMAC_CONTENT_COLOR_ALPHA;

    switch (content) {
    case COMAC_CONTENT_COLOR_ALPHA:
	xrender_format = XRenderFindStandardFormat (dpy, PictStandardARGB32);
	break;
    case COMAC_CONTENT_COLOR:
	xrender_format = XRenderFindStandardFormat (dpy, PictStandardRGB24);
	break;
    case COMAC_CONTENT_ALPHA:
    default:
	xrender_format = XRenderFindStandardFormat (dpy, PictStandardA8);
    }

    pixmap = XCreatePixmap (dpy, DefaultRootWindow (dpy),
		       width, height, xrender_format->depth);

    surface = comac_xlib_surface_create_with_xrender_format (dpy, pixmap,
							     DefaultScreenOfDisplay (dpy),
							     xrender_format,
							     width, height);
    comac_surface_set_user_data (surface, &_key,
				 (void *) pixmap, _destroy_pixmap);

    return surface;
}
#endif
#endif

#if COMAC_HAS_GL_GLX_SURFACE
#include <comac-gl.h>
static comac_gl_context_t *
_glx_get_context (comac_content_t content)
{
    static comac_gl_context_t *context;

    if (context == NULL) {
	int rgba_attribs[] = {
	    GLX_RGBA,
	    GLX_RED_SIZE, 1,
	    GLX_GREEN_SIZE, 1,
	    GLX_BLUE_SIZE, 1,
	    GLX_ALPHA_SIZE, 1,
	    GLX_DOUBLEBUFFER,
	    None
	};
	XVisualInfo *visinfo;
	GLXContext gl_ctx;
	Display *dpy;

	dpy = XOpenDisplay (NULL);
	if (dpy == NULL) {
	    fprintf (stderr, "Failed to open display.\n");
	    exit (1);
	}

	visinfo = glXChooseVisual (dpy, DefaultScreen (dpy), rgba_attribs);
	if (visinfo == NULL) {
	    fprintf (stderr, "Failed to create RGBA, double-buffered visual\n");
	    exit (1);
	}

	gl_ctx = glXCreateContext (dpy, visinfo, NULL, True);
	XFree (visinfo);

	context = comac_glx_context_create (dpy, gl_ctx);
    }

    return context;
}

static comac_surface_t *
_glx_surface_create (void *closure,
		     comac_content_t content,
		     double width, double height,
		     long uid)
{
    if (width == 0)
	width = 1;
    if (height == 0)
	height = 1;

    return comac_gl_surface_create (_glx_get_context (content),
				    content, width, height);
}
#endif

#if COMAC_HAS_PDF_SURFACE
#include <comac-pdf.h>
static comac_surface_t *
_pdf_surface_create (void *closure,
		     comac_content_t content,
		     double width, double height,
		     long uid)
{
    return comac_pdf_surface_create_for_stream (NULL, NULL, width, height);
}
#endif

#if COMAC_HAS_PS_SURFACE
#include <comac-ps.h>
static comac_surface_t *
_ps_surface_create (void *closure,
		    comac_content_t content,
		    double width, double height,
		    long uid)
{
    return comac_ps_surface_create_for_stream (NULL, NULL, width, height);
}
#endif

#if COMAC_HAS_SVG_SURFACE
#include <comac-svg.h>
static comac_surface_t *
_svg_surface_create (void *closure,
		     comac_content_t content,
		     double width, double height,
		     long uid)
{
    return comac_svg_surface_create_for_stream (NULL, NULL, width, height);
}
#endif

static comac_surface_t *
_image_surface_create (void *closure,
		       comac_content_t content,
		       double width, double height,
		       long uid)
{
    return comac_image_surface_create (COMAC_FORMAT_ARGB32, width, height);
}

int
main (int argc, char **argv)
{
    comac_script_interpreter_t *csi;
    comac_script_interpreter_hooks_t hooks = {
#if SINGLE_SURFACE
	.surface_create = _similar_surface_create,
	.context_create = _context_create,
	.context_destroy = _context_destroy
#elif COMAC_HAS_XLIB_XRENDER_SURFACE
	.surface_create = _xrender_surface_create
#elif COMAC_HAS_XLIB_SURFACE
	.surface_create = _xlib_surface_create
#elif COMAC_PDF_SURFACE
	.surface_create = _pdf_surface_create
#elif COMAC_PS_SURFACE
	.surface_create = _ps_surface_create
#elif COMAC_SVG_SURFACE
	.surface_create = _svg_surface_create
#else
	.surface_create = _image_surface_create
#endif
    };
    int i;
    const struct backends {
	const char *name;
	csi_surface_create_func_t create;
    } backends[] = {
	{ "--image", _image_surface_create },
#if COMAC_HAS_XLIB_XRENDER_SURFACE
	{ "--xrender", _xrender_surface_create },
#endif
#if COMAC_HAS_GL_GLX_SURFACE
	{ "--glx", _glx_surface_create },
#endif
#if COMAC_HAS_XLIB_SURFACE
	{ "--xlib", _xlib_surface_create },
#endif
#if COMAC_HAS_PDF_SURFACE
	{ "--pdf", _pdf_surface_create },
#endif
#if COMAC_HAS_PS_SURFACE
	{ "--ps", _ps_surface_create },
#endif
#if COMAC_HAS_SVG_SURFACE
	{ "--svg", _svg_surface_create },
#endif
	{ NULL, NULL }
    };

#if SINGLE_SURFACE
    hooks.closure = backends[0].create (NULL,
					COMAC_CONTENT_COLOR_ALPHA,
					512, 512,
					0);
#endif


    csi = comac_script_interpreter_create ();
    comac_script_interpreter_install_hooks (csi, &hooks);

    for (i = 1; i < argc; i++) {
	const struct backends *b;

	for (b = backends; b->name != NULL; b++) {
	    if (strcmp (b->name, argv[i]) == 0) {
#if SINGLE_SURFACE
		comac_surface_destroy (hooks.closure);
		hooks.closure = b->create (NULL,
					   COMAC_CONTENT_COLOR_ALPHA,
					   512, 512,
					   0);
#else
		hooks.surface_create = b->create;
#endif
		comac_script_interpreter_install_hooks (csi, &hooks);
		break;
	    }
	}

	if (b->name == NULL)
	    comac_script_interpreter_run (csi, argv[i]);
    }
    comac_surface_destroy (hooks.closure);

    return comac_script_interpreter_destroy (csi);
}
