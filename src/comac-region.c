/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc.
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
 *	Owen Taylor <otaylor@redhat.com>
 *      Vladimir Vukicevic <vladimir@pobox.com>
 *      Søren Sandmann <sandmann@daimi.au.dk>
 */

#include "comacint.h"

#include "comac-error-private.h"
#include "comac-region-private.h"

/* XXX need to update pixman headers to be const as appropriate */
#define CONST_CAST (pixman_region32_t *)

/**
 * SECTION:comac-region
 * @Title: Regions
 * @Short_Description: Representing a pixel-aligned area
 *
 * Regions are a simple graphical data type representing an area of 
 * integer-aligned rectangles. They are often used on raster surfaces 
 * to track areas of interest, such as change or clip areas.
 **/

static const comac_region_t _comac_region_nil = {
    COMAC_REFERENCE_COUNT_INVALID, /* ref_count */
    COMAC_STATUS_NO_MEMORY,	   /* status */
};

comac_region_t *
_comac_region_create_in_error (comac_status_t status)
{
    switch (status) {
    case COMAC_STATUS_NO_MEMORY:
	return (comac_region_t *) &_comac_region_nil;

    case COMAC_STATUS_SUCCESS:
    case COMAC_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	/* fall-through */
    case COMAC_STATUS_SURFACE_TYPE_MISMATCH:
    case COMAC_STATUS_INVALID_STATUS:
    case COMAC_STATUS_INVALID_CONTENT:
    case COMAC_STATUS_INVALID_FORMAT:
    case COMAC_STATUS_INVALID_VISUAL:
    case COMAC_STATUS_READ_ERROR:
    case COMAC_STATUS_WRITE_ERROR:
    case COMAC_STATUS_FILE_NOT_FOUND:
    case COMAC_STATUS_TEMP_FILE_ERROR:
    case COMAC_STATUS_INVALID_STRIDE:
    case COMAC_STATUS_INVALID_SIZE:
    case COMAC_STATUS_DEVICE_TYPE_MISMATCH:
    case COMAC_STATUS_DEVICE_ERROR:
    case COMAC_STATUS_INVALID_RESTORE:
    case COMAC_STATUS_INVALID_POP_GROUP:
    case COMAC_STATUS_NO_CURRENT_POINT:
    case COMAC_STATUS_INVALID_MATRIX:
    case COMAC_STATUS_NULL_POINTER:
    case COMAC_STATUS_INVALID_STRING:
    case COMAC_STATUS_INVALID_PATH_DATA:
    case COMAC_STATUS_SURFACE_FINISHED:
    case COMAC_STATUS_PATTERN_TYPE_MISMATCH:
    case COMAC_STATUS_INVALID_DASH:
    case COMAC_STATUS_INVALID_DSC_COMMENT:
    case COMAC_STATUS_INVALID_INDEX:
    case COMAC_STATUS_CLIP_NOT_REPRESENTABLE:
    case COMAC_STATUS_FONT_TYPE_MISMATCH:
    case COMAC_STATUS_USER_FONT_IMMUTABLE:
    case COMAC_STATUS_USER_FONT_ERROR:
    case COMAC_STATUS_NEGATIVE_COUNT:
    case COMAC_STATUS_INVALID_CLUSTERS:
    case COMAC_STATUS_INVALID_SLANT:
    case COMAC_STATUS_INVALID_WEIGHT:
    case COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED:
    case COMAC_STATUS_INVALID_MESH_CONSTRUCTION:
    case COMAC_STATUS_DEVICE_FINISHED:
    case COMAC_STATUS_JBIG2_GLOBAL_MISSING:
    case COMAC_STATUS_PNG_ERROR:
    case COMAC_STATUS_FREETYPE_ERROR:
    case COMAC_STATUS_WIN32_GDI_ERROR:
    case COMAC_STATUS_TAG_ERROR:
    case COMAC_STATUS_DWRITE_ERROR:
    default:
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_region_t *) &_comac_region_nil;
    }
}

