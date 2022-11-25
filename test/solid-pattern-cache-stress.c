/*
 * Copyright © 2007 Chris Wilson.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson. Not be used in advertising or publicity pertaining to
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
 * Author: Chris Wilson <chris at chris-wilson.co.uk>
 */

#include "config.h"

#include "comac-test.h"
#include <stdlib.h> /* drand48() */

#define LOOPS 10
#define NRAND 100

#ifndef HAVE_DRAND48
#define drand48() (rand () / (double) RAND_MAX)
#endif

static comac_scaled_font_t *scaled_font;

static comac_t *
_comac_create_similar (comac_t *cr, int width, int height)
{
    comac_surface_t *similar;

    similar = comac_surface_create_similar (
	comac_get_target (cr),
	comac_surface_get_content (comac_get_target (cr)),
	width,
	height);
    cr = comac_create (similar);
    comac_surface_destroy (similar);

    return cr;
}

static comac_t *
_comac_create_image (comac_t *cr, comac_format_t format, int width, int height)
{
    comac_surface_t *image;

    image = comac_image_surface_create (format, width, height);
    cr = comac_create (image);
    comac_surface_destroy (image);

    return cr;
}

static void
_propagate_status (comac_t *dst, comac_t *src)
{
    comac_path_t path;

    path.status = comac_status (src);
    if (path.status) {
	path.num_data = 0;
	path.data = NULL;
	comac_append_path (dst, &path);
    }
}

static void
_draw (comac_t *cr, double red, double green, double blue)
{
    comac_text_extents_t extents;

    comac_set_source_rgb (cr, red, green, blue);
    comac_paint (cr);

    comac_move_to (cr, 0, 0);
    comac_line_to (cr, 1, 1);
    comac_stroke (cr);

    comac_mask (cr, comac_get_source (cr));

    comac_set_scaled_font (cr, scaled_font);
    comac_text_extents (cr, "comac", &extents);
    comac_move_to (cr,
		   -extents.x_bearing - .5 * extents.width,
		   -extents.y_bearing - .5 * extents.height);
    comac_show_text (cr, "comac");
}

static void
use_similar (comac_t *cr, double red, double green, double blue)
{
    comac_t *cr2;

    if (comac_status (cr))
	return;

    cr2 = _comac_create_similar (cr, 1, 1);

    _draw (cr2, red, green, blue);

    _propagate_status (cr, cr2);
    comac_destroy (cr2);
}

static void
use_image (
    comac_t *cr, comac_format_t format, double red, double green, double blue)
{
    comac_t *cr2;

    if (comac_status (cr))
	return;

    cr2 = _comac_create_image (cr, format, 1, 1);

    _draw (cr2, red, green, blue);

    _propagate_status (cr, cr2);
    comac_destroy (cr2);
}

static void
use_solid (comac_t *cr, double red, double green, double blue)
{
    /* mix in dissimilar solids */
    use_image (cr, COMAC_FORMAT_A1, red, green, blue);
    use_image (cr, COMAC_FORMAT_A8, red, green, blue);
    use_image (cr, COMAC_FORMAT_RGB24, red, green, blue);
    use_image (cr, COMAC_FORMAT_ARGB32, red, green, blue);

    use_similar (cr, red, green, blue);

    _draw (cr, red, green, blue);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_status_t status;
    const double colors[8][3] = {
	{1.0, 0.0, 0.0}, /* red */
	{0.0, 1.0, 0.0}, /* green */
	{1.0, 1.0, 0.0}, /* yellow */
	{0.0, 0.0, 1.0}, /* blue */
	{1.0, 0.0, 1.0}, /* magenta */
	{0.0, 1.0, 1.0}, /* cyan */
	{1.0, 1.0, 1.0}, /* white */
	{0.0, 0.0, 0.0}, /* black */
    };
    int i, j, loop;

    /* cache a resolved scaled-font */
    scaled_font = comac_get_scaled_font (cr);

    for (loop = 0; loop < LOOPS; loop++) {
	for (i = 0; i < LOOPS; i++) {
	    for (j = 0; j < 8; j++) {
		use_solid (cr, colors[j][0], colors[j][1], colors[j][2]);
		status = comac_status (cr);
		if (status)
		    return comac_test_status_from_status (ctx, status);
	    }
	}

	for (i = 0; i < NRAND; i++) {
	    use_solid (cr, drand48 (), drand48 (), drand48 ());
	    status = comac_status (cr);
	    if (status)
		return comac_test_status_from_status (ctx, status);
	}
    }

    /* stress test only, so clear the surface before comparing */
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (solid_pattern_cache_stress,
	    "Stress the solid pattern cache and ensure it behaves",
	    "stress", /* keywords */
	    NULL,     /* requirements */
	    1,
	    1,
	    NULL,
	    draw)
