/*
 * Copyright 2014 Intel Corporation
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
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

/* Test the fidelity of the rasterisation, because Comac is my favourite
 * driver test suite.
 */

#define GENERATE_REFERENCE 0

#include "../src/comac-fixed-type-private.h"

#if GENERATE_REFERENCE
#include <assert.h>
struct coverage {
    int width, height;
    struct {
	int uncovered_area;
	int covered_height;
    } cells[0];
};

static int
pfloor (int v)
{
    return v >> COMAC_FIXED_FRAC_BITS;
}

static int
pfrac (int v)
{
    return v & ((1 << COMAC_FIXED_FRAC_BITS) - 1);
}

static void
add_edge (struct coverage *coverage, int x1, int y1, int x2, int y2, int sign)
{
    int dx, dy;
    int dxdy_quo, dxdy_rem;
    int xq, xr;
    int y, t;

    if (y2 < y1) {
	t = y1;
	y1 = y2;
	y2 = t;

	t = x1;
	x1 = x2;
	x2 = t;

	sign = -sign;
    }

    dx = x2 - x1;
    dy = y2 - y1;
    if (dy == 0)
	return;

    dy *= 2;

    dxdy_quo = 2 * dx / dy;
    dxdy_rem = 2 * dx % dy;

    xq = x1 + dxdy_quo / 2;
    xr = dxdy_rem / 2;
    if (xr < 0) {
	xq--;
	xr += dy;
    }

    for (y = MAX (0, y1); y < MIN (y2, 256 * coverage->height); y++) {
	int x = xq + (xr >= dy / 2);

	if (x < 256 * coverage->width) {
	    int i = pfloor (y) * coverage->width;
	    if (x > 0) {
		i += pfloor (x);
		coverage->cells[i].uncovered_area += sign * pfrac (x);
	    }
	    coverage->cells[i].covered_height += sign;
	}

	xq += dxdy_quo;
	xr += dxdy_rem;
	if (xr < 0) {
	    xq--;
	    xr += dy;
	} else if (xr >= dy) {
	    xq++;
	    xr -= dy;
	}
    }
}

static struct coverage *
coverage_create (int width, int height)
{
    int size;
    struct coverage *c;

    size = sizeof (struct coverage);
    size += width * height * sizeof (int) * 2;

    c = malloc (size);
    if (c == NULL)
	return c;

    memset (c, 0, size);
    c->width = width;
    c->height = height;

    return c;
}

static comac_surface_t *
coverage_to_alpha (struct coverage *c)
{
    comac_surface_t *image;
    uint8_t *data;
    int x, y, stride;

    image = comac_image_surface_create (COMAC_FORMAT_A8, c->width, c->height);

    data = comac_image_surface_get_data (image);
    stride = comac_image_surface_get_stride (image);

    comac_surface_flush (image);
    for (y = 0; y < c->height; y++) {
	uint8_t *row = data + y * stride;
	int cover = 0;
	for (x = 0; x < c->width; x++) {
	    int v = y * c->width + x;

	    cover += c->cells[v].covered_height * 256;
	    v = cover - c->cells[v].uncovered_area;

	    v /= 256;
	    if (v < 0)
		v = -v;
	    row[x] = v - (v >> 8);
	}
    }
    comac_surface_mark_dirty (image);

    free (c);
    return image;
}
#endif

