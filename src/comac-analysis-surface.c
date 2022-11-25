/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2007 Adrian Johnson
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
 * The Initial Developer of the Original Code is Keith Packard
 *
 * Contributor(s):
 *      Keith Packard <keithp@keithp.com>
 *      Adrian Johnson <ajohnson@redneon.com>
 */

#include "comacint.h"

#include "comac-analysis-surface-private.h"
#include "comac-box-inline.h"
#include "comac-default-context-private.h"
#include "comac-error-private.h"
#include "comac-paginated-private.h"
#include "comac-recording-surface-inline.h"
#include "comac-surface-snapshot-inline.h"
#include "comac-surface-subsurface-inline.h"
#include "comac-region-private.h"

typedef struct {
    comac_surface_t base;

    comac_surface_t *target;

    comac_bool_t first_op;
    comac_bool_t has_supported;
    comac_bool_t has_unsupported;

    comac_region_t supported_region;
    comac_region_t fallback_region;
    comac_box_t page_bbox;

    comac_bool_t has_ctm;
    comac_matrix_t ctm;

} comac_analysis_surface_t;

comac_int_status_t
_comac_analysis_surface_merge_status (comac_int_status_t status_a,
				      comac_int_status_t status_b)
{
    /* fatal errors should be checked and propagated at source */
    assert (! _comac_int_status_is_error (status_a));
    assert (! _comac_int_status_is_error (status_b));

    /* return the most important status */
    if (status_a == COMAC_INT_STATUS_UNSUPPORTED ||
	status_b == COMAC_INT_STATUS_UNSUPPORTED)
	return COMAC_INT_STATUS_UNSUPPORTED;

    if (status_a == COMAC_INT_STATUS_IMAGE_FALLBACK ||
	status_b == COMAC_INT_STATUS_IMAGE_FALLBACK)
	return COMAC_INT_STATUS_IMAGE_FALLBACK;

    if (status_a == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN ||
	status_b == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN)
	return COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN;

    if (status_a == COMAC_INT_STATUS_FLATTEN_TRANSPARENCY ||
	status_b == COMAC_INT_STATUS_FLATTEN_TRANSPARENCY)
	return COMAC_INT_STATUS_FLATTEN_TRANSPARENCY;

    /* at this point we have checked all the valid internal codes, so... */
    assert (status_a == COMAC_INT_STATUS_SUCCESS &&
	    status_b == COMAC_INT_STATUS_SUCCESS);

    return COMAC_INT_STATUS_SUCCESS;
}

struct proxy {
    comac_surface_t base;
    comac_surface_t *target;
};

static comac_status_t
proxy_finish (void *abstract_surface)
{
    return COMAC_STATUS_SUCCESS;
}

static const comac_surface_backend_t proxy_backend = {
    COMAC_INTERNAL_SURFACE_TYPE_NULL,
    proxy_finish,
};

static comac_surface_t *
attach_proxy (comac_surface_t *source, comac_surface_t *target)
{
    struct proxy *proxy;

    proxy = _comac_malloc (sizeof (*proxy));
    if (unlikely (proxy == NULL))
	return _comac_surface_create_in_error (COMAC_STATUS_NO_MEMORY);

    _comac_surface_init (&proxy->base,
			 &proxy_backend,
			 NULL,
			 target->content,
			 target->is_vector,
			 target->colorspace);

    proxy->target = target;
    _comac_surface_attach_snapshot (source, &proxy->base, NULL);

    return &proxy->base;
}

static void
detach_proxy (comac_surface_t *proxy)
{
    comac_surface_finish (proxy);
    comac_surface_destroy (proxy);
}

