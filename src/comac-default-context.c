/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"

#include "comac-private.h"
#include "comac-arc-private.h"
#include "comac-backend-private.h"
#include "comac-clip-inline.h"
#include "comac-default-context-private.h"
#include "comac-error-private.h"
#include "comac-freed-pool-private.h"
#include "comac-path-private.h"
#include "comac-pattern-private.h"

#define COMAC_TOLERANCE_MINIMUM _comac_fixed_to_double (1)

#if ! defined(INFINITY)
#define INFINITY HUGE_VAL
#endif

static freed_pool_t context_pool;

void
_comac_default_context_reset_static_data (void)
{
    _freed_pool_reset (&context_pool);
}

void
_comac_default_context_fini (comac_default_context_t *cr)
{
    while (cr->gstate != &cr->gstate_tail[0]) {
	if (_comac_gstate_restore (&cr->gstate, &cr->gstate_freelist))
	    break;
    }

    _comac_gstate_fini (cr->gstate);
    cr->gstate_freelist = cr->gstate_freelist->next; /* skip over tail[1] */
    while (cr->gstate_freelist != NULL) {
	comac_gstate_t *gstate = cr->gstate_freelist;
	cr->gstate_freelist = gstate->next;
	free (gstate);
    }

    _comac_path_fixed_fini (cr->path);

    _comac_fini (&cr->base);
}

static void
_comac_default_context_destroy (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_default_context_fini (cr);

    /* mark the context as invalid to protect against misuse */
    cr->base.status = COMAC_STATUS_NULL_POINTER;
    _freed_pool_put (&context_pool, cr);
}

static comac_surface_t *
_comac_default_context_get_original_target (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_original_target (cr->gstate);
}

static comac_surface_t *
_comac_default_context_get_current_target (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_target (cr->gstate);
}

static comac_status_t
_comac_default_context_save (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_save (&cr->gstate, &cr->gstate_freelist);
}

static comac_status_t
_comac_default_context_restore (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    if (unlikely (_comac_gstate_is_group (cr->gstate)))
	return _comac_error (COMAC_STATUS_INVALID_RESTORE);

    return _comac_gstate_restore (&cr->gstate, &cr->gstate_freelist);
}

static comac_status_t
_comac_default_context_push_group (void *abstract_cr, comac_content_t content)
{
    comac_default_context_t *cr = abstract_cr;
    comac_surface_t *group_surface;
    comac_clip_t *clip;
    comac_status_t status;

    clip = _comac_gstate_get_clip (cr->gstate);
    if (_comac_clip_is_all_clipped (clip)) {
	group_surface = comac_image_surface_create (COMAC_FORMAT_ARGB32, 0, 0);
	status = group_surface->status;
	if (unlikely (status))
	    goto bail;
    } else {
	comac_surface_t *parent_surface;
	comac_rectangle_int_t extents;
	comac_bool_t bounded, is_empty;

	parent_surface = _comac_gstate_get_target (cr->gstate);

	if (unlikely (parent_surface->status))
	    return parent_surface->status;
	if (unlikely (parent_surface->finished))
	    return _comac_error (COMAC_STATUS_SURFACE_FINISHED);

	/* Get the extents that we'll use in creating our new group surface */
	bounded = _comac_surface_get_extents (parent_surface, &extents);
	if (clip)
	    /* XXX: This assignment just fixes a compiler warning? */
	    is_empty =
		_comac_rectangle_intersect (&extents,
					    _comac_clip_get_extents (clip));

	if (! bounded) {
	    /* XXX: Generic solution? */
	    group_surface = comac_recording_surface_create (content, NULL);
	    extents.x = extents.y = 0;
	} else {
	    group_surface =
		_comac_surface_create_scratch (parent_surface,
					       content,
					       extents.width,
					       extents.height,
					       COMAC_COLOR_TRANSPARENT);
	}
	status = group_surface->status;
	if (unlikely (status))
	    goto bail;

	/* Set device offsets on the new surface so that logically it appears at
	 * the same location on the parent surface -- when we pop_group this,
	 * the source pattern will get fixed up for the appropriate target surface
	 * device offsets, so we want to set our own surface offsets from /that/,
	 * and not from the device origin. */
	comac_surface_set_device_offset (
	    group_surface,
	    parent_surface->device_transform.x0 - extents.x,
	    parent_surface->device_transform.y0 - extents.y);

	comac_surface_set_device_scale (group_surface,
					parent_surface->device_transform.xx,
					parent_surface->device_transform.yy);

	/* If we have a current path, we need to adjust it to compensate for
	 * the device offset just applied. */
	_comac_path_fixed_translate (cr->path,
				     _comac_fixed_from_int (-extents.x),
				     _comac_fixed_from_int (-extents.y));
    }

    /* create a new gstate for the redirect */
    status = _comac_gstate_save (&cr->gstate, &cr->gstate_freelist);
    if (unlikely (status))
	goto bail;

    status = _comac_gstate_redirect_target (cr->gstate, group_surface);

bail:
    comac_surface_destroy (group_surface);
    return status;
}

