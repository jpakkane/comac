/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2003 University of Southern California
 * Copyright © 2009,2010,2011 Intel Corporation
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

/* This compositor is slightly pointless. Just exists for testing
 * and as skeleton code.
 */

#include "comacint.h"

#include "comac-image-surface-private.h"

#include "comac-compositor-private.h"
#include "comac-region-private.h"

#error This file isn't included in any Makefile

static comac_int_status_t
acquire (void *abstract_dst)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
release (void *abstract_dst)
{
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
set_clip_region (void *_surface, comac_region_t *region)
{
    comac_image_surface_t *surface = _surface;
    pixman_region32_t *rgn = region ? &region->rgn : NULL;

    if (! pixman_image_set_clip_region32 (surface->pixman_image, rgn))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    return COMAC_STATUS_SUCCESS;
}

static comac_bool_t
has_snapshot (void *_dst, const comac_pattern_t *pattern)
{
    return FALSE;
}

static comac_int_status_t
draw_image (void *_dst,
	    comac_image_surface_t *image,
	    int src_x,
	    int src_y,
	    int width,
	    int height,
	    int dst_x,
	    int dst_y)
{
    comac_image_surface_t *dst = (comac_image_surface_t *) _dst;

    pixman_image_composite32 (PIXMAN_OP_SRC,
			      image->pixman_image,
			      NULL,
			      dst->pixman_image,
			      src_x,
			      src_y,
			      0,
			      0,
			      dst_x,
			      dst_y,
			      width,
			      height);
    return COMAC_STATUS_SUCCESS;
}

static inline uint32_t
color_to_uint32 (const comac_color_t *color)
{
    return ((uint32_t) color->alpha_short >> 8 << 24) |
	   (color->red_short >> 8 << 16) | (color->green_short & 0xff00) |
	   (color->blue_short >> 8);
}

static inline comac_bool_t
color_to_pixel (const comac_color_t *color,
		double opacity,
		pixman_format_code_t format,
		uint32_t *pixel)
{
    comac_color_t opacity_color;
    uint32_t c;

    if (! (format == PIXMAN_a8r8g8b8 || format == PIXMAN_x8r8g8b8 ||
	   format == PIXMAN_a8b8g8r8 || format == PIXMAN_x8b8g8r8 ||
	   format == PIXMAN_b8g8r8a8 || format == PIXMAN_b8g8r8x8 ||
	   format == PIXMAN_r5g6b5 || format == PIXMAN_b5g6r5 ||
	   format == PIXMAN_a8)) {
	return FALSE;
    }

    if (opacity != 1.0) {
	_comac_color_init_rgba (&opacity_color,
				color->red,
				color->green,
				color->blue,
				color->alpha * opacity);
	color = &opacity_color;
    }
    c = color_to_uint32 (color);

    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_ABGR) {
	c = ((c & 0xff000000) >> 0) | ((c & 0x00ff0000) >> 16) |
	    ((c & 0x0000ff00) >> 0) | ((c & 0x000000ff) << 16);
    }

    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_BGRA) {
	c = ((c & 0xff000000) >> 24) | ((c & 0x00ff0000) >> 8) |
	    ((c & 0x0000ff00) << 8) | ((c & 0x000000ff) << 24);
    }

    if (format == PIXMAN_a8) {
	c = c >> 24;
    } else if (format == PIXMAN_r5g6b5 || format == PIXMAN_b5g6r5) {
	c = ((((c) >> 3) & 0x001f) | (((c) >> 5) & 0x07e0) |
	     (((c) >> 8) & 0xf800));
    }

    *pixel = c;
    return TRUE;
}

