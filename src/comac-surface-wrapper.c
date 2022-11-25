/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc
 * Copyright © 2007 Adrian Johnson
 * Copyright © 2009 Chris Wilson
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-clip-inline.h"
#include "comac-error-private.h"
#include "comac-pattern-private.h"
#include "comac-surface-wrapper-private.h"

/* A collection of routines to facilitate surface wrapping */

static void
_copy_transformed_pattern (comac_pattern_t *pattern,
			   const comac_pattern_t *original,
			   const comac_matrix_t  *ctm_inverse)
{
    _comac_pattern_init_static_copy (pattern, original);

    if (! _comac_matrix_is_identity (ctm_inverse))
	_comac_pattern_transform (pattern, ctm_inverse);
}

comac_status_t
_comac_surface_wrapper_acquire_source_image (comac_surface_wrapper_t *wrapper,
					     comac_image_surface_t  **image_out,
					     void                   **image_extra)
{
    if (unlikely (wrapper->target->status))
	return wrapper->target->status;

    return _comac_surface_acquire_source_image (wrapper->target,
						image_out, image_extra);
}

void
_comac_surface_wrapper_release_source_image (comac_surface_wrapper_t *wrapper,
					     comac_image_surface_t  *image,
					     void                   *image_extra)
{
    _comac_surface_release_source_image (wrapper->target, image, image_extra);
}

static void
_comac_surface_wrapper_get_transform (comac_surface_wrapper_t *wrapper,
				      comac_matrix_t *m)
{
    comac_matrix_init_identity (m);

    if (! _comac_matrix_is_identity (&wrapper->transform))
	comac_matrix_multiply (m, &wrapper->transform, m);

    if (! _comac_matrix_is_identity (&wrapper->target->device_transform))
	comac_matrix_multiply (m, &wrapper->target->device_transform, m);
}

static void
_comac_surface_wrapper_get_inverse_transform (comac_surface_wrapper_t *wrapper,
					      comac_matrix_t *m)
{
    comac_matrix_init_identity (m);

    if (! _comac_matrix_is_identity (&wrapper->target->device_transform_inverse))
	comac_matrix_multiply (m, &wrapper->target->device_transform_inverse, m);

    if (! _comac_matrix_is_identity (&wrapper->transform)) {
	comac_matrix_t inv;
	comac_status_t status;

	inv = wrapper->transform;
	status = comac_matrix_invert (&inv);
	assert (status == COMAC_STATUS_SUCCESS);
	comac_matrix_multiply (m, &inv, m);
    }
}

static comac_clip_t *
_comac_surface_wrapper_get_clip (comac_surface_wrapper_t *wrapper,
				 const comac_clip_t *clip)
{
    comac_clip_t *copy;
    comac_matrix_t m;

    copy = _comac_clip_copy (clip);
    if (wrapper->has_extents) {
	copy = _comac_clip_intersect_rectangle (copy, &wrapper->extents);
    }
    _comac_surface_wrapper_get_transform (wrapper, &m);
    copy = _comac_clip_transform (copy, &m);
    if (wrapper->clip)
	copy = _comac_clip_intersect_clip (copy, wrapper->clip);

    return copy;
}