static comac_pattern_t *
_comac_default_context_pop_group (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;
    comac_surface_t *group_surface;
    comac_pattern_t *group_pattern;
    comac_surface_t *parent_surface;
    comac_matrix_t group_matrix;
    comac_status_t status;

    /* Verify that we are at the right nesting level */
    if (unlikely (! _comac_gstate_is_group (cr->gstate)))
	return _comac_pattern_create_in_error (COMAC_STATUS_INVALID_POP_GROUP);

    /* Get a reference to the active surface before restoring */
    group_surface = _comac_gstate_get_target (cr->gstate);
    group_surface = comac_surface_reference (group_surface);

    status = _comac_gstate_restore (&cr->gstate, &cr->gstate_freelist);
    assert (status == COMAC_STATUS_SUCCESS);

    parent_surface = _comac_gstate_get_target (cr->gstate);

    group_pattern = comac_pattern_create_for_surface (group_surface);
    status = group_pattern->status;
    if (unlikely (status))
	goto done;

    _comac_gstate_get_matrix (cr->gstate, &group_matrix);
    comac_pattern_set_matrix (group_pattern, &group_matrix);

    /* If we have a current path, we need to adjust it to compensate for
     * the device offset just removed. */
    _comac_path_fixed_translate (
	cr->path,
	_comac_fixed_from_int (parent_surface->device_transform.x0 -
			       group_surface->device_transform.x0),
	_comac_fixed_from_int (parent_surface->device_transform.y0 -
			       group_surface->device_transform.y0));

done:
    comac_surface_destroy (group_surface);

    return group_pattern;
}

static comac_status_t
_comac_default_context_set_source (void *abstract_cr, comac_pattern_t *source)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_source (cr->gstate, source);
}

static comac_bool_t
_current_source_matches_solid (const comac_pattern_t *pattern,
			       double red,
			       double green,
			       double blue,
			       double alpha)
{
    comac_color_t color;

    if (pattern->type != COMAC_PATTERN_TYPE_SOLID)
	return FALSE;

    red = _comac_restrict_value (red, 0.0, 1.0);
    green = _comac_restrict_value (green, 0.0, 1.0);
    blue = _comac_restrict_value (blue, 0.0, 1.0);
    alpha = _comac_restrict_value (alpha, 0.0, 1.0);

    _comac_color_init_rgba (&color, red, green, blue, alpha);
    return _comac_color_equal (&color,
			       &((comac_solid_pattern_t *) pattern)->color);
}

static comac_status_t
_comac_default_context_set_source_rgba (
    void *abstract_cr, double red, double green, double blue, double alpha)
{
    comac_default_context_t *cr = abstract_cr;
    comac_pattern_t *pattern;
    comac_status_t status;

    if (_current_source_matches_solid (cr->gstate->source,
				       red,
				       green,
				       blue,
				       alpha))
	return COMAC_STATUS_SUCCESS;

    /* push the current pattern to the freed lists */
    _comac_default_context_set_source (
	cr,
	(comac_pattern_t *) &_comac_pattern_black);

    pattern = comac_pattern_create_rgba (red, green, blue, alpha);
    if (unlikely (pattern->status))
	return pattern->status;

    status = _comac_default_context_set_source (cr, pattern);
    comac_pattern_destroy (pattern);

    return status;
}

static comac_status_t
_comac_default_context_set_source_surface (void *abstract_cr,
					   comac_surface_t *surface,
					   double x,
					   double y)
{
    comac_default_context_t *cr = abstract_cr;
    comac_pattern_t *pattern;
    comac_matrix_t matrix;
    comac_status_t status;

    /* push the current pattern to the freed lists */
    _comac_default_context_set_source (
	cr,
	(comac_pattern_t *) &_comac_pattern_black);

    pattern = comac_pattern_create_for_surface (surface);
    if (unlikely (pattern->status)) {
	status = pattern->status;
	comac_pattern_destroy (pattern);
	return status;
    }

    comac_matrix_init_translate (&matrix, -x, -y);
    comac_pattern_set_matrix (pattern, &matrix);

    status = _comac_default_context_set_source (cr, pattern);
    comac_pattern_destroy (pattern);

    return status;
}

static comac_pattern_t *
_comac_default_context_get_source (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_source (cr->gstate);
}

