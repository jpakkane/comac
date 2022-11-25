/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@redhat.com>
 */

#include "comacint.h"

#include "comac-private.h"
#include "comac-backend-private.h"
#include "comac-error-private.h"
#include "comac-path-private.h"
#include "comac-path-fixed-private.h"

/**
 * SECTION:comac-paths
 * @Title: Paths
 * @Short_Description: Creating paths and manipulating path data
 *
 * Paths are the most basic drawing tools and are primarily used to implicitly
 * generate simple masks.
 **/

static const comac_path_t _comac_path_nil = {COMAC_STATUS_NO_MEMORY, NULL, 0};

/* Closure for path interpretation. */
typedef struct comac_path_count {
    int count;
} cpc_t;

static comac_status_t
_cpc_move_to (void *closure, const comac_point_t *point)
{
    cpc_t *cpc = closure;

    cpc->count += 2;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_cpc_line_to (void *closure, const comac_point_t *point)
{
    cpc_t *cpc = closure;

    cpc->count += 2;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_cpc_curve_to (void *closure,
	       const comac_point_t *p1,
	       const comac_point_t *p2,
	       const comac_point_t *p3)
{
    cpc_t *cpc = closure;

    cpc->count += 4;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_cpc_close_path (void *closure)
{
    cpc_t *cpc = closure;

    cpc->count += 1;

    return COMAC_STATUS_SUCCESS;
}

static int
_comac_path_count (comac_path_t *path,
		   comac_path_fixed_t *path_fixed,
		   double tolerance,
		   comac_bool_t flatten)
{
    comac_status_t status;
    cpc_t cpc;

    cpc.count = 0;

    if (flatten) {
	status = _comac_path_fixed_interpret_flat (path_fixed,
						   _cpc_move_to,
						   _cpc_line_to,
						   _cpc_close_path,
						   &cpc,
						   tolerance);
    } else {
	status = _comac_path_fixed_interpret (path_fixed,
					      _cpc_move_to,
					      _cpc_line_to,
					      _cpc_curve_to,
					      _cpc_close_path,
					      &cpc);
    }

    if (unlikely (status))
	return -1;

    return cpc.count;
}

/* Closure for path interpretation. */
typedef struct comac_path_populate {
    comac_path_data_t *data;
    comac_t *cr;
} cpp_t;

static comac_status_t
_cpp_move_to (void *closure, const comac_point_t *point)
{
    cpp_t *cpp = closure;
    comac_path_data_t *data = cpp->data;
    double x, y;

    x = _comac_fixed_to_double (point->x);
    y = _comac_fixed_to_double (point->y);

    _comac_backend_to_user (cpp->cr, &x, &y);

    data->header.type = COMAC_PATH_MOVE_TO;
    data->header.length = 2;

    /* We index from 1 to leave room for data->header */
    data[1].point.x = x;
    data[1].point.y = y;

    cpp->data += data->header.length;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_cpp_line_to (void *closure, const comac_point_t *point)
{
    cpp_t *cpp = closure;
    comac_path_data_t *data = cpp->data;
    double x, y;

    x = _comac_fixed_to_double (point->x);
    y = _comac_fixed_to_double (point->y);

    _comac_backend_to_user (cpp->cr, &x, &y);

    data->header.type = COMAC_PATH_LINE_TO;
    data->header.length = 2;

    /* We index from 1 to leave room for data->header */
    data[1].point.x = x;
    data[1].point.y = y;

    cpp->data += data->header.length;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_cpp_curve_to (void *closure,
	       const comac_point_t *p1,
	       const comac_point_t *p2,
	       const comac_point_t *p3)
{
    cpp_t *cpp = closure;
    comac_path_data_t *data = cpp->data;
    double x1, y1;
    double x2, y2;
    double x3, y3;

    x1 = _comac_fixed_to_double (p1->x);
    y1 = _comac_fixed_to_double (p1->y);
    _comac_backend_to_user (cpp->cr, &x1, &y1);

    x2 = _comac_fixed_to_double (p2->x);
    y2 = _comac_fixed_to_double (p2->y);
    _comac_backend_to_user (cpp->cr, &x2, &y2);

    x3 = _comac_fixed_to_double (p3->x);
    y3 = _comac_fixed_to_double (p3->y);
    _comac_backend_to_user (cpp->cr, &x3, &y3);

    data->header.type = COMAC_PATH_CURVE_TO;
    data->header.length = 4;

    /* We index from 1 to leave room for data->header */
    data[1].point.x = x1;
    data[1].point.y = y1;

    data[2].point.x = x2;
    data[2].point.y = y2;

    data[3].point.x = x3;
    data[3].point.y = y3;

    cpp->data += data->header.length;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_cpp_close_path (void *closure)
{
    cpp_t *cpp = closure;
    comac_path_data_t *data = cpp->data;

    data->header.type = COMAC_PATH_CLOSE_PATH;
    data->header.length = 1;

    cpp->data += data->header.length;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_path_populate (comac_path_t *path,
		      comac_path_fixed_t *path_fixed,
		      comac_t *cr,
		      comac_bool_t flatten)
{
    comac_status_t status;
    cpp_t cpp;

    cpp.data = path->data;
    cpp.cr = cr;

    if (flatten) {
	status = _comac_path_fixed_interpret_flat (path_fixed,
						   _cpp_move_to,
						   _cpp_line_to,
						   _cpp_close_path,
						   &cpp,
						   comac_get_tolerance (cr));
    } else {
	status = _comac_path_fixed_interpret (path_fixed,
					      _cpp_move_to,
					      _cpp_line_to,
					      _cpp_curve_to,
					      _cpp_close_path,
					      &cpp);
    }

    if (unlikely (status))
	return status;

    /* Sanity check the count */
    assert (cpp.data - path->data == path->num_data);

    return COMAC_STATUS_SUCCESS;
}

comac_path_t *
_comac_path_create_in_error (comac_status_t status)
{
    comac_path_t *path;

    /* special case NO_MEMORY so as to avoid allocations */
    if (status == COMAC_STATUS_NO_MEMORY)
	return (comac_path_t *) &_comac_path_nil;

    path = _comac_malloc (sizeof (comac_path_t));
    if (unlikely (path == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_path_t *) &_comac_path_nil;
    }

    path->num_data = 0;
    path->data = NULL;
    path->status = status;

    return path;
}

static comac_path_t *
_comac_path_create_internal (comac_path_fixed_t *path_fixed,
			     comac_t *cr,
			     comac_bool_t flatten)
{
    comac_path_t *path;

    path = _comac_malloc (sizeof (comac_path_t));
    if (unlikely (path == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_path_t *) &_comac_path_nil;
    }

    path->num_data =
	_comac_path_count (path, path_fixed, comac_get_tolerance (cr), flatten);
    if (path->num_data < 0) {
	free (path);
	return (comac_path_t *) &_comac_path_nil;
    }

    if (path->num_data) {
	path->data =
	    _comac_malloc_ab (path->num_data, sizeof (comac_path_data_t));
	if (unlikely (path->data == NULL)) {
	    free (path);
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return (comac_path_t *) &_comac_path_nil;
	}

	path->status = _comac_path_populate (path, path_fixed, cr, flatten);
    } else {
	path->data = NULL;
	path->status = COMAC_STATUS_SUCCESS;
    }

    return path;
}

/**
 * comac_path_destroy:
 * @path: a path previously returned by either comac_copy_path() or
 * comac_copy_path_flat().
 *
 * Immediately releases all memory associated with @path. After a call
 * to comac_path_destroy() the @path pointer is no longer valid and
 * should not be used further.
 *
 * Note: comac_path_destroy() should only be called with a
 * pointer to a #comac_path_t returned by a comac function. Any path
 * that is created manually (ie. outside of comac) should be destroyed
 * manually as well.
 *
 * Since: 1.0
 **/
void
comac_path_destroy (comac_path_t *path)
{
    if (path == NULL || path == &_comac_path_nil)
	return;

    free (path->data);

    free (path);
}

/**
 * _comac_path_create:
 * @path: a fixed-point, device-space path to be converted and copied
 * @cr: the current graphics context
 *
 * Creates a user-space #comac_path_t copy of the given device-space
 * @path. The @cr parameter provides the inverse CTM for the
 * conversion.
 *
 * Return value: the new copy of the path. If there is insufficient
 * memory a pointer to a special static nil #comac_path_t will be
 * returned instead with status==%COMAC_STATUS_NO_MEMORY and
 * data==%NULL.
 **/
comac_path_t *
_comac_path_create (comac_path_fixed_t *path, comac_t *cr)
{
    return _comac_path_create_internal (path, cr, FALSE);
}

/**
 * _comac_path_create_flat:
 * @path: a fixed-point, device-space path to be flattened, converted and copied
 * @cr: the current graphics context
 *
 * Creates a flattened, user-space #comac_path_t copy of the given
 * device-space @path. The @cr parameter provide the inverse CTM
 * for the conversion, as well as the tolerance value to control the
 * accuracy of the flattening.
 *
 * Return value: the flattened copy of the path. If there is insufficient
 * memory a pointer to a special static nil #comac_path_t will be
 * returned instead with status==%COMAC_STATUS_NO_MEMORY and
 * data==%NULL.
 **/
comac_path_t *
_comac_path_create_flat (comac_path_fixed_t *path, comac_t *cr)
{
    return _comac_path_create_internal (path, cr, TRUE);
}

/**
 * _comac_path_append_to_context:
 * @path: the path data to be appended
 * @cr: a comac context
 *
 * Append @path to the current path within @cr.
 *
 * Return value: %COMAC_STATUS_INVALID_PATH_DATA if the data in @path
 * is invalid, and %COMAC_STATUS_SUCCESS otherwise.
 **/
comac_status_t
_comac_path_append_to_context (const comac_path_t *path, comac_t *cr)
{
    const comac_path_data_t *p, *end;

    end = &path->data[path->num_data];
    for (p = &path->data[0]; p < end; p += p->header.length) {
	switch (p->header.type) {
	case COMAC_PATH_MOVE_TO:
	    if (unlikely (p->header.length < 2))
		return _comac_error (COMAC_STATUS_INVALID_PATH_DATA);

	    comac_move_to (cr, p[1].point.x, p[1].point.y);
	    break;

	case COMAC_PATH_LINE_TO:
	    if (unlikely (p->header.length < 2))
		return _comac_error (COMAC_STATUS_INVALID_PATH_DATA);

	    comac_line_to (cr, p[1].point.x, p[1].point.y);
	    break;

	case COMAC_PATH_CURVE_TO:
	    if (unlikely (p->header.length < 4))
		return _comac_error (COMAC_STATUS_INVALID_PATH_DATA);

	    comac_curve_to (cr,
			    p[1].point.x,
			    p[1].point.y,
			    p[2].point.x,
			    p[2].point.y,
			    p[3].point.x,
			    p[3].point.y);
	    break;

	case COMAC_PATH_CLOSE_PATH:
	    if (unlikely (p->header.length < 1))
		return _comac_error (COMAC_STATUS_INVALID_PATH_DATA);

	    comac_close_path (cr);
	    break;

	default:
	    return _comac_error (COMAC_STATUS_INVALID_PATH_DATA);
	}

	if (unlikely (cr->status))
	    return cr->status;
    }

    return COMAC_STATUS_SUCCESS;
}