static comac_test_status_t
edge (comac_t *cr, int width, int height)
{
    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_paint (cr);

    comac_set_operator (cr, COMAC_OPERATOR_ADD);

#if GENERATE_REFERENCE
    {
	struct coverage *c;
	comac_surface_t *mask;

	comac_set_source_rgb (cr, 1, 0, 0);

	c = coverage_create (width, height);
	add_edge (c, 128 * 256, 129 * 256, 129 * 256, 1 * 256, 1);
	add_edge (c, 128 * 256, 129 * 256, 128 * 256, 131 * 256, -1);
	add_edge (c, 128 * 256, 131 * 256, 129 * 256, 259 * 256, -1);
	add_edge (c, 130 * 256, 129 * 256, 129 * 256, 1 * 256, -1);
	add_edge (c, 130 * 256, 129 * 256, 130 * 256, 131 * 256, 1);
	add_edge (c, 130 * 256, 131 * 256, 129 * 256, 259 * 256, 1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);

	c = coverage_create (width, height);
	add_edge (c,
		  128 * 256 / 2,
		  129 * 256 / 2,
		  129 * 256 / 2,
		  1 * 256 / 2,
		  1);
	add_edge (c,
		  128 * 256 / 2,
		  129 * 256 / 2,
		  128 * 256 / 2,
		  131 * 256 / 2,
		  -1);
	add_edge (c,
		  128 * 256 / 2,
		  131 * 256 / 2,
		  129 * 256 / 2,
		  259 * 256 / 2,
		  -1);
	add_edge (c,
		  130 * 256 / 2,
		  129 * 256 / 2,
		  129 * 256 / 2,
		  1 * 256 / 2,
		  -1);
	add_edge (c,
		  130 * 256 / 2,
		  129 * 256 / 2,
		  130 * 256 / 2,
		  131 * 256 / 2,
		  1);
	add_edge (c,
		  130 * 256 / 2,
		  131 * 256 / 2,
		  129 * 256 / 2,
		  259 * 256 / 2,
		  1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);

	c = coverage_create (width, height);
	add_edge (c, (192 - 2) * 256, 129 * 256, 192 * 256, 1 * 256, 1);
	add_edge (c,
		  (192 - 2) * 256,
		  129 * 256,
		  (192 - 2) * 256,
		  131 * 256,
		  -1);
	add_edge (c, (192 - 2) * 256, 131 * 256, 192 * 256, 259 * 256, -1);
	add_edge (c, (192 + 2) * 256, 129 * 256, 192 * 256, 1 * 256, -1);
	add_edge (c, (192 + 2) * 256, 129 * 256, (192 + 2) * 256, 131 * 256, 1);
	add_edge (c, (192 + 2) * 256, 131 * 256, 192 * 256, 259 * 256, 1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);

	c = coverage_create (width, height);
	add_edge (c, (256 - 4) * 256, 129 * 256, 256 * 256, 1 * 256, 1);
	add_edge (c,
		  (256 - 4) * 256,
		  129 * 256,
		  (256 - 4) * 256,
		  131 * 256,
		  -1);
	add_edge (c, (256 - 4) * 256, 131 * 256, 256 * 256, 259 * 256, -1);
	add_edge (c, (256 + 4) * 256, 129 * 256, 256 * 256, 1 * 256, -1);
	add_edge (c, (256 + 4) * 256, 129 * 256, (256 + 4) * 256, 131 * 256, 1);
	add_edge (c, (256 + 4) * 256, 131 * 256, 256 * 256, 259 * 256, 1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);

	comac_set_source_rgb (cr, 0, 1, 0);

	c = coverage_create (width, height);
	add_edge (c, 1 * 256, 129 * 256, 129 * 256, 128 * 256, 1);
	add_edge (c, 131 * 256, 128 * 256, 259 * 256, 129 * 256, 1);
	add_edge (c, 1 * 256, 129 * 256, 129 * 256, 130 * 256, -1);
	add_edge (c, 131 * 256, 130 * 256, 259 * 256, 129 * 256, -1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);

	c = coverage_create (width, height);
	add_edge (c,
		  1 * 256 / 2,
		  129 * 256 / 2,
		  129 * 256 / 2,
		  128 * 256 / 2,
		  1);
	add_edge (c,
		  131 * 256 / 2,
		  128 * 256 / 2,
		  259 * 256 / 2,
		  129 * 256 / 2,
		  1);
	add_edge (c,
		  1 * 256 / 2,
		  129 * 256 / 2,
		  129 * 256 / 2,
		  130 * 256 / 2,
		  -1);
	add_edge (c,
		  131 * 256 / 2,
		  130 * 256 / 2,
		  259 * 256 / 2,
		  129 * 256 / 2,
		  -1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);

	c = coverage_create (width, height);
	add_edge (c, 1 * 256, (192 - 0) * 256, 129 * 256, (192 - 2) * 256, 1);
	add_edge (c, 131 * 256, (192 - 2) * 256, 259 * 256, (192 - 0) * 256, 1);
	add_edge (c, 1 * 256, (192 + 0) * 256, 129 * 256, (192 + 2) * 256, -1);
	add_edge (c,
		  131 * 256,
		  (192 + 2) * 256,
		  259 * 256,
		  (192 + 0) * 256,
		  -1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);

	c = coverage_create (width, height);
	add_edge (c, 1 * 256, (256 - 0) * 256, 129 * 256, (256 - 4) * 256, 1);
	add_edge (c, 131 * 256, (256 - 4) * 256, 259 * 256, (256 - 0) * 256, 1);
	add_edge (c, 1 * 256, (256 + 0) * 256, 129 * 256, (256 + 4) * 256, -1);
	add_edge (c,
		  131 * 256,
		  (256 + 4) * 256,
		  259 * 256,
		  (256 + 0) * 256,
		  -1);
	mask = coverage_to_alpha (c);
	comac_mask_surface (cr, mask, 0, 0);
	comac_surface_destroy (mask);
    }
#else
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_move_to (cr, 129, 1);
    comac_line_to (cr, 128, 129);
    comac_line_to (cr, 128, 131);
    comac_line_to (cr, 129, 259);
    comac_line_to (cr, 130, 131);
    comac_line_to (cr, 130, 129);
    comac_fill (cr);

    comac_move_to (cr, 129 / 2., 1 / 2.);
    comac_line_to (cr, 128 / 2., 129 / 2.);
    comac_line_to (cr, 128 / 2., 131 / 2.);
    comac_line_to (cr, 129 / 2., 259 / 2.);
    comac_line_to (cr, 130 / 2., 131 / 2.);
    comac_line_to (cr, 130 / 2., 129 / 2.);
    comac_fill (cr);

    comac_move_to (cr, 192, 1);
    comac_line_to (cr, 192 - 2, 129);
    comac_line_to (cr, 192 - 2, 131);
    comac_line_to (cr, 192, 259);
    comac_line_to (cr, 192 + 2, 131);
    comac_line_to (cr, 192 + 2, 129);
    comac_fill (cr);

    comac_move_to (cr, 256, 1);
    comac_line_to (cr, 256 - 4, 129);
    comac_line_to (cr, 256 - 4, 131);
    comac_line_to (cr, 256, 259);
    comac_line_to (cr, 256 + 4, 131);
    comac_line_to (cr, 256 + 4, 129);
    comac_fill (cr);

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_move_to (cr, 1, 129);
    comac_line_to (cr, 129, 128);
    comac_line_to (cr, 131, 128);
    comac_line_to (cr, 259, 129);
    comac_line_to (cr, 131, 130);
    comac_line_to (cr, 129, 130);
    comac_fill (cr);

    comac_move_to (cr, 1 / 2., 129 / 2.);
    comac_line_to (cr, 129 / 2., 128 / 2.);
    comac_line_to (cr, 131 / 2., 128 / 2.);
    comac_line_to (cr, 259 / 2., 129 / 2.);
    comac_line_to (cr, 131 / 2., 130 / 2.);
    comac_line_to (cr, 129 / 2., 130 / 2.);
    comac_fill (cr);

    comac_move_to (cr, 1, 192);
    comac_line_to (cr, 129, 192 - 2);
    comac_line_to (cr, 131, 192 - 2);
    comac_line_to (cr, 259, 192);
    comac_line_to (cr, 131, 192 + 2);
    comac_line_to (cr, 129, 192 + 2);
    comac_fill (cr);

    comac_move_to (cr, 1, 256);
    comac_line_to (cr, 129, 256 - 4);
    comac_line_to (cr, 131, 256 - 4);
    comac_line_to (cr, 259, 256);
    comac_line_to (cr, 131, 256 + 4);
    comac_line_to (cr, 129, 256 + 4);
    comac_fill (cr);
#endif

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (simple_edge,
	    "Check the fidelity of the rasterisation.",
	    NULL,	     /* keywords */
	    "target=raster", /* requirements */
	    260,
	    260,
	    NULL,
	    edge)