/**
 * _comac_region_set_error:
 * @region: a region
 * @status: a status value indicating an error
 *
 * Atomically sets region->status to @status and calls _comac_error;
 * Does nothing if status is %COMAC_STATUS_SUCCESS or any of the internal
 * status values.
 *
 * All assignments of an error status to region->status should happen
 * through _comac_region_set_error(). Note that due to the nature of
 * the atomic operation, it is not safe to call this function on the
 * nil objects.
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _comac_error() to generate a stack trace for when the
 * user causes comac to detect an error.
 *
 * Return value: the error status.
 **/
static comac_status_t
_comac_region_set_error (comac_region_t *region, comac_status_t status)
{
    if (status == COMAC_STATUS_SUCCESS)
	return COMAC_STATUS_SUCCESS;

    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. */
    _comac_status_set_error (&region->status, status);

    return _comac_error (status);
}

void
_comac_region_init (comac_region_t *region)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (region, sizeof (comac_region_t)));

    region->status = COMAC_STATUS_SUCCESS;
    COMAC_REFERENCE_COUNT_INIT (&region->ref_count, 0);
    pixman_region32_init (&region->rgn);
}

void
_comac_region_init_rectangle (comac_region_t *region,
			      const comac_rectangle_int_t *rectangle)
{
    VG (VALGRIND_MAKE_MEM_UNDEFINED (region, sizeof (comac_region_t)));

    region->status = COMAC_STATUS_SUCCESS;
    COMAC_REFERENCE_COUNT_INIT (&region->ref_count, 0);
    pixman_region32_init_rect (&region->rgn,
			       rectangle->x,
			       rectangle->y,
			       rectangle->width,
			       rectangle->height);
}

void
_comac_region_fini (comac_region_t *region)
{
    assert (! COMAC_REFERENCE_COUNT_HAS_REFERENCE (&region->ref_count));
    pixman_region32_fini (&region->rgn);
    VG (VALGRIND_MAKE_MEM_UNDEFINED (region, sizeof (comac_region_t)));
}

/**
 * comac_region_create:
 *
 * Allocates a new empty region object.
 *
 * Return value: A newly allocated #comac_region_t. Free with
 *   comac_region_destroy(). This function always returns a
 *   valid pointer; if memory cannot be allocated, then a special
 *   error object is returned where all operations on the object do nothing.
 *   You can check for this with comac_region_status().
 *
 * Since: 1.10
 **/
comac_region_t *
comac_region_create (void)
{
    comac_region_t *region;

    region = _comac_malloc (sizeof (comac_region_t));
    if (region == NULL)
	return (comac_region_t *) &_comac_region_nil;

    region->status = COMAC_STATUS_SUCCESS;
    COMAC_REFERENCE_COUNT_INIT (&region->ref_count, 1);

    pixman_region32_init (&region->rgn);

    return region;
}

/**
 * comac_region_create_rectangles:
 * @rects: an array of @count rectangles
 * @count: number of rectangles
 *
 * Allocates a new region object containing the union of all given @rects.
 *
 * Return value: A newly allocated #comac_region_t. Free with
 *   comac_region_destroy(). This function always returns a
 *   valid pointer; if memory cannot be allocated, then a special
 *   error object is returned where all operations on the object do nothing.
 *   You can check for this with comac_region_status().
 *
 * Since: 1.10
 **/
comac_region_t *
comac_region_create_rectangles (const comac_rectangle_int_t *rects, int count)
{
    pixman_box32_t stack_pboxes[COMAC_STACK_ARRAY_LENGTH (pixman_box32_t)];
    pixman_box32_t *pboxes = stack_pboxes;
    comac_region_t *region;
    int i;

    region = _comac_malloc (sizeof (comac_region_t));
    if (unlikely (region == NULL))
	return _comac_region_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    COMAC_REFERENCE_COUNT_INIT (&region->ref_count, 1);
    region->status = COMAC_STATUS_SUCCESS;

    if (count == 1) {
	pixman_region32_init_rect (&region->rgn,
				   rects->x,
				   rects->y,
				   rects->width,
				   rects->height);

	return region;
    }

    if (count > ARRAY_LENGTH (stack_pboxes)) {
	pboxes = _comac_malloc_ab (count, sizeof (pixman_box32_t));
	if (unlikely (pboxes == NULL)) {
	    free (region);
	    return _comac_region_create_in_error (
		_comac_error (COMAC_STATUS_NO_MEMORY));
	}
    }

    for (i = 0; i < count; i++) {
	pboxes[i].x1 = rects[i].x;
	pboxes[i].y1 = rects[i].y;
	pboxes[i].x2 = rects[i].x + rects[i].width;
	pboxes[i].y2 = rects[i].y + rects[i].height;
    }

    i = pixman_region32_init_rects (&region->rgn, pboxes, count);

    if (pboxes != stack_pboxes)
	free (pboxes);

    if (unlikely (i == 0)) {
	free (region);
	return _comac_region_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));
    }

    return region;
}

