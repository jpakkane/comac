/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc
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
 *	Carl Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

/* This surface supports redirecting all its input to multiple surfaces.
 */

#include "comacint.h"

#include "comac-tee.h"

#include "comac-default-context-private.h"
#include "comac-error-private.h"
#include "comac-tee-surface-private.h"
#include "comac-recording-surface-inline.h"
#include "comac-surface-wrapper-private.h"
#include "comac-array-private.h"
#include "comac-image-surface-inline.h"

typedef struct _comac_tee_surface {
    comac_surface_t base;

    comac_surface_wrapper_t master;
    comac_array_t slaves;
} comac_tee_surface_t;

static comac_surface_t *
_comac_tee_surface_create_similar (void *abstract_surface,
				   comac_content_t content,
				   int width,
				   int height)
{

    comac_tee_surface_t *other = abstract_surface;
    comac_surface_t *similar;
    comac_surface_t *surface;
    comac_surface_wrapper_t *slaves;
    int n, num_slaves;

    similar = _comac_surface_wrapper_create_similar (&other->master,
						     content,
						     width,
						     height);
    surface = comac_tee_surface_create (similar);
    comac_surface_destroy (similar);
    if (unlikely (surface->status))
	return surface;

    num_slaves = _comac_array_num_elements (&other->slaves);
    slaves = _comac_array_index (&other->slaves, 0);
    for (n = 0; n < num_slaves; n++) {

	similar = _comac_surface_wrapper_create_similar (&slaves[n],
							 content,
							 width,
							 height);
	comac_tee_surface_add (surface, similar);
	comac_surface_destroy (similar);
    }

    if (unlikely (surface->status)) {
	comac_status_t status = surface->status;
	comac_surface_destroy (surface);
	surface = _comac_surface_create_in_error (status);
    }

    return surface;
}

static comac_status_t
_comac_tee_surface_finish (void *abstract_surface)
{
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    int n, num_slaves;

    _comac_surface_wrapper_fini (&surface->master);

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++)
	_comac_surface_wrapper_fini (&slaves[n]);

    _comac_array_fini (&surface->slaves);

    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
_comac_tee_surface_source (void *abstract_surface,
			   comac_rectangle_int_t *extents)
{
    comac_tee_surface_t *surface = abstract_surface;
    return _comac_surface_get_source (surface->master.target, extents);
}

static comac_status_t
_comac_tee_surface_acquire_source_image (void *abstract_surface,
					 comac_image_surface_t **image_out,
					 void **image_extra)
{
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    int num_slaves, n;

    /* we prefer to use a real image surface if available */
    if (_comac_surface_is_image (surface->master.target)) {
	return _comac_surface_wrapper_acquire_source_image (&surface->master,
							    image_out,
							    image_extra);
    }

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	if (_comac_surface_is_image (slaves[n].target)) {
	    return _comac_surface_wrapper_acquire_source_image (&slaves[n],
								image_out,
								image_extra);
	}
    }

    return _comac_surface_wrapper_acquire_source_image (&surface->master,
							image_out,
							image_extra);
}

static void
_comac_tee_surface_release_source_image (void *abstract_surface,
					 comac_image_surface_t *image,
					 void *image_extra)
{
    comac_tee_surface_t *surface = abstract_surface;

    _comac_surface_wrapper_release_source_image (&surface->master,
						 image,
						 image_extra);
}

static comac_surface_t *
_comac_tee_surface_snapshot (void *abstract_surface)
{
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    int num_slaves, n;

    /* we prefer to use a recording surface for our snapshots */
    if (_comac_surface_is_recording (surface->master.target))
	return _comac_surface_wrapper_snapshot (&surface->master);

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	if (_comac_surface_is_recording (slaves[n].target))
	    return _comac_surface_wrapper_snapshot (&slaves[n]);
    }

    return _comac_surface_wrapper_snapshot (&surface->master);
}

static comac_bool_t
_comac_tee_surface_get_extents (void *abstract_surface,
				comac_rectangle_int_t *rectangle)
{
    comac_tee_surface_t *surface = abstract_surface;

    return _comac_surface_wrapper_get_extents (&surface->master, rectangle);
}

static void
_comac_tee_surface_get_font_options (void *abstract_surface,
				     comac_font_options_t *options)
{
    comac_tee_surface_t *surface = abstract_surface;

    _comac_surface_wrapper_get_font_options (&surface->master, options);
}

static comac_int_status_t
_comac_tee_surface_paint (void *abstract_surface,
			  comac_operator_t op,
			  const comac_pattern_t *source,
			  const comac_clip_t *clip)
{
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    int n, num_slaves;
    comac_int_status_t status;

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	status = _comac_surface_wrapper_paint (&slaves[n], op, source, clip);
	if (unlikely (status))
	    return status;
    }

    return _comac_surface_wrapper_paint (&surface->master, op, source, clip);
}