static comac_status_t
_comac_default_context_set_tolerance (void *abstract_cr, double tolerance)
{
    comac_default_context_t *cr = abstract_cr;

    if (tolerance < COMAC_TOLERANCE_MINIMUM)
	tolerance = COMAC_TOLERANCE_MINIMUM;

    return _comac_gstate_set_tolerance (cr->gstate, tolerance);
}

static comac_status_t
_comac_default_context_set_operator (void *abstract_cr, comac_operator_t op)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_operator (cr->gstate, op);
}

static comac_status_t
_comac_default_context_set_opacity (void *abstract_cr, double opacity)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_opacity (cr->gstate, opacity);
}

static comac_status_t
_comac_default_context_set_antialias (void *abstract_cr,
				      comac_antialias_t antialias)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_antialias (cr->gstate, antialias);
}

static comac_status_t
_comac_default_context_set_fill_rule (void *abstract_cr,
				      comac_fill_rule_t fill_rule)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_fill_rule (cr->gstate, fill_rule);
}

static comac_status_t
_comac_default_context_set_line_width (void *abstract_cr, double line_width)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_line_width (cr->gstate, line_width);
}

static comac_status_t
_comac_default_context_set_hairline (void *abstract_cr,
				     comac_bool_t set_hairline)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_hairline (cr->gstate, set_hairline);
}

static comac_status_t
_comac_default_context_set_line_cap (void *abstract_cr,
				     comac_line_cap_t line_cap)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_line_cap (cr->gstate, line_cap);
}

static comac_status_t
_comac_default_context_set_line_join (void *abstract_cr,
				      comac_line_join_t line_join)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_line_join (cr->gstate, line_join);
}

static comac_status_t
_comac_default_context_set_dash (void *abstract_cr,
				 const double *dashes,
				 int num_dashes,
				 double offset)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_dash (cr->gstate, dashes, num_dashes, offset);
}

static comac_status_t
_comac_default_context_set_miter_limit (void *abstract_cr, double limit)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_miter_limit (cr->gstate, limit);
}

static comac_antialias_t
_comac_default_context_get_antialias (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_antialias (cr->gstate);
}

static void
_comac_default_context_get_dash (void *abstract_cr,
				 double *dashes,
				 int *num_dashes,
				 double *offset)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_get_dash (cr->gstate, dashes, num_dashes, offset);
}

static comac_fill_rule_t
_comac_default_context_get_fill_rule (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_fill_rule (cr->gstate);
}

static double
_comac_default_context_get_line_width (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_line_width (cr->gstate);
}

static comac_bool_t
_comac_default_context_get_hairline (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_hairline (cr->gstate);
}

static comac_line_cap_t
_comac_default_context_get_line_cap (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_line_cap (cr->gstate);
}

static comac_line_join_t
_comac_default_context_get_line_join (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_line_join (cr->gstate);
}

static double
_comac_default_context_get_miter_limit (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_miter_limit (cr->gstate);
}

static comac_operator_t
_comac_default_context_get_operator (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_operator (cr->gstate);
}

static double
_comac_default_context_get_opacity (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_opacity (cr->gstate);
}

static double
_comac_default_context_get_tolerance (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_tolerance (cr->gstate);
}

/* Current transformation matrix */

static comac_status_t
_comac_default_context_translate (void *abstract_cr, double tx, double ty)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_translate (cr->gstate, tx, ty);
}

static comac_status_t
_comac_default_context_scale (void *abstract_cr, double sx, double sy)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_scale (cr->gstate, sx, sy);
}

static comac_status_t
_comac_default_context_rotate (void *abstract_cr, double theta)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_rotate (cr->gstate, theta);
}

static comac_status_t
_comac_default_context_transform (void *abstract_cr,
				  const comac_matrix_t *matrix)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_transform (cr->gstate, matrix);
}

static comac_status_t
_comac_default_context_set_matrix (void *abstract_cr,
				   const comac_matrix_t *matrix)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_matrix (cr->gstate, matrix);
}

static comac_status_t
_comac_default_context_set_identity_matrix (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_identity_matrix (cr->gstate);
    return COMAC_STATUS_SUCCESS;
}

static void
_comac_default_context_get_matrix (void *abstract_cr, comac_matrix_t *matrix)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_get_matrix (cr->gstate, matrix);
}

static void
_comac_default_context_user_to_device (void *abstract_cr, double *x, double *y)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_user_to_device (cr->gstate, x, y);
}

static void
_comac_default_context_user_to_device_distance (void *abstract_cr,
						double *dx,
						double *dy)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_user_to_device_distance (cr->gstate, dx, dy);
}

