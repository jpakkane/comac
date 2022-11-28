/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2004 David Reveman
 * Copyright © 2005 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of David
 * Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. David Reveman makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: David Reveman <davidr@novell.com>
 *	    Keith Packard <keithp@keithp.com>
 *	    Carl Worth <cworth@cworth.org>
 */

#include "comacint.h"

#include "comac-array-private.h"
#include "comac-error-private.h"
#include "comac-freed-pool-private.h"
#include "comac-image-surface-private.h"
#include "comac-list-inline.h"
#include "comac-path-private.h"
#include "comac-pattern-private.h"
#include "comac-recording-surface-inline.h"
#include "comac-surface-snapshot-inline.h"

#include <float.h>

#define PIXMAN_MAX_INT                                                         \
    ((pixman_fixed_1 >> 1) -                                                   \
     pixman_fixed_e) /* need to ensure deltas also fit */

/**
 * SECTION:comac-pattern
 * @Title: comac_pattern_t
 * @Short_Description: Sources for drawing
 * @See_Also: #comac_t, #comac_surface_t
 *
 * #comac_pattern_t is the paint with which comac draws.
 * The primary use of patterns is as the source for all comac drawing
 * operations, although they can also be used as masks, that is, as the
 * brush too.
 *
 * A comac pattern is created by using one of the many constructors,
 * of the form
 * <function>comac_pattern_create_<emphasis>type</emphasis>()</function>
 * or implicitly through
 * <function>comac_set_source_<emphasis>type</emphasis>()</function>
 * functions.
 **/

static freed_pool_t freed_pattern_pool[5];

static const comac_solid_pattern_t _comac_pattern_nil = {{
    COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
    COMAC_STATUS_NO_MEMORY,	   /* status */
    {0, 0, 0, NULL},		   /* user_data */
    {NULL, NULL},		   /* observers */

    COMAC_PATTERN_TYPE_SOLID,	   /* type */
    COMAC_FILTER_DEFAULT,	   /* filter */
    COMAC_EXTEND_GRADIENT_DEFAULT, /* extend */
    FALSE,			   /* has component alpha */
    FALSE,			   /* is_userfont_foreground */
    {
	1.,
	0.,
	0.,
	1.,
	0.,
	0.,
    },	/* matrix */
    1.0 /* opacity */
}};

static const comac_solid_pattern_t _comac_pattern_nil_null_pointer = {
    {
	COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
	COMAC_STATUS_NULL_POINTER,     /* status */
	{0, 0, 0, NULL},	       /* user_data */
	{NULL, NULL},		       /* observers */

	COMAC_PATTERN_TYPE_SOLID,      /* type */
	COMAC_FILTER_DEFAULT,	       /* filter */
	COMAC_EXTEND_GRADIENT_DEFAULT, /* extend */
	FALSE,			       /* has component alpha */
	FALSE,			       /* is_userfont_foreground */
	{
	    1.,
	    0.,
	    0.,
	    1.,
	    0.,
	    0.,
	},  /* matrix */
	1.0 /* opacity */
    },
    {COMAC_COLORSPACE_RGB, {{}}}};

const comac_solid_pattern_t _comac_pattern_black = {
    {
	COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
	COMAC_STATUS_SUCCESS,	       /* status */
	{0, 0, 0, NULL},	       /* user_data */
	{NULL, NULL},		       /* observers */

	COMAC_PATTERN_TYPE_SOLID, /* type */
	COMAC_FILTER_NEAREST,	  /* filter */
	COMAC_EXTEND_REPEAT,	  /* extend */
	FALSE,			  /* has component alpha */
	FALSE,			  /* is_userfont_foreground */
	{
	    1.,
	    0.,
	    0.,
	    1.,
	    0.,
	    0.,
	},  /* matrix */
	1.0 /* opacity */
    },
    {COMAC_COLORSPACE_RGB,
     {{0., 0., 0., 1., 0, 0, 0, 0xffff}}}, /* color (double rgba, short rgba) */
};

const comac_solid_pattern_t _comac_pattern_clear = {
    {
	COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
	COMAC_STATUS_SUCCESS,	       /* status */
	{0, 0, 0, NULL},	       /* user_data */
	{NULL, NULL},		       /* observers */

	COMAC_PATTERN_TYPE_SOLID, /* type */
	COMAC_FILTER_NEAREST,	  /* filter */
	COMAC_EXTEND_REPEAT,	  /* extend */
	FALSE,			  /* has component alpha */
	FALSE,			  /* is_userfont_foreground */
	{
	    1.,
	    0.,
	    0.,
	    1.,
	    0.,
	    0.,
	},  /* matrix */
	1.0 /* opacity */
    },
    {COMAC_COLORSPACE_RGB,
     {{0., 0., 0., 0., 0, 0, 0, 0}}}, /* color (double rgba, short rgba) */
};

const comac_solid_pattern_t _comac_pattern_white = {
    {
	COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
	COMAC_STATUS_SUCCESS,	       /* status */
	{0, 0, 0, NULL},	       /* user_data */
	{NULL, NULL},		       /* observers */

	COMAC_PATTERN_TYPE_SOLID, /* type */
	COMAC_FILTER_NEAREST,	  /* filter */
	COMAC_EXTEND_REPEAT,	  /* extend */
	FALSE,			  /* has component alpha */
	FALSE,			  /* is_userfont_foreground */
	{
	    1.,
	    0.,
	    0.,
	    1.,
	    0.,
	    0.,
	},  /* matrix */
	1.0 /* opacity */
    },
    {COMAC_COLORSPACE_RGB,
     {{1.,
       1.,
       1.,
       1.,
       0xffff,
       0xffff,
       0xffff,
       0xffff}}}, /* color (double rgba, short rgba) */
};

static void
_comac_pattern_notify_observers (comac_pattern_t *pattern, unsigned int flags)
{
    comac_pattern_observer_t *pos;

    comac_list_foreach_entry (pos,
			      comac_pattern_observer_t,
			      &pattern->observers,
			      link) pos->notify (pos, pattern, flags);
}

/**
 * _comac_pattern_set_error:
 * @pattern: a pattern
 * @status: a status value indicating an error
 *
 * Atomically sets pattern->status to @status and calls _comac_error;
 * Does nothing if status is %COMAC_STATUS_SUCCESS.
 *
 * All assignments of an error status to pattern->status should happen
 * through _comac_pattern_set_error(). Note that due to the nature of
 * the atomic operation, it is not safe to call this function on the nil
 * objects.
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _comac_error() to generate a stack trace for when the
 * user causes comac to detect an error.
 **/
static comac_status_t
_comac_pattern_set_error (comac_pattern_t *pattern, comac_status_t status)
{
    if (status == COMAC_STATUS_SUCCESS)
	return status;

    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. */
    _comac_status_set_error (&pattern->status, status);

    return _comac_error (status);
}

void
_comac_pattern_init (comac_pattern_t *pattern, comac_pattern_type_t type)
{
#if HAVE_VALGRIND
    switch (type) {
    case COMAC_PATTERN_TYPE_SOLID:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_solid_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_surface_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_linear_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_radial_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_MESH:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_mesh_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	break;
    }
#endif

    pattern->type = type;
    pattern->status = COMAC_STATUS_SUCCESS;

    /* Set the reference count to zero for on-stack patterns.
     * Callers needs to explicitly increment the count for heap allocations. */
    COMAC_REFERENCE_COUNT_INIT (&pattern->ref_count, 0);

    _comac_user_data_array_init (&pattern->user_data);

    if (type == COMAC_PATTERN_TYPE_SURFACE ||
	type == COMAC_PATTERN_TYPE_RASTER_SOURCE)
	pattern->extend = COMAC_EXTEND_SURFACE_DEFAULT;
    else
	pattern->extend = COMAC_EXTEND_GRADIENT_DEFAULT;

    pattern->filter = COMAC_FILTER_DEFAULT;
    pattern->opacity = 1.0;

    pattern->has_component_alpha = FALSE;
    pattern->is_userfont_foreground = FALSE;

    comac_matrix_init_identity (&pattern->matrix);

    comac_list_init (&pattern->observers);
}

