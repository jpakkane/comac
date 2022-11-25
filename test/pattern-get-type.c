/*
 * Copyright © 2006 Red Hat, Inc.
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

#include "comac-test.h"

static comac_test_status_t
preamble (comac_test_context_t *Ctx)
{
    comac_surface_t *surface;
    comac_pattern_t *solid_rgb, *solid_rgba, *surface_pattern, *linear, *radial, *mesh;
    comac_test_status_t result = COMAC_TEST_SUCCESS;

    solid_rgb = comac_pattern_create_rgb (0.0, 0.1, 0.2);
    solid_rgba = comac_pattern_create_rgba (0.3, 0.4, 0.5, 0.6);
    surface = comac_image_surface_create (COMAC_FORMAT_ARGB32,
					  1, 1);
    surface_pattern = comac_pattern_create_for_surface (surface);
    linear = comac_pattern_create_linear (0.0, 0.0, 10.0, 10.0);
    radial = comac_pattern_create_radial (10.0, 10.0, 0.1,
					  10.0, 10.0, 1.0);
    mesh = comac_pattern_create_mesh ();

    if (comac_pattern_get_type (solid_rgb) != COMAC_PATTERN_TYPE_SOLID)
	result = COMAC_TEST_FAILURE;

    if (comac_pattern_get_type (solid_rgba) != COMAC_PATTERN_TYPE_SOLID)
	result = COMAC_TEST_FAILURE;

    if (comac_pattern_get_type (surface_pattern) != COMAC_PATTERN_TYPE_SURFACE)
	result = COMAC_TEST_FAILURE;

    if (comac_pattern_get_type (linear) != COMAC_PATTERN_TYPE_LINEAR)
	result = COMAC_TEST_FAILURE;

    if (comac_pattern_get_type (radial) != COMAC_PATTERN_TYPE_RADIAL)
	result = COMAC_TEST_FAILURE;

    if (comac_pattern_get_type (mesh) != COMAC_PATTERN_TYPE_MESH)
	result = COMAC_TEST_FAILURE;

    comac_pattern_destroy (solid_rgb);
    comac_pattern_destroy (solid_rgba);
    comac_pattern_destroy (surface_pattern);
    comac_surface_destroy (surface);
    comac_pattern_destroy (linear);
    comac_pattern_destroy (radial);
    comac_pattern_destroy (mesh);

    return result;
}

COMAC_TEST (pattern_get_type,
	    "Creating patterns of all types",
	    "pattern, api", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    preamble, NULL)
