/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2009 Adrian Johnson
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
 * The Initial Developer of the Original Code is Adrian Johnson.
 *
 * Contributor(s):
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#include "comacint.h"

#if COMAC_HAS_PDF_OPERATORS

#include "comac-pdf-shading-private.h"

#include "comac-array-private.h"
#include "comac-error-private.h"
#include <float.h>

static unsigned char *
encode_coordinate (unsigned char *p, double c)
{
    uint32_t f;

    f = c;
    *p++ = f >> 24;
    *p++ = (f >> 16) & 0xff;
    *p++ = (f >> 8) & 0xff;
    *p++ = f & 0xff;

    return p;
}

static unsigned char *
encode_point (unsigned char *p, const comac_point_double_t *point)
{
    p = encode_coordinate (p, point->x);
    p = encode_coordinate (p, point->y);

    return p;
}

static unsigned char *
encode_color_component (unsigned char *p, double color)
{
    uint16_t c;

    c = _comac_color_double_to_short (color);
    *p++ = c >> 8;
    *p++ = c & 0xff;

    return p;
}

static unsigned char *
encode_color (unsigned char *p, const comac_color_t *color)
{
    assert (color->colorspace == COMAC_COLORSPACE_RGB);
    p = encode_color_component (p, color->c.rgb.red);
    p = encode_color_component (p, color->c.rgb.green);
    p = encode_color_component (p, color->c.rgb.blue);

    return p;
}

static unsigned char *
encode_alpha (unsigned char *p, const comac_color_t *color)
{
    assert (color->colorspace == COMAC_COLORSPACE_RGB);
    p = encode_color_component (p, color->c.rgb.alpha);

    return p;
}

static comac_status_t
_comac_pdf_shading_generate_decode_array (comac_pdf_shading_t *shading,
					  const comac_mesh_pattern_t *mesh,
					  comac_bool_t is_alpha)
{
    unsigned int num_color_components, i;
    comac_bool_t is_valid;

    if (is_alpha)
	num_color_components = 1;
    else
	num_color_components = 3;

    shading->decode_array_length = 4 + num_color_components * 2;
    shading->decode_array =
	_comac_malloc_ab (shading->decode_array_length, sizeof (double));
    if (unlikely (shading->decode_array == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    is_valid = _comac_mesh_pattern_coord_box (mesh,
					      &shading->decode_array[0],
					      &shading->decode_array[2],
					      &shading->decode_array[1],
					      &shading->decode_array[3]);

    assert (is_valid);
    assert (shading->decode_array[1] - shading->decode_array[0] >= DBL_EPSILON);
    assert (shading->decode_array[3] - shading->decode_array[2] >= DBL_EPSILON);

    for (i = 0; i < num_color_components; i++) {
	shading->decode_array[4 + 2 * i] = 0;
	shading->decode_array[5 + 2 * i] = 1;
    }

    return COMAC_STATUS_SUCCESS;
}

/* The ISO32000 specification mandates this order for the points which
 * define the patch. */
static const int pdf_points_order_i[16] = {
    0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2};
static const int pdf_points_order_j[16] = {
    0, 1, 2, 3, 3, 3, 3, 2, 1, 0, 0, 0, 1, 2, 2, 1};

static comac_status_t
_comac_pdf_shading_generate_data (comac_pdf_shading_t *shading,
				  const comac_mesh_pattern_t *mesh,
				  comac_bool_t is_alpha)
{
    const comac_mesh_patch_t *patch;
    double x_off, y_off, x_scale, y_scale;
    unsigned int num_patches;
    unsigned int num_color_components;
    unsigned char *p;
    unsigned int i, j;

    if (is_alpha)
	num_color_components = 1;
    else
	num_color_components = 3;

    num_patches = _comac_array_num_elements (&mesh->patches);
    patch = _comac_array_index_const (&mesh->patches, 0);

    /* Each patch requires:
     *
     * 1 flag - 1 byte
     * 16 points. Each point is 2 coordinates. Each coordinate is
     * stored in 4 bytes.
     *
     * 4 colors. Each color is stored in 2 bytes * num_color_components.
     */
    shading->data_length =
	num_patches * (1 + 16 * 2 * 4 + 4 * 2 * num_color_components);
    shading->data = _comac_malloc (shading->data_length);
    if (unlikely (shading->data == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    x_off = shading->decode_array[0];
    y_off = shading->decode_array[2];
    x_scale = UINT32_MAX / (shading->decode_array[1] - x_off);
    y_scale = UINT32_MAX / (shading->decode_array[3] - y_off);

    p = shading->data;
    for (i = 0; i < num_patches; i++) {
	/* edge flag */
	*p++ = 0;

	/* 16 points */
	for (j = 0; j < 16; j++) {
	    comac_point_double_t point;
	    int pi, pj;

	    pi = pdf_points_order_i[j];
	    pj = pdf_points_order_j[j];
	    point = patch[i].points[pi][pj];

	    /* Transform the point as specified in the decode array */
	    point.x -= x_off;
	    point.y -= y_off;
	    point.x *= x_scale;
	    point.y *= y_scale;

	    /* Make sure that rounding errors don't cause
	     * wraparounds */
	    point.x = _comac_restrict_value (point.x, 0, UINT32_MAX);
	    point.y = _comac_restrict_value (point.y, 0, UINT32_MAX);

	    p = encode_point (p, &point);
	}

	/* 4 colors */
	for (j = 0; j < 4; j++) {
	    if (is_alpha)
		p = encode_alpha (p, &patch[i].colors[j]);
	    else
		p = encode_color (p, &patch[i].colors[j]);
	}
    }

    assert (p == shading->data + shading->data_length);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_pdf_shading_init (comac_pdf_shading_t *shading,
			 const comac_mesh_pattern_t *mesh,
			 comac_bool_t is_alpha)
{
    comac_status_t status;

    assert (mesh->base.status == COMAC_STATUS_SUCCESS);
    assert (mesh->current_patch == NULL);

    shading->shading_type = 7;

    /*
     * Coordinates from the minimum to the maximum value of the mesh
     * map to the [0..UINT32_MAX] range and are represented as
     * uint32_t values.
     *
     * Color components are represented as uint16_t (in a 0.16 fixed
     * point format, as in the rest of comac).
     */
    shading->bits_per_coordinate = 32;
    shading->bits_per_component = 16;
    shading->bits_per_flag = 8;

    shading->decode_array = NULL;
    shading->data = NULL;

    status = _comac_pdf_shading_generate_decode_array (shading, mesh, is_alpha);
    if (unlikely (status))
	return status;

    return _comac_pdf_shading_generate_data (shading, mesh, is_alpha);
}

comac_status_t
_comac_pdf_shading_init_color (comac_pdf_shading_t *shading,
			       const comac_mesh_pattern_t *pattern)
{
    return _comac_pdf_shading_init (shading, pattern, FALSE);
}

comac_status_t
_comac_pdf_shading_init_alpha (comac_pdf_shading_t *shading,
			       const comac_mesh_pattern_t *pattern)
{
    return _comac_pdf_shading_init (shading, pattern, TRUE);
}

void
_comac_pdf_shading_fini (comac_pdf_shading_t *shading)
{
    free (shading->data);
    free (shading->decode_array);
}

#endif /* COMAC_HAS_PDF_OPERATORS */