static comac_int_status_t
fill_rectangles (void *_dst,
		 comac_operator_t op,
		 const comac_color_t *color,
		 comac_rectangle_int_t *rects,
		 int num_rects)
{
    comac_image_surface_t *dst = _dst;
    uint32_t pixel;
    int i;

    if (! color_to_pixel (color, 1.0, dst->pixman_format, &pixel))
	return COMAC_INT_STATUS_UNSUPPORTED;

    for (i = 0; i < num_rects; i++) {
	pixman_fill ((uint32_t *) dst->data,
		     dst->stride / sizeof (uint32_t),
		     PIXMAN_FORMAT_BPP (dst->pixman_format),
		     rects[i].x,
		     rects[i].y,
		     rects[i].width,
		     rects[i].height,
		     pixel);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
fill_boxes (void *_dst,
	    comac_operator_t op,
	    const comac_color_t *color,
	    comac_boxes_t *boxes)
{
    comac_image_surface_t *dst = _dst;
    struct _comac_boxes_chunk *chunk;
    uint32_t pixel;
    int i;

    assert (boxes->is_pixel_aligned);

    if (! color_to_pixel (color, 1.0, dst->pixman_format, &pixel))
	return COMAC_INT_STATUS_UNSUPPORTED;

    for (chunk = &boxes->chunks; chunk; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++) {
	    int x1 = _comac_fixed_integer_part (chunk->base[i].p1.x);
	    int y1 = _comac_fixed_integer_part (chunk->base[i].p1.y);
	    int x2 = _comac_fixed_integer_part (chunk->base[i].p2.x);
	    int y2 = _comac_fixed_integer_part (chunk->base[i].p2.y);
	    pixman_fill ((uint32_t *) dst->data,
			 dst->stride / sizeof (uint32_t),
			 PIXMAN_FORMAT_BPP (dst->pixman_format),
			 x1,
			 y1,
			 x2 - x1,
			 y2 - y1,
			 pixel);
	}
    }

    return COMAC_STATUS_SUCCESS;
}

static pixman_op_t
_pixman_operator (comac_operator_t op)
{
    switch ((int) op) {
    case COMAC_OPERATOR_CLEAR:
	return PIXMAN_OP_CLEAR;

    case COMAC_OPERATOR_SOURCE:
	return PIXMAN_OP_SRC;
    case COMAC_OPERATOR_OVER:
	return PIXMAN_OP_OVER;
    case COMAC_OPERATOR_IN:
	return PIXMAN_OP_IN;
    case COMAC_OPERATOR_OUT:
	return PIXMAN_OP_OUT;
    case COMAC_OPERATOR_ATOP:
	return PIXMAN_OP_ATOP;

    case COMAC_OPERATOR_DEST:
	return PIXMAN_OP_DST;
    case COMAC_OPERATOR_DEST_OVER:
	return PIXMAN_OP_OVER_REVERSE;
    case COMAC_OPERATOR_DEST_IN:
	return PIXMAN_OP_IN_REVERSE;
    case COMAC_OPERATOR_DEST_OUT:
	return PIXMAN_OP_OUT_REVERSE;
    case COMAC_OPERATOR_DEST_ATOP:
	return PIXMAN_OP_ATOP_REVERSE;

    case COMAC_OPERATOR_XOR:
	return PIXMAN_OP_XOR;
    case COMAC_OPERATOR_ADD:
	return PIXMAN_OP_ADD;
    case COMAC_OPERATOR_SATURATE:
	return PIXMAN_OP_SATURATE;

    case COMAC_OPERATOR_MULTIPLY:
	return PIXMAN_OP_MULTIPLY;
    case COMAC_OPERATOR_SCREEN:
	return PIXMAN_OP_SCREEN;
    case COMAC_OPERATOR_OVERLAY:
	return PIXMAN_OP_OVERLAY;
    case COMAC_OPERATOR_DARKEN:
	return PIXMAN_OP_DARKEN;
    case COMAC_OPERATOR_LIGHTEN:
	return PIXMAN_OP_LIGHTEN;
    case COMAC_OPERATOR_COLOR_DODGE:
	return PIXMAN_OP_COLOR_DODGE;
    case COMAC_OPERATOR_COLOR_BURN:
	return PIXMAN_OP_COLOR_BURN;
    case COMAC_OPERATOR_HARD_LIGHT:
	return PIXMAN_OP_HARD_LIGHT;
    case COMAC_OPERATOR_SOFT_LIGHT:
	return PIXMAN_OP_SOFT_LIGHT;
    case COMAC_OPERATOR_DIFFERENCE:
	return PIXMAN_OP_DIFFERENCE;
    case COMAC_OPERATOR_EXCLUSION:
	return PIXMAN_OP_EXCLUSION;
    case COMAC_OPERATOR_HSL_HUE:
	return PIXMAN_OP_HSL_HUE;
    case COMAC_OPERATOR_HSL_SATURATION:
	return PIXMAN_OP_HSL_SATURATION;
    case COMAC_OPERATOR_HSL_COLOR:
	return PIXMAN_OP_HSL_COLOR;
    case COMAC_OPERATOR_HSL_LUMINOSITY:
	return PIXMAN_OP_HSL_LUMINOSITY;

    default:
	ASSERT_NOT_REACHED;
	return PIXMAN_OP_OVER;
    }
}

static comac_int_status_t
composite (void *_dst,
	   comac_operator_t op,
	   comac_surface_t *abstract_src,
	   comac_surface_t *abstract_mask,
	   int src_x,
	   int src_y,
	   int mask_x,
	   int mask_y,
	   int dst_x,
	   int dst_y,
	   unsigned int width,
	   unsigned int height)
{
    comac_image_surface_t *dst = _dst;
    comac_pixman_source_t *src = (comac_pixman_source_t *) abstract_src;
    comac_pixman_source_t *mask = (comac_pixman_source_t *) abstract_mask;
    if (mask) {
	pixman_image_composite32 (_pixman_operator (op),
				  src->pixman_image,
				  mask->pixman_image,
				  dst->pixman_image,
				  src_x,
				  src_y,
				  mask_x,
				  mask_y,
				  dst_x,
				  dst_y,
				  width,
				  height);
    } else {
	pixman_image_composite32 (_pixman_operator (op),
				  src->pixman_image,
				  NULL,
				  dst->pixman_image,
				  src_x,
				  src_y,
				  0,
				  0,
				  dst_x,
				  dst_y,
				  width,
				  height);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
composite_boxes (void *_dst,
		 comac_operator_t op,
		 comac_surface_t *abstract_src,
		 comac_surface_t *abstract_mask,
		 int src_x,
		 int src_y,
		 int mask_x,
		 int mask_y,
		 int dst_x,
		 int dst_y,
		 comac_boxes_t *boxes)
{
    comac_image_surface_t *dst = _dst;
    comac_pixman_source_t *src = (comac_pixman_source_t *) abstract_src;
    comac_pixman_source_t *mask = (comac_pixman_source_t *) abstract_mask;
    struct _comac_boxes_chunk *chunk;
    int i;

    assert (boxes->is_pixel_aligned);

    op = _pixman_operator (op);
    for (chunk = &boxes->chunks; chunk; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++) {
	    int x1 = _comac_fixed_integer_part (chunk->base[i].p1.x);
	    int y1 = _comac_fixed_integer_part (chunk->base[i].p1.y);
	    int x2 = _comac_fixed_integer_part (chunk->base[i].p2.x);
	    int y2 = _comac_fixed_integer_part (chunk->base[i].p2.y);

	    if (mask) {
		pixman_image_composite32 (op,
					  src->pixman_image,
					  mask->pixman_image,
					  dst->pixman_image,
					  x1 + src_x,
					  y1 + src_y,
					  x1 + mask_x,
					  y1 + mask_y,
					  x1 + dst_x,
					  y1 + dst_y,
					  x2 - x1,
					  y2 - y1);
	    } else {
		pixman_image_composite32 (op,
					  src->pixman_image,
					  NULL,
					  dst->pixman_image,
					  x1 + src_x,
					  y1 + src_y,
					  0,
					  0,
					  x1 + dst_x,
					  y1 + dst_y,
					  x2 - x1,
					  y2 - y1);
	    }
	}
    }

    return COMAC_STATUS_SUCCESS;
}

const comac_compositor_t *
_comac_image_mask_compositor_get (void)
{
    static comac_atomic_once_t once = COMAC_ATOMIC_ONCE_INIT;
    static comac_mask_compositor_t compositor;

    if (_comac_atomic_init_once_enter (&once)) {
	_comac_mask_compositor_init (&compositor,
				     _comac_image_traps_compositor_get ());
	compositor.acquire = acquire;
	compositor.release = release;
	compositor.set_clip_region = set_clip_region;
	compositor.pattern_to_surface = _comac_pixman_source_create_for_pattern;
	compositor.has_snapshot = has_snapshot;
	compositor.draw_image = draw_image;
	compositor.fill_rectangles = fill_rectangles;
	compositor.fill_boxes = fill_boxes;
#error check_composite must never be NULL, because it gets called without a NULL pointer check
	//compositor.check_composite = check_composite;
	compositor.composite = composite;
	//compositor.check_composite_boxes = check_composite_boxes;
	compositor.composite_boxes = composite_boxes;

	_comac_atomic_init_once_leave (&once);
    }

    return &compositor.base;
}