static comac_int_status_t
_add_operation (comac_analysis_surface_t *surface,
		comac_rectangle_int_t *rect,
		comac_int_status_t backend_status)
{
    comac_int_status_t status;
    comac_box_t bbox;

    if (rect->width == 0 || rect->height == 0) {
	/* Even though the operation is not visible we must be careful
	 * to not allow unsupported operations to be replayed to the
	 * backend during COMAC_PAGINATED_MODE_RENDER */
	if (backend_status == COMAC_INT_STATUS_SUCCESS ||
	    backend_status == COMAC_INT_STATUS_FLATTEN_TRANSPARENCY ||
	    backend_status == COMAC_INT_STATUS_NOTHING_TO_DO) {
	    return COMAC_INT_STATUS_SUCCESS;
	} else {
	    return COMAC_INT_STATUS_IMAGE_FALLBACK;
	}
    }

    _comac_box_from_rectangle (&bbox, rect);

    if (surface->has_ctm) {
	int tx, ty;

	if (_comac_matrix_is_integer_translation (&surface->ctm, &tx, &ty)) {
	    rect->x += tx;
	    rect->y += ty;

	    tx = _comac_fixed_from_int (tx);
	    bbox.p1.x += tx;
	    bbox.p2.x += tx;

	    ty = _comac_fixed_from_int (ty);
	    bbox.p1.y += ty;
	    bbox.p2.y += ty;
	} else {
	    _comac_matrix_transform_bounding_box_fixed (&surface->ctm,
							&bbox,
							NULL);

	    if (bbox.p1.x == bbox.p2.x || bbox.p1.y == bbox.p2.y) {
		/* Even though the operation is not visible we must be
		 * careful to not allow unsupported operations to be
		 * replayed to the backend during
		 * COMAC_PAGINATED_MODE_RENDER */
		if (backend_status == COMAC_INT_STATUS_SUCCESS ||
		    backend_status == COMAC_INT_STATUS_FLATTEN_TRANSPARENCY ||
		    backend_status == COMAC_INT_STATUS_NOTHING_TO_DO) {
		    return COMAC_INT_STATUS_SUCCESS;
		} else {
		    return COMAC_INT_STATUS_IMAGE_FALLBACK;
		}
	    }

	    _comac_box_round_to_rectangle (&bbox, rect);
	}
    }

    if (surface->first_op) {
	surface->first_op = FALSE;
	surface->page_bbox = bbox;
    } else
	_comac_box_add_box (&surface->page_bbox, &bbox);

    /* If the operation is completely enclosed within the fallback
     * region there is no benefit in emitting a native operation as
     * the fallback image will be painted on top.
     */
    if (comac_region_contains_rectangle (&surface->fallback_region, rect) ==
	COMAC_REGION_OVERLAP_IN)
	return COMAC_INT_STATUS_IMAGE_FALLBACK;

    if (backend_status == COMAC_INT_STATUS_FLATTEN_TRANSPARENCY) {
	/* A status of COMAC_INT_STATUS_FLATTEN_TRANSPARENCY indicates
	 * that the backend only supports this operation if the
	 * transparency removed. If the extents of this operation does
	 * not intersect any other native operation, the operation is
	 * natively supported and the backend will blend the
	 * transparency into the white background.
	 */
	if (comac_region_contains_rectangle (&surface->supported_region,
					     rect) == COMAC_REGION_OVERLAP_OUT)
	    backend_status = COMAC_INT_STATUS_SUCCESS;
    }

    if (backend_status == COMAC_INT_STATUS_SUCCESS) {
	/* Add the operation to the supported region. Operations in
	 * this region will be emitted as native operations.
	 */
	surface->has_supported = TRUE;
	return comac_region_union_rectangle (&surface->supported_region, rect);
    }

    /* Add the operation to the unsupported region. This region will
     * be painted as an image after all native operations have been
     * emitted.
     */
    surface->has_unsupported = TRUE;
    status = comac_region_union_rectangle (&surface->fallback_region, rect);

    /* The status COMAC_INT_STATUS_IMAGE_FALLBACK is used to indicate
     * unsupported operations to the recording surface as using
     * COMAC_INT_STATUS_UNSUPPORTED would cause comac-surface to
     * invoke the comac-surface-fallback path then return
     * COMAC_STATUS_SUCCESS.
     */
    if (status == COMAC_INT_STATUS_SUCCESS)
	return COMAC_INT_STATUS_IMAGE_FALLBACK;
    else
	return status;
}