static comac_int_status_t
_comac_tee_surface_mask (void *abstract_surface,
			 comac_operator_t op,
			 const comac_pattern_t *source,
			 const comac_pattern_t *mask,
			 const comac_clip_t *clip)
{
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    comac_int_status_t status;
    int n, num_slaves;

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	status =
	    _comac_surface_wrapper_mask (&slaves[n], op, source, mask, clip);
	if (unlikely (status))
	    return status;
    }

    return _comac_surface_wrapper_mask (&surface->master,
					op,
					source,
					mask,
					clip);
}

static comac_int_status_t
_comac_tee_surface_stroke (void *abstract_surface,
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
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    comac_int_status_t status;
    int n, num_slaves;

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	status = _comac_surface_wrapper_stroke (&slaves[n],
						op,
						source,
						path,
						style,
						ctm,
						ctm_inverse,
						tolerance,
						antialias,
						clip);
	if (unlikely (status))
	    return status;
    }

    return _comac_surface_wrapper_stroke (&surface->master,
					  op,
					  source,
					  path,
					  style,
					  ctm,
					  ctm_inverse,
					  tolerance,
					  antialias,
					  clip);
}

static comac_int_status_t
_comac_tee_surface_fill (void *abstract_surface,
			 comac_operator_t op,
			 const comac_pattern_t *source,
			 const comac_path_fixed_t *path,
			 comac_fill_rule_t fill_rule,
			 double tolerance,
			 comac_antialias_t antialias,
			 const comac_clip_t *clip)
{
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    comac_int_status_t status;
    int n, num_slaves;

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	status = _comac_surface_wrapper_fill (&slaves[n],
					      op,
					      source,
					      path,
					      fill_rule,
					      tolerance,
					      antialias,
					      clip);
	if (unlikely (status))
	    return status;
    }

    return _comac_surface_wrapper_fill (&surface->master,
					op,
					source,
					path,
					fill_rule,
					tolerance,
					antialias,
					clip);
}

static comac_bool_t
_comac_tee_surface_has_show_text_glyphs (void *abstract_surface)
{
    return TRUE;
}

