/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2011 Intel Corporation
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
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_SPANS_COMPOSITOR_PRIVATE_H
#define COMAC_SPANS_COMPOSITOR_PRIVATE_H

#include "comac-compositor-private.h"
#include "comac-types-private.h"
#include "comac-spans-private.h"

COMAC_BEGIN_DECLS

typedef struct _comac_abstract_span_renderer {
    comac_span_renderer_t base;
    char data[4096];
} comac_abstract_span_renderer_t;

struct comac_spans_compositor {
    comac_compositor_t base;

    unsigned int flags;
#define COMAC_SPANS_COMPOSITOR_HAS_LERP 0x1

    /* pixel-aligned fast paths */
    comac_int_status_t (*fill_boxes)	(void			*surface,
					 comac_operator_t	 op,
					 const comac_color_t	*color,
					 comac_boxes_t		*boxes);

    comac_int_status_t (*draw_image_boxes) (void *surface,
					    comac_image_surface_t *image,
					    comac_boxes_t *boxes,
					    int dx, int dy);

    comac_int_status_t (*copy_boxes) (void *surface,
				      comac_surface_t *src,
				      comac_boxes_t *boxes,
				      const comac_rectangle_int_t *extents,
				      int dx, int dy);

    comac_surface_t * (*pattern_to_surface) (comac_surface_t *dst,
					     const comac_pattern_t *pattern,
					     comac_bool_t is_mask,
					     const comac_rectangle_int_t *extents,
					     const comac_rectangle_int_t *sample,
					     int *src_x, int *src_y);

    comac_int_status_t (*composite_boxes) (void			*surface,
					   comac_operator_t	 op,
					   comac_surface_t	*source,
					   comac_surface_t	*mask,
					   int			 src_x,
					   int			 src_y,
					   int			 mask_x,
					   int			 mask_y,
					   int			 dst_x,
					   int			 dst_y,
					   comac_boxes_t		*boxes,
					   const comac_rectangle_int_t  *extents);

    /* general shape masks using a span renderer */
    comac_int_status_t (*renderer_init) (comac_abstract_span_renderer_t *renderer,
					 const comac_composite_rectangles_t *extents,
					 comac_antialias_t antialias,
					 comac_bool_t	 needs_clip);

    void (*renderer_fini) (comac_abstract_span_renderer_t *renderer,
			   comac_int_status_t status);
};

comac_private void
_comac_spans_compositor_init (comac_spans_compositor_t *compositor,
			      const comac_compositor_t  *delegate);

COMAC_END_DECLS

#endif /* COMAC_SPANS_COMPOSITOR_PRIVATE_H */