static void
_comac_default_context_device_to_user (void *abstract_cr, double *x, double *y)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_device_to_user (cr->gstate, x, y);
}

static void
_comac_default_context_device_to_user_distance (void *abstract_cr,
						double *dx,
						double *dy)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_device_to_user_distance (cr->gstate, dx, dy);
}

static void
_comac_default_context_backend_to_user (void *abstract_cr, double *x, double *y)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_backend_to_user (cr->gstate, x, y);
}

static void
_comac_default_context_backend_to_user_distance (void *abstract_cr,
						 double *dx,
						 double *dy)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_backend_to_user_distance (cr->gstate, dx, dy);
}

static void
_comac_default_context_user_to_backend (void *abstract_cr, double *x, double *y)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_user_to_backend (cr->gstate, x, y);
}

static void
_comac_default_context_user_to_backend_distance (void *abstract_cr,
						 double *dx,
						 double *dy)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_user_to_backend_distance (cr->gstate, dx, dy);
}

/* Path constructor */

static comac_status_t
_comac_default_context_new_path (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_path_fixed_fini (cr->path);
    _comac_path_fixed_init (cr->path);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_default_context_new_sub_path (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_path_fixed_new_sub_path (cr->path);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_default_context_move_to (void *abstract_cr, double x, double y)
{
    comac_default_context_t *cr = abstract_cr;
    comac_fixed_t x_fixed, y_fixed;
    double width;

    _comac_gstate_user_to_backend (cr->gstate, &x, &y);
    width = _comac_gstate_get_line_width (cr->gstate);
    x_fixed = _comac_fixed_from_double_clamped (x, width);
    y_fixed = _comac_fixed_from_double_clamped (y, width);

    return _comac_path_fixed_move_to (cr->path, x_fixed, y_fixed);
}

static comac_status_t
_comac_default_context_line_to (void *abstract_cr, double x, double y)
{
    comac_default_context_t *cr = abstract_cr;
    comac_fixed_t x_fixed, y_fixed;
    double width;

    _comac_gstate_user_to_backend (cr->gstate, &x, &y);
    width = _comac_gstate_get_line_width (cr->gstate);
    x_fixed = _comac_fixed_from_double_clamped (x, width);
    y_fixed = _comac_fixed_from_double_clamped (y, width);

    return _comac_path_fixed_line_to (cr->path, x_fixed, y_fixed);
}

static comac_status_t
_comac_default_context_curve_to (void *abstract_cr,
				 double x1,
				 double y1,
				 double x2,
				 double y2,
				 double x3,
				 double y3)
{
    comac_default_context_t *cr = abstract_cr;
    comac_fixed_t x1_fixed, y1_fixed;
    comac_fixed_t x2_fixed, y2_fixed;
    comac_fixed_t x3_fixed, y3_fixed;
    double width;

    _comac_gstate_user_to_backend (cr->gstate, &x1, &y1);
    _comac_gstate_user_to_backend (cr->gstate, &x2, &y2);
    _comac_gstate_user_to_backend (cr->gstate, &x3, &y3);
    width = _comac_gstate_get_line_width (cr->gstate);

    x1_fixed = _comac_fixed_from_double_clamped (x1, width);
    y1_fixed = _comac_fixed_from_double_clamped (y1, width);

    x2_fixed = _comac_fixed_from_double_clamped (x2, width);
    y2_fixed = _comac_fixed_from_double_clamped (y2, width);

    x3_fixed = _comac_fixed_from_double_clamped (x3, width);
    y3_fixed = _comac_fixed_from_double_clamped (y3, width);

    return _comac_path_fixed_curve_to (cr->path,
				       x1_fixed,
				       y1_fixed,
				       x2_fixed,
				       y2_fixed,
				       x3_fixed,
				       y3_fixed);
}

static comac_status_t
_comac_default_context_arc (void *abstract_cr,
			    double xc,
			    double yc,
			    double radius,
			    double angle1,
			    double angle2,
			    comac_bool_t forward)
{
    comac_default_context_t *cr = abstract_cr;
    comac_status_t status;

    /* Do nothing, successfully, if radius is <= 0 */
    if (radius <= 0.0) {
	comac_fixed_t x_fixed, y_fixed;

	_comac_gstate_user_to_backend (cr->gstate, &xc, &yc);
	x_fixed = _comac_fixed_from_double (xc);
	y_fixed = _comac_fixed_from_double (yc);
	status = _comac_path_fixed_line_to (cr->path, x_fixed, y_fixed);
	if (unlikely (status))
	    return status;

	status = _comac_path_fixed_line_to (cr->path, x_fixed, y_fixed);
	if (unlikely (status))
	    return status;

	return COMAC_STATUS_SUCCESS;
    }

    status = _comac_default_context_line_to (cr,
					     xc + radius * cos (angle1),
					     yc + radius * sin (angle1));

    if (unlikely (status))
	return status;

    if (forward)
	_comac_arc_path (&cr->base, xc, yc, radius, angle1, angle2);
    else
	_comac_arc_path_negative (&cr->base, xc, yc, radius, angle1, angle2);

    return COMAC_STATUS_SUCCESS; /* any error will have already been set on cr */
}

static comac_status_t
_comac_default_context_rel_move_to (void *abstract_cr, double dx, double dy)
{
    comac_default_context_t *cr = abstract_cr;
    comac_fixed_t dx_fixed, dy_fixed;

    _comac_gstate_user_to_backend_distance (cr->gstate, &dx, &dy);

    dx_fixed = _comac_fixed_from_double (dx);
    dy_fixed = _comac_fixed_from_double (dy);

    return _comac_path_fixed_rel_move_to (cr->path, dx_fixed, dy_fixed);
}

static comac_status_t
_comac_default_context_rel_line_to (void *abstract_cr, double dx, double dy)
{
    comac_default_context_t *cr = abstract_cr;
    comac_fixed_t dx_fixed, dy_fixed;

    _comac_gstate_user_to_backend_distance (cr->gstate, &dx, &dy);

    dx_fixed = _comac_fixed_from_double (dx);
    dy_fixed = _comac_fixed_from_double (dy);

    return _comac_path_fixed_rel_line_to (cr->path, dx_fixed, dy_fixed);
}

static comac_status_t
_comac_default_context_rel_curve_to (void *abstract_cr,
				     double dx1,
				     double dy1,
				     double dx2,
				     double dy2,
				     double dx3,
				     double dy3)
{
    comac_default_context_t *cr = abstract_cr;
    comac_fixed_t dx1_fixed, dy1_fixed;
    comac_fixed_t dx2_fixed, dy2_fixed;
    comac_fixed_t dx3_fixed, dy3_fixed;

    _comac_gstate_user_to_backend_distance (cr->gstate, &dx1, &dy1);
    _comac_gstate_user_to_backend_distance (cr->gstate, &dx2, &dy2);
    _comac_gstate_user_to_backend_distance (cr->gstate, &dx3, &dy3);

    dx1_fixed = _comac_fixed_from_double (dx1);
    dy1_fixed = _comac_fixed_from_double (dy1);

    dx2_fixed = _comac_fixed_from_double (dx2);
    dy2_fixed = _comac_fixed_from_double (dy2);

    dx3_fixed = _comac_fixed_from_double (dx3);
    dy3_fixed = _comac_fixed_from_double (dy3);

    return _comac_path_fixed_rel_curve_to (cr->path,
					   dx1_fixed,
					   dy1_fixed,
					   dx2_fixed,
					   dy2_fixed,
					   dx3_fixed,
					   dy3_fixed);
}

static comac_status_t
_comac_default_context_close_path (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_path_fixed_close_path (cr->path);
}

static comac_status_t
_comac_default_context_rectangle (
    void *abstract_cr, double x, double y, double width, double height)
{
    comac_default_context_t *cr = abstract_cr;
    comac_status_t status;

    status = _comac_default_context_move_to (cr, x, y);
    if (unlikely (status))
	return status;

    status = _comac_default_context_rel_line_to (cr, width, 0);
    if (unlikely (status))
	return status;

    status = _comac_default_context_rel_line_to (cr, 0, height);
    if (unlikely (status))
	return status;

    status = _comac_default_context_rel_line_to (cr, -width, 0);
    if (unlikely (status))
	return status;

    return _comac_default_context_close_path (cr);
}

static void
_comac_default_context_path_extents (
    void *abstract_cr, double *x1, double *y1, double *x2, double *y2)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_path_extents (cr->gstate, cr->path, x1, y1, x2, y2);
}

static comac_bool_t
_comac_default_context_has_current_point (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return cr->path->has_current_point;
}

static comac_bool_t
_comac_default_context_get_current_point (void *abstract_cr,
					  double *x,
					  double *y)
{
    comac_default_context_t *cr = abstract_cr;
    comac_fixed_t x_fixed, y_fixed;

    if (_comac_path_fixed_get_current_point (cr->path, &x_fixed, &y_fixed)) {
	*x = _comac_fixed_to_double (x_fixed);
	*y = _comac_fixed_to_double (y_fixed);
	_comac_gstate_backend_to_user (cr->gstate, x, y);

	return TRUE;
    } else {
	return FALSE;
    }
}

static comac_path_t *
_comac_default_context_copy_path (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_path_create (cr->path, &cr->base);
}

static comac_path_t *
_comac_default_context_copy_path_flat (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_path_create_flat (cr->path, &cr->base);
}

static comac_status_t
_comac_default_context_append_path (void *abstract_cr, const comac_path_t *path)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_path_append_to_context (path, &cr->base);
}