comac_region_t *
_comac_region_create_from_boxes (const comac_box_t *boxes, int count)
{
    comac_region_t *region;

    region = _comac_malloc (sizeof (comac_region_t));
    if (unlikely (region == NULL))
	return _comac_region_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    COMAC_REFERENCE_COUNT_INIT (&region->ref_count, 1);
    region->status = COMAC_STATUS_SUCCESS;

    if (! pixman_region32_init_rects (&region->rgn,
				      (pixman_box32_t *) boxes,
				      count)) {
	free (region);
	return _comac_region_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));
    }

    return region;
}

comac_box_t *
_comac_region_get_boxes (const comac_region_t *region, int *nbox)
{
    if (region->status) {
	nbox = 0;
	return NULL;
    }

    return (comac_box_t *) pixman_region32_rectangles (CONST_CAST & region->rgn,
						       nbox);
}

/**
 * comac_region_create_rectangle:
 * @rectangle: a #comac_rectangle_int_t
 *
 * Allocates a new region object containing @rectangle.
 *
 * Return value: A newly allocated #comac_region_t. Free with
 *   comac_region_destroy(). This function always returns a
 *   valid pointer; if memory cannot be allocated, then a special
 *   error object is returned where all operations on the object do nothing.
 *   You can check for this with comac_region_status().
 *
 * Since: 1.10
 **/
comac_region_t *
comac_region_create_rectangle (const comac_rectangle_int_t *rectangle)
{
    comac_region_t *region;

    region = _comac_malloc (sizeof (comac_region_t));
    if (unlikely (region == NULL))
	return (comac_region_t *) &_comac_region_nil;

    region->status = COMAC_STATUS_SUCCESS;
    COMAC_REFERENCE_COUNT_INIT (&region->ref_count, 1);

    pixman_region32_init_rect (&region->rgn,
			       rectangle->x,
			       rectangle->y,
			       rectangle->width,
			       rectangle->height);

    return region;
}

/**
 * comac_region_copy:
 * @original: a #comac_region_t
 *
 * Allocates a new region object copying the area from @original.
 *
 * Return value: A newly allocated #comac_region_t. Free with
 *   comac_region_destroy(). This function always returns a
 *   valid pointer; if memory cannot be allocated, then a special
 *   error object is returned where all operations on the object do nothing.
 *   You can check for this with comac_region_status().
 *
 * Since: 1.10
 **/
comac_region_t *
comac_region_copy (const comac_region_t *original)
{
    comac_region_t *copy;

    if (original != NULL && original->status)
	return (comac_region_t *) &_comac_region_nil;

    copy = comac_region_create ();
    if (unlikely (copy->status))
	return copy;

    if (original != NULL &&
	! pixman_region32_copy (&copy->rgn, CONST_CAST & original->rgn)) {
	comac_region_destroy (copy);
	return (comac_region_t *) &_comac_region_nil;
    }

    return copy;
}

/**
 * comac_region_reference:
 * @region: a #comac_region_t
 *
 * Increases the reference count on @region by one. This prevents
 * @region from being destroyed until a matching call to
 * comac_region_destroy() is made.
 *
 * Return value: the referenced #comac_region_t.
 *
 * Since: 1.10
 **/
comac_region_t *
comac_region_reference (comac_region_t *region)
{
    if (region == NULL || COMAC_REFERENCE_COUNT_IS_INVALID (&region->ref_count))
	return NULL;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&region->ref_count));

    _comac_reference_count_inc (&region->ref_count);
    return region;
}