static comac_status_t
_comac_gradient_pattern_init_copy (comac_gradient_pattern_t *pattern,
				   const comac_gradient_pattern_t *other)
{
    if (COMAC_INJECT_FAULT ())
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    if (other->base.type == COMAC_PATTERN_TYPE_LINEAR) {
	comac_linear_pattern_t *dst = (comac_linear_pattern_t *) pattern;
	comac_linear_pattern_t *src = (comac_linear_pattern_t *) other;

	*dst = *src;
    } else {
	comac_radial_pattern_t *dst = (comac_radial_pattern_t *) pattern;
	comac_radial_pattern_t *src = (comac_radial_pattern_t *) other;

	*dst = *src;
    }

    if (other->stops == other->stops_embedded)
	pattern->stops = pattern->stops_embedded;
    else if (other->stops) {
	pattern->stops = _comac_malloc_ab (other->stops_size,
					   sizeof (comac_gradient_stop_t));
	if (unlikely (pattern->stops == NULL)) {
	    pattern->stops_size = 0;
	    pattern->n_stops = 0;
	    return _comac_pattern_set_error (&pattern->base,
					     COMAC_STATUS_NO_MEMORY);
	}

	memcpy (pattern->stops,
		other->stops,
		other->n_stops * sizeof (comac_gradient_stop_t));
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_mesh_pattern_init_copy (comac_mesh_pattern_t *pattern,
			       const comac_mesh_pattern_t *other)
{
    *pattern = *other;

    _comac_array_init (&pattern->patches, sizeof (comac_mesh_patch_t));
    return _comac_array_append_multiple (
	&pattern->patches,
	_comac_array_index_const (&other->patches, 0),
	_comac_array_num_elements (&other->patches));
}

comac_status_t
_comac_pattern_init_copy (comac_pattern_t *pattern,
			  const comac_pattern_t *other)
{
    comac_status_t status;

    if (other->status)
	return _comac_pattern_set_error (pattern, other->status);

    switch (other->type) {
    case COMAC_PATTERN_TYPE_SOLID: {
	comac_solid_pattern_t *dst = (comac_solid_pattern_t *) pattern;
	comac_solid_pattern_t *src = (comac_solid_pattern_t *) other;

	VG (VALGRIND_MAKE_MEM_UNDEFINED (pattern,
					 sizeof (comac_solid_pattern_t)));

	*dst = *src;
    } break;
    case COMAC_PATTERN_TYPE_SURFACE: {
	comac_surface_pattern_t *dst = (comac_surface_pattern_t *) pattern;
	comac_surface_pattern_t *src = (comac_surface_pattern_t *) other;

	VG (VALGRIND_MAKE_MEM_UNDEFINED (pattern,
					 sizeof (comac_surface_pattern_t)));

	*dst = *src;
	comac_surface_reference (dst->surface);
    } break;
    case COMAC_PATTERN_TYPE_LINEAR:
    case COMAC_PATTERN_TYPE_RADIAL: {
	comac_gradient_pattern_t *dst = (comac_gradient_pattern_t *) pattern;
	comac_gradient_pattern_t *src = (comac_gradient_pattern_t *) other;

	if (other->type == COMAC_PATTERN_TYPE_LINEAR) {
	    VG (VALGRIND_MAKE_MEM_UNDEFINED (pattern,
					     sizeof (comac_linear_pattern_t)));
	} else {
	    VG (VALGRIND_MAKE_MEM_UNDEFINED (pattern,
					     sizeof (comac_radial_pattern_t)));
	}

	status = _comac_gradient_pattern_init_copy (dst, src);
	if (unlikely (status))
	    return status;

    } break;
    case COMAC_PATTERN_TYPE_MESH: {
	comac_mesh_pattern_t *dst = (comac_mesh_pattern_t *) pattern;
	comac_mesh_pattern_t *src = (comac_mesh_pattern_t *) other;

	VG (VALGRIND_MAKE_MEM_UNDEFINED (pattern,
					 sizeof (comac_mesh_pattern_t)));

	status = _comac_mesh_pattern_init_copy (dst, src);
	if (unlikely (status))
	    return status;

    } break;

    case COMAC_PATTERN_TYPE_RASTER_SOURCE: {
	status = _comac_raster_source_pattern_init_copy (pattern, other);
	if (unlikely (status))
	    return status;
    } break;
    }

    /* The reference count and user_data array are unique to the copy. */
    COMAC_REFERENCE_COUNT_INIT (&pattern->ref_count, 0);
    _comac_user_data_array_init (&pattern->user_data);
    comac_list_init (&pattern->observers);

    return COMAC_STATUS_SUCCESS;
}

void
_comac_pattern_init_static_copy (comac_pattern_t *pattern,
				 const comac_pattern_t *other)
{
    int size;

    assert (other->status == COMAC_STATUS_SUCCESS);

    switch (other->type) {
    default:
	ASSERT_NOT_REACHED;
    case COMAC_PATTERN_TYPE_SOLID:
	size = sizeof (comac_solid_pattern_t);
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	size = sizeof (comac_surface_pattern_t);
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	size = sizeof (comac_linear_pattern_t);
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	size = sizeof (comac_radial_pattern_t);
	break;
    case COMAC_PATTERN_TYPE_MESH:
	size = sizeof (comac_mesh_pattern_t);
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	size = sizeof (comac_raster_source_pattern_t);
	break;
    }

    memcpy (pattern, other, size);

    COMAC_REFERENCE_COUNT_INIT (&pattern->ref_count, 0);
    _comac_user_data_array_init (&pattern->user_data);
    comac_list_init (&pattern->observers);
}

comac_status_t
_comac_pattern_init_snapshot (comac_pattern_t *pattern,
			      const comac_pattern_t *other)
{
    comac_status_t status;

    /* We don't bother doing any fancy copy-on-write implementation
     * for the pattern's data. It's generally quite tiny. */
    status = _comac_pattern_init_copy (pattern, other);
    if (unlikely (status))
	return status;

    /* But we do let the surface snapshot stuff be as fancy as it
     * would like to be. */
    if (pattern->type == COMAC_PATTERN_TYPE_SURFACE) {
	comac_surface_pattern_t *surface_pattern =
	    (comac_surface_pattern_t *) pattern;
	comac_surface_t *surface = surface_pattern->surface;

	surface_pattern->surface = _comac_surface_snapshot (surface);

	comac_surface_destroy (surface);

	status = surface_pattern->surface->status;
    } else if (pattern->type == COMAC_PATTERN_TYPE_RASTER_SOURCE)
	status = _comac_raster_source_pattern_snapshot (pattern);

    return status;
}

void
_comac_pattern_fini (comac_pattern_t *pattern)
{
    _comac_user_data_array_fini (&pattern->user_data);

    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	break;
    case COMAC_PATTERN_TYPE_SURFACE: {
	comac_surface_pattern_t *surface_pattern =
	    (comac_surface_pattern_t *) pattern;

	comac_surface_destroy (surface_pattern->surface);
    } break;
    case COMAC_PATTERN_TYPE_LINEAR:
    case COMAC_PATTERN_TYPE_RADIAL: {
	comac_gradient_pattern_t *gradient =
	    (comac_gradient_pattern_t *) pattern;

	if (gradient->stops && gradient->stops != gradient->stops_embedded)
	    free (gradient->stops);
    } break;
    case COMAC_PATTERN_TYPE_MESH: {
	comac_mesh_pattern_t *mesh = (comac_mesh_pattern_t *) pattern;

	_comac_array_fini (&mesh->patches);
    } break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	_comac_raster_source_pattern_finish (pattern);
	break;
    }

#if HAVE_VALGRIND
    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_solid_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_surface_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_linear_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_radial_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_MESH:
	VALGRIND_MAKE_MEM_UNDEFINED (pattern, sizeof (comac_mesh_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	break;
    }
#endif
}

comac_status_t
_comac_pattern_create_copy (comac_pattern_t **pattern_out,
			    const comac_pattern_t *other)
{
    comac_pattern_t *pattern;
    comac_status_t status;

    if (other->status)
	return other->status;

    switch (other->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	pattern = _comac_malloc (sizeof (comac_solid_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	pattern = _comac_malloc (sizeof (comac_surface_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	pattern = _comac_malloc (sizeof (comac_linear_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	pattern = _comac_malloc (sizeof (comac_radial_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_MESH:
	pattern = _comac_malloc (sizeof (comac_mesh_pattern_t));
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	pattern = _comac_malloc (sizeof (comac_raster_source_pattern_t));
	break;
    default:
	ASSERT_NOT_REACHED;
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);
    }
    if (unlikely (pattern == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = _comac_pattern_init_copy (pattern, other);
    if (unlikely (status)) {
	free (pattern);
	return status;
    }

    COMAC_REFERENCE_COUNT_INIT (&pattern->ref_count, 1);
    *pattern_out = pattern;
    return COMAC_STATUS_SUCCESS;
}

void
_comac_pattern_init_solid (comac_solid_pattern_t *pattern,
			   const comac_color_t *color)
{
    _comac_pattern_init (&pattern->base, COMAC_PATTERN_TYPE_SOLID);
    pattern->color = *color;
}

void
_comac_pattern_init_for_surface (comac_surface_pattern_t *pattern,
				 comac_surface_t *surface)
{
    if (surface->status) {
	/* Force to solid to simplify the pattern_fini process. */
	_comac_pattern_init (&pattern->base, COMAC_PATTERN_TYPE_SOLID);
	_comac_pattern_set_error (&pattern->base, surface->status);
	return;
    }

    _comac_pattern_init (&pattern->base, COMAC_PATTERN_TYPE_SURFACE);

    pattern->surface = comac_surface_reference (surface);
}

static void
_comac_pattern_init_gradient (comac_gradient_pattern_t *pattern,
			      comac_pattern_type_t type)
{
    _comac_pattern_init (&pattern->base, type);

    pattern->n_stops = 0;
    pattern->stops_size = 0;
    pattern->stops = NULL;
}

static void
_comac_pattern_init_linear (
    comac_linear_pattern_t *pattern, double x0, double y0, double x1, double y1)
{
    _comac_pattern_init_gradient (&pattern->base, COMAC_PATTERN_TYPE_LINEAR);

    pattern->pd1.x = x0;
    pattern->pd1.y = y0;
    pattern->pd2.x = x1;
    pattern->pd2.y = y1;
}

static void
_comac_pattern_init_radial (comac_radial_pattern_t *pattern,
			    double cx0,
			    double cy0,
			    double radius0,
			    double cx1,
			    double cy1,
			    double radius1)
{
    _comac_pattern_init_gradient (&pattern->base, COMAC_PATTERN_TYPE_RADIAL);

    pattern->cd1.center.x = cx0;
    pattern->cd1.center.y = cy0;
    pattern->cd1.radius = fabs (radius0);
    pattern->cd2.center.x = cx1;
    pattern->cd2.center.y = cy1;
    pattern->cd2.radius = fabs (radius1);
}

comac_pattern_t *
_comac_pattern_create_solid (const comac_color_t *color)
{
    comac_solid_pattern_t *pattern;

    pattern = _freed_pool_get (&freed_pattern_pool[COMAC_PATTERN_TYPE_SOLID]);
    if (unlikely (pattern == NULL)) {
	/* None cached, need to create a new pattern. */
	pattern = _comac_malloc (sizeof (comac_solid_pattern_t));
	if (unlikely (pattern == NULL)) {
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return (comac_pattern_t *) &_comac_pattern_nil;
	}
    }

    _comac_pattern_init_solid (pattern, color);
    COMAC_REFERENCE_COUNT_INIT (&pattern->base.ref_count, 1);

    return &pattern->base;
}

comac_pattern_t *
_comac_pattern_create_in_error (comac_status_t status)
{
    comac_pattern_t *pattern;

    if (status == COMAC_STATUS_NO_MEMORY)
	return (comac_pattern_t *) &_comac_pattern_nil.base;

    COMAC_MUTEX_INITIALIZE ();

    pattern = _comac_pattern_create_solid (COMAC_COLOR_BLACK);
    if (pattern->status == COMAC_STATUS_SUCCESS)
	status = _comac_pattern_set_error (pattern, status);

    return pattern;
}

/**
 * comac_pattern_create_rgb:
 * @red: red component of the color
 * @green: green component of the color
 * @blue: blue component of the color
 *
 * Creates a new #comac_pattern_t corresponding to an opaque color.  The
 * color components are floating point numbers in the range 0 to 1.
 * If the values passed in are outside that range, they will be
 * clamped.
 *
 * Return value: the newly created #comac_pattern_t if successful, or
 * an error pattern in case of no memory.  The caller owns the
 * returned object and should call comac_pattern_destroy() when
 * finished with it.
 *
 * This function will always return a valid pointer, but if an error
 * occurred the pattern status will be set to an error.  To inspect
 * the status of a pattern use comac_pattern_status().
 *
 * Since: 1.0
 **/
comac_pattern_t *
comac_pattern_create_rgb (double red, double green, double blue)
{
    return comac_pattern_create_rgba (red, green, blue, 1.0);
}

/**
 * comac_pattern_create_rgba:
 * @red: red component of the color
 * @green: green component of the color
 * @blue: blue component of the color
 * @alpha: alpha component of the color
 *
 * Creates a new #comac_pattern_t corresponding to a translucent color.
 * The color components are floating point numbers in the range 0 to
 * 1.  If the values passed in are outside that range, they will be
 * clamped.
 *
 * Return value: the newly created #comac_pattern_t if successful, or
 * an error pattern in case of no memory.  The caller owns the
 * returned object and should call comac_pattern_destroy() when
 * finished with it.
 *
 * This function will always return a valid pointer, but if an error
 * occurred the pattern status will be set to an error.  To inspect
 * the status of a pattern use comac_pattern_status().
 *
 * Since: 1.0
 **/
comac_pattern_t *
comac_pattern_create_rgba (double red, double green, double blue, double alpha)
{
    comac_color_t color;

    red = _comac_restrict_value (red, 0.0, 1.0);
    green = _comac_restrict_value (green, 0.0, 1.0);
    blue = _comac_restrict_value (blue, 0.0, 1.0);
    alpha = _comac_restrict_value (alpha, 0.0, 1.0);

    _comac_color_init_rgba (&color, red, green, blue, alpha);

    COMAC_MUTEX_INITIALIZE ();

    return _comac_pattern_create_solid (&color);
}

/**
 * comac_pattern_create_for_surface:
 * @surface: the surface
 *
 * Create a new #comac_pattern_t for the given surface.
 *
 * Return value: the newly created #comac_pattern_t if successful, or
 * an error pattern in case of no memory.  The caller owns the
 * returned object and should call comac_pattern_destroy() when
 * finished with it.
 *
 * This function will always return a valid pointer, but if an error
 * occurred the pattern status will be set to an error.  To inspect
 * the status of a pattern use comac_pattern_status().
 *
 * Since: 1.0
 **/
comac_pattern_t *
comac_pattern_create_for_surface (comac_surface_t *surface)
{
    comac_surface_pattern_t *pattern;

    if (surface == NULL) {
	_comac_error_throw (COMAC_STATUS_NULL_POINTER);
	return (comac_pattern_t *) &_comac_pattern_nil_null_pointer;
    }

    if (surface->status)
	return _comac_pattern_create_in_error (surface->status);

    pattern = _freed_pool_get (&freed_pattern_pool[COMAC_PATTERN_TYPE_SURFACE]);
    if (unlikely (pattern == NULL)) {
	pattern = _comac_malloc (sizeof (comac_surface_pattern_t));
	if (unlikely (pattern == NULL)) {
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return (comac_pattern_t *) &_comac_pattern_nil.base;
	}
    }

    COMAC_MUTEX_INITIALIZE ();

    _comac_pattern_init_for_surface (pattern, surface);
    COMAC_REFERENCE_COUNT_INIT (&pattern->base.ref_count, 1);

    return &pattern->base;
}

/**
 * comac_pattern_create_linear:
 * @x0: x coordinate of the start point
 * @y0: y coordinate of the start point
 * @x1: x coordinate of the end point
 * @y1: y coordinate of the end point
 *
 * Create a new linear gradient #comac_pattern_t along the line defined
 * by (x0, y0) and (x1, y1).  Before using the gradient pattern, a
 * number of color stops should be defined using
 * comac_pattern_add_color_stop_rgb() or
 * comac_pattern_add_color_stop_rgba().
 *
 * Note: The coordinates here are in pattern space. For a new pattern,
 * pattern space is identical to user space, but the relationship
 * between the spaces can be changed with comac_pattern_set_matrix().
 *
 * Return value: the newly created #comac_pattern_t if successful, or
 * an error pattern in case of no memory.  The caller owns the
 * returned object and should call comac_pattern_destroy() when
 * finished with it.
 *
 * This function will always return a valid pointer, but if an error
 * occurred the pattern status will be set to an error.  To inspect
 * the status of a pattern use comac_pattern_status().
 *
 * Since: 1.0
 **/
comac_pattern_t *
comac_pattern_create_linear (double x0, double y0, double x1, double y1)
{
    comac_linear_pattern_t *pattern;

    pattern = _freed_pool_get (&freed_pattern_pool[COMAC_PATTERN_TYPE_LINEAR]);
    if (unlikely (pattern == NULL)) {
	pattern = _comac_malloc (sizeof (comac_linear_pattern_t));
	if (unlikely (pattern == NULL)) {
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return (comac_pattern_t *) &_comac_pattern_nil.base;
	}
    }

    COMAC_MUTEX_INITIALIZE ();

    _comac_pattern_init_linear (pattern, x0, y0, x1, y1);
    COMAC_REFERENCE_COUNT_INIT (&pattern->base.base.ref_count, 1);

    return &pattern->base.base;
}

/**
 * comac_pattern_create_radial:
 * @cx0: x coordinate for the center of the start circle
 * @cy0: y coordinate for the center of the start circle
 * @radius0: radius of the start circle
 * @cx1: x coordinate for the center of the end circle
 * @cy1: y coordinate for the center of the end circle
 * @radius1: radius of the end circle
 *
 * Creates a new radial gradient #comac_pattern_t between the two
 * circles defined by (cx0, cy0, radius0) and (cx1, cy1, radius1).  Before using the
 * gradient pattern, a number of color stops should be defined using
 * comac_pattern_add_color_stop_rgb() or
 * comac_pattern_add_color_stop_rgba().
 *
 * Note: The coordinates here are in pattern space. For a new pattern,
 * pattern space is identical to user space, but the relationship
 * between the spaces can be changed with comac_pattern_set_matrix().
 *
 * Return value: the newly created #comac_pattern_t if successful, or
 * an error pattern in case of no memory.  The caller owns the
 * returned object and should call comac_pattern_destroy() when
 * finished with it.
 *
 * This function will always return a valid pointer, but if an error
 * occurred the pattern status will be set to an error.  To inspect
 * the status of a pattern use comac_pattern_status().
 *
 * Since: 1.0
 **/
comac_pattern_t *
comac_pattern_create_radial (double cx0,
			     double cy0,
			     double radius0,
			     double cx1,
			     double cy1,
			     double radius1)
{
    comac_radial_pattern_t *pattern;

    pattern = _freed_pool_get (&freed_pattern_pool[COMAC_PATTERN_TYPE_RADIAL]);
    if (unlikely (pattern == NULL)) {
	pattern = _comac_malloc (sizeof (comac_radial_pattern_t));
	if (unlikely (pattern == NULL)) {
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return (comac_pattern_t *) &_comac_pattern_nil.base;
	}
    }

    COMAC_MUTEX_INITIALIZE ();

    _comac_pattern_init_radial (pattern, cx0, cy0, radius0, cx1, cy1, radius1);
    COMAC_REFERENCE_COUNT_INIT (&pattern->base.base.ref_count, 1);

    return &pattern->base.base;
}

/* This order is specified in the diagram in the documentation for
 * comac_pattern_create_mesh() */
static const int mesh_path_point_i[12] = {0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 2, 1};
static const int mesh_path_point_j[12] = {0, 1, 2, 3, 3, 3, 3, 2, 1, 0, 0, 0};
static const int mesh_control_point_i[4] = {1, 1, 2, 2};
static const int mesh_control_point_j[4] = {1, 2, 2, 1};

/**
 * comac_pattern_create_mesh:
 *
 * Create a new mesh pattern.
 *
 * Mesh patterns are tensor-product patch meshes (type 7 shadings in
 * PDF). Mesh patterns may also be used to create other types of
 * shadings that are special cases of tensor-product patch meshes such
 * as Coons patch meshes (type 6 shading in PDF) and Gouraud-shaded
 * triangle meshes (type 4 and 5 shadings in PDF).
 *
 * Mesh patterns consist of one or more tensor-product patches, which
 * should be defined before using the mesh pattern. Using a mesh
 * pattern with a partially defined patch as source or mask will put
 * the context in an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * A tensor-product patch is defined by 4 Bézier curves (side 0, 1, 2,
 * 3) and by 4 additional control points (P0, P1, P2, P3) that provide
 * further control over the patch and complete the definition of the
 * tensor-product patch. The corner C0 is the first point of the
 * patch.
 *
 * Degenerate sides are permitted so straight lines may be used. A
 * zero length line on one side may be used to create 3 sided patches.
 *
 * <informalexample><screen>
 *       C1     Side 1       C2
 *        +---------------+
 *        |               |
 *        |  P1       P2  |
 *        |               |
 * Side 0 |               | Side 2
 *        |               |
 *        |               |
 *        |  P0       P3  |
 *        |               |
 *        +---------------+
 *      C0     Side 3        C3
 * </screen></informalexample>
 *
 * Each patch is constructed by first calling
 * comac_mesh_pattern_begin_patch(), then comac_mesh_pattern_move_to()
 * to specify the first point in the patch (C0). Then the sides are
 * specified with calls to comac_mesh_pattern_curve_to() and
 * comac_mesh_pattern_line_to().
 *
 * The four additional control points (P0, P1, P2, P3) in a patch can
 * be specified with comac_mesh_pattern_set_control_point().
 *
 * At each corner of the patch (C0, C1, C2, C3) a color may be
 * specified with comac_mesh_pattern_set_corner_color_rgb() or
 * comac_mesh_pattern_set_corner_color_rgba(). Any corner whose color
 * is not explicitly specified defaults to transparent black.
 *
 * A Coons patch is a special case of the tensor-product patch where
 * the control points are implicitly defined by the sides of the
 * patch. The default value for any control point not specified is the
 * implicit value for a Coons patch, i.e. if no control points are
 * specified the patch is a Coons patch.
 *
 * A triangle is a special case of the tensor-product patch where the
 * control points are implicitly defined by the sides of the patch,
 * all the sides are lines and one of them has length 0, i.e. if the
 * patch is specified using just 3 lines, it is a triangle. If the
 * corners connected by the 0-length side have the same color, the
 * patch is a Gouraud-shaded triangle.
 *
 * Patches may be oriented differently to the above diagram. For
 * example the first point could be at the top left. The diagram only
 * shows the relationship between the sides, corners and control
 * points. Regardless of where the first point is located, when
 * specifying colors, corner 0 will always be the first point, corner
 * 1 the point between side 0 and side 1 etc.
 *
 * Calling comac_mesh_pattern_end_patch() completes the current
 * patch. If less than 4 sides have been defined, the first missing
 * side is defined as a line from the current point to the first point
 * of the patch (C0) and the other sides are degenerate lines from C0
 * to C0. The corners between the added sides will all be coincident
 * with C0 of the patch and their color will be set to be the same as
 * the color of C0.
 *
 * Additional patches may be added with additional calls to
 * comac_mesh_pattern_begin_patch()/comac_mesh_pattern_end_patch().
 *
 * <informalexample><programlisting>
 * comac_pattern_t *pattern = comac_pattern_create_mesh ();
 *
 * /&ast; Add a Coons patch &ast;/
 * comac_mesh_pattern_begin_patch (pattern);
 * comac_mesh_pattern_move_to (pattern, 0, 0);
 * comac_mesh_pattern_curve_to (pattern, 30, -30,  60,  30, 100, 0);
 * comac_mesh_pattern_curve_to (pattern, 60,  30, 130,  60, 100, 100);
 * comac_mesh_pattern_curve_to (pattern, 60,  70,  30, 130,   0, 100);
 * comac_mesh_pattern_curve_to (pattern, 30,  70, -30,  30,   0, 0);
 * comac_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0);
 * comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0);
 * comac_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1);
 * comac_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0);
 * comac_mesh_pattern_end_patch (pattern);
 *
 * /&ast; Add a Gouraud-shaded triangle &ast;/
 * comac_mesh_pattern_begin_patch (pattern)
 * comac_mesh_pattern_move_to (pattern, 100, 100);
 * comac_mesh_pattern_line_to (pattern, 130, 130);
 * comac_mesh_pattern_line_to (pattern, 130,  70);
 * comac_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0);
 * comac_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0);
 * comac_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1);
 * comac_mesh_pattern_end_patch (pattern)
 * </programlisting></informalexample>
 *
 * When two patches overlap, the last one that has been added is drawn
 * over the first one.
 *
 * When a patch folds over itself, points are sorted depending on
 * their parameter coordinates inside the patch. The v coordinate
 * ranges from 0 to 1 when moving from side 3 to side 1; the u
 * coordinate ranges from 0 to 1 when going from side 0 to side
 * 2. Points with higher v coordinate hide points with lower v
 * coordinate. When two points have the same v coordinate, the one
 * with higher u coordinate is above. This means that points nearer to
 * side 1 are above points nearer to side 3; when this is not
 * sufficient to decide which point is above (for example when both
 * points belong to side 1 or side 3) points nearer to side 2 are
 * above points nearer to side 0.
 *
 * For a complete definition of tensor-product patches, see the PDF
 * specification (ISO32000), which describes the parametrization in
 * detail.
 *
 * Note: The coordinates are always in pattern space. For a new
 * pattern, pattern space is identical to user space, but the
 * relationship between the spaces can be changed with
 * comac_pattern_set_matrix().
 *
 * Return value: the newly created #comac_pattern_t if successful, or
 * an error pattern in case of no memory. The caller owns the returned
 * object and should call comac_pattern_destroy() when finished with
 * it.
 *
 * This function will always return a valid pointer, but if an error
 * occurred the pattern status will be set to an error. To inspect the
 * status of a pattern use comac_pattern_status().
 *
 * Since: 1.12
 **/
comac_pattern_t *
comac_pattern_create_mesh (void)
{
    comac_mesh_pattern_t *pattern;

    pattern = _freed_pool_get (&freed_pattern_pool[COMAC_PATTERN_TYPE_MESH]);
    if (unlikely (pattern == NULL)) {
	pattern = _comac_malloc (sizeof (comac_mesh_pattern_t));
	if (unlikely (pattern == NULL)) {
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return (comac_pattern_t *) &_comac_pattern_nil.base;
	}
    }

    COMAC_MUTEX_INITIALIZE ();

    _comac_pattern_init (&pattern->base, COMAC_PATTERN_TYPE_MESH);
    _comac_array_init (&pattern->patches, sizeof (comac_mesh_patch_t));
    pattern->current_patch = NULL;
    COMAC_REFERENCE_COUNT_INIT (&pattern->base.ref_count, 1);

    return &pattern->base;
}

/**
 * comac_pattern_reference:
 * @pattern: a #comac_pattern_t
 *
 * Increases the reference count on @pattern by one. This prevents
 * @pattern from being destroyed until a matching call to
 * comac_pattern_destroy() is made.
 *
 * Use comac_pattern_get_reference_count() to get the number of
 * references to a #comac_pattern_t.
 *
 * Return value: the referenced #comac_pattern_t.
 *
 * Since: 1.0
 **/
comac_pattern_t *
comac_pattern_reference (comac_pattern_t *pattern)
{
    if (pattern == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return pattern;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&pattern->ref_count));

    _comac_reference_count_inc (&pattern->ref_count);

    return pattern;
}

/**
 * comac_pattern_get_type:
 * @pattern: a #comac_pattern_t
 *
 * Get the pattern's type.  See #comac_pattern_type_t for available
 * types.
 *
 * Return value: The type of @pattern.
 *
 * Since: 1.2
 **/
comac_pattern_type_t
comac_pattern_get_type (comac_pattern_t *pattern)
{
    return pattern->type;
}

/**
 * comac_pattern_status:
 * @pattern: a #comac_pattern_t
 *
 * Checks whether an error has previously occurred for this
 * pattern.
 *
 * Return value: %COMAC_STATUS_SUCCESS, %COMAC_STATUS_NO_MEMORY,
 * %COMAC_STATUS_INVALID_MATRIX, %COMAC_STATUS_PATTERN_TYPE_MISMATCH,
 * or %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.0
 **/
comac_status_t
comac_pattern_status (comac_pattern_t *pattern)
{
    return pattern->status;
}

/**
 * comac_pattern_destroy:
 * @pattern: a #comac_pattern_t
 *
 * Decreases the reference count on @pattern by one. If the result is
 * zero, then @pattern and all associated resources are freed.  See
 * comac_pattern_reference().
 *
 * Since: 1.0
 **/
void
comac_pattern_destroy (comac_pattern_t *pattern)
{
    comac_pattern_type_t type;

    if (pattern == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&pattern->ref_count));

    if (! _comac_reference_count_dec_and_test (&pattern->ref_count))
	return;

    type = pattern->type;
    _comac_pattern_fini (pattern);

    /* maintain a small cache of freed patterns */
    if (type < ARRAY_LENGTH (freed_pattern_pool))
	_freed_pool_put (&freed_pattern_pool[type], pattern);
    else
	free (pattern);
}

/**
 * comac_pattern_get_reference_count:
 * @pattern: a #comac_pattern_t
 *
 * Returns the current reference count of @pattern.
 *
 * Return value: the current reference count of @pattern.  If the
 * object is a nil object, 0 will be returned.
 *
 * Since: 1.4
 **/
unsigned int
comac_pattern_get_reference_count (comac_pattern_t *pattern)
{
    if (pattern == NULL ||
	COMAC_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return 0;

    return COMAC_REFERENCE_COUNT_GET_VALUE (&pattern->ref_count);
}

/**
 * comac_pattern_get_user_data:
 * @pattern: a #comac_pattern_t
 * @key: the address of the #comac_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @pattern using the
 * specified key.  If no user data has been attached with the given
 * key this function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 *
 * Since: 1.4
 **/
void *
comac_pattern_get_user_data (comac_pattern_t *pattern,
			     const comac_user_data_key_t *key)
{
    return _comac_user_data_array_get_data (&pattern->user_data, key);
}

/**
 * comac_pattern_set_user_data:
 * @pattern: a #comac_pattern_t
 * @key: the address of a #comac_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the #comac_pattern_t
 * @destroy: a #comac_destroy_func_t which will be called when the
 * #comac_t is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @pattern.  To remove user data from a surface,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 *
 * Since: 1.4
 **/
comac_status_t
comac_pattern_set_user_data (comac_pattern_t *pattern,
			     const comac_user_data_key_t *key,
			     void *user_data,
			     comac_destroy_func_t destroy)
{
    if (COMAC_REFERENCE_COUNT_IS_INVALID (&pattern->ref_count))
	return pattern->status;

    return _comac_user_data_array_set_data (&pattern->user_data,
					    key,
					    user_data,
					    destroy);
}

/**
 * comac_mesh_pattern_begin_patch:
 * @pattern: a #comac_pattern_t
 *
 * Begin a patch in a mesh pattern.
 *
 * After calling this function, the patch shape should be defined with
 * comac_mesh_pattern_move_to(), comac_mesh_pattern_line_to() and
 * comac_mesh_pattern_curve_to().
 *
 * After defining the patch, comac_mesh_pattern_end_patch() must be
 * called before using @pattern as a source or mask.
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @pattern already has a
 * current patch, it will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_begin_patch (comac_pattern_t *pattern)
{
    comac_mesh_pattern_t *mesh;
    comac_status_t status;
    comac_mesh_patch_t *current_patch;
    int i;

    if (unlikely (pattern->status))
	return;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    mesh = (comac_mesh_pattern_t *) pattern;
    if (unlikely (mesh->current_patch)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    status =
	_comac_array_allocate (&mesh->patches, 1, (void **) &current_patch);
    if (unlikely (status)) {
	_comac_pattern_set_error (pattern, status);
	return;
    }

    mesh->current_patch = current_patch;
    mesh->current_side = -2; /* no current point */

    for (i = 0; i < 4; i++)
	mesh->has_control_point[i] = FALSE;

    for (i = 0; i < 4; i++)
	mesh->has_color[i] = FALSE;
}

static void
_calc_control_point (comac_mesh_patch_t *patch, int control_point)
{
    /* The Coons patch is a special case of the Tensor Product patch
     * where the four control points are:
     *
     * P11 = S(1/3, 1/3)
     * P12 = S(1/3, 2/3)
     * P21 = S(2/3, 1/3)
     * P22 = S(2/3, 2/3)
     *
     * where S is the gradient surface.
     *
     * When one or more control points has not been specified
     * calculated the Coons patch control points are substituted. If
     * no control points are specified the gradient will be a Coons
     * patch.
     *
     * The equations below are defined in the ISO32000 standard.
     */
    comac_point_double_t *p[3][3];
    int cp_i, cp_j, i, j;

    cp_i = mesh_control_point_i[control_point];
    cp_j = mesh_control_point_j[control_point];

    for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	    p[i][j] = &patch->points[cp_i ^ i][cp_j ^ j];

    p[0][0]->x = (-4 * p[1][1]->x + 6 * (p[1][0]->x + p[0][1]->x) -
		  2 * (p[1][2]->x + p[2][1]->x) +
		  3 * (p[2][0]->x + p[0][2]->x) - 1 * p[2][2]->x) *
		 (1. / 9);

    p[0][0]->y = (-4 * p[1][1]->y + 6 * (p[1][0]->y + p[0][1]->y) -
		  2 * (p[1][2]->y + p[2][1]->y) +
		  3 * (p[2][0]->y + p[0][2]->y) - 1 * p[2][2]->y) *
		 (1. / 9);
}

/**
 * comac_mesh_pattern_end_patch:
 * @pattern: a #comac_pattern_t
 *
 * Indicates the end of the current patch in a mesh pattern.
 *
 * If the current patch has less than 4 sides, it is closed with a
 * straight line from the current point to the first point of the
 * patch as if comac_mesh_pattern_line_to() was used.
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @pattern has no current
 * patch or the current patch has no current point, @pattern will be
 * put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_end_patch (comac_pattern_t *pattern)
{
    comac_mesh_pattern_t *mesh;
    comac_mesh_patch_t *current_patch;
    int i;

    if (unlikely (pattern->status))
	return;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    mesh = (comac_mesh_pattern_t *) pattern;
    current_patch = mesh->current_patch;
    if (unlikely (! current_patch)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    if (unlikely (mesh->current_side == -2)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    while (mesh->current_side < 3) {
	int corner_num;

	comac_mesh_pattern_line_to (pattern,
				    current_patch->points[0][0].x,
				    current_patch->points[0][0].y);

	corner_num = mesh->current_side + 1;
	if (corner_num < 4 && ! mesh->has_color[corner_num]) {
	    current_patch->colors[corner_num] = current_patch->colors[0];
	    mesh->has_color[corner_num] = TRUE;
	}
    }

    for (i = 0; i < 4; i++) {
	if (! mesh->has_control_point[i])
	    _calc_control_point (current_patch, i);
    }

    for (i = 0; i < 4; i++) {
	if (! mesh->has_color[i])
	    current_patch->colors[i] = *COMAC_COLOR_TRANSPARENT;
    }

    mesh->current_patch = NULL;
}

/**
 * comac_mesh_pattern_curve_to:
 * @pattern: a #comac_pattern_t
 * @x1: the X coordinate of the first control point
 * @y1: the Y coordinate of the first control point
 * @x2: the X coordinate of the second control point
 * @y2: the Y coordinate of the second control point
 * @x3: the X coordinate of the end of the curve
 * @y3: the Y coordinate of the end of the curve
 *
 * Adds a cubic Bézier spline to the current patch from the current
 * point to position (@x3, @y3) in pattern-space coordinates, using
 * (@x1, @y1) and (@x2, @y2) as the control points.
 *
 * If the current patch has no current point before the call to
 * comac_mesh_pattern_curve_to(), this function will behave as if
 * preceded by a call to comac_mesh_pattern_move_to(@pattern, @x1,
 * @y1).
 *
 * After this call the current point will be (@x3, @y3).
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @pattern has no current
 * patch or the current patch already has 4 sides, @pattern will be
 * put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_curve_to (comac_pattern_t *pattern,
			     double x1,
			     double y1,
			     double x2,
			     double y2,
			     double x3,
			     double y3)
{
    comac_mesh_pattern_t *mesh;
    int current_point, i, j;

    if (unlikely (pattern->status))
	return;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    mesh = (comac_mesh_pattern_t *) pattern;
    if (unlikely (! mesh->current_patch)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    if (unlikely (mesh->current_side == 3)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    if (mesh->current_side == -2)
	comac_mesh_pattern_move_to (pattern, x1, y1);

    assert (mesh->current_side >= -1);
    assert (pattern->status == COMAC_STATUS_SUCCESS);

    mesh->current_side++;

    current_point = 3 * mesh->current_side;

    current_point++;
    i = mesh_path_point_i[current_point];
    j = mesh_path_point_j[current_point];
    mesh->current_patch->points[i][j].x = x1;
    mesh->current_patch->points[i][j].y = y1;

    current_point++;
    i = mesh_path_point_i[current_point];
    j = mesh_path_point_j[current_point];
    mesh->current_patch->points[i][j].x = x2;
    mesh->current_patch->points[i][j].y = y2;

    current_point++;
    if (current_point < 12) {
	i = mesh_path_point_i[current_point];
	j = mesh_path_point_j[current_point];
	mesh->current_patch->points[i][j].x = x3;
	mesh->current_patch->points[i][j].y = y3;
    }
}

/**
 * comac_mesh_pattern_line_to:
 * @pattern: a #comac_pattern_t
 * @x: the X coordinate of the end of the new line
 * @y: the Y coordinate of the end of the new line
 *
 * Adds a line to the current patch from the current point to position
 * (@x, @y) in pattern-space coordinates.
 *
 * If there is no current point before the call to
 * comac_mesh_pattern_line_to() this function will behave as
 * comac_mesh_pattern_move_to(@pattern, @x, @y).
 *
 * After this call the current point will be (@x, @y).
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @pattern has no current
 * patch or the current patch already has 4 sides, @pattern will be
 * put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_line_to (comac_pattern_t *pattern, double x, double y)
{
    comac_mesh_pattern_t *mesh;
    comac_point_double_t last_point;
    int last_point_idx, i, j;

    if (unlikely (pattern->status))
	return;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    mesh = (comac_mesh_pattern_t *) pattern;
    if (unlikely (! mesh->current_patch)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    if (unlikely (mesh->current_side == 3)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    if (mesh->current_side == -2) {
	comac_mesh_pattern_move_to (pattern, x, y);
	return;
    }

    last_point_idx = 3 * (mesh->current_side + 1);
    i = mesh_path_point_i[last_point_idx];
    j = mesh_path_point_j[last_point_idx];

    last_point = mesh->current_patch->points[i][j];

    comac_mesh_pattern_curve_to (pattern,
				 (2 * last_point.x + x) * (1. / 3),
				 (2 * last_point.y + y) * (1. / 3),
				 (last_point.x + 2 * x) * (1. / 3),
				 (last_point.y + 2 * y) * (1. / 3),
				 x,
				 y);
}

/**
 * comac_mesh_pattern_move_to:
 * @pattern: a #comac_pattern_t
 * @x: the X coordinate of the new position
 * @y: the Y coordinate of the new position
 *
 * Define the first point of the current patch in a mesh pattern.
 *
 * After this call the current point will be (@x, @y).
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @pattern has no current
 * patch or the current patch already has at least one side, @pattern
 * will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_move_to (comac_pattern_t *pattern, double x, double y)
{
    comac_mesh_pattern_t *mesh;

    if (unlikely (pattern->status))
	return;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    mesh = (comac_mesh_pattern_t *) pattern;
    if (unlikely (! mesh->current_patch)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    if (unlikely (mesh->current_side >= 0)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    mesh->current_side = -1;
    mesh->current_patch->points[0][0].x = x;
    mesh->current_patch->points[0][0].y = y;
}

/**
 * comac_mesh_pattern_set_control_point:
 * @pattern: a #comac_pattern_t
 * @point_num: the control point to set the position for
 * @x: the X coordinate of the control point
 * @y: the Y coordinate of the control point
 *
 * Set an internal control point of the current patch.
 *
 * Valid values for @point_num are from 0 to 3 and identify the
 * control points as explained in comac_pattern_create_mesh().
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @point_num is not valid,
 * @pattern will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_INDEX.  If @pattern has no current patch,
 * @pattern will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_set_control_point (comac_pattern_t *pattern,
				      unsigned int point_num,
				      double x,
				      double y)
{
    comac_mesh_pattern_t *mesh;
    int i, j;

    if (unlikely (pattern->status))
	return;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    if (unlikely (point_num > 3)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_INVALID_INDEX);
	return;
    }

    mesh = (comac_mesh_pattern_t *) pattern;
    if (unlikely (! mesh->current_patch)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    i = mesh_control_point_i[point_num];
    j = mesh_control_point_j[point_num];

    mesh->current_patch->points[i][j].x = x;
    mesh->current_patch->points[i][j].y = y;
    mesh->has_control_point[point_num] = TRUE;
}

/* make room for at least one more color stop */
static comac_status_t
_comac_pattern_gradient_grow (comac_gradient_pattern_t *pattern)
{
    comac_gradient_stop_t *new_stops;
    int old_size = pattern->stops_size;
    int embedded_size = ARRAY_LENGTH (pattern->stops_embedded);
    int new_size = 2 * MAX (old_size, 4);

    /* we have a local buffer at pattern->stops_embedded.  try to fulfill the request
     * from there. */
    if (old_size < embedded_size) {
	pattern->stops = pattern->stops_embedded;
	pattern->stops_size = embedded_size;
	return COMAC_STATUS_SUCCESS;
    }

    if (COMAC_INJECT_FAULT ())
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    assert (pattern->n_stops <= pattern->stops_size);

    if (pattern->stops == pattern->stops_embedded) {
	new_stops = _comac_malloc_ab (new_size, sizeof (comac_gradient_stop_t));
	if (new_stops)
	    memcpy (new_stops,
		    pattern->stops,
		    old_size * sizeof (comac_gradient_stop_t));
    } else {
	new_stops = _comac_realloc_ab (pattern->stops,
				       new_size,
				       sizeof (comac_gradient_stop_t));
    }

    if (unlikely (new_stops == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    pattern->stops = new_stops;
    pattern->stops_size = new_size;

    return COMAC_STATUS_SUCCESS;
}

static void
_comac_mesh_pattern_set_corner_color (comac_mesh_pattern_t *mesh,
				      unsigned int corner_num,
				      double red,
				      double green,
				      double blue,
				      double alpha)
{
    comac_color_t *color;

    assert (mesh->current_patch);
    assert (corner_num <= 3);

    color = &mesh->current_patch->colors[corner_num];
    color->colorspace = COMAC_COLORSPACE_RGB;
    color->c.rgb.red = red;
    color->c.rgb.green = green;
    color->c.rgb.blue = blue;
    color->c.rgb.alpha = alpha;

    color->c.rgb.red_short = _comac_color_double_to_short (red);
    color->c.rgb.green_short = _comac_color_double_to_short (green);
    color->c.rgb.blue_short = _comac_color_double_to_short (blue);
    color->c.rgb.alpha_short = _comac_color_double_to_short (alpha);

    mesh->has_color[corner_num] = TRUE;
}

/**
 * comac_mesh_pattern_set_corner_color_rgb:
 * @pattern: a #comac_pattern_t
 * @corner_num: the corner to set the color for
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 *
 * Sets the color of a corner of the current patch in a mesh pattern.
 *
 * The color is specified in the same way as in comac_set_source_rgb().
 *
 * Valid values for @corner_num are from 0 to 3 and identify the
 * corners as explained in comac_pattern_create_mesh().
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @corner_num is not valid,
 * @pattern will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_INDEX.  If @pattern has no current patch,
 * @pattern will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_set_corner_color_rgb (comac_pattern_t *pattern,
					 unsigned int corner_num,
					 double red,
					 double green,
					 double blue)
{
    comac_mesh_pattern_set_corner_color_rgba (pattern,
					      corner_num,
					      red,
					      green,
					      blue,
					      1.0);
}

/**
 * comac_mesh_pattern_set_corner_color_rgba:
 * @pattern: a #comac_pattern_t
 * @corner_num: the corner to set the color for
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 * @alpha: alpha component of color
 *
 * Sets the color of a corner of the current patch in a mesh pattern.
 *
 * The color is specified in the same way as in comac_set_source_rgba().
 *
 * Valid values for @corner_num are from 0 to 3 and identify the
 * corners as explained in comac_pattern_create_mesh().
 *
 * Note: If @pattern is not a mesh pattern then @pattern will be put
 * into an error status with a status of
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH. If @corner_num is not valid,
 * @pattern will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_INDEX.  If @pattern has no current patch,
 * @pattern will be put into an error status with a status of
 * %COMAC_STATUS_INVALID_MESH_CONSTRUCTION.
 *
 * Since: 1.12
 **/
void
comac_mesh_pattern_set_corner_color_rgba (comac_pattern_t *pattern,
					  unsigned int corner_num,
					  double red,
					  double green,
					  double blue,
					  double alpha)
{
    comac_mesh_pattern_t *mesh;

    if (unlikely (pattern->status))
	return;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    if (unlikely (corner_num > 3)) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_INVALID_INDEX);
	return;
    }

    mesh = (comac_mesh_pattern_t *) pattern;
    if (unlikely (! mesh->current_patch)) {
	_comac_pattern_set_error (pattern,
				  COMAC_STATUS_INVALID_MESH_CONSTRUCTION);
	return;
    }

    red = _comac_restrict_value (red, 0.0, 1.0);
    green = _comac_restrict_value (green, 0.0, 1.0);
    blue = _comac_restrict_value (blue, 0.0, 1.0);
    alpha = _comac_restrict_value (alpha, 0.0, 1.0);

    _comac_mesh_pattern_set_corner_color (mesh,
					  corner_num,
					  red,
					  green,
					  blue,
					  alpha);
}

static void
_comac_pattern_add_color_stop (comac_gradient_pattern_t *pattern,
			       double offset,
			       double red,
			       double green,
			       double blue,
			       double alpha)
{
    comac_gradient_stop_t *stops;
    unsigned int i;

    if (pattern->n_stops >= pattern->stops_size) {
	comac_status_t status = _comac_pattern_gradient_grow (pattern);
	if (unlikely (status)) {
	    status = _comac_pattern_set_error (&pattern->base, status);
	    return;
	}
    }

    stops = pattern->stops;

    for (i = 0; i < pattern->n_stops; i++) {
	if (offset < stops[i].offset) {
	    memmove (&stops[i + 1],
		     &stops[i],
		     sizeof (comac_gradient_stop_t) * (pattern->n_stops - i));

	    break;
	}
    }

    stops[i].offset = offset;

    stops[i].color.red = red;
    stops[i].color.green = green;
    stops[i].color.blue = blue;
    stops[i].color.alpha = alpha;

    stops[i].color.red_short = _comac_color_double_to_short (red);
    stops[i].color.green_short = _comac_color_double_to_short (green);
    stops[i].color.blue_short = _comac_color_double_to_short (blue);
    stops[i].color.alpha_short = _comac_color_double_to_short (alpha);

    pattern->n_stops++;
}

/**
 * comac_pattern_add_color_stop_rgb:
 * @pattern: a #comac_pattern_t
 * @offset: an offset in the range [0.0 .. 1.0]
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 *
 * Adds an opaque color stop to a gradient pattern. The offset
 * specifies the location along the gradient's control vector. For
 * example, a linear gradient's control vector is from (x0,y0) to
 * (x1,y1) while a radial gradient's control vector is from any point
 * on the start circle to the corresponding point on the end circle.
 *
 * The color is specified in the same way as in comac_set_source_rgb().
 *
 * If two (or more) stops are specified with identical offset values,
 * they will be sorted according to the order in which the stops are
 * added, (stops added earlier will compare less than stops added
 * later). This can be useful for reliably making sharp color
 * transitions instead of the typical blend.
 *
 *
 * Note: If the pattern is not a gradient pattern, (eg. a linear or
 * radial pattern), then the pattern will be put into an error status
 * with a status of %COMAC_STATUS_PATTERN_TYPE_MISMATCH.
 *
 * Since: 1.0
 **/
void
comac_pattern_add_color_stop_rgb (comac_pattern_t *pattern,
				  double offset,
				  double red,
				  double green,
				  double blue)
{
    comac_pattern_add_color_stop_rgba (pattern, offset, red, green, blue, 1.0);
}

/**
 * comac_pattern_add_color_stop_rgba:
 * @pattern: a #comac_pattern_t
 * @offset: an offset in the range [0.0 .. 1.0]
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 * @alpha: alpha component of color
 *
 * Adds a translucent color stop to a gradient pattern. The offset
 * specifies the location along the gradient's control vector. For
 * example, a linear gradient's control vector is from (x0,y0) to
 * (x1,y1) while a radial gradient's control vector is from any point
 * on the start circle to the corresponding point on the end circle.
 *
 * The color is specified in the same way as in comac_set_source_rgba().
 *
 * If two (or more) stops are specified with identical offset values,
 * they will be sorted according to the order in which the stops are
 * added, (stops added earlier will compare less than stops added
 * later). This can be useful for reliably making sharp color
 * transitions instead of the typical blend.
 *
 * Note: If the pattern is not a gradient pattern, (eg. a linear or
 * radial pattern), then the pattern will be put into an error status
 * with a status of %COMAC_STATUS_PATTERN_TYPE_MISMATCH.
 *
 * Since: 1.0
 **/
void
comac_pattern_add_color_stop_rgba (comac_pattern_t *pattern,
				   double offset,
				   double red,
				   double green,
				   double blue,
				   double alpha)
{
    if (pattern->status)
	return;

    if (pattern->type != COMAC_PATTERN_TYPE_LINEAR &&
	pattern->type != COMAC_PATTERN_TYPE_RADIAL) {
	_comac_pattern_set_error (pattern, COMAC_STATUS_PATTERN_TYPE_MISMATCH);
	return;
    }

    offset = _comac_restrict_value (offset, 0.0, 1.0);
    red = _comac_restrict_value (red, 0.0, 1.0);
    green = _comac_restrict_value (green, 0.0, 1.0);
    blue = _comac_restrict_value (blue, 0.0, 1.0);
    alpha = _comac_restrict_value (alpha, 0.0, 1.0);

    _comac_pattern_add_color_stop ((comac_gradient_pattern_t *) pattern,
				   offset,
				   red,
				   green,
				   blue,
				   alpha);
}

/**
 * comac_pattern_set_matrix:
 * @pattern: a #comac_pattern_t
 * @matrix: a #comac_matrix_t
 *
 * Sets the pattern's transformation matrix to @matrix. This matrix is
 * a transformation from user space to pattern space.
 *
 * When a pattern is first created it always has the identity matrix
 * for its transformation matrix, which means that pattern space is
 * initially identical to user space.
 *
 * Important: Please note that the direction of this transformation
 * matrix is from user space to pattern space. This means that if you
 * imagine the flow from a pattern to user space (and on to device
 * space), then coordinates in that flow will be transformed by the
 * inverse of the pattern matrix.
 *
 * For example, if you want to make a pattern appear twice as large as
 * it does by default the correct code to use is:
 *
 * <informalexample><programlisting>
 * comac_matrix_init_scale (&amp;matrix, 0.5, 0.5);
 * comac_pattern_set_matrix (pattern, &amp;matrix);
 * </programlisting></informalexample>
 *
 * Meanwhile, using values of 2.0 rather than 0.5 in the code above
 * would cause the pattern to appear at half of its default size.
 *
 * Also, please note the discussion of the user-space locking
 * semantics of comac_set_source().
 *
 * Since: 1.0
 **/
void
comac_pattern_set_matrix (comac_pattern_t *pattern,
			  const comac_matrix_t *matrix)
{
    comac_matrix_t inverse;
    comac_status_t status;

    if (pattern->status)
	return;

    if (memcmp (&pattern->matrix, matrix, sizeof (comac_matrix_t)) == 0)
	return;

    pattern->matrix = *matrix;
    _comac_pattern_notify_observers (pattern, COMAC_PATTERN_NOTIFY_MATRIX);

    inverse = *matrix;
    status = comac_matrix_invert (&inverse);
    if (unlikely (status))
	status = _comac_pattern_set_error (pattern, status);
}

/**
 * comac_pattern_get_matrix:
 * @pattern: a #comac_pattern_t
 * @matrix: return value for the matrix
 *
 * Stores the pattern's transformation matrix into @matrix.
 *
 * Since: 1.0
 **/
void
comac_pattern_get_matrix (comac_pattern_t *pattern, comac_matrix_t *matrix)
{
    *matrix = pattern->matrix;
}

/**
 * comac_pattern_set_filter:
 * @pattern: a #comac_pattern_t
 * @filter: a #comac_filter_t describing the filter to use for resizing
 * the pattern
 *
 * Sets the filter to be used for resizing when using this pattern.
 * See #comac_filter_t for details on each filter.
 *
 * * Note that you might want to control filtering even when you do not
 * have an explicit #comac_pattern_t object, (for example when using
 * comac_set_source_surface()). In these cases, it is convenient to
 * use comac_get_source() to get access to the pattern that comac
 * creates implicitly. For example:
 *
 * <informalexample><programlisting>
 * comac_set_source_surface (cr, image, x, y);
 * comac_pattern_set_filter (comac_get_source (cr), COMAC_FILTER_NEAREST);
 * </programlisting></informalexample>
 *
 * Since: 1.0
 **/
void
comac_pattern_set_filter (comac_pattern_t *pattern, comac_filter_t filter)
{
    if (pattern->status)
	return;

    pattern->filter = filter;
    _comac_pattern_notify_observers (pattern, COMAC_PATTERN_NOTIFY_FILTER);
}

/**
 * comac_pattern_get_filter:
 * @pattern: a #comac_pattern_t
 *
 * Gets the current filter for a pattern.  See #comac_filter_t
 * for details on each filter.
 *
 * Return value: the current filter used for resizing the pattern.
 *
 * Since: 1.0
 **/
comac_filter_t
comac_pattern_get_filter (comac_pattern_t *pattern)
{
    return pattern->filter;
}

/**
 * comac_pattern_set_extend:
 * @pattern: a #comac_pattern_t
 * @extend: a #comac_extend_t describing how the area outside of the
 * pattern will be drawn
 *
 * Sets the mode to be used for drawing outside the area of a pattern.
 * See #comac_extend_t for details on the semantics of each extend
 * strategy.
 *
 * The default extend mode is %COMAC_EXTEND_NONE for surface patterns
 * and %COMAC_EXTEND_PAD for gradient patterns.
 *
 * Since: 1.0
 **/
void
comac_pattern_set_extend (comac_pattern_t *pattern, comac_extend_t extend)
{
    if (pattern->status)
	return;

    pattern->extend = extend;
    _comac_pattern_notify_observers (pattern, COMAC_PATTERN_NOTIFY_EXTEND);
}

/**
 * comac_pattern_get_extend:
 * @pattern: a #comac_pattern_t
 *
 * Gets the current extend mode for a pattern.  See #comac_extend_t
 * for details on the semantics of each extend strategy.
 *
 * Return value: the current extend strategy used for drawing the
 * pattern.
 *
 * Since: 1.0
 **/
comac_extend_t
comac_pattern_get_extend (comac_pattern_t *pattern)
{
    return pattern->extend;
}

void
_comac_pattern_pretransform (comac_pattern_t *pattern,
			     const comac_matrix_t *ctm)
{
    if (pattern->status)
	return;

    comac_matrix_multiply (&pattern->matrix, &pattern->matrix, ctm);
}

void
_comac_pattern_transform (comac_pattern_t *pattern,
			  const comac_matrix_t *ctm_inverse)
{
    if (pattern->status)
	return;

    comac_matrix_multiply (&pattern->matrix, ctm_inverse, &pattern->matrix);
}

static comac_bool_t
_linear_pattern_is_degenerate (const comac_linear_pattern_t *linear)
{
    return fabs (linear->pd1.x - linear->pd2.x) < DBL_EPSILON &&
	   fabs (linear->pd1.y - linear->pd2.y) < DBL_EPSILON;
}

static comac_bool_t
_radial_pattern_is_degenerate (const comac_radial_pattern_t *radial)
{
    /* A radial pattern is considered degenerate if it can be
     * represented as a solid or clear pattern.  This corresponds to
     * one of the two cases:
     *
     * 1) The radii are both very small:
     *      |dr| < DBL_EPSILON && min (r0, r1) < DBL_EPSILON
     *
     * 2) The two circles have about the same radius and are very
     *    close to each other (approximately a cylinder gradient that
     *    doesn't move with the parameter):
     *      |dr| < DBL_EPSILON && max (|dx|, |dy|) < 2 * DBL_EPSILON
     *
     * These checks are consistent with the assumptions used in
     * _comac_radial_pattern_box_to_parameter ().
     */

    return fabs (radial->cd1.radius - radial->cd2.radius) < DBL_EPSILON &&
	   (MIN (radial->cd1.radius, radial->cd2.radius) < DBL_EPSILON ||
	    MAX (fabs (radial->cd1.center.x - radial->cd2.center.x),
		 fabs (radial->cd1.center.y - radial->cd2.center.y)) <
		2 * DBL_EPSILON);
}

static void
_comac_linear_pattern_box_to_parameter (const comac_linear_pattern_t *linear,
					double x0,
					double y0,
					double x1,
					double y1,
					double range[2])
{
    double t0, tdx, tdy;
    double p1x, p1y, pdx, pdy, invsqnorm;

    assert (! _linear_pattern_is_degenerate (linear));

    /*
     * Linear gradients are othrogonal to the line passing through
     * their extremes. Because of convexity, the parameter range can
     * be computed as the convex hull (one the real line) of the
     * parameter values of the 4 corners of the box.
     *
     * The parameter value t for a point (x,y) can be computed as:
     *
     *   t = (p2 - p1) . (x,y) / |p2 - p1|^2
     *
     * t0  is the t value for the top left corner
     * tdx is the difference between left and right corners
     * tdy is the difference between top and bottom corners
     */

    p1x = linear->pd1.x;
    p1y = linear->pd1.y;
    pdx = linear->pd2.x - p1x;
    pdy = linear->pd2.y - p1y;
    invsqnorm = 1.0 / (pdx * pdx + pdy * pdy);
    pdx *= invsqnorm;
    pdy *= invsqnorm;

    t0 = (x0 - p1x) * pdx + (y0 - p1y) * pdy;
    tdx = (x1 - x0) * pdx;
    tdy = (y1 - y0) * pdy;

    /*
     * Because of the linearity of the t value, tdx can simply be
     * added the t0 to move along the top edge. After this, range[0]
     * and range[1] represent the parameter range for the top edge, so
     * extending it to include the whole box simply requires adding
     * tdy to the correct extreme.
     */

    range[0] = range[1] = t0;
    if (tdx < 0)
	range[0] += tdx;
    else
	range[1] += tdx;

    if (tdy < 0)
	range[0] += tdy;
    else
	range[1] += tdy;
}

static comac_bool_t
_extend_range (double range[2], double value, comac_bool_t valid)
{
    if (! valid)
	range[0] = range[1] = value;
    else if (value < range[0])
	range[0] = value;
    else if (value > range[1])
	range[1] = value;

    return TRUE;
}

/*
 * _comac_radial_pattern_focus_is_inside:
 *
 * Returns %TRUE if and only if the focus point exists and is
 * contained in one of the two extreme circles. This condition is
 * equivalent to one of the two extreme circles being completely
 * contained in the other one.
 *
 * Note: if the focus is on the border of one of the two circles (in
 * which case the circles are tangent in the focus point), it is not
 * considered as contained in the circle, hence this function returns
 * %FALSE.
 *
 */
comac_bool_t
_comac_radial_pattern_focus_is_inside (const comac_radial_pattern_t *radial)
{
    double cx, cy, cr, dx, dy, dr;

    cx = radial->cd1.center.x;
    cy = radial->cd1.center.y;
    cr = radial->cd1.radius;
    dx = radial->cd2.center.x - cx;
    dy = radial->cd2.center.y - cy;
    dr = radial->cd2.radius - cr;

    return dx * dx + dy * dy < dr * dr;
}

static void
_comac_radial_pattern_box_to_parameter (const comac_radial_pattern_t *radial,
					double x0,
					double y0,
					double x1,
					double y1,
					double tolerance,
					double range[2])
{
    double cx, cy, cr, dx, dy, dr;
    double a, x_focus, y_focus;
    double mindr, minx, miny, maxx, maxy;
    comac_bool_t valid;

    assert (! _radial_pattern_is_degenerate (radial));
    assert (x0 < x1);
    assert (y0 < y1);

    tolerance = MAX (tolerance, DBL_EPSILON);

    range[0] = range[1] = 0;
    valid = FALSE;

    x_focus = y_focus = 0; /* silence gcc */

    cx = radial->cd1.center.x;
    cy = radial->cd1.center.y;
    cr = radial->cd1.radius;
    dx = radial->cd2.center.x - cx;
    dy = radial->cd2.center.y - cy;
    dr = radial->cd2.radius - cr;

    /* translate by -(cx, cy) to simplify computations */
    x0 -= cx;
    y0 -= cy;
    x1 -= cx;
    y1 -= cy;

    /* enlarge boundaries slightly to avoid rounding problems in the
     * parameter range computation */
    x0 -= DBL_EPSILON;
    y0 -= DBL_EPSILON;
    x1 += DBL_EPSILON;
    y1 += DBL_EPSILON;

    /* enlarge boundaries even more to avoid rounding problems when
     * testing if a point belongs to the box */
    minx = x0 - DBL_EPSILON;
    miny = y0 - DBL_EPSILON;
    maxx = x1 + DBL_EPSILON;
    maxy = y1 + DBL_EPSILON;

    /* we don't allow negative radiuses, so we will be checking that
     * t*dr >= mindr to consider t valid */
    mindr = -(cr + DBL_EPSILON);

    /*
     * After the previous transformations, the start circle is
     * centered in the origin and has radius cr. A 1-unit change in
     * the t parameter corresponds to dx,dy,dr changes in the x,y,r of
     * the circle (center coordinates, radius).
     *
     * To compute the minimum range needed to correctly draw the
     * pattern, we start with an empty range and extend it to include
     * the circles touching the bounding box or within it.
     */

    /*
     * Focus, the point where the circle has radius == 0.
     *
     * r = cr + t * dr = 0
     * t = -cr / dr
     *
     * If the radius is constant (dr == 0) there is no focus (the
     * gradient represents a cylinder instead of a cone).
     */
    if (fabs (dr) >= DBL_EPSILON) {
	double t_focus;

	t_focus = -cr / dr;
	x_focus = t_focus * dx;
	y_focus = t_focus * dy;
	if (minx <= x_focus && x_focus <= maxx && miny <= y_focus &&
	    y_focus <= maxy) {
	    valid = _extend_range (range, t_focus, valid);
	}
    }

    /*
     * Circles externally tangent to box edges.
     *
     * All circles have center in (dx, dy) * t
     *
     * If the circle is tangent to the line defined by the edge of the
     * box, then at least one of the following holds true:
     *
     *   (dx*t) + (cr + dr*t) == x0 (left   edge)
     *   (dx*t) - (cr + dr*t) == x1 (right  edge)
     *   (dy*t) + (cr + dr*t) == y0 (top    edge)
     *   (dy*t) - (cr + dr*t) == y1 (bottom edge)
     *
     * The solution is only valid if the tangent point is actually on
     * the edge, i.e. if its y coordinate is in [y0,y1] for left/right
     * edges and if its x coordinate is in [x0,x1] for top/bottom
     * edges.
     *
     * For the first equation:
     *
     *   (dx + dr) * t = x0 - cr
     *   t = (x0 - cr) / (dx + dr)
     *   y = dy * t
     *
     * in the code this becomes:
     *
     *   t_edge = (num) / (den)
     *   v = (delta) * t_edge
     *
     * If the denominator in t is 0, the pattern is tangent to a line
     * parallel to the edge under examination. The corner-case where
     * the boundary line is the same as the edge is handled by the
     * focus point case and/or by the a==0 case.
     */
#define T_EDGE(num, den, delta, lower, upper)                                  \
    if (fabs (den) >= DBL_EPSILON) {                                           \
	double t_edge, v;                                                      \
                                                                               \
	t_edge = (num) / (den);                                                \
	v = t_edge * (delta);                                                  \
	if (t_edge * dr >= mindr && (lower) <= v && v <= (upper))              \
	    valid = _extend_range (range, t_edge, valid);                      \
    }

    /* circles tangent (externally) to left/right/top/bottom edge */
    T_EDGE (x0 - cr, dx + dr, dy, miny, maxy);
    T_EDGE (x1 + cr, dx - dr, dy, miny, maxy);
    T_EDGE (y0 - cr, dy + dr, dx, minx, maxx);
    T_EDGE (y1 + cr, dy - dr, dx, minx, maxx);

#undef T_EDGE

    /*
     * Circles passing through a corner.
     *
     * A circle passing through the point (x,y) satisfies:
     *
     * (x-t*dx)^2 + (y-t*dy)^2 == (cr + t*dr)^2
     *
     * If we set:
     *   a = dx^2 + dy^2 - dr^2
     *   b = x*dx + y*dy + cr*dr
     *   c = x^2 + y^2 - cr^2
     * we have:
     *   a*t^2 - 2*b*t + c == 0
     */
    a = dx * dx + dy * dy - dr * dr;
    if (fabs (a) < DBL_EPSILON * DBL_EPSILON) {
	double b, maxd2;

	/* Ensure that gradients with both a and dr small are
	 * considered degenerate.
	 * The floating point version of the degeneracy test implemented
	 * in _radial_pattern_is_degenerate() is:
	 *
	 *  1) The circles are practically the same size:
	 *     |dr| < DBL_EPSILON
	 *  AND
	 *  2a) The circles are both very small:
	 *      min (r0, r1) < DBL_EPSILON
	 *   OR
	 *  2b) The circles are very close to each other:
	 *      max (|dx|, |dy|) < 2 * DBL_EPSILON
	 *
	 * Assuming that the gradient is not degenerate, we want to
	 * show that |a| < DBL_EPSILON^2 implies |dr| >= DBL_EPSILON.
	 *
	 * If the gradient is not degenerate yet it has |dr| <
	 * DBL_EPSILON, (2b) is false, thus:
	 *
	 *   max (|dx|, |dy|) >= 2*DBL_EPSILON
	 * which implies:
	 *   4*DBL_EPSILON^2 <= max (|dx|, |dy|)^2 <= dx^2 + dy^2
	 *
	 * From the definition of a, we get:
	 *   a = dx^2 + dy^2 - dr^2 < DBL_EPSILON^2
	 *   dx^2 + dy^2 - DBL_EPSILON^2 < dr^2
	 *   3*DBL_EPSILON^2 < dr^2
	 *
	 * which is inconsistent with the hypotheses, thus |dr| <
	 * DBL_EPSILON is false or the gradient is degenerate.
	 */
	assert (fabs (dr) >= DBL_EPSILON);

	/*
	 * If a == 0, all the circles are tangent to a line in the
	 * focus point. If this line is within the box extents, we
	 * should add the circle with infinite radius, but this would
	 * make the range unbounded, so we add the smallest circle whose
	 * distance to the desired (degenerate) circle within the
	 * bounding box does not exceed tolerance.
	 *
	 * The equation of the line is b==0, i.e.:
	 *   x*dx + y*dy + cr*dr == 0
	 *
	 * We compute the intersection of the line with the box and
	 * keep the intersection with maximum square distance (maxd2)
	 * from the focus point.
	 *
	 * In the code the intersection is represented in another
	 * coordinate system, whose origin is the focus point and
	 * which has a u,v axes, which are respectively orthogonal and
	 * parallel to the edge being intersected.
	 *
	 * The intersection is valid only if it belongs to the box,
	 * otherwise it is ignored.
	 *
	 * For example:
	 *
	 *   y = y0
	 *   x*dx + y0*dy + cr*dr == 0
	 *   x = -(y0*dy + cr*dr) / dx
	 *
	 * which in (u,v) is:
	 *   u = y0 - y_focus
	 *   v = -(y0*dy + cr*dr) / dx - x_focus
	 *
	 * In the code:
	 *   u = (edge) - (u_origin)
	 *   v = -((edge) * (delta) + cr*dr) / (den) - v_focus
	 */
#define T_EDGE(edge, delta, den, lower, upper, u_origin, v_origin)             \
    if (fabs (den) >= DBL_EPSILON) {                                           \
	double v;                                                              \
                                                                               \
	v = -((edge) * (delta) + cr * dr) / (den);                             \
	if ((lower) <= v && v <= (upper)) {                                    \
	    double u, d2;                                                      \
                                                                               \
	    u = (edge) - (u_origin);                                           \
	    v -= (v_origin);                                                   \
	    d2 = u * u + v * v;                                                \
	    if (maxd2 < d2)                                                    \
		maxd2 = d2;                                                    \
	}                                                                      \
    }

	maxd2 = 0;

	/* degenerate circles (lines) passing through each edge */
	T_EDGE (y0, dy, dx, minx, maxx, y_focus, x_focus);
	T_EDGE (y1, dy, dx, minx, maxx, y_focus, x_focus);
	T_EDGE (x0, dx, dy, miny, maxy, x_focus, y_focus);
	T_EDGE (x1, dx, dy, miny, maxy, x_focus, y_focus);

#undef T_EDGE

	/*
	 * The limit circle can be transformed rigidly to the y=0 line
	 * and the circles tangent to it in (0,0) are:
	 *
	 *   x^2 + (y-r)^2 = r^2  <=>  x^2 + y^2 - 2*y*r = 0
	 *
	 * y is the distance from the line, in our case tolerance;
	 * x is the distance along the line, i.e. sqrt(maxd2),
	 * so:
	 *
	 *   r = cr + dr * t = (maxd2 + tolerance^2) / (2*tolerance)
	 *   t = (r - cr) / dr =
	 *       (maxd2 + tolerance^2 - 2*tolerance*cr) / (2*tolerance*dr)
	 */
	if (maxd2 > 0) {
	    double t_limit = maxd2 + tolerance * tolerance - 2 * tolerance * cr;
	    t_limit /= 2 * tolerance * dr;
	    valid = _extend_range (range, t_limit, valid);
	}

	/*
	 * Nondegenerate, nonlimit circles passing through the corners.
	 *
	 * a == 0 && a*t^2 - 2*b*t + c == 0
	 *
	 * t = c / (2*b)
	 *
	 * The b == 0 case has just been handled, so we only have to
	 * compute this if b != 0.
	 */
#define T_CORNER(x, y)                                                         \
    b = (x) *dx + (y) *dy + cr * dr;                                           \
    if (fabs (b) >= DBL_EPSILON) {                                             \
	double t_corner;                                                       \
	double x2 = (x) * (x);                                                 \
	double y2 = (y) * (y);                                                 \
	double cr2 = (cr) * (cr);                                              \
	double c = x2 + y2 - cr2;                                              \
                                                                               \
	t_corner = 0.5 * c / b;                                                \
	if (t_corner * dr >= mindr)                                            \
	    valid = _extend_range (range, t_corner, valid);                    \
    }

	/* circles touching each corner */
	T_CORNER (x0, y0);
	T_CORNER (x0, y1);
	T_CORNER (x1, y0);
	T_CORNER (x1, y1);

#undef T_CORNER
    } else {
	double inva, b, c, d;

	inva = 1 / a;

	/*
	 * Nondegenerate, nonlimit circles passing through the corners.
	 *
	 * a != 0 && a*t^2 - 2*b*t + c == 0
	 *
	 * t = (b +- sqrt (b*b - a*c)) / a
	 *
	 * If the argument of sqrt() is negative, then no circle
	 * passes through the corner.
	 */
#define T_CORNER(x, y)                                                         \
    b = (x) *dx + (y) *dy + cr * dr;                                           \
    c = (x) * (x) + (y) * (y) -cr * cr;                                        \
    d = b * b - a * c;                                                         \
    if (d >= 0) {                                                              \
	double t_corner;                                                       \
                                                                               \
	d = sqrt (d);                                                          \
	t_corner = (b + d) * inva;                                             \
	if (t_corner * dr >= mindr)                                            \
	    valid = _extend_range (range, t_corner, valid);                    \
	t_corner = (b - d) * inva;                                             \
	if (t_corner * dr >= mindr)                                            \
	    valid = _extend_range (range, t_corner, valid);                    \
    }

	/* circles touching each corner */
	T_CORNER (x0, y0);
	T_CORNER (x0, y1);
	T_CORNER (x1, y0);
	T_CORNER (x1, y1);

#undef T_CORNER
    }
}

/**
 * _comac_gradient_pattern_box_to_parameter:
 *
 * Compute a interpolation range sufficient to draw (within the given
 * tolerance) the gradient in the given box getting the same result as
 * using the (-inf, +inf) range.
 *
 * Assumes that the pattern is not degenerate. This can be guaranteed
 * by simplifying it to a solid clear if _comac_pattern_is_clear or to
 * a solid color if _comac_gradient_pattern_is_solid.
 *
 * The range isn't guaranteed to be minimal, but it tries to.
 **/
void
_comac_gradient_pattern_box_to_parameter (
    const comac_gradient_pattern_t *gradient,
    double x0,
    double y0,
    double x1,
    double y1,
    double tolerance,
    double out_range[2])
{
    assert (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR ||
	    gradient->base.type == COMAC_PATTERN_TYPE_RADIAL);

    if (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR) {
	_comac_linear_pattern_box_to_parameter (
	    (comac_linear_pattern_t *) gradient,
	    x0,
	    y0,
	    x1,
	    y1,
	    out_range);
    } else {
	_comac_radial_pattern_box_to_parameter (
	    (comac_radial_pattern_t *) gradient,
	    x0,
	    y0,
	    x1,
	    y1,
	    tolerance,
	    out_range);
    }
}

/**
 * _comac_gradient_pattern_interpolate:
 *
 * Interpolate between the start and end objects of linear or radial
 * gradients.  The interpolated object is stored in out_circle, with
 * the radius being zero in the linear gradient case.
 **/
void
_comac_gradient_pattern_interpolate (const comac_gradient_pattern_t *gradient,
				     double t,
				     comac_circle_double_t *out_circle)
{
    assert (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR ||
	    gradient->base.type == COMAC_PATTERN_TYPE_RADIAL);

#define lerp(a, b) (a) * (1 - t) + (b) *t

    if (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR) {
	comac_linear_pattern_t *linear = (comac_linear_pattern_t *) gradient;
	out_circle->center.x = lerp (linear->pd1.x, linear->pd2.x);
	out_circle->center.y = lerp (linear->pd1.y, linear->pd2.y);
	out_circle->radius = 0;
    } else {
	comac_radial_pattern_t *radial = (comac_radial_pattern_t *) gradient;
	out_circle->center.x =
	    lerp (radial->cd1.center.x, radial->cd2.center.x);
	out_circle->center.y =
	    lerp (radial->cd1.center.y, radial->cd2.center.y);
	out_circle->radius = lerp (radial->cd1.radius, radial->cd2.radius);
    }

#undef lerp
}

/**
 * _comac_gradient_pattern_fit_to_range:
 *
 * Scale the extremes of a gradient to guarantee that the coordinates
 * and their deltas are within the range (-max_value, max_value). The
 * new extremes are stored in out_circle.
 *
 * The pattern matrix is scaled to guarantee that the aspect of the
 * gradient is the same and the result is stored in out_matrix.
 *
 **/
void
_comac_gradient_pattern_fit_to_range (const comac_gradient_pattern_t *gradient,
				      double max_value,
				      comac_matrix_t *out_matrix,
				      comac_circle_double_t out_circle[2])
{
    double dim;

    assert (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR ||
	    gradient->base.type == COMAC_PATTERN_TYPE_RADIAL);

    if (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR) {
	comac_linear_pattern_t *linear = (comac_linear_pattern_t *) gradient;

	out_circle[0].center = linear->pd1;
	out_circle[0].radius = 0;
	out_circle[1].center = linear->pd2;
	out_circle[1].radius = 0;

	dim = fabs (linear->pd1.x);
	dim = MAX (dim, fabs (linear->pd1.y));
	dim = MAX (dim, fabs (linear->pd2.x));
	dim = MAX (dim, fabs (linear->pd2.y));
	dim = MAX (dim, fabs (linear->pd1.x - linear->pd2.x));
	dim = MAX (dim, fabs (linear->pd1.y - linear->pd2.y));
    } else {
	comac_radial_pattern_t *radial = (comac_radial_pattern_t *) gradient;

	out_circle[0] = radial->cd1;
	out_circle[1] = radial->cd2;

	dim = fabs (radial->cd1.center.x);
	dim = MAX (dim, fabs (radial->cd1.center.y));
	dim = MAX (dim, fabs (radial->cd1.radius));
	dim = MAX (dim, fabs (radial->cd2.center.x));
	dim = MAX (dim, fabs (radial->cd2.center.y));
	dim = MAX (dim, fabs (radial->cd2.radius));
	dim = MAX (dim, fabs (radial->cd1.center.x - radial->cd2.center.x));
	dim = MAX (dim, fabs (radial->cd1.center.y - radial->cd2.center.y));
	dim = MAX (dim, fabs (radial->cd1.radius - radial->cd2.radius));
    }

    if (unlikely (dim > max_value)) {
	comac_matrix_t scale;

	dim = max_value / dim;

	out_circle[0].center.x *= dim;
	out_circle[0].center.y *= dim;
	out_circle[0].radius *= dim;
	out_circle[1].center.x *= dim;
	out_circle[1].center.y *= dim;
	out_circle[1].radius *= dim;

	comac_matrix_init_scale (&scale, dim, dim);
	comac_matrix_multiply (out_matrix, &gradient->base.matrix, &scale);
    } else {
	*out_matrix = gradient->base.matrix;
    }
}

static comac_bool_t
_gradient_is_clear (const comac_gradient_pattern_t *gradient,
		    const comac_rectangle_int_t *extents)
{
    unsigned int i;

    assert (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR ||
	    gradient->base.type == COMAC_PATTERN_TYPE_RADIAL);

    if (gradient->n_stops == 0 ||
	(gradient->base.extend == COMAC_EXTEND_NONE &&
	 gradient->stops[0].offset ==
	     gradient->stops[gradient->n_stops - 1].offset))
	return TRUE;

    if (gradient->base.type == COMAC_PATTERN_TYPE_RADIAL) {
	/* degenerate radial gradients are clear */
	if (_radial_pattern_is_degenerate ((comac_radial_pattern_t *) gradient))
	    return TRUE;
    } else if (gradient->base.extend == COMAC_EXTEND_NONE) {
	/* EXTEND_NONE degenerate linear gradients are clear */
	if (_linear_pattern_is_degenerate ((comac_linear_pattern_t *) gradient))
	    return TRUE;
    }

    /* Check if the extents intersect the drawn part of the pattern. */
    if (extents != NULL && (gradient->base.extend == COMAC_EXTEND_NONE ||
			    gradient->base.type == COMAC_PATTERN_TYPE_RADIAL)) {
	double t[2];

	_comac_gradient_pattern_box_to_parameter (gradient,
						  extents->x,
						  extents->y,
						  extents->x + extents->width,
						  extents->y + extents->height,
						  DBL_EPSILON,
						  t);

	if (gradient->base.extend == COMAC_EXTEND_NONE &&
	    (t[0] >= gradient->stops[gradient->n_stops - 1].offset ||
	     t[1] <= gradient->stops[0].offset)) {
	    return TRUE;
	}

	if (t[0] == t[1])
	    return TRUE;
    }

    for (i = 0; i < gradient->n_stops; i++)
	if (! COMAC_COLOR_IS_CLEAR (&gradient->stops[i].color))
	    return FALSE;

    return TRUE;
}

static void
_gradient_color_average (const comac_gradient_pattern_t *gradient,
			 comac_color_t *color)
{
    double delta0, delta1;
    double r, g, b, a;
    unsigned int i, start = 1, end;

    assert (gradient->n_stops > 0);
    assert (gradient->base.extend != COMAC_EXTEND_NONE);

    if (gradient->n_stops == 1) {
	_comac_color_init_rgba (color,
				gradient->stops[0].color.red,
				gradient->stops[0].color.green,
				gradient->stops[0].color.blue,
				gradient->stops[0].color.alpha);
	return;
    }

    end = gradient->n_stops - 1;

    switch (gradient->base.extend) {
    case COMAC_EXTEND_REPEAT:
	/*
       * Sa, Sb and Sy, Sz are the first two and last two stops respectively.
       * The weight of the first and last stop can be computed as the area of
       * the following triangles (taken with height 1, since the whole [0-1]
       * will have total weight 1 this way): b*h/2
       *
       *              +                   +
       *            / |\                / | \
       *          /   | \             /   |   \
       *        /     |  \          /     |     \
       * ~~~~~+---+---+---+~~~~~~~+-------+---+---+~~~~~
       *   -1+Sz  0  Sa   Sb      Sy     Sz   1  1+Sa
       *
       * For the first stop: (Sb-(-1+Sz)/2 = (1+Sb-Sz)/2
       * For the last stop: ((1+Sa)-Sy)/2 = (1+Sa-Sy)/2
       * Halving the result is done after summing up all the areas.
       */
	delta0 = 1.0 + gradient->stops[1].offset - gradient->stops[end].offset;
	delta1 =
	    1.0 + gradient->stops[0].offset - gradient->stops[end - 1].offset;
	break;

    case COMAC_EXTEND_REFLECT:
	/*
       * Sa, Sb and Sy, Sz are the first two and last two stops respectively.
       * The weight of the first and last stop can be computed as the area of
       * the following trapezoids (taken with height 1, since the whole [0-1]
       * will have total weight 1 this way): (b+B)*h/2
       *
       * +-------+                   +---+
       * |       |\                / |   |
       * |       | \             /   |   |
       * |       |  \          /     |   |
       * +-------+---+~~~~~~~+-------+---+
       * 0      Sa   Sb      Sy     Sz   1
       *
       * For the first stop: (Sa+Sb)/2
       * For the last stop: ((1-Sz) + (1-Sy))/2 = (2-Sy-Sz)/2
       * Halving the result is done after summing up all the areas.
       */
	delta0 = gradient->stops[0].offset + gradient->stops[1].offset;
	delta1 =
	    2.0 - gradient->stops[end - 1].offset - gradient->stops[end].offset;
	break;

    case COMAC_EXTEND_PAD:
	/* PAD is computed as the average of the first and last stop:
       *  - take both of them with weight 1 (they will be halved
       *    after the whole sum has been computed).
       *  - avoid summing any of the inner stops.
       */
	delta0 = delta1 = 1.0;
	start = end;
	break;

    case COMAC_EXTEND_NONE:
    default:
	ASSERT_NOT_REACHED;
	_comac_color_init_rgba (color, 0, 0, 0, 0);
	return;
    }

    r = delta0 * gradient->stops[0].color.red;
    g = delta0 * gradient->stops[0].color.green;
    b = delta0 * gradient->stops[0].color.blue;
    a = delta0 * gradient->stops[0].color.alpha;

    for (i = start; i < end; ++i) {
	/* Inner stops weight is the same as the area of the triangle they influence
       * (which goes from the stop before to the stop after), again with height 1
       * since the whole must sum up to 1: b*h/2
       * Halving is done after the whole sum has been computed.
       */
	double delta =
	    gradient->stops[i + 1].offset - gradient->stops[i - 1].offset;
	r += delta * gradient->stops[i].color.red;
	g += delta * gradient->stops[i].color.green;
	b += delta * gradient->stops[i].color.blue;
	a += delta * gradient->stops[i].color.alpha;
    }

    r += delta1 * gradient->stops[end].color.red;
    g += delta1 * gradient->stops[end].color.green;
    b += delta1 * gradient->stops[end].color.blue;
    a += delta1 * gradient->stops[end].color.alpha;

    _comac_color_init_rgba (color, r * .5, g * .5, b * .5, a * .5);
}

/**
 * _comac_pattern_alpha_range:
 *
 * Convenience function to determine the minimum and maximum alpha in
 * the drawn part of a pattern (i.e. ignoring clear parts caused by
 * extend modes and/or pattern shape).
 *
 * If not NULL, out_min and out_max will be set respectively to the
 * minimum and maximum alpha value of the pattern.
 **/
void
_comac_pattern_alpha_range (const comac_pattern_t *pattern,
			    double *out_min,
			    double *out_max)
{
    double alpha_min, alpha_max;

    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID: {
	const comac_solid_pattern_t *solid = (comac_solid_pattern_t *) pattern;
	assert (solid->color.colorspace == COMAC_COLORSPACE_RGB);
	alpha_min = alpha_max = solid->color.c.rgb.alpha;
	break;
    }

    case COMAC_PATTERN_TYPE_LINEAR:
    case COMAC_PATTERN_TYPE_RADIAL: {
	const comac_gradient_pattern_t *gradient =
	    (comac_gradient_pattern_t *) pattern;
	unsigned int i;

	assert (gradient->n_stops >= 1);

	alpha_min = alpha_max = gradient->stops[0].color.alpha;
	for (i = 1; i < gradient->n_stops; i++) {
	    if (alpha_min > gradient->stops[i].color.alpha)
		alpha_min = gradient->stops[i].color.alpha;
	    else if (alpha_max < gradient->stops[i].color.alpha)
		alpha_max = gradient->stops[i].color.alpha;
	}

	break;
    }

    case COMAC_PATTERN_TYPE_MESH: {
	const comac_mesh_pattern_t *mesh =
	    (const comac_mesh_pattern_t *) pattern;
	const comac_mesh_patch_t *patch =
	    _comac_array_index_const (&mesh->patches, 0);
	unsigned int i, j, n = _comac_array_num_elements (&mesh->patches);

	assert (n >= 1);

	assert (patch[0].colors[0].colorspace == COMAC_COLORSPACE_RGB);
	alpha_min = alpha_max = patch[0].colors[0].c.rgb.alpha;
	for (i = 0; i < n; i++) {
	    for (j = 0; j < 4; j++) {
		assert (patch[0].colors[j].colorspace == COMAC_COLORSPACE_RGB);
		if (patch[i].colors[j].c.rgb.alpha < alpha_min)
		    alpha_min = patch[i].colors[j].c.rgb.alpha;
		else if (patch[i].colors[j].c.rgb.alpha > alpha_max)
		    alpha_max = patch[i].colors[j].c.rgb.alpha;
	    }
	}

	break;
    }

    default:
	ASSERT_NOT_REACHED;
	/* fall through */

    case COMAC_PATTERN_TYPE_SURFACE:
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	alpha_min = 0;
	alpha_max = 1;
	break;
    }

    if (out_min)
	*out_min = alpha_min;
    if (out_max)
	*out_max = alpha_max;
}

/**
 * _comac_mesh_pattern_coord_box:
 *
 * Convenience function to determine the range of the coordinates of
 * the points used to define the patches of the mesh.
 *
 * This is guaranteed to contain the pattern extents, but might not be
 * tight, just like a Bezier curve is always inside the convex hull of
 * the control points.
 *
 * This function cannot be used while the mesh is being constructed.
 *
 * The function returns TRUE and sets the output parameters to define
 * the coordinate range if the mesh pattern contains at least one
 * patch, otherwise it returns FALSE.
 **/
comac_bool_t
_comac_mesh_pattern_coord_box (const comac_mesh_pattern_t *mesh,
			       double *out_xmin,
			       double *out_ymin,
			       double *out_xmax,
			       double *out_ymax)
{
    const comac_mesh_patch_t *patch;
    unsigned int num_patches, i, j, k;
    double x0, y0, x1, y1;

    assert (mesh->current_patch == NULL);

    num_patches = _comac_array_num_elements (&mesh->patches);

    if (num_patches == 0)
	return FALSE;

    patch = _comac_array_index_const (&mesh->patches, 0);
    x0 = x1 = patch->points[0][0].x;
    y0 = y1 = patch->points[0][0].y;

    for (i = 0; i < num_patches; i++) {
	for (j = 0; j < 4; j++) {
	    for (k = 0; k < 4; k++) {
		x0 = MIN (x0, patch[i].points[j][k].x);
		y0 = MIN (y0, patch[i].points[j][k].y);
		x1 = MAX (x1, patch[i].points[j][k].x);
		y1 = MAX (y1, patch[i].points[j][k].y);
	    }
	}
    }

    *out_xmin = x0;
    *out_ymin = y0;
    *out_xmax = x1;
    *out_ymax = y1;

    return TRUE;
}

/**
 * _comac_gradient_pattern_is_solid:
 *
 * Convenience function to determine whether a gradient pattern is
 * a solid color within the given extents. In this case the color
 * argument is initialized to the color the pattern represents.
 * This functions doesn't handle completely transparent gradients,
 * thus it should be called only after _comac_pattern_is_clear has
 * returned FALSE.
 *
 * Return value: %TRUE if the pattern is a solid color.
 **/
comac_bool_t
_comac_gradient_pattern_is_solid (const comac_gradient_pattern_t *gradient,
				  const comac_rectangle_int_t *extents,
				  comac_color_t *color)
{
    unsigned int i;

    assert (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR ||
	    gradient->base.type == COMAC_PATTERN_TYPE_RADIAL);

    /* TODO: radial */
    if (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR) {
	comac_linear_pattern_t *linear = (comac_linear_pattern_t *) gradient;
	if (_linear_pattern_is_degenerate (linear)) {
	    _gradient_color_average (gradient, color);
	    return TRUE;
	}

	if (gradient->base.extend == COMAC_EXTEND_NONE) {
	    double t[2];

	    /* We already know that the pattern is not clear, thus if some
	     * part of it is clear, the whole is not solid.
	     */

	    if (extents == NULL)
		return FALSE;

	    _comac_linear_pattern_box_to_parameter (linear,
						    extents->x,
						    extents->y,
						    extents->x + extents->width,
						    extents->y +
							extents->height,
						    t);

	    if (t[0] < 0.0 || t[1] > 1.0)
		return FALSE;
	}
    } else
	return FALSE;

    for (i = 1; i < gradient->n_stops; i++)
	if (! _comac_color_stop_equal (&gradient->stops[0].color,
				       &gradient->stops[i].color))
	    return FALSE;

    _comac_color_init_rgba (color,
			    gradient->stops[0].color.red,
			    gradient->stops[0].color.green,
			    gradient->stops[0].color.blue,
			    gradient->stops[0].color.alpha);

    return TRUE;
}

/**
 * _comac_pattern_is_constant_alpha:
 *
 * Convenience function to determine whether a pattern has constant
 * alpha within the given extents. In this case the alpha argument is
 * initialized to the alpha within the extents.
 *
 * Return value: %TRUE if the pattern has constant alpha.
 **/
comac_bool_t
_comac_pattern_is_constant_alpha (const comac_pattern_t *abstract_pattern,
				  const comac_rectangle_int_t *extents,
				  double *alpha)
{
    const comac_pattern_union_t *pattern;
    comac_color_t color;

    if (_comac_pattern_is_clear (abstract_pattern)) {
	*alpha = 0.0;
	return TRUE;
    }

    if (_comac_pattern_is_opaque (abstract_pattern, extents)) {
	*alpha = 1.0;
	return TRUE;
    }

    pattern = (comac_pattern_union_t *) abstract_pattern;
    switch (pattern->base.type) {
    case COMAC_PATTERN_TYPE_SOLID:
	assert (pattern->solid.color.colorspace == COMAC_COLORSPACE_RGB);
	*alpha = pattern->solid.color.c.rgb.alpha;
	return TRUE;

    case COMAC_PATTERN_TYPE_LINEAR:
    case COMAC_PATTERN_TYPE_RADIAL:
	if (_comac_gradient_pattern_is_solid (&pattern->gradient.base,
					      extents,
					      &color)) {
	    assert (color.colorspace == COMAC_COLORSPACE_RGB);
	    *alpha = color.c.rgb.alpha;
	    return TRUE;
	} else {
	    return FALSE;
	}

	/* TODO: need to test these as well */
    case COMAC_PATTERN_TYPE_SURFACE:
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
    case COMAC_PATTERN_TYPE_MESH:
	return FALSE;
    }

    ASSERT_NOT_REACHED;
    return FALSE;
}

static comac_bool_t
_mesh_is_clear (const comac_mesh_pattern_t *mesh)
{
    double x1, y1, x2, y2;
    comac_bool_t is_valid;

    is_valid = _comac_mesh_pattern_coord_box (mesh, &x1, &y1, &x2, &y2);
    if (! is_valid)
	return TRUE;

    if (x2 - x1 < DBL_EPSILON || y2 - y1 < DBL_EPSILON)
	return TRUE;

    return FALSE;
}

/**
 * _comac_pattern_is_opaque_solid:
 *
 * Convenience function to determine whether a pattern is an opaque
 * (alpha==1.0) solid color pattern. This is done by testing whether
 * the pattern's alpha value when converted to a byte is 255, so if a
 * backend actually supported deep alpha channels this function might
 * not do the right thing.
 *
 * Return value: %TRUE if the pattern is an opaque, solid color.
 **/
comac_bool_t
_comac_pattern_is_opaque_solid (const comac_pattern_t *pattern)
{
    comac_solid_pattern_t *solid;

    if (pattern->type != COMAC_PATTERN_TYPE_SOLID)
	return FALSE;

    solid = (comac_solid_pattern_t *) pattern;

    assert (solid->color.colorspace == COMAC_COLORSPACE_RGB);
    return COMAC_COLOR_IS_OPAQUE (&solid->color.c.rgb);
}

static comac_bool_t
_surface_is_opaque (const comac_surface_pattern_t *pattern,
		    const comac_rectangle_int_t *sample)
{
    comac_rectangle_int_t extents;

    if (pattern->surface->content & COMAC_CONTENT_ALPHA)
	return FALSE;

    if (pattern->base.extend != COMAC_EXTEND_NONE)
	return TRUE;

    if (! _comac_surface_get_extents (pattern->surface, &extents))
	return TRUE;

    if (sample == NULL)
	return FALSE;

    return _comac_rectangle_contains_rectangle (&extents, sample);
}

static comac_bool_t
_raster_source_is_opaque (const comac_raster_source_pattern_t *pattern,
			  const comac_rectangle_int_t *sample)
{
    if (pattern->content & COMAC_CONTENT_ALPHA)
	return FALSE;

    if (pattern->base.extend != COMAC_EXTEND_NONE)
	return TRUE;

    if (sample == NULL)
	return FALSE;

    return _comac_rectangle_contains_rectangle (&pattern->extents, sample);
}

static comac_bool_t
_surface_is_clear (const comac_surface_pattern_t *pattern)
{
    comac_rectangle_int_t extents;

    if (_comac_surface_get_extents (pattern->surface, &extents) &&
	(extents.width == 0 || extents.height == 0))
	return TRUE;

    return pattern->surface->is_clear &&
	   pattern->surface->content & COMAC_CONTENT_ALPHA;
}

static comac_bool_t
_raster_source_is_clear (const comac_raster_source_pattern_t *pattern)
{
    return pattern->extents.width == 0 || pattern->extents.height == 0;
}

static comac_bool_t
_gradient_is_opaque (const comac_gradient_pattern_t *gradient,
		     const comac_rectangle_int_t *sample)
{
    unsigned int i;

    assert (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR ||
	    gradient->base.type == COMAC_PATTERN_TYPE_RADIAL);

    if (gradient->n_stops == 0 ||
	(gradient->base.extend == COMAC_EXTEND_NONE &&
	 gradient->stops[0].offset ==
	     gradient->stops[gradient->n_stops - 1].offset))
	return FALSE;

    if (gradient->base.type == COMAC_PATTERN_TYPE_LINEAR) {
	if (gradient->base.extend == COMAC_EXTEND_NONE) {
	    double t[2];
	    comac_linear_pattern_t *linear =
		(comac_linear_pattern_t *) gradient;

	    /* EXTEND_NONE degenerate radial gradients are clear */
	    if (_linear_pattern_is_degenerate (linear))
		return FALSE;

	    if (sample == NULL)
		return FALSE;

	    _comac_linear_pattern_box_to_parameter (linear,
						    sample->x,
						    sample->y,
						    sample->x + sample->width,
						    sample->y + sample->height,
						    t);

	    if (t[0] < 0.0 || t[1] > 1.0)
		return FALSE;
	}
    } else
	return FALSE; /* TODO: check actual intersection */

    for (i = 0; i < gradient->n_stops; i++)
	if (! COMAC_COLOR_IS_OPAQUE (&gradient->stops[i].color))
	    return FALSE;

    return TRUE;
}

/**
 * _comac_pattern_is_opaque:
 *
 * Convenience function to determine whether a pattern is an opaque
 * pattern (of any type). The same caveats that apply to
 * _comac_pattern_is_opaque_solid apply here as well.
 *
 * Return value: %TRUE if the pattern is a opaque.
 **/
comac_bool_t
_comac_pattern_is_opaque (const comac_pattern_t *abstract_pattern,
			  const comac_rectangle_int_t *sample)
{
    const comac_pattern_union_t *pattern;

    if (abstract_pattern->has_component_alpha)
	return FALSE;

    pattern = (comac_pattern_union_t *) abstract_pattern;
    switch (pattern->base.type) {
    case COMAC_PATTERN_TYPE_SOLID:
	return _comac_pattern_is_opaque_solid (abstract_pattern);
    case COMAC_PATTERN_TYPE_SURFACE:
	return _surface_is_opaque (&pattern->surface, sample);
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	return _raster_source_is_opaque (&pattern->raster_source, sample);
    case COMAC_PATTERN_TYPE_LINEAR:
    case COMAC_PATTERN_TYPE_RADIAL:
	return _gradient_is_opaque (&pattern->gradient.base, sample);
    case COMAC_PATTERN_TYPE_MESH:
	return FALSE;
    }

    ASSERT_NOT_REACHED;
    return FALSE;
}

comac_bool_t
_comac_pattern_is_clear (const comac_pattern_t *abstract_pattern)
{
    const comac_pattern_union_t *pattern;

    if (abstract_pattern->has_component_alpha)
	return FALSE;

    pattern = (comac_pattern_union_t *) abstract_pattern;
    assert (pattern->solid.color.colorspace == COMAC_COLORSPACE_RGB);
    switch (abstract_pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	return COMAC_COLOR_IS_CLEAR (&pattern->solid.color.c.rgb);
    case COMAC_PATTERN_TYPE_SURFACE:
	return _surface_is_clear (&pattern->surface);
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	return _raster_source_is_clear (&pattern->raster_source);
    case COMAC_PATTERN_TYPE_LINEAR:
    case COMAC_PATTERN_TYPE_RADIAL:
	return _gradient_is_clear (&pattern->gradient.base, NULL);
    case COMAC_PATTERN_TYPE_MESH:
	return _mesh_is_clear (&pattern->mesh);
    }

    ASSERT_NOT_REACHED;
    return FALSE;
}

/*
 * Will given row of back-translation matrix work with bilinear scale?
 * This is true for scales larger than 1. Also it was judged acceptable
 * for scales larger than .75. And if there is integer translation
 * then a scale of exactly .5 works.
 */
static int
use_bilinear (double x, double y, double t)
{
    /* This is the inverse matrix! */
    double h = x * x + y * y;
    if (h < 1.0 / (0.75 * 0.75))
	return TRUE;			      /* scale > .75 */
    if ((h > 3.99 && h < 4.01)		      /* scale is 1/2 */
	&& ! _comac_fixed_from_double (x * y) /* parallel to an axis */
	&& _comac_fixed_is_integer (_comac_fixed_from_double (t)))
	return TRUE;
    return FALSE;
}

/**
 * _comac_pattern_analyze_filter:
 * @pattern: surface pattern
 * Returns: the optimized #comac_filter_t to use with @pattern.
 *
 * Possibly optimize the filter to a simpler value depending on transformation
 **/
comac_filter_t
_comac_pattern_analyze_filter (const comac_pattern_t *pattern)
{
    switch (pattern->filter) {
    case COMAC_FILTER_GOOD:
    case COMAC_FILTER_BEST:
    case COMAC_FILTER_BILINEAR:
    case COMAC_FILTER_FAST:
	/* If source pixels map 1:1 onto destination pixels, we do
	 * not need to filter (and do not want to filter, since it
	 * will cause blurriness)
	 */
	if (_comac_matrix_is_pixel_exact (&pattern->matrix)) {
	    return COMAC_FILTER_NEAREST;
	} else {
	    /* Use BILINEAR for any scale greater than .75 instead
	     * of GOOD. For scales of 1 and larger this is identical,
	     * for the smaller sizes it was judged that the artifacts
	     * were not worse than the artifacts from a box filer.
	     * BILINEAR can also be used if the scale is exactly .5
	     * and the translation in that direction is an integer.
	     */
	    if (pattern->filter == COMAC_FILTER_GOOD &&
		use_bilinear (pattern->matrix.xx,
			      pattern->matrix.xy,
			      pattern->matrix.x0) &&
		use_bilinear (pattern->matrix.yx,
			      pattern->matrix.yy,
			      pattern->matrix.y0))
		return COMAC_FILTER_BILINEAR;
	}
	break;

    case COMAC_FILTER_NEAREST:
    case COMAC_FILTER_GAUSSIAN:
    default:
	break;
    }

    return pattern->filter;
}

/**
 * _comac_hypot:
 * Returns: value similar to hypot(@x,@y)
 *
 * May want to replace this with Manhattan distance (abs(x)+abs(y)) if
 * hypot is too slow, as there is no need for accuracy here.
 **/
static inline double
_comac_hypot (double x, double y)
{
    return hypot (x, y);
}

/**
 * _comac_pattern_sampled_area:
 *
 * Return region of @pattern that will be sampled to fill @extents,
 * based on the transformation and filter.
 *
 * This does not include pixels that are mulitiplied by values very
 * close to zero by the ends of filters. This is so that transforms
 * that should be the identity or 90 degree rotations do not expand
 * the source unexpectedly.
 *
 * XXX: We don't actually have any way of querying the backend for
 *      the filter radius, so we just guess base on what we know that
 *      backends do currently (see bug #10508)
 **/
void
_comac_pattern_sampled_area (const comac_pattern_t *pattern,
			     const comac_rectangle_int_t *extents,
			     comac_rectangle_int_t *sample)
{
    double x1, x2, y1, y2;
    double padx, pady;

    /* Assume filters are interpolating, which means identity
       cannot change the image */
    if (_comac_matrix_is_identity (&pattern->matrix)) {
	*sample = *extents;
	return;
    }

    /* Transform the centers of the corner pixels */
    x1 = extents->x + 0.5;
    y1 = extents->y + 0.5;
    x2 = x1 + (extents->width - 1);
    y2 = y1 + (extents->height - 1);
    _comac_matrix_transform_bounding_box (&pattern->matrix,
					  &x1,
					  &y1,
					  &x2,
					  &y2,
					  NULL);

    /* How far away from center will it actually sample?
     * This is the distance from a transformed pixel center to the
     * furthest sample of reasonable size.
     */
    switch (pattern->filter) {
    case COMAC_FILTER_NEAREST:
    case COMAC_FILTER_FAST:
	/* Correct value is zero, but when the sample is on an integer
	 * it is unknown if the backend will sample the pixel to the
	 * left or right. This value makes it include both possible pixels.
	 */
	padx = pady = 0.004;
	break;
    case COMAC_FILTER_BILINEAR:
    case COMAC_FILTER_GAUSSIAN:
    default:
	/* Correct value is .5 */
	padx = pady = 0.495;
	break;
    case COMAC_FILTER_GOOD:
	/* Correct value is max(width,1)*.5 */
	padx = _comac_hypot (pattern->matrix.xx, pattern->matrix.xy);
	if (padx <= 1.0)
	    padx = 0.495;
	else if (padx >= 16.0)
	    padx = 7.92;
	else
	    padx *= 0.495;
	pady = _comac_hypot (pattern->matrix.yx, pattern->matrix.yy);
	if (pady <= 1.0)
	    pady = 0.495;
	else if (pady >= 16.0)
	    pady = 7.92;
	else
	    pady *= 0.495;
	break;
    case COMAC_FILTER_BEST:
	/* Correct value is width*2 */
	padx = _comac_hypot (pattern->matrix.xx, pattern->matrix.xy) * 1.98;
	if (padx > 7.92)
	    padx = 7.92;
	pady = _comac_hypot (pattern->matrix.yx, pattern->matrix.yy) * 1.98;
	if (pady > 7.92)
	    pady = 7.92;
	break;
    }

    /* round furthest samples to edge of pixels */
    x1 = floor (x1 - padx);
    if (x1 < COMAC_RECT_INT_MIN)
	x1 = COMAC_RECT_INT_MIN;
    sample->x = x1;

    y1 = floor (y1 - pady);
    if (y1 < COMAC_RECT_INT_MIN)
	y1 = COMAC_RECT_INT_MIN;
    sample->y = y1;

    x2 = floor (x2 + padx) + 1.0;
    if (x2 > COMAC_RECT_INT_MAX)
	x2 = COMAC_RECT_INT_MAX;
    sample->width = x2 - x1;

    y2 = floor (y2 + pady) + 1.0;
    if (y2 > COMAC_RECT_INT_MAX)
	y2 = COMAC_RECT_INT_MAX;
    sample->height = y2 - y1;
}

/**
 * _comac_pattern_get_extents:
 *
 * Return the "target-space" extents of @pattern in @extents.
 *
 * For unbounded patterns, the @extents will be initialized with
 * "infinite" extents, (minimum and maximum fixed-point values).
 *
 * When is_vector is TRUE, avoid rounding to zero widths or heights that
 * are less than 1 unit.
 *
 * XXX: Currently, bounded gradient patterns will also return
 * "infinite" extents, though it would be possible to optimize these
 * with a little more work.
 **/
void
_comac_pattern_get_extents (const comac_pattern_t *pattern,
			    comac_rectangle_int_t *extents,
			    comac_bool_t is_vector)
{
    double x1, y1, x2, y2;
    int ix1, ix2, iy1, iy2;
    comac_bool_t round_x = FALSE;
    comac_bool_t round_y = FALSE;

    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	goto UNBOUNDED;

    case COMAC_PATTERN_TYPE_SURFACE: {
	comac_rectangle_int_t surface_extents;
	const comac_surface_pattern_t *surface_pattern =
	    (const comac_surface_pattern_t *) pattern;
	comac_surface_t *surface = surface_pattern->surface;

	if (! _comac_surface_get_extents (surface, &surface_extents))
	    goto UNBOUNDED;

	if (surface_extents.width == 0 || surface_extents.height == 0)
	    goto EMPTY;

	if (pattern->extend != COMAC_EXTEND_NONE)
	    goto UNBOUNDED;

	x1 = surface_extents.x;
	y1 = surface_extents.y;
	x2 = surface_extents.x + (int) surface_extents.width;
	y2 = surface_extents.y + (int) surface_extents.height;

	goto HANDLE_FILTER;
    } break;

    case COMAC_PATTERN_TYPE_RASTER_SOURCE: {
	const comac_raster_source_pattern_t *raster =
	    (const comac_raster_source_pattern_t *) pattern;

	if (raster->extents.width == 0 || raster->extents.height == 0)
	    goto EMPTY;

	if (pattern->extend != COMAC_EXTEND_NONE)
	    goto UNBOUNDED;

	x1 = raster->extents.x;
	y1 = raster->extents.y;
	x2 = raster->extents.x + (int) raster->extents.width;
	y2 = raster->extents.y + (int) raster->extents.height;
    }
    HANDLE_FILTER:
	switch (pattern->filter) {
	case COMAC_FILTER_NEAREST:
	case COMAC_FILTER_FAST:
	    round_x = round_y = TRUE;
	    /* We don't know which way .5 will go, so fudge it slightly. */
	    x1 -= 0.004;
	    y1 -= 0.004;
	    x2 += 0.004;
	    y2 += 0.004;
	    break;
	case COMAC_FILTER_BEST:
	    /* Assume best filter will produce nice antialiased edges */
	    break;
	case COMAC_FILTER_BILINEAR:
	case COMAC_FILTER_GAUSSIAN:
	case COMAC_FILTER_GOOD:
	default:
	    /* These filters can blur the edge out 1/2 pixel when scaling up */
	    if (_comac_hypot (pattern->matrix.xx, pattern->matrix.yx) < 1.0) {
		x1 -= 0.5;
		x2 += 0.5;
		round_x = TRUE;
	    }
	    if (_comac_hypot (pattern->matrix.xy, pattern->matrix.yy) < 1.0) {
		y1 -= 0.5;
		y2 += 0.5;
		round_y = TRUE;
	    }
	    break;
	}
	break;

    case COMAC_PATTERN_TYPE_RADIAL: {
	const comac_radial_pattern_t *radial =
	    (const comac_radial_pattern_t *) pattern;
	double cx1, cy1;
	double cx2, cy2;
	double r1, r2;

	if (_radial_pattern_is_degenerate (radial)) {
	    /* comac-gstate should have optimised degenerate
		 * patterns to solid clear patterns, so we can ignore
		 * them here. */
	    goto EMPTY;
	}

	/* TODO: in some cases (focus outside/on the circle) it is
	     * half-bounded. */
	if (pattern->extend != COMAC_EXTEND_NONE)
	    goto UNBOUNDED;

	cx1 = radial->cd1.center.x;
	cy1 = radial->cd1.center.y;
	r1 = radial->cd1.radius;

	cx2 = radial->cd2.center.x;
	cy2 = radial->cd2.center.y;
	r2 = radial->cd2.radius;

	x1 = MIN (cx1 - r1, cx2 - r2);
	y1 = MIN (cy1 - r1, cy2 - r2);
	x2 = MAX (cx1 + r1, cx2 + r2);
	y2 = MAX (cy1 + r1, cy2 + r2);
    } break;

    case COMAC_PATTERN_TYPE_LINEAR: {
	const comac_linear_pattern_t *linear =
	    (const comac_linear_pattern_t *) pattern;

	if (pattern->extend != COMAC_EXTEND_NONE)
	    goto UNBOUNDED;

	if (_linear_pattern_is_degenerate (linear)) {
	    /* comac-gstate should have optimised degenerate
		 * patterns to solid ones, so we can again ignore
		 * them here. */
	    goto EMPTY;
	}

	/* TODO: to get tight extents, use the matrix to transform
	     * the pattern instead of transforming the extents later. */
	if (pattern->matrix.xy != 0. || pattern->matrix.yx != 0.)
	    goto UNBOUNDED;

	if (linear->pd1.x == linear->pd2.x) {
	    x1 = -HUGE_VAL;
	    x2 = HUGE_VAL;
	    y1 = MIN (linear->pd1.y, linear->pd2.y);
	    y2 = MAX (linear->pd1.y, linear->pd2.y);
	} else if (linear->pd1.y == linear->pd2.y) {
	    x1 = MIN (linear->pd1.x, linear->pd2.x);
	    x2 = MAX (linear->pd1.x, linear->pd2.x);
	    y1 = -HUGE_VAL;
	    y2 = HUGE_VAL;
	} else {
	    goto UNBOUNDED;
	}

	/* The current linear renderer just point-samples in the middle
	       of the pixels, similar to the NEAREST filter: */
	round_x = round_y = TRUE;
    } break;

    case COMAC_PATTERN_TYPE_MESH: {
	const comac_mesh_pattern_t *mesh =
	    (const comac_mesh_pattern_t *) pattern;
	if (! _comac_mesh_pattern_coord_box (mesh, &x1, &y1, &x2, &y2))
	    goto EMPTY;
    } break;

    default:
	ASSERT_NOT_REACHED;
    }

    if (_comac_matrix_is_translation (&pattern->matrix)) {
	x1 -= pattern->matrix.x0;
	x2 -= pattern->matrix.x0;
	y1 -= pattern->matrix.y0;
	y2 -= pattern->matrix.y0;
    } else {
	comac_matrix_t imatrix;
	comac_status_t status;

	imatrix = pattern->matrix;
	status = comac_matrix_invert (&imatrix);
	/* comac_pattern_set_matrix ensures the matrix is invertible */
	assert (status == COMAC_STATUS_SUCCESS);

	_comac_matrix_transform_bounding_box (&imatrix,
					      &x1,
					      &y1,
					      &x2,
					      &y2,
					      NULL);
    }

    if (! round_x) {
	x1 -= 0.5;
	x2 += 0.5;
    }
    if (x1 < COMAC_RECT_INT_MIN)
	ix1 = COMAC_RECT_INT_MIN;
    else
	ix1 = _comac_lround (x1);
    if (x2 > COMAC_RECT_INT_MAX)
	ix2 = COMAC_RECT_INT_MAX;
    else
	ix2 = _comac_lround (x2);
    extents->x = ix1;
    extents->width = ix2 - ix1;
    if (is_vector && extents->width == 0 && x1 != x2)
	extents->width += 1;

    if (! round_y) {
	y1 -= 0.5;
	y2 += 0.5;
    }
    if (y1 < COMAC_RECT_INT_MIN)
	iy1 = COMAC_RECT_INT_MIN;
    else
	iy1 = _comac_lround (y1);
    if (y2 > COMAC_RECT_INT_MAX)
	iy2 = COMAC_RECT_INT_MAX;
    else
	iy2 = _comac_lround (y2);
    extents->y = iy1;
    extents->height = iy2 - iy1;
    if (is_vector && extents->height == 0 && y1 != y2)
	extents->height += 1;

    return;

UNBOUNDED:
    /* unbounded patterns -> 'infinite' extents */
    _comac_unbounded_rectangle_init (extents);
    return;

EMPTY:
    extents->x = extents->y = 0;
    extents->width = extents->height = 0;
    return;
}

/**
 * _comac_pattern_get_ink_extents:
 *
 * Return the "target-space" inked extents of @pattern in @extents.
 **/
comac_int_status_t
_comac_pattern_get_ink_extents (const comac_pattern_t *pattern,
				comac_rectangle_int_t *extents)
{
    if (pattern->type == COMAC_PATTERN_TYPE_SURFACE &&
	pattern->extend == COMAC_EXTEND_NONE) {
	const comac_surface_pattern_t *surface_pattern =
	    (const comac_surface_pattern_t *) pattern;
	comac_surface_t *surface = surface_pattern->surface;

	surface = _comac_surface_get_source (surface, NULL);
	if (_comac_surface_is_recording (surface)) {
	    comac_matrix_t imatrix;
	    comac_box_t box;
	    comac_status_t status;

	    imatrix = pattern->matrix;
	    status = comac_matrix_invert (&imatrix);
	    /* comac_pattern_set_matrix ensures the matrix is invertible */
	    assert (status == COMAC_STATUS_SUCCESS);

	    status = _comac_recording_surface_get_ink_bbox (
		(comac_recording_surface_t *) surface,
		&box,
		&imatrix);
	    if (unlikely (status))
		return status;

	    _comac_box_round_to_rectangle (&box, extents);
	    return COMAC_STATUS_SUCCESS;
	}
    }

    _comac_pattern_get_extents (pattern, extents, TRUE);
    return COMAC_STATUS_SUCCESS;
}

static uintptr_t
_comac_solid_pattern_hash (uintptr_t hash, const comac_solid_pattern_t *solid)
{
    hash = _comac_hash_bytes (hash, &solid->color, sizeof (solid->color));

    return hash;
}

static uintptr_t
_comac_gradient_color_stops_hash (uintptr_t hash,
				  const comac_gradient_pattern_t *gradient)
{
    unsigned int n;

    hash = _comac_hash_bytes (hash,
			      &gradient->n_stops,
			      sizeof (gradient->n_stops));

    for (n = 0; n < gradient->n_stops; n++) {
	hash = _comac_hash_bytes (hash,
				  &gradient->stops[n].offset,
				  sizeof (double));
	hash = _comac_hash_bytes (hash,
				  &gradient->stops[n].color,
				  sizeof (comac_color_stop_t));
    }

    return hash;
}

uintptr_t
_comac_linear_pattern_hash (uintptr_t hash,
			    const comac_linear_pattern_t *linear)
{
    hash = _comac_hash_bytes (hash, &linear->pd1, sizeof (linear->pd1));
    hash = _comac_hash_bytes (hash, &linear->pd2, sizeof (linear->pd2));

    return _comac_gradient_color_stops_hash (hash, &linear->base);
}

uintptr_t
_comac_radial_pattern_hash (uintptr_t hash,
			    const comac_radial_pattern_t *radial)
{
    hash = _comac_hash_bytes (hash,
			      &radial->cd1.center,
			      sizeof (radial->cd1.center));
    hash = _comac_hash_bytes (hash,
			      &radial->cd1.radius,
			      sizeof (radial->cd1.radius));
    hash = _comac_hash_bytes (hash,
			      &radial->cd2.center,
			      sizeof (radial->cd2.center));
    hash = _comac_hash_bytes (hash,
			      &radial->cd2.radius,
			      sizeof (radial->cd2.radius));

    return _comac_gradient_color_stops_hash (hash, &radial->base);
}

static uintptr_t
_comac_mesh_pattern_hash (uintptr_t hash, const comac_mesh_pattern_t *mesh)
{
    const comac_mesh_patch_t *patch =
	_comac_array_index_const (&mesh->patches, 0);
    unsigned int i, n = _comac_array_num_elements (&mesh->patches);

    for (i = 0; i < n; i++)
	hash = _comac_hash_bytes (hash, patch + i, sizeof (comac_mesh_patch_t));

    return hash;
}

static uintptr_t
_comac_surface_pattern_hash (uintptr_t hash,
			     const comac_surface_pattern_t *surface)
{
    hash ^= surface->surface->unique_id;

    return hash;
}

static uintptr_t
_comac_raster_source_pattern_hash (uintptr_t hash,
				   const comac_raster_source_pattern_t *raster)
{
    hash ^= (uintptr_t) raster->user_data;

    return hash;
}

uintptr_t
_comac_pattern_hash (const comac_pattern_t *pattern)
{
    uintptr_t hash = _COMAC_HASH_INIT_VALUE;

    if (pattern->status)
	return 0;

    hash = _comac_hash_bytes (hash, &pattern->type, sizeof (pattern->type));
    if (pattern->type != COMAC_PATTERN_TYPE_SOLID) {
	hash = _comac_hash_bytes (hash,
				  &pattern->matrix,
				  sizeof (pattern->matrix));
	hash = _comac_hash_bytes (hash,
				  &pattern->filter,
				  sizeof (pattern->filter));
	hash = _comac_hash_bytes (hash,
				  &pattern->extend,
				  sizeof (pattern->extend));
	hash = _comac_hash_bytes (hash,
				  &pattern->has_component_alpha,
				  sizeof (pattern->has_component_alpha));
    }

    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	return _comac_solid_pattern_hash (hash,
					  (comac_solid_pattern_t *) pattern);
    case COMAC_PATTERN_TYPE_LINEAR:
	return _comac_linear_pattern_hash (hash,
					   (comac_linear_pattern_t *) pattern);
    case COMAC_PATTERN_TYPE_RADIAL:
	return _comac_radial_pattern_hash (hash,
					   (comac_radial_pattern_t *) pattern);
    case COMAC_PATTERN_TYPE_MESH:
	return _comac_mesh_pattern_hash (hash,
					 (comac_mesh_pattern_t *) pattern);
    case COMAC_PATTERN_TYPE_SURFACE:
	return _comac_surface_pattern_hash (
	    hash,
	    (comac_surface_pattern_t *) pattern);
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	return _comac_raster_source_pattern_hash (
	    hash,
	    (comac_raster_source_pattern_t *) pattern);
    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

static comac_bool_t
_comac_solid_pattern_equal (const comac_solid_pattern_t *a,
			    const comac_solid_pattern_t *b)
{
    return _comac_color_equal (&a->color, &b->color);
}

static comac_bool_t
_comac_gradient_color_stops_equal (const comac_gradient_pattern_t *a,
				   const comac_gradient_pattern_t *b)
{
    unsigned int n;

    if (a->n_stops != b->n_stops)
	return FALSE;

    for (n = 0; n < a->n_stops; n++) {
	if (a->stops[n].offset != b->stops[n].offset)
	    return FALSE;
	if (! _comac_color_stop_equal (&a->stops[n].color, &b->stops[n].color))
	    return FALSE;
    }

    return TRUE;
}

comac_bool_t
_comac_linear_pattern_equal (const comac_linear_pattern_t *a,
			     const comac_linear_pattern_t *b)
{
    if (a->pd1.x != b->pd1.x)
	return FALSE;

    if (a->pd1.y != b->pd1.y)
	return FALSE;

    if (a->pd2.x != b->pd2.x)
	return FALSE;

    if (a->pd2.y != b->pd2.y)
	return FALSE;

    return _comac_gradient_color_stops_equal (&a->base, &b->base);
}

comac_bool_t
_comac_radial_pattern_equal (const comac_radial_pattern_t *a,
			     const comac_radial_pattern_t *b)
{
    if (a->cd1.center.x != b->cd1.center.x)
	return FALSE;

    if (a->cd1.center.y != b->cd1.center.y)
	return FALSE;

    if (a->cd1.radius != b->cd1.radius)
	return FALSE;

    if (a->cd2.center.x != b->cd2.center.x)
	return FALSE;

    if (a->cd2.center.y != b->cd2.center.y)
	return FALSE;

    if (a->cd2.radius != b->cd2.radius)
	return FALSE;

    return _comac_gradient_color_stops_equal (&a->base, &b->base);
}

static comac_bool_t
_comac_mesh_pattern_equal (const comac_mesh_pattern_t *a,
			   const comac_mesh_pattern_t *b)
{
    const comac_mesh_patch_t *patch_a, *patch_b;
    unsigned int i, num_patches_a, num_patches_b;

    num_patches_a = _comac_array_num_elements (&a->patches);
    num_patches_b = _comac_array_num_elements (&b->patches);

    if (num_patches_a != num_patches_b)
	return FALSE;

    for (i = 0; i < num_patches_a; i++) {
	patch_a = _comac_array_index_const (&a->patches, i);
	patch_b = _comac_array_index_const (&b->patches, i);
	if (memcmp (patch_a, patch_b, sizeof (comac_mesh_patch_t)) != 0)
	    return FALSE;
    }

    return TRUE;
}

static comac_bool_t
_comac_surface_pattern_equal (const comac_surface_pattern_t *a,
			      const comac_surface_pattern_t *b)
{
    return a->surface->unique_id == b->surface->unique_id;
}

static comac_bool_t
_comac_raster_source_pattern_equal (const comac_raster_source_pattern_t *a,
				    const comac_raster_source_pattern_t *b)
{
    return a->user_data == b->user_data;
}

comac_bool_t
_comac_pattern_equal (const comac_pattern_t *a, const comac_pattern_t *b)
{
    if (a->status || b->status)
	return FALSE;

    if (a == b)
	return TRUE;

    if (a->type != b->type)
	return FALSE;

    if (a->has_component_alpha != b->has_component_alpha)
	return FALSE;

    if (a->type != COMAC_PATTERN_TYPE_SOLID) {
	if (memcmp (&a->matrix, &b->matrix, sizeof (comac_matrix_t)))
	    return FALSE;

	if (a->filter != b->filter)
	    return FALSE;

	if (a->extend != b->extend)
	    return FALSE;
    }

    switch (a->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	return _comac_solid_pattern_equal ((comac_solid_pattern_t *) a,
					   (comac_solid_pattern_t *) b);
    case COMAC_PATTERN_TYPE_LINEAR:
	return _comac_linear_pattern_equal ((comac_linear_pattern_t *) a,
					    (comac_linear_pattern_t *) b);
    case COMAC_PATTERN_TYPE_RADIAL:
	return _comac_radial_pattern_equal ((comac_radial_pattern_t *) a,
					    (comac_radial_pattern_t *) b);
    case COMAC_PATTERN_TYPE_MESH:
	return _comac_mesh_pattern_equal ((comac_mesh_pattern_t *) a,
					  (comac_mesh_pattern_t *) b);
    case COMAC_PATTERN_TYPE_SURFACE:
	return _comac_surface_pattern_equal ((comac_surface_pattern_t *) a,
					     (comac_surface_pattern_t *) b);
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	return _comac_raster_source_pattern_equal (
	    (comac_raster_source_pattern_t *) a,
	    (comac_raster_source_pattern_t *) b);
    default:
	ASSERT_NOT_REACHED;
	return FALSE;
    }
}

/**
 * comac_pattern_get_rgba:
 * @pattern: a #comac_pattern_t
 * @red: return value for red component of color, or %NULL
 * @green: return value for green component of color, or %NULL
 * @blue: return value for blue component of color, or %NULL
 * @alpha: return value for alpha component of color, or %NULL
 *
 * Gets the solid color for a solid color pattern.
 *
 * Return value: %COMAC_STATUS_SUCCESS, or
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH if the pattern is not a solid
 * color pattern.
 *
 * Since: 1.4
 **/
comac_status_t
comac_pattern_get_rgba (comac_pattern_t *pattern,
			double *red,
			double *green,
			double *blue,
			double *alpha)
{
    comac_solid_pattern_t *solid = (comac_solid_pattern_t *) pattern;
    double r0, g0, b0, a0;

    if (pattern->status)
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_SOLID)
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    _comac_color_get_rgba (&solid->color, &r0, &g0, &b0, &a0);

    if (red)
	*red = r0;
    if (green)
	*green = g0;
    if (blue)
	*blue = b0;
    if (alpha)
	*alpha = a0;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_pattern_get_surface:
 * @pattern: a #comac_pattern_t
 * @surface: return value for surface of pattern, or %NULL
 *
 * Gets the surface of a surface pattern.  The reference returned in
 * @surface is owned by the pattern; the caller should call
 * comac_surface_reference() if the surface is to be retained.
 *
 * Return value: %COMAC_STATUS_SUCCESS, or
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH if the pattern is not a surface
 * pattern.
 *
 * Since: 1.4
 **/
comac_status_t
comac_pattern_get_surface (comac_pattern_t *pattern, comac_surface_t **surface)
{
    comac_surface_pattern_t *spat = (comac_surface_pattern_t *) pattern;

    if (pattern->status)
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_SURFACE)
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (surface)
	*surface = spat->surface;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_pattern_get_color_stop_rgba:
 * @pattern: a #comac_pattern_t
 * @index: index of the stop to return data for
 * @offset: return value for the offset of the stop, or %NULL
 * @red: return value for red component of color, or %NULL
 * @green: return value for green component of color, or %NULL
 * @blue: return value for blue component of color, or %NULL
 * @alpha: return value for alpha component of color, or %NULL
 *
 * Gets the color and offset information at the given @index for a
 * gradient pattern.  Values of @index range from 0 to n-1
 * where n is the number returned
 * by comac_pattern_get_color_stop_count().
 *
 * Return value: %COMAC_STATUS_SUCCESS, or %COMAC_STATUS_INVALID_INDEX
 * if @index is not valid for the given pattern.  If the pattern is
 * not a gradient pattern, %COMAC_STATUS_PATTERN_TYPE_MISMATCH is
 * returned.
 *
 * Since: 1.4
 **/
comac_status_t
comac_pattern_get_color_stop_rgba (comac_pattern_t *pattern,
				   int index,
				   double *offset,
				   double *red,
				   double *green,
				   double *blue,
				   double *alpha)
{
    comac_gradient_pattern_t *gradient = (comac_gradient_pattern_t *) pattern;

    if (pattern->status)
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_LINEAR &&
	pattern->type != COMAC_PATTERN_TYPE_RADIAL)
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (index < 0 || (unsigned int) index >= gradient->n_stops)
	return _comac_error (COMAC_STATUS_INVALID_INDEX);

    if (offset)
	*offset = gradient->stops[index].offset;
    if (red)
	*red = gradient->stops[index].color.red;
    if (green)
	*green = gradient->stops[index].color.green;
    if (blue)
	*blue = gradient->stops[index].color.blue;
    if (alpha)
	*alpha = gradient->stops[index].color.alpha;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_pattern_get_color_stop_count:
 * @pattern: a #comac_pattern_t
 * @count: return value for the number of color stops, or %NULL
 *
 * Gets the number of color stops specified in the given gradient
 * pattern.
 *
 * Return value: %COMAC_STATUS_SUCCESS, or
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH if @pattern is not a gradient
 * pattern.
 *
 * Since: 1.4
 **/
comac_status_t
comac_pattern_get_color_stop_count (comac_pattern_t *pattern, int *count)
{
    comac_gradient_pattern_t *gradient = (comac_gradient_pattern_t *) pattern;

    if (pattern->status)
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_LINEAR &&
	pattern->type != COMAC_PATTERN_TYPE_RADIAL)
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (count)
	*count = gradient->n_stops;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_pattern_get_linear_points:
 * @pattern: a #comac_pattern_t
 * @x0: return value for the x coordinate of the first point, or %NULL
 * @y0: return value for the y coordinate of the first point, or %NULL
 * @x1: return value for the x coordinate of the second point, or %NULL
 * @y1: return value for the y coordinate of the second point, or %NULL
 *
 * Gets the gradient endpoints for a linear gradient.
 *
 * Return value: %COMAC_STATUS_SUCCESS, or
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH if @pattern is not a linear
 * gradient pattern.
 *
 * Since: 1.4
 **/
comac_status_t
comac_pattern_get_linear_points (
    comac_pattern_t *pattern, double *x0, double *y0, double *x1, double *y1)
{
    comac_linear_pattern_t *linear = (comac_linear_pattern_t *) pattern;

    if (pattern->status)
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_LINEAR)
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (x0)
	*x0 = linear->pd1.x;
    if (y0)
	*y0 = linear->pd1.y;
    if (x1)
	*x1 = linear->pd2.x;
    if (y1)
	*y1 = linear->pd2.y;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_pattern_get_radial_circles:
 * @pattern: a #comac_pattern_t
 * @x0: return value for the x coordinate of the center of the first circle, or %NULL
 * @y0: return value for the y coordinate of the center of the first circle, or %NULL
 * @r0: return value for the radius of the first circle, or %NULL
 * @x1: return value for the x coordinate of the center of the second circle, or %NULL
 * @y1: return value for the y coordinate of the center of the second circle, or %NULL
 * @r1: return value for the radius of the second circle, or %NULL
 *
 * Gets the gradient endpoint circles for a radial gradient, each
 * specified as a center coordinate and a radius.
 *
 * Return value: %COMAC_STATUS_SUCCESS, or
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH if @pattern is not a radial
 * gradient pattern.
 *
 * Since: 1.4
 **/
comac_status_t
comac_pattern_get_radial_circles (comac_pattern_t *pattern,
				  double *x0,
				  double *y0,
				  double *r0,
				  double *x1,
				  double *y1,
				  double *r1)
{
    comac_radial_pattern_t *radial = (comac_radial_pattern_t *) pattern;

    if (pattern->status)
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_RADIAL)
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (x0)
	*x0 = radial->cd1.center.x;
    if (y0)
	*y0 = radial->cd1.center.y;
    if (r0)
	*r0 = radial->cd1.radius;
    if (x1)
	*x1 = radial->cd2.center.x;
    if (y1)
	*y1 = radial->cd2.center.y;
    if (r1)
	*r1 = radial->cd2.radius;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_mesh_pattern_get_patch_count:
 * @pattern: a #comac_pattern_t
 * @count: return value for the number patches, or %NULL
 *
 * Gets the number of patches specified in the given mesh pattern.
 *
 * The number only includes patches which have been finished by
 * calling comac_mesh_pattern_end_patch(). For example it will be 0
 * during the definition of the first patch.
 *
 * Return value: %COMAC_STATUS_SUCCESS, or
 * %COMAC_STATUS_PATTERN_TYPE_MISMATCH if @pattern is not a mesh
 * pattern.
 *
 * Since: 1.12
 **/
comac_status_t
comac_mesh_pattern_get_patch_count (comac_pattern_t *pattern,
				    unsigned int *count)
{
    comac_mesh_pattern_t *mesh = (comac_mesh_pattern_t *) pattern;

    if (unlikely (pattern->status))
	return pattern->status;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH))
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (count) {
	*count = _comac_array_num_elements (&mesh->patches);
	if (mesh->current_patch)
	    *count -= 1;
    }

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_mesh_pattern_get_path:
 * @pattern: a #comac_pattern_t
 * @patch_num: the patch number to return data for
 *
 * Gets path defining the patch @patch_num for a mesh
 * pattern.
 *
 * @patch_num can range from 0 to n-1 where n is the number returned by
 * comac_mesh_pattern_get_patch_count().
 *
 * Return value: the path defining the patch, or a path with status
 * %COMAC_STATUS_INVALID_INDEX if @patch_num or @point_num is not
 * valid for @pattern. If @pattern is not a mesh pattern, a path with
 * status %COMAC_STATUS_PATTERN_TYPE_MISMATCH is returned.
 *
 * Since: 1.12
 **/
comac_path_t *
comac_mesh_pattern_get_path (comac_pattern_t *pattern, unsigned int patch_num)
{
    comac_mesh_pattern_t *mesh = (comac_mesh_pattern_t *) pattern;
    const comac_mesh_patch_t *patch;
    comac_path_t *path;
    comac_path_data_t *data;
    unsigned int patch_count;
    int l, current_point;

    if (unlikely (pattern->status))
	return _comac_path_create_in_error (pattern->status);

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH))
	return _comac_path_create_in_error (
	    _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH));

    patch_count = _comac_array_num_elements (&mesh->patches);
    if (mesh->current_patch)
	patch_count--;

    if (unlikely (patch_num >= patch_count))
	return _comac_path_create_in_error (
	    _comac_error (COMAC_STATUS_INVALID_INDEX));

    patch = _comac_array_index_const (&mesh->patches, patch_num);

    path = _comac_malloc (sizeof (comac_path_t));
    if (path == NULL)
	return _comac_path_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    path->num_data = 18;
    path->data = _comac_malloc_ab (path->num_data, sizeof (comac_path_data_t));
    if (path->data == NULL) {
	free (path);
	return _comac_path_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));
    }

    data = path->data;
    data[0].header.type = COMAC_PATH_MOVE_TO;
    data[0].header.length = 2;
    data[1].point.x = patch->points[0][0].x;
    data[1].point.y = patch->points[0][0].y;
    data += data[0].header.length;

    current_point = 0;

    for (l = 0; l < 4; l++) {
	int i, j, k;

	data[0].header.type = COMAC_PATH_CURVE_TO;
	data[0].header.length = 4;

	for (k = 1; k < 4; k++) {
	    current_point = (current_point + 1) % 12;
	    i = mesh_path_point_i[current_point];
	    j = mesh_path_point_j[current_point];
	    data[k].point.x = patch->points[i][j].x;
	    data[k].point.y = patch->points[i][j].y;
	}

	data += data[0].header.length;
    }

    path->status = COMAC_STATUS_SUCCESS;

    return path;
}

/**
 * comac_mesh_pattern_get_corner_color_rgba:
 * @pattern: a #comac_pattern_t
 * @patch_num: the patch number to return data for
 * @corner_num: the corner number to return data for
 * @red: return value for red component of color, or %NULL
 * @green: return value for green component of color, or %NULL
 * @blue: return value for blue component of color, or %NULL
 * @alpha: return value for alpha component of color, or %NULL
 *
 * Gets the color information in corner @corner_num of patch
 * @patch_num for a mesh pattern.
 *
 * @patch_num can range from 0 to n-1 where n is the number returned by
 * comac_mesh_pattern_get_patch_count().
 *
 * Valid values for @corner_num are from 0 to 3 and identify the
 * corners as explained in comac_pattern_create_mesh().
 *
 * Return value: %COMAC_STATUS_SUCCESS, or %COMAC_STATUS_INVALID_INDEX
 * if @patch_num or @corner_num is not valid for @pattern. If
 * @pattern is not a mesh pattern, %COMAC_STATUS_PATTERN_TYPE_MISMATCH
 * is returned.
 *
 * Since: 1.12
 **/
comac_status_t
comac_mesh_pattern_get_corner_color_rgba (comac_pattern_t *pattern,
					  unsigned int patch_num,
					  unsigned int corner_num,
					  double *red,
					  double *green,
					  double *blue,
					  double *alpha)
{
    comac_mesh_pattern_t *mesh = (comac_mesh_pattern_t *) pattern;
    unsigned int patch_count;
    const comac_mesh_patch_t *patch;

    if (unlikely (pattern->status))
	return pattern->status;

    if (unlikely (pattern->type != COMAC_PATTERN_TYPE_MESH))
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (unlikely (corner_num > 3))
	return _comac_error (COMAC_STATUS_INVALID_INDEX);

    patch_count = _comac_array_num_elements (&mesh->patches);
    if (mesh->current_patch)
	patch_count--;

    if (unlikely (patch_num >= patch_count))
	return _comac_error (COMAC_STATUS_INVALID_INDEX);

    patch = _comac_array_index_const (&mesh->patches, patch_num);

    assert (patch->colors[corner_num].colorspace == COMAC_COLORSPACE_RGB);
    if (red)
	*red = patch->colors[corner_num].c.rgb.red;
    if (green)
	*green = patch->colors[corner_num].c.rgb.green;
    if (blue)
	*blue = patch->colors[corner_num].c.rgb.blue;
    if (alpha)
	*alpha = patch->colors[corner_num].c.rgb.alpha;

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_mesh_pattern_get_control_point:
 * @pattern: a #comac_pattern_t
 * @patch_num: the patch number to return data for
 * @point_num: the control point number to return data for
 * @x: return value for the x coordinate of the control point, or %NULL
 * @y: return value for the y coordinate of the control point, or %NULL
 *
 * Gets the control point @point_num of patch @patch_num for a mesh
 * pattern.
 *
 * @patch_num can range from 0 to n-1 where n is the number returned by
 * comac_mesh_pattern_get_patch_count().
 *
 * Valid values for @point_num are from 0 to 3 and identify the
 * control points as explained in comac_pattern_create_mesh().
 *
 * Return value: %COMAC_STATUS_SUCCESS, or %COMAC_STATUS_INVALID_INDEX
 * if @patch_num or @point_num is not valid for @pattern. If @pattern
 * is not a mesh pattern, %COMAC_STATUS_PATTERN_TYPE_MISMATCH is
 * returned.
 *
 * Since: 1.12
 **/
comac_status_t
comac_mesh_pattern_get_control_point (comac_pattern_t *pattern,
				      unsigned int patch_num,
				      unsigned int point_num,
				      double *x,
				      double *y)
{
    comac_mesh_pattern_t *mesh = (comac_mesh_pattern_t *) pattern;
    const comac_mesh_patch_t *patch;
    unsigned int patch_count;
    int i, j;

    if (pattern->status)
	return pattern->status;

    if (pattern->type != COMAC_PATTERN_TYPE_MESH)
	return _comac_error (COMAC_STATUS_PATTERN_TYPE_MISMATCH);

    if (point_num > 3)
	return _comac_error (COMAC_STATUS_INVALID_INDEX);

    patch_count = _comac_array_num_elements (&mesh->patches);
    if (mesh->current_patch)
	patch_count--;

    if (unlikely (patch_num >= patch_count))
	return _comac_error (COMAC_STATUS_INVALID_INDEX);

    patch = _comac_array_index_const (&mesh->patches, patch_num);

    i = mesh_control_point_i[point_num];
    j = mesh_control_point_j[point_num];

    if (x)
	*x = patch->points[i][j].x;
    if (y)
	*y = patch->points[i][j].y;

    return COMAC_STATUS_SUCCESS;
}

void
_comac_pattern_reset_static_data (void)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (freed_pattern_pool); i++)
	_freed_pool_reset (&freed_pattern_pool[i]);
}

static void
_comac_debug_print_surface_pattern (FILE *file,
				    const comac_surface_pattern_t *pattern)
{
    const char *s;
    switch (pattern->surface->type) {
    case COMAC_SURFACE_TYPE_IMAGE:
	s = "image";
	break;
    case COMAC_SURFACE_TYPE_PDF:
	s = "pdf";
	break;
    case COMAC_SURFACE_TYPE_PS:
	s = "ps";
	break;
    case COMAC_SURFACE_TYPE_XLIB:
	s = "xlib";
	break;
    case COMAC_SURFACE_TYPE_XCB:
	s = "xcb";
	break;
    case COMAC_SURFACE_TYPE_GLITZ:
	s = "glitz";
	break;
    case COMAC_SURFACE_TYPE_QUARTZ:
	s = "quartz";
	break;
    case COMAC_SURFACE_TYPE_WIN32:
	s = "win32";
	break;
    case COMAC_SURFACE_TYPE_BEOS:
	s = "beos";
	break;
    case COMAC_SURFACE_TYPE_DIRECTFB:
	s = "directfb";
	break;
    case COMAC_SURFACE_TYPE_SVG:
	s = "svg";
	break;
    case COMAC_SURFACE_TYPE_OS2:
	s = "os2";
	break;
    case COMAC_SURFACE_TYPE_WIN32_PRINTING:
	s = "win32_printing";
	break;
    case COMAC_SURFACE_TYPE_QUARTZ_IMAGE:
	s = "quartz_image";
	break;
    case COMAC_SURFACE_TYPE_SCRIPT:
	s = "script";
	break;
    case COMAC_SURFACE_TYPE_QT:
	s = "qt";
	break;
    case COMAC_SURFACE_TYPE_RECORDING:
	s = "recording";
	break;
    case COMAC_SURFACE_TYPE_VG:
	s = "vg";
	break;
    case COMAC_SURFACE_TYPE_GL:
	s = "gl";
	break;
    case COMAC_SURFACE_TYPE_DRM:
	s = "drm";
	break;
    case COMAC_SURFACE_TYPE_TEE:
	s = "tee";
	break;
    case COMAC_SURFACE_TYPE_XML:
	s = "xml";
	break;
    case COMAC_SURFACE_TYPE_SKIA:
	s = "skia";
	break; /* Deprecated */
    case COMAC_SURFACE_TYPE_SUBSURFACE:
	s = "subsurface";
	break;
    case COMAC_SURFACE_TYPE_COGL:
	s = "cogl";
	break;
    default:
	s = "invalid";
	ASSERT_NOT_REACHED;
	break;
    }
    fprintf (file, "  surface type: %s\n", s);
}

static void
_comac_debug_print_raster_source_pattern (
    FILE *file, const comac_raster_source_pattern_t *raster)
{
    fprintf (file,
	     "  content: %x, size %dx%d\n",
	     raster->content,
	     raster->extents.width,
	     raster->extents.height);
}

static void
_comac_debug_print_linear_pattern (FILE *file,
				   const comac_linear_pattern_t *pattern)
{
}

static void
_comac_debug_print_radial_pattern (FILE *file,
				   const comac_radial_pattern_t *pattern)
{
}

static void
_comac_debug_print_mesh_pattern (FILE *file,
				 const comac_mesh_pattern_t *pattern)
{
}

void
_comac_debug_print_pattern (FILE *file, const comac_pattern_t *pattern)
{
    const char *s;
    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	s = "solid";
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	s = "surface";
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	s = "linear";
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	s = "radial";
	break;
    case COMAC_PATTERN_TYPE_MESH:
	s = "mesh";
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	s = "raster";
	break;
    default:
	s = "invalid";
	ASSERT_NOT_REACHED;
	break;
    }

    fprintf (file, "pattern: %s\n", s);
    if (pattern->type == COMAC_PATTERN_TYPE_SOLID)
	return;

    switch (pattern->extend) {
    case COMAC_EXTEND_NONE:
	s = "none";
	break;
    case COMAC_EXTEND_REPEAT:
	s = "repeat";
	break;
    case COMAC_EXTEND_REFLECT:
	s = "reflect";
	break;
    case COMAC_EXTEND_PAD:
	s = "pad";
	break;
    default:
	s = "invalid";
	ASSERT_NOT_REACHED;
	break;
    }
    fprintf (file, "  extend: %s\n", s);

    switch (pattern->filter) {
    case COMAC_FILTER_FAST:
	s = "fast";
	break;
    case COMAC_FILTER_GOOD:
	s = "good";
	break;
    case COMAC_FILTER_BEST:
	s = "best";
	break;
    case COMAC_FILTER_NEAREST:
	s = "nearest";
	break;
    case COMAC_FILTER_BILINEAR:
	s = "bilinear";
	break;
    case COMAC_FILTER_GAUSSIAN:
	s = "gaussian";
	break;
    default:
	s = "invalid";
	ASSERT_NOT_REACHED;
	break;
    }
    fprintf (file, "  filter: %s\n", s);
    fprintf (file,
	     "  matrix: [%g %g %g %g %g %g]\n",
	     pattern->matrix.xx,
	     pattern->matrix.yx,
	     pattern->matrix.xy,
	     pattern->matrix.yy,
	     pattern->matrix.x0,
	     pattern->matrix.y0);
    switch (pattern->type) {
    default:
    case COMAC_PATTERN_TYPE_SOLID:
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	_comac_debug_print_raster_source_pattern (
	    file,
	    (comac_raster_source_pattern_t *) pattern);
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	_comac_debug_print_surface_pattern (
	    file,
	    (comac_surface_pattern_t *) pattern);
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	_comac_debug_print_linear_pattern (file,
					   (comac_linear_pattern_t *) pattern);
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	_comac_debug_print_radial_pattern (file,
					   (comac_radial_pattern_t *) pattern);
	break;
    case COMAC_PATTERN_TYPE_MESH:
	_comac_debug_print_mesh_pattern (file,
					 (comac_mesh_pattern_t *) pattern);
	break;
    }
}