static comac_status_t
_comac_default_context_paint (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_paint (cr->gstate);
}

static comac_status_t
_comac_default_context_paint_with_alpha (void *abstract_cr, double alpha)
{
    comac_default_context_t *cr = abstract_cr;
    comac_solid_pattern_t pattern;
    comac_status_t status;
    comac_color_t color;

    if (COMAC_ALPHA_IS_OPAQUE (alpha))
	return _comac_gstate_paint (cr->gstate);

    if (COMAC_ALPHA_IS_ZERO (alpha) &&
	_comac_operator_bounded_by_mask (cr->gstate->op)) {
	return COMAC_STATUS_SUCCESS;
    }

    _comac_color_init_rgba (&color, 0., 0., 0., alpha);
    _comac_pattern_init_solid (&pattern, &color);

    status = _comac_gstate_mask (cr->gstate, &pattern.base);
    _comac_pattern_fini (&pattern.base);

    return status;
}

static comac_status_t
_comac_default_context_mask (void *abstract_cr, comac_pattern_t *mask)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_mask (cr->gstate, mask);
}

static comac_status_t
_comac_default_context_stroke_preserve (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_stroke (cr->gstate, cr->path);
}

static comac_status_t
_comac_default_context_stroke (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;
    comac_status_t status;

    status = _comac_gstate_stroke (cr->gstate, cr->path);
    if (unlikely (status))
	return status;

    return _comac_default_context_new_path (cr);
}

