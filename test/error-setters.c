/*
 * Copyright © 2010 Red Hat Inc.
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
 * Author: Benjamin Otte <otte@redhat.com>
 */

#include "config.h"

#include <limits.h>

#include "comac-test.h"

#if COMAC_HAS_GL_SURFACE
#include <comac-gl.h>
#endif
#if COMAC_HAS_PDF_SURFACE
#include <comac-pdf.h>
#endif
#if COMAC_HAS_PS_SURFACE
#include <comac-ps.h>
#endif
#if COMAC_HAS_XCB_SURFACE
#include <comac-xcb.h>
#endif
#if COMAC_HAS_XLIB_SURFACE
#include <comac-xlib.h>
#endif

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_surface_t *surface;

    /* get the error surface */
    surface =
	comac_image_surface_create (COMAC_FORMAT_ARGB32, INT_MAX, INT_MAX);

#if COMAC_HAS_GL_SURFACE
    comac_gl_surface_set_size (surface, 0, 0);
    comac_gl_surface_swapbuffers (surface);
#endif

#if COMAC_HAS_PDF_SURFACE
    comac_pdf_surface_restrict_to_version (surface, COMAC_PDF_VERSION_1_4);
    comac_pdf_surface_set_size (surface, 0, 0);
#endif

#if COMAC_HAS_PS_SURFACE
    comac_ps_surface_set_eps (surface, FALSE);
    comac_ps_surface_set_size (surface, 0, 0);
    comac_ps_surface_restrict_to_level (surface, COMAC_PS_LEVEL_2);
    comac_ps_surface_dsc_comment (surface, NULL);
    comac_ps_surface_dsc_begin_setup (surface);
    comac_ps_surface_dsc_begin_page_setup (surface);
#endif

#if COMAC_HAS_XCB_SURFACE
    comac_xcb_surface_set_size (surface, 0, 0);
#endif

#if COMAC_HAS_XLIB_SURFACE
    comac_xlib_surface_set_size (surface, 0, 0);
    comac_xlib_surface_set_drawable (surface, 0, 0, 0);
#endif

    comac_surface_set_mime_data (surface, NULL, NULL, 0, NULL, 0);
    comac_surface_set_device_offset (surface, 0, 0);
    comac_surface_set_fallback_resolution (surface, 0, 0);

    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (error_setters,
	    "Check setters properly error out on read-only error surfaces",
	    NULL, /* keywords */
	    NULL, /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