/**
 * comac_region_destroy:
 * @region: a #comac_region_t
 *
 * Destroys a #comac_region_t object created with
 * comac_region_create(), comac_region_copy(), or
 * or comac_region_create_rectangle().
 *
 * Since: 1.10
 **/
void
comac_region_destroy (comac_region_t *region)
{
    if (region == NULL || COMAC_REFERENCE_COUNT_IS_INVALID (&region->ref_count))
	return;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&region->ref_count));

    if (! _comac_reference_count_dec_and_test (&region->ref_count))
	return;

    _comac_region_fini (region);
    free (region);
}

/**
 * comac_region_num_rectangles:
 * @region: a #comac_region_t
 *
 * Returns the number of rectangles contained in @region.
 *
 * Return value: The number of rectangles contained in @region.
 *
 * Since: 1.10
 **/
int
comac_region_num_rectangles (const comac_region_t *region)
{
    if (region->status)
	return 0;

    return pixman_region32_n_rects (CONST_CAST & region->rgn);
}

/**
 * comac_region_get_rectangle:
 * @region: a #comac_region_t
 * @nth: a number indicating which rectangle should be returned
 * @rectangle: return location for a #comac_rectangle_int_t
 *
 * Stores the @nth rectangle from the region in @rectangle.
 *
 * Since: 1.10
 **/
void
comac_region_get_rectangle (const comac_region_t *region,
			    int nth,
			    comac_rectangle_int_t *rectangle)
{
    pixman_box32_t *pbox;

    if (region->status) {
	rectangle->x = rectangle->y = 0;
	rectangle->width = rectangle->height = 0;
	return;
    }

    pbox = pixman_region32_rectangles (CONST_CAST & region->rgn, NULL) + nth;

    rectangle->x = pbox->x1;
    rectangle->y = pbox->y1;
    rectangle->width = pbox->x2 - pbox->x1;
    rectangle->height = pbox->y2 - pbox->y1;
}

/**
 * comac_region_get_extents:
 * @region: a #comac_region_t
 * @extents: rectangle into which to store the extents
 *
 * Gets the bounding rectangle of @region as a #comac_rectangle_int_t
 *
 * Since: 1.10
 **/
void
comac_region_get_extents (const comac_region_t *region,
			  comac_rectangle_int_t *extents)
{
    pixman_box32_t *pextents;

    if (region->status) {
	extents->x = extents->y = 0;
	extents->width = extents->height = 0;
	return;
    }

    pextents = pixman_region32_extents (CONST_CAST & region->rgn);

    extents->x = pextents->x1;
    extents->y = pextents->y1;
    extents->width = pextents->x2 - pextents->x1;
    extents->height = pextents->y2 - pextents->y1;
}