static comac_status_t
_comac_default_context_in_stroke (void *abstract_cr,
				  double x,
				  double y,
				  comac_bool_t *inside)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_in_stroke (cr->gstate, cr->path, x, y, inside);
}

static comac_status_t
_comac_default_context_stroke_extents (
    void *abstract_cr, double *x1, double *y1, double *x2, double *y2)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_stroke_extents (cr->gstate, cr->path, x1, y1, x2, y2);
}

static comac_status_t
_comac_default_context_fill_preserve (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_fill (cr->gstate, cr->path);
}

static comac_status_t
_comac_default_context_fill (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;
    comac_status_t status;

    status = _comac_gstate_fill (cr->gstate, cr->path);
    if (unlikely (status))
	return status;

    return _comac_default_context_new_path (cr);
}

static comac_status_t
_comac_default_context_in_fill (void *abstract_cr,
				double x,
				double y,
				comac_bool_t *inside)
{
    comac_default_context_t *cr = abstract_cr;

    *inside = _comac_gstate_in_fill (cr->gstate, cr->path, x, y);
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_default_context_fill_extents (
    void *abstract_cr, double *x1, double *y1, double *x2, double *y2)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_fill_extents (cr->gstate, cr->path, x1, y1, x2, y2);
}

static comac_status_t
_comac_default_context_clip_preserve (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_clip (cr->gstate, cr->path);
}

static comac_status_t
_comac_default_context_clip (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;
    comac_status_t status;

    status = _comac_gstate_clip (cr->gstate, cr->path);
    if (unlikely (status))
	return status;

    return _comac_default_context_new_path (cr);
}

static comac_status_t
_comac_default_context_in_clip (void *abstract_cr,
				double x,
				double y,
				comac_bool_t *inside)
{
    comac_default_context_t *cr = abstract_cr;

    *inside = _comac_gstate_in_clip (cr->gstate, x, y);
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_default_context_reset_clip (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_reset_clip (cr->gstate);
}

static comac_status_t
_comac_default_context_clip_extents (
    void *abstract_cr, double *x1, double *y1, double *x2, double *y2)
{
    comac_default_context_t *cr = abstract_cr;

    if (! _comac_gstate_clip_extents (cr->gstate, x1, y1, x2, y2)) {
	*x1 = -INFINITY;
	*y1 = -INFINITY;
	*x2 = +INFINITY;
	*y2 = +INFINITY;
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_rectangle_list_t *
_comac_default_context_copy_clip_rectangle_list (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_copy_clip_rectangle_list (cr->gstate);
}

static comac_status_t
_comac_default_context_copy_page (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_copy_page (cr->gstate);
}

static comac_status_t
_comac_default_context_tag_begin (void *abstract_cr,
				  const char *tag_name,
				  const char *attributes)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_tag_begin (cr->gstate, tag_name, attributes);
}

static comac_status_t
_comac_default_context_tag_end (void *abstract_cr, const char *tag_name)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_tag_end (cr->gstate, tag_name);
}

static comac_status_t
_comac_default_context_show_page (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_show_page (cr->gstate);
}

static comac_status_t
_comac_default_context_set_font_face (void *abstract_cr,
				      comac_font_face_t *font_face)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_font_face (cr->gstate, font_face);
}