static comac_int_status_t
_comac_tee_surface_show_text_glyphs (void *abstract_surface,
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
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    comac_int_status_t status;
    int n, num_slaves;
    comac_glyph_t *glyphs_copy;

    /* XXX: This copying is ugly. */
    glyphs_copy = _comac_malloc_ab (num_glyphs, sizeof (comac_glyph_t));
    if (unlikely (glyphs_copy == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	memcpy (glyphs_copy, glyphs, sizeof (comac_glyph_t) * num_glyphs);
	status = _comac_surface_wrapper_show_text_glyphs (&slaves[n],
							  op,
							  source,
							  utf8,
							  utf8_len,
							  glyphs_copy,
							  num_glyphs,
							  clusters,
							  num_clusters,
							  cluster_flags,
							  scaled_font,
							  clip);
	if (unlikely (status))
	    goto CLEANUP;
    }

    memcpy (glyphs_copy, glyphs, sizeof (comac_glyph_t) * num_glyphs);
    status = _comac_surface_wrapper_show_text_glyphs (&surface->master,
						      op,
						      source,
						      utf8,
						      utf8_len,
						      glyphs_copy,
						      num_glyphs,
						      clusters,
						      num_clusters,
						      cluster_flags,
						      scaled_font,
						      clip);
CLEANUP:
    free (glyphs_copy);
    return status;
}

static const comac_surface_backend_t comac_tee_surface_backend = {
    COMAC_SURFACE_TYPE_TEE,
    _comac_tee_surface_finish,

    _comac_default_context_create, /* XXX */

    _comac_tee_surface_create_similar,
    NULL, /* create similar image */
    NULL, /* map to image */
    NULL, /* unmap image */

    _comac_tee_surface_source,
    _comac_tee_surface_acquire_source_image,
    _comac_tee_surface_release_source_image,
    _comac_tee_surface_snapshot,
    NULL, /* copy_page */
    NULL, /* show_page */
    _comac_tee_surface_get_extents,
    _comac_tee_surface_get_font_options,
    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    _comac_tee_surface_paint,
    _comac_tee_surface_mask,
    _comac_tee_surface_stroke,
    _comac_tee_surface_fill,
    NULL, /* fill_stroke */

    NULL, /* show_glyphs */

    _comac_tee_surface_has_show_text_glyphs,
    _comac_tee_surface_show_text_glyphs};

comac_surface_t *
comac_tee_surface_create (comac_surface_t *master)
{
    comac_tee_surface_t *surface;

    if (unlikely (master->status))
	return _comac_surface_create_in_error (master->status);

    surface = _comac_malloc (sizeof (comac_tee_surface_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_surface_init (&surface->base,
			 &comac_tee_surface_backend,
			 master->device,
			 master->content,
			 TRUE); /* is_vector */

    _comac_surface_wrapper_init (&surface->master, master);

    _comac_array_init (&surface->slaves, sizeof (comac_surface_wrapper_t));

    return &surface->base;
}

void
comac_tee_surface_add (comac_surface_t *abstract_surface,
		       comac_surface_t *target)
{
    comac_tee_surface_t *surface;
    comac_surface_wrapper_t slave;
    comac_status_t status;

    if (unlikely (abstract_surface->status))
	return;
    if (unlikely (abstract_surface->finished)) {
	status = _comac_surface_set_error (
	    abstract_surface,
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    if (abstract_surface->backend != &comac_tee_surface_backend) {
	status = _comac_surface_set_error (
	    abstract_surface,
	    _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH));
	return;
    }

    if (unlikely (target->status)) {
	status = _comac_surface_set_error (abstract_surface, target->status);
	return;
    }

    surface = (comac_tee_surface_t *) abstract_surface;

    _comac_surface_wrapper_init (&slave, target);
    status = _comac_array_append (&surface->slaves, &slave);
    if (unlikely (status)) {
	_comac_surface_wrapper_fini (&slave);
	status = _comac_surface_set_error (&surface->base, status);
    }
}

void
comac_tee_surface_remove (comac_surface_t *abstract_surface,
			  comac_surface_t *target)
{
    comac_tee_surface_t *surface;
    comac_surface_wrapper_t *slaves;
    int n, num_slaves;

    if (unlikely (abstract_surface->status))
	return;
    if (unlikely (abstract_surface->finished)) {
	_comac_surface_set_error (abstract_surface,
				  _comac_error (COMAC_STATUS_SURFACE_FINISHED));
	return;
    }

    if (abstract_surface->backend != &comac_tee_surface_backend) {
	_comac_surface_set_error (
	    abstract_surface,
	    _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH));
	return;
    }

    surface = (comac_tee_surface_t *) abstract_surface;
    if (target == surface->master.target) {
	_comac_surface_set_error (abstract_surface,
				  _comac_error (COMAC_STATUS_INVALID_INDEX));
	return;
    }

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	if (slaves[n].target == target)
	    break;
    }

    if (n == num_slaves) {
	_comac_surface_set_error (abstract_surface,
				  _comac_error (COMAC_STATUS_INVALID_INDEX));
	return;
    }

    _comac_surface_wrapper_fini (&slaves[n]);
    for (n++; n < num_slaves; n++)
	slaves[n - 1] = slaves[n];
    surface->slaves.num_elements--; /* XXX: comac_array_remove()? */
}

comac_surface_t *
comac_tee_surface_index (comac_surface_t *abstract_surface, unsigned int index)
{
    comac_tee_surface_t *surface;

    if (unlikely (abstract_surface->status))
	return _comac_surface_create_in_error (abstract_surface->status);
    if (unlikely (abstract_surface->finished))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (abstract_surface->backend != &comac_tee_surface_backend)
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH));

    surface = (comac_tee_surface_t *) abstract_surface;
    if (index == 0) {
	return surface->master.target;
    } else {
	comac_surface_wrapper_t *slave;

	index--;

	if (index >= _comac_array_num_elements (&surface->slaves))
	    return _comac_surface_create_in_error (
		_comac_error (COMAC_STATUS_INVALID_INDEX));

	slave = _comac_array_index (&surface->slaves, index);
	return slave->target;
    }
}

comac_surface_t *
_comac_tee_surface_find_match (void *abstract_surface,
			       const comac_surface_backend_t *backend,
			       comac_content_t content)
{
    comac_tee_surface_t *surface = abstract_surface;
    comac_surface_wrapper_t *slaves;
    int num_slaves, n;

    /* exact match first */
    if (surface->master.target->backend == backend &&
	surface->master.target->content == content) {
	return surface->master.target;
    }

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	if (slaves[n].target->backend == backend &&
	    slaves[n].target->content == content) {
	    return slaves[n].target;
	}
    }

    /* matching backend? */
    if (surface->master.target->backend == backend)
	return surface->master.target;

    num_slaves = _comac_array_num_elements (&surface->slaves);
    slaves = _comac_array_index (&surface->slaves, 0);
    for (n = 0; n < num_slaves; n++) {
	if (slaves[n].target->backend == backend)
	    return slaves[n].target;
    }

    return NULL;
}