/**
 * comac_region_status:
 * @region: a #comac_region_t
 *
 * Checks whether an error has previous occurred for this
 * region object.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_status (const comac_region_t *region)
{
    return region->status;
}

/**
 * comac_region_subtract:
 * @dst: a #comac_region_t
 * @other: another #comac_region_t
 *
 * Subtracts @other from @dst and places the result in @dst
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_subtract (comac_region_t *dst, const comac_region_t *other)
{
    if (dst->status)
	return dst->status;

    if (other->status)
	return _comac_region_set_error (dst, other->status);

    if (! pixman_region32_subtract (&dst->rgn,
				    &dst->rgn,
				    CONST_CAST & other->rgn)) {
	return _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);
    }

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_region_subtract_rectangle:
 * @dst: a #comac_region_t
 * @rectangle: a #comac_rectangle_int_t
 *
 * Subtracts @rectangle from @dst and places the result in @dst
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_subtract_rectangle (comac_region_t *dst,
				 const comac_rectangle_int_t *rectangle)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    pixman_region32_t region;

    if (dst->status)
	return dst->status;

    pixman_region32_init_rect (&region,
			       rectangle->x,
			       rectangle->y,
			       rectangle->width,
			       rectangle->height);

    if (! pixman_region32_subtract (&dst->rgn, &dst->rgn, &region))
	status = _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);

    pixman_region32_fini (&region);

    return status;
}

/**
 * comac_region_intersect:
 * @dst: a #comac_region_t
 * @other: another #comac_region_t
 *
 * Computes the intersection of @dst with @other and places the result in @dst
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_intersect (comac_region_t *dst, const comac_region_t *other)
{
    if (dst->status)
	return dst->status;

    if (other->status)
	return _comac_region_set_error (dst, other->status);

    if (! pixman_region32_intersect (&dst->rgn,
				     &dst->rgn,
				     CONST_CAST & other->rgn))
	return _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_region_intersect_rectangle:
 * @dst: a #comac_region_t
 * @rectangle: a #comac_rectangle_int_t
 *
 * Computes the intersection of @dst with @rectangle and places the
 * result in @dst
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_intersect_rectangle (comac_region_t *dst,
				  const comac_rectangle_int_t *rectangle)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    pixman_region32_t region;

    if (dst->status)
	return dst->status;

    pixman_region32_init_rect (&region,
			       rectangle->x,
			       rectangle->y,
			       rectangle->width,
			       rectangle->height);

    if (! pixman_region32_intersect (&dst->rgn, &dst->rgn, &region))
	status = _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);

    pixman_region32_fini (&region);

    return status;
}

/**
 * comac_region_union:
 * @dst: a #comac_region_t
 * @other: another #comac_region_t
 *
 * Computes the union of @dst with @other and places the result in @dst
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_union (comac_region_t *dst, const comac_region_t *other)
{
    if (dst->status)
	return dst->status;

    if (other->status)
	return _comac_region_set_error (dst, other->status);

    if (! pixman_region32_union (&dst->rgn, &dst->rgn, CONST_CAST & other->rgn))
	return _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);

    return COMAC_STATUS_SUCCESS;
}

/**
 * comac_region_union_rectangle:
 * @dst: a #comac_region_t
 * @rectangle: a #comac_rectangle_int_t
 *
 * Computes the union of @dst with @rectangle and places the result in @dst.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_union_rectangle (comac_region_t *dst,
			      const comac_rectangle_int_t *rectangle)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    pixman_region32_t region;

    if (dst->status)
	return dst->status;

    pixman_region32_init_rect (&region,
			       rectangle->x,
			       rectangle->y,
			       rectangle->width,
			       rectangle->height);

    if (! pixman_region32_union (&dst->rgn, &dst->rgn, &region))
	status = _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);

    pixman_region32_fini (&region);

    return status;
}

/**
 * comac_region_xor:
 * @dst: a #comac_region_t
 * @other: another #comac_region_t
 *
 * Computes the exclusive difference of @dst with @other and places the
 * result in @dst. That is, @dst will be set to contain all areas that
 * are either in @dst or in @other, but not in both.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_xor (comac_region_t *dst, const comac_region_t *other)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    pixman_region32_t tmp;

    if (dst->status)
	return dst->status;

    if (other->status)
	return _comac_region_set_error (dst, other->status);

    pixman_region32_init (&tmp);

    /* XXX: get an xor function into pixman */
    if (! pixman_region32_subtract (&tmp, CONST_CAST & other->rgn, &dst->rgn) ||
	! pixman_region32_subtract (&dst->rgn,
				    &dst->rgn,
				    CONST_CAST & other->rgn) ||
	! pixman_region32_union (&dst->rgn, &dst->rgn, &tmp))
	status = _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);

    pixman_region32_fini (&tmp);

    return status;
}

/**
 * comac_region_xor_rectangle:
 * @dst: a #comac_region_t
 * @rectangle: a #comac_rectangle_int_t
 *
 * Computes the exclusive difference of @dst with @rectangle and places the
 * result in @dst. That is, @dst will be set to contain all areas that are 
 * either in @dst or in @rectangle, but not in both.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY
 *
 * Since: 1.10
 **/
comac_status_t
comac_region_xor_rectangle (comac_region_t *dst,
			    const comac_rectangle_int_t *rectangle)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;
    pixman_region32_t region, tmp;

    if (dst->status)
	return dst->status;

    pixman_region32_init_rect (&region,
			       rectangle->x,
			       rectangle->y,
			       rectangle->width,
			       rectangle->height);
    pixman_region32_init (&tmp);

    /* XXX: get an xor function into pixman */
    if (! pixman_region32_subtract (&tmp, &region, &dst->rgn) ||
	! pixman_region32_subtract (&dst->rgn, &dst->rgn, &region) ||
	! pixman_region32_union (&dst->rgn, &dst->rgn, &tmp))
	status = _comac_region_set_error (dst, COMAC_STATUS_NO_MEMORY);

    pixman_region32_fini (&tmp);
    pixman_region32_fini (&region);

    return status;
}