static comac_font_face_t *
_comac_default_context_get_font_face (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;
    comac_font_face_t *font_face;
    comac_status_t status;

    status = _comac_gstate_get_font_face (cr->gstate, &font_face);
    if (unlikely (status)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_font_face_t *) &_comac_font_face_nil;
    }

    return font_face;
}

static comac_status_t
_comac_default_context_font_extents (void *abstract_cr,
				     comac_font_extents_t *extents)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_get_font_extents (cr->gstate, extents);
}

static comac_status_t
_comac_default_context_set_font_size (void *abstract_cr, double size)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_font_size (cr->gstate, size);
}

static comac_status_t
_comac_default_context_set_font_matrix (void *abstract_cr,
					const comac_matrix_t *matrix)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_set_font_matrix (cr->gstate, matrix);
}

static void
_comac_default_context_get_font_matrix (void *abstract_cr,
					comac_matrix_t *matrix)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_get_font_matrix (cr->gstate, matrix);
}

static comac_status_t
_comac_default_context_set_font_options (void *abstract_cr,
					 const comac_font_options_t *options)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_set_font_options (cr->gstate, options);
    return COMAC_STATUS_SUCCESS;
}

static void
_comac_default_context_get_font_options (void *abstract_cr,
					 comac_font_options_t *options)
{
    comac_default_context_t *cr = abstract_cr;

    _comac_gstate_get_font_options (cr->gstate, options);
}

static comac_status_t
_comac_default_context_set_scaled_font (void *abstract_cr,
					comac_scaled_font_t *scaled_font)
{
    comac_default_context_t *cr = abstract_cr;
    comac_bool_t was_previous;
    comac_status_t status;

    if (scaled_font == cr->gstate->scaled_font)
	return COMAC_STATUS_SUCCESS;

    was_previous = scaled_font == cr->gstate->previous_scaled_font;

    status = _comac_gstate_set_font_face (cr->gstate, scaled_font->font_face);
    if (unlikely (status))
	return status;

    status =
	_comac_gstate_set_font_matrix (cr->gstate, &scaled_font->font_matrix);
    if (unlikely (status))
	return status;

    _comac_gstate_set_font_options (cr->gstate, &scaled_font->options);

    if (was_previous)
	cr->gstate->scaled_font = comac_scaled_font_reference (scaled_font);

    return COMAC_STATUS_SUCCESS;
}

static comac_scaled_font_t *
_comac_default_context_get_scaled_font (void *abstract_cr)
{
    comac_default_context_t *cr = abstract_cr;
    comac_scaled_font_t *scaled_font;
    comac_status_t status;

    status = _comac_gstate_get_scaled_font (cr->gstate, &scaled_font);
    if (unlikely (status))
	return _comac_scaled_font_create_in_error (status);

    return scaled_font;
}

static comac_status_t
_comac_default_context_glyphs (void *abstract_cr,
			       const comac_glyph_t *glyphs,
			       int num_glyphs,
			       comac_glyph_text_info_t *info)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_show_text_glyphs (cr->gstate,
					   glyphs,
					   num_glyphs,
					   info);
}

static comac_status_t
_comac_default_context_glyph_path (void *abstract_cr,
				   const comac_glyph_t *glyphs,
				   int num_glyphs)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_glyph_path (cr->gstate, glyphs, num_glyphs, cr->path);
}

static comac_status_t
_comac_default_context_glyph_extents (void *abstract_cr,
				      const comac_glyph_t *glyphs,
				      int num_glyphs,
				      comac_text_extents_t *extents)
{
    comac_default_context_t *cr = abstract_cr;

    return _comac_gstate_glyph_extents (cr->gstate,
					glyphs,
					num_glyphs,
					extents);
}

