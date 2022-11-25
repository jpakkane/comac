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
#include "comac-surface-offset-private.h"

/* A collection of routines to facilitate drawing to an alternate surface. */

static void
_copy_transformed_pattern (comac_pattern_t *pattern,
			   const comac_pattern_t *original,
			   const comac_matrix_t *ctm_inverse)
{
    _comac_pattern_init_static_copy (pattern, original);

    if (! _comac_matrix_is_identity (ctm_inverse))
	_comac_pattern_transform (pattern, ctm_inverse);
}

comac_status_t
_comac_surface_offset_paint (comac_surface_t *target,
			     int x,
			     int y,
			     comac_operator_t op,
			     const comac_pattern_t *source,
			     const comac_clip_t *clip)
{
    comac_status_t status;
    comac_clip_t *dev_clip = (comac_clip_t *) clip;
    comac_pattern_union_t source_copy;

    if (unlikely (target->status))
	return target->status;

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    if (x | y) {
	comac_matrix_t m;

	dev_clip = _comac_clip_copy_with_translation (clip, -x, -y);

	comac_matrix_init_translate (&m, x, y);
	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;
    }

    status = _comac_surface_paint (target, op, source, dev_clip);

    if (dev_clip != clip)
	_comac_clip_destroy (dev_clip);

    return status;
}

comac_status_t
_comac_surface_offset_mask (comac_surface_t *target,
			    int x,
			    int y,
			    comac_operator_t op,
			    const comac_pattern_t *source,
			    const comac_pattern_t *mask,
			    const comac_clip_t *clip)
{
    comac_status_t status;
    comac_clip_t *dev_clip = (comac_clip_t *) clip;
    comac_pattern_union_t source_copy;
    comac_pattern_union_t mask_copy;

    if (unlikely (target->status))
	return target->status;

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    if (x | y) {
	comac_matrix_t m;

	dev_clip = _comac_clip_copy_with_translation (clip, -x, -y);

	comac_matrix_init_translate (&m, x, y);
	_copy_transformed_pattern (&source_copy.base, source, &m);
	_copy_transformed_pattern (&mask_copy.base, mask, &m);
	source = &source_copy.base;
	mask = &mask_copy.base;
    }

    status = _comac_surface_mask (target, op, source, mask, dev_clip);

    if (dev_clip != clip)
	_comac_clip_destroy (dev_clip);

    return status;
}

comac_status_t
_comac_surface_offset_stroke (comac_surface_t *surface,
			      int x,
			      int y,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_path_fixed_t *path,
			      const comac_stroke_style_t *stroke_style,
			      const comac_matrix_t *ctm,
			      const comac_matrix_t *ctm_inverse,
			      double tolerance,
			      comac_antialias_t antialias,
			      const comac_clip_t *clip)
{
    comac_path_fixed_t path_copy, *dev_path = (comac_path_fixed_t *) path;
    comac_clip_t *dev_clip = (comac_clip_t *) clip;
    comac_matrix_t dev_ctm = *ctm;
    comac_matrix_t dev_ctm_inverse = *ctm_inverse;
    comac_pattern_union_t source_copy;
    comac_status_t status;

    if (unlikely (surface->status))
	return surface->status;

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    if (x | y) {
	comac_matrix_t m;

	dev_clip = _comac_clip_copy_with_translation (clip, -x, -y);

	status = _comac_path_fixed_init_copy (&path_copy, dev_path);
	if (unlikely (status))
	    goto FINISH;

	_comac_path_fixed_translate (&path_copy,
				     _comac_fixed_from_int (-x),
				     _comac_fixed_from_int (-y));
	dev_path = &path_copy;

	comac_matrix_init_translate (&m, -x, -y);
	comac_matrix_multiply (&dev_ctm, &dev_ctm, &m);

	comac_matrix_init_translate (&m, x, y);
	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;
	comac_matrix_multiply (&dev_ctm_inverse, &m, &dev_ctm_inverse);
    }

    status = _comac_surface_stroke (surface,
				    op,
				    source,
				    dev_path,
				    stroke_style,
				    &dev_ctm,
				    &dev_ctm_inverse,
				    tolerance,
				    antialias,
				    dev_clip);

FINISH:
    if (dev_path != path)
	_comac_path_fixed_fini (dev_path);
    if (dev_clip != clip)
	_comac_clip_destroy (dev_clip);

    return status;
}

comac_status_t
_comac_surface_offset_fill (comac_surface_t *surface,
			    int x,
			    int y,
			    comac_operator_t op,
			    const comac_pattern_t *source,
			    const comac_path_fixed_t *path,
			    comac_fill_rule_t fill_rule,
			    double tolerance,
			    comac_antialias_t antialias,
			    const comac_clip_t *clip)
{
    comac_status_t status;
    comac_path_fixed_t path_copy, *dev_path = (comac_path_fixed_t *) path;
    comac_clip_t *dev_clip = (comac_clip_t *) clip;
    comac_pattern_union_t source_copy;

    if (unlikely (surface->status))
	return surface->status;

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    if (x | y) {
	comac_matrix_t m;

	dev_clip = _comac_clip_copy_with_translation (clip, -x, -y);

	status = _comac_path_fixed_init_copy (&path_copy, dev_path);
	if (unlikely (status))
	    goto FINISH;

	_comac_path_fixed_translate (&path_copy,
				     _comac_fixed_from_int (-x),
				     _comac_fixed_from_int (-y));
	dev_path = &path_copy;

	comac_matrix_init_translate (&m, x, y);
	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;
    }

    status = _comac_surface_fill (surface,
				  op,
				  source,
				  dev_path,
				  fill_rule,
				  tolerance,
				  antialias,
				  dev_clip);

FINISH:
    if (dev_path != path)
	_comac_path_fixed_fini (dev_path);
    if (dev_clip != clip)
	_comac_clip_destroy (dev_clip);

    return status;
}

comac_status_t
_comac_surface_offset_glyphs (comac_surface_t *surface,
			      int x,
			      int y,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      comac_scaled_font_t *scaled_font,
			      comac_glyph_t *glyphs,
			      int num_glyphs,
			      const comac_clip_t *clip)
{
    comac_status_t status;
    comac_clip_t *dev_clip = (comac_clip_t *) clip;
    comac_pattern_union_t source_copy;
    comac_glyph_t *dev_glyphs;
    int i;

    if (unlikely (surface->status))
	return surface->status;

    if (_comac_clip_is_all_clipped (clip))
	return COMAC_STATUS_SUCCESS;

    dev_glyphs = _comac_malloc_ab (num_glyphs, sizeof (comac_glyph_t));
    if (dev_glyphs == NULL)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    memcpy (dev_glyphs, glyphs, sizeof (comac_glyph_t) * num_glyphs);

    if (x | y) {
	comac_matrix_t m;

	dev_clip = _comac_clip_copy_with_translation (clip, -x, -y);

	comac_matrix_init_translate (&m, x, y);
	_copy_transformed_pattern (&source_copy.base, source, &m);
	source = &source_copy.base;

	for (i = 0; i < num_glyphs; i++) {
	    dev_glyphs[i].x -= x;
	    dev_glyphs[i].y -= y;
	}
    }

    status = _comac_surface_show_text_glyphs (surface,
					      op,
					      source,
					      NULL,
					      0,
					      dev_glyphs,
					      num_glyphs,
					      NULL,
					      0,
					      0,
					      scaled_font,
					      dev_clip);

    if (dev_clip != clip)
	_comac_clip_destroy (dev_clip);
    free (dev_glyphs);

    return status;
}