static comac_int_status_t
_analyze_recording_surface_pattern (comac_analysis_surface_t *surface,
				    const comac_pattern_t *pattern,
				    comac_rectangle_int_t *extents)
{
    const comac_surface_pattern_t *surface_pattern;
    comac_analysis_surface_t *tmp;
    comac_surface_t *source, *proxy;
    comac_matrix_t p2d;
    comac_int_status_t status;
    comac_int_status_t analysis_status = COMAC_INT_STATUS_SUCCESS;
    comac_bool_t surface_is_unbounded;
    comac_bool_t unused;

    assert (pattern->type == COMAC_PATTERN_TYPE_SURFACE);
    surface_pattern = (const comac_surface_pattern_t *) pattern;
    assert (surface_pattern->surface->type == COMAC_SURFACE_TYPE_RECORDING);
    source = surface_pattern->surface;

    proxy = _comac_surface_has_snapshot (source, &proxy_backend);
    if (proxy != NULL) {
	/* nothing untoward found so far */
	return COMAC_STATUS_SUCCESS;
    }

    tmp = (comac_analysis_surface_t *) _comac_analysis_surface_create (
	surface->target);
    if (unlikely (tmp->base.status)) {
	status = tmp->base.status;
	goto cleanup1;
    }
    proxy = attach_proxy (source, &tmp->base);

    p2d = pattern->matrix;
    status = comac_matrix_invert (&p2d);
    assert (status == COMAC_INT_STATUS_SUCCESS);
    _comac_analysis_surface_set_ctm (&tmp->base, &p2d);

    source = _comac_surface_get_source (source, NULL);
    surface_is_unbounded = (pattern->extend == COMAC_EXTEND_REPEAT ||
			    pattern->extend == COMAC_EXTEND_REFLECT);
    status = _comac_recording_surface_replay_and_create_regions (
	source,
	&pattern->matrix,
	&tmp->base,
	surface_is_unbounded);
    if (unlikely (status))
	goto cleanup2;

    /* black background or mime data fills entire extents */
    if (! (source->content & COMAC_CONTENT_ALPHA) ||
	_comac_surface_has_mime_image (source)) {
	comac_rectangle_int_t rect;

	if (_comac_surface_get_extents (source, &rect)) {
	    comac_box_t bbox;

	    _comac_box_from_rectangle (&bbox, &rect);
	    _comac_matrix_transform_bounding_box_fixed (&p2d, &bbox, NULL);
	    _comac_box_round_to_rectangle (&bbox, &rect);
	    status = _add_operation (tmp, &rect, COMAC_INT_STATUS_SUCCESS);
	    if (status == COMAC_INT_STATUS_IMAGE_FALLBACK)
		status = COMAC_INT_STATUS_SUCCESS;
	    if (unlikely (status))
		goto cleanup2;
	}
    }

    if (tmp->has_supported) {
	surface->has_supported = TRUE;
	unused = comac_region_union (&surface->supported_region,
				     &tmp->supported_region);
    }

    if (tmp->has_unsupported) {
	surface->has_unsupported = TRUE;
	unused = comac_region_union (&surface->fallback_region,
				     &tmp->fallback_region);
    }

    analysis_status = tmp->has_unsupported ? COMAC_INT_STATUS_IMAGE_FALLBACK
					   : COMAC_INT_STATUS_SUCCESS;
    if (pattern->extend != COMAC_EXTEND_NONE) {
	_comac_unbounded_rectangle_init (extents);
    } else {
	status = comac_matrix_invert (&tmp->ctm);
	_comac_matrix_transform_bounding_box_fixed (&tmp->ctm,
						    &tmp->page_bbox,
						    NULL);
	_comac_box_round_to_rectangle (&tmp->page_bbox, extents);
    }

cleanup2:
    detach_proxy (proxy);
cleanup1:
    comac_surface_destroy (&tmp->base);

    if (unlikely (status))
	return status;
    else
	return analysis_status;
}

