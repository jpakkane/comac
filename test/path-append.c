/*
 * Copyright © 2009 Jeff Muizelaar
 * Copyright © 2009 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "comac-test.h"
#include <stdlib.h>

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_matrix_t m;
    int xoffset = 50;
    int yoffset = 50;

    comac_surface_t *shadow;
    comac_t *shadow_cr;
    comac_path_t *path;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_translate (cr, 130, 130);
    comac_rotate (cr, .5);//2*M_PI*angle/360);
    comac_rectangle (cr, 0, 0, 50, 100);
    comac_get_matrix (cr, &m);

    shadow = comac_surface_create_similar (comac_get_target (cr),
					   COMAC_CONTENT_COLOR_ALPHA,
					   600 - xoffset,
					   600 - yoffset);
    comac_surface_set_device_offset (shadow, xoffset, yoffset);
    shadow_cr = comac_create (shadow);
    comac_surface_destroy (shadow);

    comac_set_source_rgb (shadow_cr, 0, 1, 0);
    comac_set_matrix (shadow_cr, &m);

    path = comac_copy_path (cr);
    comac_new_path (shadow_cr);
    comac_append_path (shadow_cr, path);
    comac_fill (shadow_cr);
    comac_path_destroy (path);

    comac_identity_matrix (cr);
    comac_translate (cr, 10, 50);
    comac_set_source_surface (cr, comac_get_target (shadow_cr), 0, 0);
    comac_paint (cr);
    comac_set_matrix (cr, &m);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill (cr);

    comac_destroy (shadow_cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (path_append,
	    "Test appending path to a context, in particular to exercise a regression in 005436",
	    "path", /* keywords */
	    NULL, /* requirements */
	    600, 600,
	    NULL, draw)