/**
 * comac_region_is_empty:
 * @region: a #comac_region_t
 *
 * Checks whether @region is empty.
 *
 * Return value: %TRUE if @region is empty, %FALSE if it isn't.
 *
 * Since: 1.10
 **/
comac_bool_t
comac_region_is_empty (const comac_region_t *region)
{
    if (region->status)
	return TRUE;

    return ! pixman_region32_not_empty (CONST_CAST & region->rgn);
}

/**
 * comac_region_translate:
 * @region: a #comac_region_t
 * @dx: Amount to translate in the x direction
 * @dy: Amount to translate in the y direction
 *
 * Translates @region by (@dx, @dy).
 *
 * Since: 1.10
 **/
void
comac_region_translate (comac_region_t *region, int dx, int dy)
{
    if (region->status)
	return;

    pixman_region32_translate (&region->rgn, dx, dy);
}

/**
 * comac_region_contains_rectangle:
 * @region: a #comac_region_t
 * @rectangle: a #comac_rectangle_int_t
 *
 * Checks whether @rectangle is inside, outside or partially contained
 * in @region
 *
 * Return value:
 *   %COMAC_REGION_OVERLAP_IN if @rectangle is entirely inside @region,
 *   %COMAC_REGION_OVERLAP_OUT if @rectangle is entirely outside @region, or
 *   %COMAC_REGION_OVERLAP_PART if @rectangle is partially inside and partially outside @region.
 *
 * Since: 1.10
 **/
comac_region_overlap_t
comac_region_contains_rectangle (const comac_region_t *region,
				 const comac_rectangle_int_t *rectangle)
{
    pixman_box32_t pbox;
    pixman_region_overlap_t poverlap;

    if (region->status)
	return COMAC_REGION_OVERLAP_OUT;

    pbox.x1 = rectangle->x;
    pbox.y1 = rectangle->y;
    pbox.x2 = rectangle->x + rectangle->width;
    pbox.y2 = rectangle->y + rectangle->height;

    poverlap =
	pixman_region32_contains_rectangle (CONST_CAST & region->rgn, &pbox);
    switch (poverlap) {
    default:
    case PIXMAN_REGION_OUT:
	return COMAC_REGION_OVERLAP_OUT;
    case PIXMAN_REGION_IN:
	return COMAC_REGION_OVERLAP_IN;
    case PIXMAN_REGION_PART:
	return COMAC_REGION_OVERLAP_PART;
    }
}

/**
 * comac_region_contains_point:
 * @region: a #comac_region_t
 * @x: the x coordinate of a point
 * @y: the y coordinate of a point
 *
 * Checks whether (@x, @y) is contained in @region.
 *
 * Return value: %TRUE if (@x, @y) is contained in @region, %FALSE if it is not.
 *
 * Since: 1.10
 **/
comac_bool_t
comac_region_contains_point (const comac_region_t *region, int x, int y)
{
    pixman_box32_t box;

    if (region->status)
	return FALSE;

    return pixman_region32_contains_point (CONST_CAST & region->rgn,
					   x,
					   y,
					   &box);
}

/**
 * comac_region_equal:
 * @a: a #comac_region_t or %NULL
 * @b: a #comac_region_t or %NULL
 *
 * Compares whether region_a is equivalent to region_b. %NULL as an argument
 * is equal to itself, but not to any non-%NULL region.
 *
 * Return value: %TRUE if both regions contained the same coverage,
 * %FALSE if it is not or any region is in an error status.
 *
 * Since: 1.10
 **/
comac_bool_t
comac_region_equal (const comac_region_t *a, const comac_region_t *b)
{
    /* error objects are never equal */
    if ((a != NULL && a->status) || (b != NULL && b->status))
	return FALSE;

    if (a == b)
	return TRUE;

    if (a == NULL || b == NULL)
	return FALSE;

    return pixman_region32_equal (CONST_CAST & a->rgn, CONST_CAST & b->rgn);
}