static comac_status_t
_comac_analysis_surface_finish (void *abstract_surface)
{
    comac_analysis_surface_t *surface =
	(comac_analysis_surface_t *) abstract_surface;

    _comac_region_fini (&surface->supported_region);
    _comac_region_fini (&surface->fallback_region);

    comac_surface_destroy (surface->target);

    return COMAC_STATUS_SUCCESS;
}

static comac_bool_t
_comac_analysis_surface_get_extents (void *abstract_surface,
				     comac_rectangle_int_t *rectangle)
{
    comac_analysis_surface_t *surface = abstract_surface;

    return _comac_surface_get_extents (surface->target, rectangle);
}

static void
_rectangle_intersect_clip (comac_rectangle_int_t *extents,
			   const comac_clip_t *clip)
{
    if (clip != NULL)
	_comac_rectangle_intersect (extents, _comac_clip_get_extents (clip));
}

static void
_comac_analysis_surface_operation_extents (comac_analysis_surface_t *surface,
					   comac_operator_t op,
					   const comac_pattern_t *source,
					   const comac_clip_t *clip,
					   comac_rectangle_int_t *extents)
{
    comac_bool_t is_empty;

    is_empty = _comac_surface_get_extents (&surface->base, extents);

    if (_comac_operator_bounded_by_source (op)) {
	comac_rectangle_int_t source_extents;

	_comac_pattern_get_extents (source,
				    &source_extents,
				    surface->target->is_vector);
	_comac_rectangle_intersect (extents, &source_extents);
    }

    _rectangle_intersect_clip (extents, clip);
}