static const comac_backend_t _comac_default_context_backend = {
    COMAC_TYPE_DEFAULT,
    _comac_default_context_destroy,

    _comac_default_context_get_original_target,
    _comac_default_context_get_current_target,

    _comac_default_context_save,
    _comac_default_context_restore,

    _comac_default_context_push_group,
    _comac_default_context_pop_group,

    _comac_default_context_set_source_rgba,
    _comac_default_context_set_source_surface,
    _comac_default_context_set_source,
    _comac_default_context_get_source,

    _comac_default_context_set_antialias,
    _comac_default_context_set_dash,
    _comac_default_context_set_fill_rule,
    _comac_default_context_set_line_cap,
    _comac_default_context_set_line_join,
    _comac_default_context_set_line_width,
    _comac_default_context_set_hairline,
    _comac_default_context_set_miter_limit,
    _comac_default_context_set_opacity,
    _comac_default_context_set_operator,
    _comac_default_context_set_tolerance,
    _comac_default_context_get_antialias,
    _comac_default_context_get_dash,
    _comac_default_context_get_fill_rule,
    _comac_default_context_get_line_cap,
    _comac_default_context_get_line_join,
    _comac_default_context_get_line_width,
    _comac_default_context_get_hairline,
    _comac_default_context_get_miter_limit,
    _comac_default_context_get_opacity,
    _comac_default_context_get_operator,
    _comac_default_context_get_tolerance,

    _comac_default_context_translate,
    _comac_default_context_scale,
    _comac_default_context_rotate,
    _comac_default_context_transform,
    _comac_default_context_set_matrix,
    _comac_default_context_set_identity_matrix,
    _comac_default_context_get_matrix,

    _comac_default_context_user_to_device,
    _comac_default_context_user_to_device_distance,
    _comac_default_context_device_to_user,
    _comac_default_context_device_to_user_distance,

    _comac_default_context_user_to_backend,
    _comac_default_context_user_to_backend_distance,
    _comac_default_context_backend_to_user,
    _comac_default_context_backend_to_user_distance,

    _comac_default_context_new_path,
    _comac_default_context_new_sub_path,
    _comac_default_context_move_to,
    _comac_default_context_rel_move_to,
    _comac_default_context_line_to,
    _comac_default_context_rel_line_to,
    _comac_default_context_curve_to,
    _comac_default_context_rel_curve_to,
    NULL, /* arc-to */
    NULL, /* rel-arc-to */
    _comac_default_context_close_path,
    _comac_default_context_arc,
    _comac_default_context_rectangle,
    _comac_default_context_path_extents,
    _comac_default_context_has_current_point,
    _comac_default_context_get_current_point,
    _comac_default_context_copy_path,
    _comac_default_context_copy_path_flat,
    _comac_default_context_append_path,

    NULL, /* stroke-to-path */

    _comac_default_context_clip,
    _comac_default_context_clip_preserve,
    _comac_default_context_in_clip,
    _comac_default_context_clip_extents,
    _comac_default_context_reset_clip,
    _comac_default_context_copy_clip_rectangle_list,

    _comac_default_context_paint,
    _comac_default_context_paint_with_alpha,
    _comac_default_context_mask,

    _comac_default_context_stroke,
    _comac_default_context_stroke_preserve,
    _comac_default_context_in_stroke,
    _comac_default_context_stroke_extents,

    _comac_default_context_fill,
    _comac_default_context_fill_preserve,
    _comac_default_context_in_fill,
    _comac_default_context_fill_extents,

    _comac_default_context_set_font_face,
    _comac_default_context_get_font_face,
    _comac_default_context_set_font_size,
    _comac_default_context_set_font_matrix,
    _comac_default_context_get_font_matrix,
    _comac_default_context_set_font_options,
    _comac_default_context_get_font_options,
    _comac_default_context_set_scaled_font,
    _comac_default_context_get_scaled_font,
    _comac_default_context_font_extents,

    _comac_default_context_glyphs,
    _comac_default_context_glyph_path,
    _comac_default_context_glyph_extents,

    _comac_default_context_copy_page,
    _comac_default_context_show_page,

    _comac_default_context_tag_begin,
    _comac_default_context_tag_end,
};

comac_status_t
_comac_default_context_init (comac_default_context_t *cr, void *target)
{
    _comac_init (&cr->base, &_comac_default_context_backend);
    _comac_path_fixed_init (cr->path);

    cr->gstate = &cr->gstate_tail[0];
    cr->gstate_freelist = &cr->gstate_tail[1];
    cr->gstate_tail[1].next = NULL;

    return _comac_gstate_init (cr->gstate, target);
}

comac_t *
_comac_default_context_create (void *target)
{
    comac_default_context_t *cr;
    comac_status_t status;

    cr = _freed_pool_get (&context_pool);
    if (unlikely (cr == NULL)) {
	cr = _comac_malloc (sizeof (comac_default_context_t));
	if (unlikely (cr == NULL))
	    return _comac_create_in_error (
		_comac_error (COMAC_STATUS_NO_MEMORY));
    }

    status = _comac_default_context_init (cr, target);
    if (unlikely (status)) {
	_freed_pool_put (&context_pool, cr);
	return _comac_create_in_error (status);
    }

    return &cr->base;
}