comac_status_t
_comac_surface_wrapper_paint (comac_surface_wrapper_t *wrapper,
			      comac_operator_t	 op,
			      const comac_pattern_t *source,
			      const comac_clip_t    *clip)
{
    comac_status_t status;
    comac_clip_t *dev_clip;
    comac_pattern_union_t source_copy;

    if (unlikely (wrapper->target->status))
	return wrapper->target->status;

    dev_clip = _comac_surface_wrapper_get_clip (wrapper, clip);
    if (_comac_clip_is_all_clipped (dev_clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (source->is_userfont_foreground && wrapper->foreground_source)
        source = wrapper->foreground_source;

    if (wrapper->needs_transform) {
	comac_matrix_t m;

	_comac_surface_wrapper_get_transform (wrapper, &m);

	status = comac_matrix_invert (&m);
	assert (status == COMAC_STATUS_SUCCESS);

	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;
    }

    status = _comac_surface_paint (wrapper->target, op, source, dev_clip);

    _comac_clip_destroy (dev_clip);
    return status;
}


comac_status_t
_comac_surface_wrapper_mask (comac_surface_wrapper_t *wrapper,
			     comac_operator_t	 op,
			     const comac_pattern_t *source,
			     const comac_pattern_t *mask,
			     const comac_clip_t	    *clip)
{
    comac_status_t status;
    comac_clip_t *dev_clip;
    comac_pattern_union_t source_copy;
    comac_pattern_union_t mask_copy;

    if (unlikely (wrapper->target->status))
	return wrapper->target->status;

    dev_clip = _comac_surface_wrapper_get_clip (wrapper, clip);
    if (_comac_clip_is_all_clipped (dev_clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (source->is_userfont_foreground && wrapper->foreground_source)
        source = wrapper->foreground_source;

    if (wrapper->needs_transform) {
	comac_matrix_t m;

	_comac_surface_wrapper_get_transform (wrapper, &m);

	status = comac_matrix_invert (&m);
	assert (status == COMAC_STATUS_SUCCESS);

	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;

	_copy_transformed_pattern (&mask_copy.base, mask, &m);
	mask = &mask_copy.base;
    }

    status = _comac_surface_mask (wrapper->target, op, source, mask, dev_clip);

    _comac_clip_destroy (dev_clip);
    return status;
}

comac_status_t
_comac_surface_wrapper_stroke (comac_surface_wrapper_t *wrapper,
			       comac_operator_t		 op,
			       const comac_pattern_t	*source,
			       const comac_path_fixed_t	*path,
			       const comac_stroke_style_t	*stroke_style,
			       const comac_matrix_t		*ctm,
			       const comac_matrix_t		*ctm_inverse,
			       double			 tolerance,
			       comac_antialias_t	 antialias,
			       const comac_clip_t		*clip)
{
    comac_status_t status;
    comac_path_fixed_t path_copy, *dev_path = (comac_path_fixed_t *) path;
    comac_clip_t *dev_clip;
    comac_matrix_t dev_ctm = *ctm;
    comac_matrix_t dev_ctm_inverse = *ctm_inverse;
    comac_pattern_union_t source_copy;

    if (unlikely (wrapper->target->status))
	return wrapper->target->status;

    dev_clip = _comac_surface_wrapper_get_clip (wrapper, clip);
    if (_comac_clip_is_all_clipped (dev_clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (source->is_userfont_foreground && wrapper->foreground_source)
        source = wrapper->foreground_source;

    if (wrapper->needs_transform) {
	comac_matrix_t m;

	_comac_surface_wrapper_get_transform (wrapper, &m);

	status = _comac_path_fixed_init_copy (&path_copy, dev_path);
	if (unlikely (status))
	    goto FINISH;

	_comac_path_fixed_transform (&path_copy, &m);
	dev_path = &path_copy;

	comac_matrix_multiply (&dev_ctm, &dev_ctm, &m);

	status = comac_matrix_invert (&m);
	assert (status == COMAC_STATUS_SUCCESS);

	comac_matrix_multiply (&dev_ctm_inverse, &m, &dev_ctm_inverse);

	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;
    }

    status = _comac_surface_stroke (wrapper->target, op, source,
				    dev_path, stroke_style,
				    &dev_ctm, &dev_ctm_inverse,
				    tolerance, antialias,
				    dev_clip);

 FINISH:
    if (dev_path != path)
	_comac_path_fixed_fini (dev_path);
    _comac_clip_destroy (dev_clip);
    return status;
}

comac_status_t
_comac_surface_wrapper_fill_stroke (comac_surface_wrapper_t *wrapper,
				    comac_operator_t	     fill_op,
				    const comac_pattern_t   *fill_source,
				    comac_fill_rule_t	     fill_rule,
				    double		     fill_tolerance,
				    comac_antialias_t	     fill_antialias,
				    const comac_path_fixed_t*path,
				    comac_operator_t	     stroke_op,
				    const comac_pattern_t   *stroke_source,
				    const comac_stroke_style_t    *stroke_style,
				    const comac_matrix_t	    *stroke_ctm,
				    const comac_matrix_t	    *stroke_ctm_inverse,
				    double		     stroke_tolerance,
				    comac_antialias_t	     stroke_antialias,
				    const comac_clip_t	    *clip)
{
    comac_status_t status;
    comac_path_fixed_t path_copy, *dev_path = (comac_path_fixed_t *)path;
    comac_matrix_t dev_ctm = *stroke_ctm;
    comac_matrix_t dev_ctm_inverse = *stroke_ctm_inverse;
    comac_clip_t *dev_clip;
    comac_pattern_union_t stroke_source_copy;
    comac_pattern_union_t fill_source_copy;

    if (unlikely (wrapper->target->status))
	return wrapper->target->status;

    dev_clip = _comac_surface_wrapper_get_clip (wrapper, clip);
    if (_comac_clip_is_all_clipped (dev_clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (fill_source->is_userfont_foreground && wrapper->foreground_source)
        fill_source = wrapper->foreground_source;

    if (stroke_source->is_userfont_foreground && wrapper->foreground_source)
        stroke_source = wrapper->foreground_source;

    if (wrapper->needs_transform) {
	comac_matrix_t m;

	_comac_surface_wrapper_get_transform (wrapper, &m);

	status = _comac_path_fixed_init_copy (&path_copy, dev_path);
	if (unlikely (status))
	    goto FINISH;

	_comac_path_fixed_transform (&path_copy, &m);
	dev_path = &path_copy;

	comac_matrix_multiply (&dev_ctm, &dev_ctm, &m);

	status = comac_matrix_invert (&m);
	assert (status == COMAC_STATUS_SUCCESS);

	comac_matrix_multiply (&dev_ctm_inverse, &m, &dev_ctm_inverse);

	_copy_transformed_pattern (&stroke_source_copy.base, stroke_source, &m);
	stroke_source = &stroke_source_copy.base;

	_copy_transformed_pattern (&fill_source_copy.base, fill_source, &m);
	fill_source = &fill_source_copy.base;
    }

    status = _comac_surface_fill_stroke (wrapper->target,
					 fill_op, fill_source, fill_rule,
					 fill_tolerance, fill_antialias,
					 dev_path,
					 stroke_op, stroke_source,
					 stroke_style,
					 &dev_ctm, &dev_ctm_inverse,
					 stroke_tolerance, stroke_antialias,
					 dev_clip);

  FINISH:
    if (dev_path != path)
	_comac_path_fixed_fini (dev_path);
    _comac_clip_destroy (dev_clip);
    return status;
}

comac_status_t
_comac_surface_wrapper_fill (comac_surface_wrapper_t	*wrapper,
			     comac_operator_t	 op,
			     const comac_pattern_t *source,
			     const comac_path_fixed_t	*path,
			     comac_fill_rule_t	 fill_rule,
			     double		 tolerance,
			     comac_antialias_t	 antialias,
			     const comac_clip_t	*clip)
{
    comac_status_t status;
    comac_path_fixed_t path_copy, *dev_path = (comac_path_fixed_t *) path;
    comac_pattern_union_t source_copy;
    comac_clip_t *dev_clip;

    if (unlikely (wrapper->target->status))
	return wrapper->target->status;

    dev_clip = _comac_surface_wrapper_get_clip (wrapper, clip);
    if (_comac_clip_is_all_clipped (dev_clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    if (source->is_userfont_foreground && wrapper->foreground_source)
        source = wrapper->foreground_source;

    if (wrapper->needs_transform) {
	comac_matrix_t m;

	_comac_surface_wrapper_get_transform (wrapper, &m);

	status = _comac_path_fixed_init_copy (&path_copy, dev_path);
	if (unlikely (status))
	    goto FINISH;

	_comac_path_fixed_transform (&path_copy, &m);
	dev_path = &path_copy;

	status = comac_matrix_invert (&m);
	assert (status == COMAC_STATUS_SUCCESS);

	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;
    }

    status = _comac_surface_fill (wrapper->target, op, source,
				  dev_path, fill_rule,
				  tolerance, antialias,
				  dev_clip);

 FINISH:
    if (dev_path != path)
	_comac_path_fixed_fini (dev_path);
    _comac_clip_destroy (dev_clip);
    return status;
}

comac_status_t
_comac_surface_wrapper_show_text_glyphs (comac_surface_wrapper_t *wrapper,
					 comac_operator_t	     op,
					 const comac_pattern_t	    *source,
					 const char		    *utf8,
					 int			     utf8_len,
					 const comac_glyph_t	    *glyphs,
					 int			     num_glyphs,
					 const comac_text_cluster_t *clusters,
					 int			     num_clusters,
					 comac_text_cluster_flags_t  cluster_flags,
					 comac_scaled_font_t	    *scaled_font,
					 const comac_clip_t	    *clip)
{
    comac_status_t status;
    comac_clip_t *dev_clip;
    comac_glyph_t stack_glyphs [COMAC_STACK_ARRAY_LENGTH(comac_glyph_t)];
    comac_glyph_t *dev_glyphs = stack_glyphs;
    comac_scaled_font_t *dev_scaled_font = scaled_font;
    comac_pattern_union_t source_copy;
    comac_font_options_t options;

    if (unlikely (wrapper->target->status))
	return wrapper->target->status;

    dev_clip = _comac_surface_wrapper_get_clip (wrapper, clip);
    if (_comac_clip_is_all_clipped (dev_clip))
	return COMAC_INT_STATUS_NOTHING_TO_DO;

    comac_surface_get_font_options (wrapper->target, &options);
    comac_font_options_merge (&options, &scaled_font->options);

    if (source->is_userfont_foreground && wrapper->foreground_source)
        source = wrapper->foreground_source;

    if (wrapper->needs_transform) {
	comac_matrix_t m;
	int i;

	_comac_surface_wrapper_get_transform (wrapper, &m);

	if (! _comac_matrix_is_translation (&m)) {
	    comac_matrix_t ctm;

	    _comac_matrix_multiply (&ctm,
				    &m,
				    &scaled_font->ctm);
	    dev_scaled_font = comac_scaled_font_create (scaled_font->font_face,
							&scaled_font->font_matrix,
							&ctm, &options);
	}

	if (num_glyphs > ARRAY_LENGTH (stack_glyphs)) {
	    dev_glyphs = _comac_malloc_ab (num_glyphs, sizeof (comac_glyph_t));
	    if (unlikely (dev_glyphs == NULL)) {
		status = _comac_error (COMAC_STATUS_NO_MEMORY);
		goto FINISH;
	    }
	}

	for (i = 0; i < num_glyphs; i++) {
	    dev_glyphs[i] = glyphs[i];
	    comac_matrix_transform_point (&m,
					  &dev_glyphs[i].x,
					  &dev_glyphs[i].y);
	}

	status = comac_matrix_invert (&m);
	assert (status == COMAC_STATUS_SUCCESS);

	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;
    } else {
	if (! comac_font_options_equal (&options, &scaled_font->options)) {
	    dev_scaled_font = comac_scaled_font_create (scaled_font->font_face,
							&scaled_font->font_matrix,
							&scaled_font->ctm,
							&options);
	}

	/* show_text_glyphs is special because _comac_surface_show_text_glyphs is allowed
	 * to modify the glyph array that's passed in.  We must always
	 * copy the array before handing it to the backend.
	 */
	if (num_glyphs > ARRAY_LENGTH (stack_glyphs)) {
	    dev_glyphs = _comac_malloc_ab (num_glyphs, sizeof (comac_glyph_t));
	    if (unlikely (dev_glyphs == NULL)) {
		status = _comac_error (COMAC_STATUS_NO_MEMORY);
		goto FINISH;
	    }
	}

	memcpy (dev_glyphs, glyphs, sizeof (comac_glyph_t) * num_glyphs);
    }

    status = _comac_surface_show_text_glyphs (wrapper->target, op, source,
					      utf8, utf8_len,
					      dev_glyphs, num_glyphs,
					      clusters, num_clusters,
					      cluster_flags,
					      dev_scaled_font,
					      dev_clip);
 FINISH:
    _comac_clip_destroy (dev_clip);
    if (dev_glyphs != stack_glyphs)
	free (dev_glyphs);
    if (dev_scaled_font != scaled_font)
	comac_scaled_font_destroy (dev_scaled_font);
    return status;
}

comac_status_t
_comac_surface_wrapper_tag (comac_surface_wrapper_t     *wrapper,
			    comac_bool_t                 begin,
			    const char                  *tag_name,
			    const char                  *attributes)
{
    if (unlikely (wrapper->target->status))
	return wrapper->target->status;


    return _comac_surface_tag (wrapper->target, begin, tag_name, attributes);
}

comac_surface_t *
_comac_surface_wrapper_create_similar (comac_surface_wrapper_t *wrapper,
				       comac_content_t	content,
				       int		width,
				       int		height)
{
    return _comac_surface_create_scratch (wrapper->target,
					  content, width, height, NULL);
}

comac_bool_t
_comac_surface_wrapper_get_extents (comac_surface_wrapper_t *wrapper,
				    comac_rectangle_int_t   *extents)
{
    if (wrapper->has_extents) {
	if (_comac_surface_get_extents (wrapper->target, extents))
	    _comac_rectangle_intersect (extents, &wrapper->extents);
	else
	    *extents = wrapper->extents;

	return TRUE;
    } else {
	return _comac_surface_get_extents (wrapper->target, extents);
    }
}

static comac_bool_t
_comac_surface_wrapper_needs_device_transform (comac_surface_wrapper_t *wrapper)
{
    return
	(wrapper->has_extents && (wrapper->extents.x | wrapper->extents.y)) ||
	! _comac_matrix_is_identity (&wrapper->transform) ||
	! _comac_matrix_is_identity (&wrapper->target->device_transform);
}

void
_comac_surface_wrapper_intersect_extents (comac_surface_wrapper_t *wrapper,
					  const comac_rectangle_int_t *extents)
{
    if (! wrapper->has_extents) {
	wrapper->extents = *extents;
	wrapper->has_extents = TRUE;
    } else
	_comac_rectangle_intersect (&wrapper->extents, extents);

    wrapper->needs_transform =
	_comac_surface_wrapper_needs_device_transform (wrapper);
}

void
_comac_surface_wrapper_set_inverse_transform (comac_surface_wrapper_t *wrapper,
					      const comac_matrix_t *transform)
{
    comac_status_t status;

    if (transform == NULL || _comac_matrix_is_identity (transform)) {
	comac_matrix_init_identity (&wrapper->transform);

	wrapper->needs_transform =
	    _comac_surface_wrapper_needs_device_transform (wrapper);
    } else {
	wrapper->transform = *transform;
	status = comac_matrix_invert (&wrapper->transform);
	/* should always be invertible unless given pathological input */
	assert (status == COMAC_STATUS_SUCCESS);

	wrapper->needs_transform = TRUE;
    }
}

void
_comac_surface_wrapper_set_clip (comac_surface_wrapper_t *wrapper,
				 const comac_clip_t *clip)
{
    wrapper->clip = clip;
}

void
_comac_surface_wrapper_set_foreground_color (comac_surface_wrapper_t *wrapper,
                                             const comac_color_t *color)
{
    if (color)
        wrapper->foreground_source = _comac_pattern_create_solid (color);
}

void
_comac_surface_wrapper_get_font_options (comac_surface_wrapper_t    *wrapper,
					 comac_font_options_t	    *options)
{
    comac_surface_get_font_options (wrapper->target, options);
}

comac_surface_t *
_comac_surface_wrapper_snapshot (comac_surface_wrapper_t *wrapper)
{
    if (wrapper->target->backend->snapshot)
	return wrapper->target->backend->snapshot (wrapper->target);

    return NULL;
}

comac_bool_t
_comac_surface_wrapper_has_show_text_glyphs (comac_surface_wrapper_t *wrapper)
{
    return comac_surface_has_show_text_glyphs (wrapper->target);
}

void
_comac_surface_wrapper_init (comac_surface_wrapper_t *wrapper,
			     comac_surface_t *target)
{
    wrapper->target = comac_surface_reference (target);
    comac_matrix_init_identity (&wrapper->transform);
    wrapper->has_extents = FALSE;
    wrapper->extents.x = wrapper->extents.y = 0;
    wrapper->clip = NULL;
    wrapper->foreground_source = NULL;

    wrapper->needs_transform = FALSE;
    if (target) {
	wrapper->needs_transform =
	    ! _comac_matrix_is_identity (&target->device_transform);
    }
}

void
_comac_surface_wrapper_fini (comac_surface_wrapper_t *wrapper)
{
    if (wrapper->foreground_source)
        comac_pattern_destroy (wrapper->foreground_source);

    comac_surface_destroy (wrapper->target);
}

comac_bool_t
_comac_surface_wrapper_get_target_extents (comac_surface_wrapper_t *wrapper,
					   comac_bool_t surface_is_unbounded,
					   comac_rectangle_int_t *extents)
{
    comac_rectangle_int_t clip;
    comac_bool_t has_clip = FALSE;

    if (!surface_is_unbounded)
	has_clip = _comac_surface_get_extents (wrapper->target, &clip);

    if (wrapper->clip) {
	if (has_clip) {
	    if (! _comac_rectangle_intersect (&clip,
					      _comac_clip_get_extents (wrapper->clip)))
		return FALSE;
	} else {
	    has_clip = TRUE;
	    clip = *_comac_clip_get_extents (wrapper->clip);
	}
    }

    if (has_clip && wrapper->needs_transform) {
	comac_matrix_t m;
	double x1, y1, x2, y2;

	_comac_surface_wrapper_get_inverse_transform (wrapper, &m);

	x1 = clip.x;
	y1 = clip.y;
	x2 = clip.x + clip.width;
	y2 = clip.y + clip.height;

	_comac_matrix_transform_bounding_box (&m, &x1, &y1, &x2, &y2, NULL);

	clip.x = floor (x1);
	clip.y = floor (y1);
	clip.width  = ceil (x2) - clip.x;
	clip.height = ceil (y2) - clip.y;
    }

    if (has_clip) {
	if (wrapper->has_extents) {
	    *extents = wrapper->extents;
	    return _comac_rectangle_intersect (extents, &clip);
	} else {
	    *extents = clip;
	    return TRUE;
	}
    } else if (wrapper->has_extents) {
	*extents = wrapper->extents;
	return TRUE;
    } else {
	_comac_unbounded_rectangle_init (extents);
	return TRUE;
    }
}