static comac_int_status_t
_comac_analysis_surface_paint (void *abstract_surface,
			       comac_operator_t op,
			       const comac_pattern_t *source,
			       const comac_clip_t *clip)
{
    comac_analysis_surface_t *surface = abstract_surface;
    comac_int_status_t backend_status;
    comac_rectangle_int_t extents;

    if (surface->target->backend->paint == NULL) {
	backend_status = COMAC_INT_STATUS_UNSUPPORTED;
    } else {
	backend_status =
	    surface->target->backend->paint (surface->target, op, source, clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    }

    _comac_analysis_surface_operation_extents (surface,
					       op,
					       source,
					       clip,
					       &extents);
    if (backend_status == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN) {
	comac_rectangle_int_t rec_extents;
	backend_status =
	    _analyze_recording_surface_pattern (surface, source, &rec_extents);
	_comac_rectangle_intersect (&extents, &rec_extents);
    }

    return _add_operation (surface, &extents, backend_status);
}

static comac_int_status_t
_comac_analysis_surface_mask (void *abstract_surface,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_pattern_t *mask,
			      const comac_clip_t *clip)
{
    comac_analysis_surface_t *surface = abstract_surface;
    comac_int_status_t backend_status;
    comac_rectangle_int_t extents;

    if (surface->target->backend->mask == NULL) {
	backend_status = COMAC_INT_STATUS_UNSUPPORTED;
    } else {
	backend_status = surface->target->backend->mask (surface->target,
							 op,
							 source,
							 mask,
							 clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    }

    _comac_analysis_surface_operation_extents (surface,
					       op,
					       source,
					       clip,
					       &extents);
    if (backend_status == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN) {
	comac_int_status_t backend_source_status = COMAC_STATUS_SUCCESS;
	comac_int_status_t backend_mask_status = COMAC_STATUS_SUCCESS;
	comac_rectangle_int_t rec_extents;

	if (source->type == COMAC_PATTERN_TYPE_SURFACE) {
	    comac_surface_t *src_surface =
		((comac_surface_pattern_t *) source)->surface;
	    src_surface = _comac_surface_get_source (src_surface, NULL);
	    if (_comac_surface_is_recording (src_surface)) {
		backend_source_status =
		    _analyze_recording_surface_pattern (surface,
							source,
							&rec_extents);
		if (_comac_int_status_is_error (backend_source_status))
		    return backend_source_status;

		_comac_rectangle_intersect (&extents, &rec_extents);
	    }
	}

	if (mask->type == COMAC_PATTERN_TYPE_SURFACE) {
	    comac_surface_t *mask_surface =
		((comac_surface_pattern_t *) mask)->surface;
	    mask_surface = _comac_surface_get_source (mask_surface, NULL);
	    if (_comac_surface_is_recording (mask_surface)) {
		backend_mask_status =
		    _analyze_recording_surface_pattern (surface,
							mask,
							&rec_extents);
		if (_comac_int_status_is_error (backend_mask_status))
		    return backend_mask_status;

		_comac_rectangle_intersect (&extents, &rec_extents);
	    }
	}

	backend_status =
	    _comac_analysis_surface_merge_status (backend_source_status,
						  backend_mask_status);
    }

    if (_comac_operator_bounded_by_mask (op)) {
	comac_rectangle_int_t mask_extents;

	_comac_pattern_get_extents (mask,
				    &mask_extents,
				    surface->target->is_vector);
	_comac_rectangle_intersect (&extents, &mask_extents);
    }

    return _add_operation (surface, &extents, backend_status);
}

static comac_int_status_t
_comac_analysis_surface_stroke (void *abstract_surface,
				comac_operator_t op,
				const comac_pattern_t *source,
				const comac_path_fixed_t *path,
				const comac_stroke_style_t *style,
				const comac_matrix_t *ctm,
				const comac_matrix_t *ctm_inverse,
				double tolerance,
				comac_antialias_t antialias,
				const comac_clip_t *clip)
{
    comac_analysis_surface_t *surface = abstract_surface;
    comac_int_status_t backend_status;
    comac_rectangle_int_t extents;

    if (surface->target->backend->stroke == NULL) {
	backend_status = COMAC_INT_STATUS_UNSUPPORTED;
    } else {
	backend_status = surface->target->backend->stroke (surface->target,
							   op,
							   source,
							   path,
							   style,
							   ctm,
							   ctm_inverse,
							   tolerance,
							   antialias,
							   clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    }

    _comac_analysis_surface_operation_extents (surface,
					       op,
					       source,
					       clip,
					       &extents);
    if (backend_status == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN) {
	comac_rectangle_int_t rec_extents;
	backend_status =
	    _analyze_recording_surface_pattern (surface, source, &rec_extents);
	_comac_rectangle_intersect (&extents, &rec_extents);
    }

    if (_comac_operator_bounded_by_mask (op)) {
	comac_rectangle_int_t mask_extents;
	comac_int_status_t status;

	status = _comac_path_fixed_stroke_extents (path,
						   style,
						   ctm,
						   ctm_inverse,
						   tolerance,
						   &mask_extents);
	if (unlikely (status))
	    return status;

	_comac_rectangle_intersect (&extents, &mask_extents);
    }

    return _add_operation (surface, &extents, backend_status);
}

static comac_int_status_t
_comac_analysis_surface_fill (void *abstract_surface,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_path_fixed_t *path,
			      comac_fill_rule_t fill_rule,
			      double tolerance,
			      comac_antialias_t antialias,
			      const comac_clip_t *clip)
{
    comac_analysis_surface_t *surface = abstract_surface;
    comac_int_status_t backend_status;
    comac_rectangle_int_t extents;

    if (surface->target->backend->fill == NULL) {
	backend_status = COMAC_INT_STATUS_UNSUPPORTED;
    } else {
	backend_status = surface->target->backend->fill (surface->target,
							 op,
							 source,
							 path,
							 fill_rule,
							 tolerance,
							 antialias,
							 clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    }

    _comac_analysis_surface_operation_extents (surface,
					       op,
					       source,
					       clip,
					       &extents);
    if (backend_status == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN) {
	comac_rectangle_int_t rec_extents;
	backend_status =
	    _analyze_recording_surface_pattern (surface, source, &rec_extents);
	_comac_rectangle_intersect (&extents, &rec_extents);
    }

    if (_comac_operator_bounded_by_mask (op)) {
	comac_rectangle_int_t mask_extents;

	_comac_path_fixed_fill_extents (path,
					fill_rule,
					tolerance,
					&mask_extents);

	_comac_rectangle_intersect (&extents, &mask_extents);
    }

    return _add_operation (surface, &extents, backend_status);
}

static comac_int_status_t
_comac_analysis_surface_show_glyphs (void *abstract_surface,
				     comac_operator_t op,
				     const comac_pattern_t *source,
				     comac_glyph_t *glyphs,
				     int num_glyphs,
				     comac_scaled_font_t *scaled_font,
				     const comac_clip_t *clip)
{
    comac_analysis_surface_t *surface = abstract_surface;
    comac_int_status_t status, backend_status;
    comac_rectangle_int_t extents, glyph_extents;

    /* Adapted from _comac_surface_show_glyphs */
    if (surface->target->backend->show_glyphs != NULL) {
	backend_status = surface->target->backend->show_glyphs (surface->target,
								op,
								source,
								glyphs,
								num_glyphs,
								scaled_font,
								clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    } else if (surface->target->backend->show_text_glyphs != NULL) {
	backend_status =
	    surface->target->backend->show_text_glyphs (surface->target,
							op,
							source,
							NULL,
							0,
							glyphs,
							num_glyphs,
							NULL,
							0,
							FALSE,
							scaled_font,
							clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    } else {
	backend_status = COMAC_INT_STATUS_UNSUPPORTED;
    }

    _comac_analysis_surface_operation_extents (surface,
					       op,
					       source,
					       clip,
					       &extents);
    if (backend_status == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN) {
	comac_rectangle_int_t rec_extents;
	backend_status =
	    _analyze_recording_surface_pattern (surface, source, &rec_extents);
	_comac_rectangle_intersect (&extents, &rec_extents);
    }

    if (_comac_operator_bounded_by_mask (op)) {
	status = _comac_scaled_font_glyph_device_extents (scaled_font,
							  glyphs,
							  num_glyphs,
							  &glyph_extents,
							  NULL);
	if (unlikely (status))
	    return status;

	_comac_rectangle_intersect (&extents, &glyph_extents);
    }

    return _add_operation (surface, &extents, backend_status);
}

static comac_bool_t
_comac_analysis_surface_has_show_text_glyphs (void *abstract_surface)
{
    comac_analysis_surface_t *surface = abstract_surface;

    return comac_surface_has_show_text_glyphs (surface->target);
}

static comac_int_status_t
_comac_analysis_surface_show_text_glyphs (
    void *abstract_surface,
    comac_operator_t op,
    const comac_pattern_t *source,
    const char *utf8,
    int utf8_len,
    comac_glyph_t *glyphs,
    int num_glyphs,
    const comac_text_cluster_t *clusters,
    int num_clusters,
    comac_text_cluster_flags_t cluster_flags,
    comac_scaled_font_t *scaled_font,
    const comac_clip_t *clip)
{
    comac_analysis_surface_t *surface = abstract_surface;
    comac_int_status_t status, backend_status;
    comac_rectangle_int_t extents, glyph_extents;

    /* Adapted from _comac_surface_show_glyphs */
    backend_status = COMAC_INT_STATUS_UNSUPPORTED;
    if (surface->target->backend->show_text_glyphs != NULL) {
	backend_status =
	    surface->target->backend->show_text_glyphs (surface->target,
							op,
							source,
							utf8,
							utf8_len,
							glyphs,
							num_glyphs,
							clusters,
							num_clusters,
							cluster_flags,
							scaled_font,
							clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    }
    if (backend_status == COMAC_INT_STATUS_UNSUPPORTED &&
	surface->target->backend->show_glyphs != NULL) {
	backend_status = surface->target->backend->show_glyphs (surface->target,
								op,
								source,
								glyphs,
								num_glyphs,
								scaled_font,
								clip);
	if (_comac_int_status_is_error (backend_status))
	    return backend_status;
    }

    _comac_analysis_surface_operation_extents (surface,
					       op,
					       source,
					       clip,
					       &extents);
    if (backend_status == COMAC_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN) {
	comac_rectangle_int_t rec_extents;
	backend_status =
	    _analyze_recording_surface_pattern (surface, source, &rec_extents);
	_comac_rectangle_intersect (&extents, &rec_extents);
    }

    if (_comac_operator_bounded_by_mask (op)) {
	status = _comac_scaled_font_glyph_device_extents (scaled_font,
							  glyphs,
							  num_glyphs,
							  &glyph_extents,
							  NULL);
	if (unlikely (status))
	    return status;

	_comac_rectangle_intersect (&extents, &glyph_extents);
    }

    return _add_operation (surface, &extents, backend_status);
}

static comac_int_status_t
_comac_analysis_surface_tag (void *abstract_surface,
			     comac_bool_t begin,
			     const char *tag_name,
			     const char *attributes)
{
    comac_analysis_surface_t *surface = abstract_surface;
    comac_int_status_t backend_status;

    backend_status = COMAC_INT_STATUS_SUCCESS;
    if (surface->target->backend->tag != NULL) {
	backend_status = surface->target->backend->tag (surface->target,
							begin,
							tag_name,
							attributes);
	if (backend_status == COMAC_INT_STATUS_SUCCESS)
	    surface->has_supported = TRUE;
    }

    return backend_status;
}

static const comac_surface_backend_t comac_analysis_surface_backend = {
    COMAC_INTERNAL_SURFACE_TYPE_ANALYSIS,

    _comac_analysis_surface_finish,
    NULL,

    NULL, /* create_similar */
    NULL, /* create_similar_image */
    NULL, /* map_to_image */
    NULL, /* unmap */

    NULL, /* source */
    NULL, /* acquire_source_image */
    NULL, /* release_source_image */
    NULL, /* snapshot */

    NULL, /* copy_page */
    NULL, /* show_page */

    _comac_analysis_surface_get_extents,
    NULL, /* get_font_options */

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    _comac_analysis_surface_paint,
    _comac_analysis_surface_mask,
    _comac_analysis_surface_stroke,
    _comac_analysis_surface_fill,
    NULL, /* fill_stroke */
    _comac_analysis_surface_show_glyphs,
    _comac_analysis_surface_has_show_text_glyphs,
    _comac_analysis_surface_show_text_glyphs,
    NULL, /* get_supported_mime_types */
    _comac_analysis_surface_tag};

comac_surface_t *
_comac_analysis_surface_create (comac_surface_t *target)
{
    comac_analysis_surface_t *surface;
    comac_status_t status;

    status = target->status;
    if (unlikely (status))
	return _comac_surface_create_in_error (status);

    surface = _comac_malloc (sizeof (comac_analysis_surface_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    /* I believe the content type here is truly arbitrary. I'm quite
     * sure nothing will ever use this value. */
    _comac_surface_init (&surface->base,
			 &comac_analysis_surface_backend,
			 NULL, /* device */
			 COMAC_CONTENT_COLOR_ALPHA,
			 target->is_vector,
			 target->colorspace);

    comac_matrix_init_identity (&surface->ctm);
    surface->has_ctm = FALSE;

    surface->target = comac_surface_reference (target);
    surface->first_op = TRUE;
    surface->has_supported = FALSE;
    surface->has_unsupported = FALSE;

    _comac_region_init (&surface->supported_region);
    _comac_region_init (&surface->fallback_region);

    surface->page_bbox.p1.x = 0;
    surface->page_bbox.p1.y = 0;
    surface->page_bbox.p2.x = 0;
    surface->page_bbox.p2.y = 0;

    return &surface->base;
}

void
_comac_analysis_surface_set_ctm (comac_surface_t *abstract_surface,
				 const comac_matrix_t *ctm)
{
    comac_analysis_surface_t *surface;

    if (abstract_surface->status)
	return;

    surface = (comac_analysis_surface_t *) abstract_surface;

    surface->ctm = *ctm;
    surface->has_ctm = ! _comac_matrix_is_identity (&surface->ctm);
}

void
_comac_analysis_surface_get_ctm (comac_surface_t *abstract_surface,
				 comac_matrix_t *ctm)
{
    comac_analysis_surface_t *surface =
	(comac_analysis_surface_t *) abstract_surface;

    *ctm = surface->ctm;
}

comac_region_t *
_comac_analysis_surface_get_supported (comac_surface_t *abstract_surface)
{
    comac_analysis_surface_t *surface =
	(comac_analysis_surface_t *) abstract_surface;

    return &surface->supported_region;
}

comac_region_t *
_comac_analysis_surface_get_unsupported (comac_surface_t *abstract_surface)
{
    comac_analysis_surface_t *surface =
	(comac_analysis_surface_t *) abstract_surface;

    return &surface->fallback_region;
}

comac_bool_t
_comac_analysis_surface_has_supported (comac_surface_t *abstract_surface)
{
    comac_analysis_surface_t *surface =
	(comac_analysis_surface_t *) abstract_surface;

    return surface->has_supported;
}

comac_bool_t
_comac_analysis_surface_has_unsupported (comac_surface_t *abstract_surface)
{
    comac_analysis_surface_t *surface =
	(comac_analysis_surface_t *) abstract_surface;

    return surface->has_unsupported;
}

void
_comac_analysis_surface_get_bounding_box (comac_surface_t *abstract_surface,
					  comac_box_t *bbox)
{
    comac_analysis_surface_t *surface =
	(comac_analysis_surface_t *) abstract_surface;

    *bbox = surface->page_bbox;
}

/* null surface type: a surface that does nothing (has no side effects, yay!) */

static comac_int_status_t
_paint_return_success (void *surface,
		       comac_operator_t op,
		       const comac_pattern_t *source,
		       const comac_clip_t *clip)
{
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_mask_return_success (void *surface,
		      comac_operator_t op,
		      const comac_pattern_t *source,
		      const comac_pattern_t *mask,
		      const comac_clip_t *clip)
{
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_stroke_return_success (void *surface,
			comac_operator_t op,
			const comac_pattern_t *source,
			const comac_path_fixed_t *path,
			const comac_stroke_style_t *style,
			const comac_matrix_t *ctm,
			const comac_matrix_t *ctm_inverse,
			double tolerance,
			comac_antialias_t antialias,
			const comac_clip_t *clip)
{
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_fill_return_success (void *surface,
		      comac_operator_t op,
		      const comac_pattern_t *source,
		      const comac_path_fixed_t *path,
		      comac_fill_rule_t fill_rule,
		      double tolerance,
		      comac_antialias_t antialias,
		      const comac_clip_t *clip)
{
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_show_glyphs_return_success (void *surface,
			     comac_operator_t op,
			     const comac_pattern_t *source,
			     comac_glyph_t *glyphs,
			     int num_glyphs,
			     comac_scaled_font_t *scaled_font,
			     const comac_clip_t *clip)
{
    return COMAC_INT_STATUS_SUCCESS;
}

static const comac_surface_backend_t comac_null_surface_backend = {
    COMAC_INTERNAL_SURFACE_TYPE_NULL,
    NULL, /* finish */

    NULL, /* only accessed through the surface functions */

    NULL, /* create_similar */
    NULL, /* create similar image */
    NULL, /* map to image */
    NULL, /* unmap image*/

    NULL, /* source */
    NULL, /* acquire_source_image */
    NULL, /* release_source_image */
    NULL, /* snapshot */

    NULL, /* copy_page */
    NULL, /* show_page */

    NULL, /* get_extents */
    NULL, /* get_font_options */

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    _paint_return_success,	 /* paint */
    _mask_return_success,	 /* mask */
    _stroke_return_success,	 /* stroke */
    _fill_return_success,	 /* fill */
    NULL,			 /* fill_stroke */
    _show_glyphs_return_success, /* show_glyphs */
    NULL,			 /* has_show_text_glyphs */
    NULL			 /* show_text_glyphs */
};

comac_surface_t *
_comac_null_surface_create (comac_content_t content)
{
    comac_surface_t *surface;

    surface = _comac_malloc (sizeof (comac_surface_t));
    if (unlikely (surface == NULL)) {
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));
    }

    _comac_surface_init (surface,
			 &comac_null_surface_backend,
			 NULL, /* device */
			 content,
			 TRUE,
			 COMAC_COLORSPACE_RGB); /* is_vector */

    return surface;
}
