/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2016 Adrian Johnson
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

#include "comac-array-private.h"
#include "comac-list-inline.h"
#include "comac-tag-attributes-private.h"
#include "comac-tag-stack-private.h"

#include <string.h>

typedef enum {
    ATTRIBUTE_BOOL, /* Either true/false or 1/0 may be used. */
    ATTRIBUTE_INT,
    ATTRIBUTE_FLOAT,  /* Decimal separator is in current locale. */
    ATTRIBUTE_STRING, /* Enclose in single quotes. String escapes:
                       *   \'  - single quote
                       *   \\  - backslash
                       */
} attribute_type_t;

typedef struct _attribute_spec {
    const char *name;
    attribute_type_t type;
    int array_size; /* 0 = scalar, -1 = variable size array */
} attribute_spec_t;

/*
 * name [required] Unique name of this destination (UTF-8)
 * x    [optional] x coordinate of destination on page. Default is x coord of
 *                 extents of operations enclosed by the dest begin/end tags.
 * y    [optional] y coordinate of destination on page. Default is y coord of
 *                 extents of operations enclosed by the dest begin/end tags.
 * internal [optional] If true, the name may be optimized out of the PDF where
 *                     possible. Default false.
 */
static attribute_spec_t _dest_attrib_spec[] = {{"name", ATTRIBUTE_STRING},
					       {"x", ATTRIBUTE_FLOAT},
					       {"y", ATTRIBUTE_FLOAT},
					       {"internal", ATTRIBUTE_BOOL},
					       {NULL}};

/*
 * rect [optional] One or more rectangles to define link region. Default
 *                 is the extents of the operations enclosed by the link begin/end tags.
 *                 Each rectangle is specified by four array elements: x, y, width, height.
 *                 ie the array size must be a multiple of four.
 *
 * Internal Links
 * --------------
 * either:
 *   dest - name of dest tag in the PDF file to link to (UTF8)
 * or
 *   page - Page number in the PDF file to link to
 *   pos  - [optional] Position of destination on page. Default is 0,0.
 *
 * URI Links
 * ---------
 * uri [required] Uniform resource identifier (ASCII).

 * File Links
 * ----------
 * file - [required] File name of PDF file to link to.
 *   either:
 *     dest - name of dest tag in the PDF file to link to (UTF8)
 *   or
 *     page - Page number in the PDF file to link to
 *     pos  - [optional] Position of destination on page. Default is 0,0.
 */
static attribute_spec_t _link_attrib_spec[] = {{"rect", ATTRIBUTE_FLOAT, -1},
					       {"dest", ATTRIBUTE_STRING},
					       {"uri", ATTRIBUTE_STRING},
					       {"file", ATTRIBUTE_STRING},
					       {"page", ATTRIBUTE_INT},
					       {"pos", ATTRIBUTE_FLOAT, 2},
					       {NULL}};

/*
 * Required:
 *   Columns - width of the image in pixels.
 *   Rows    - height of the image in scan lines.
 *
 * Optional:
 *   K - An integer identifying the encoding scheme used. < 0 is 2 dimensional
 *       Group 4, = 0 is Group3 1 dimensional, > 0 is mixed 1 and 2 dimensional
 *       encoding. Default: 0.
 *   EndOfLine  - If true end-of-line bit patterns are present. Default: false.
 *   EncodedByteAlign - If true the end of line is padded with 0 bits so the next
 *                      line begins on a byte boundary. Default: false.
 *   EndOfBlock - If true the data contains an end-of-block pattern. Default: true.
 *   BlackIs1   - If true 1 bits are black pixels. Default: false.
 *   DamagedRowsBeforeError - Number of damages rows tolerated before an error
 *                            occurs. Default: 0.
 */
static attribute_spec_t _ccitt_params_spec[] = {
    {"Columns", ATTRIBUTE_INT},
    {"Rows", ATTRIBUTE_INT},
    {"K", ATTRIBUTE_INT},
    {"EndOfLine", ATTRIBUTE_BOOL},
    {"EncodedByteAlign", ATTRIBUTE_BOOL},
    {"EndOfBlock", ATTRIBUTE_BOOL},
    {"BlackIs1", ATTRIBUTE_BOOL},
    {"DamagedRowsBeforeError", ATTRIBUTE_INT},
    {NULL}};

/*
 * bbox - Bounding box of EPS file. The format is [ llx lly urx ury ]
 *          llx - lower left x xoordinate
 *          lly - lower left y xoordinate
 *          urx - upper right x xoordinate
 *          ury - upper right y xoordinate
 *        all coordinates are in PostScript coordinates.
 */
static attribute_spec_t _eps_params_spec[] = {{"bbox", ATTRIBUTE_FLOAT, 4},
					      {NULL}};

typedef union {
    comac_bool_t b;
    int i;
    double f;
    char *s;
} attrib_val_t;

typedef struct _attribute {
    char *name;
    attribute_type_t type;
    int array_len; /* 0 = scalar */
    attrib_val_t scalar;
    comac_array_t array; /* array of attrib_val_t */
    comac_list_t link;
} attribute_t;

static const char *
skip_space (const char *p)
{
    while (_comac_isspace (*p))
	p++;

    return p;
}

static const char *
parse_bool (const char *p, comac_bool_t *b)
{
    if (*p == '1') {
	*b = TRUE;
	return p + 1;
    } else if (*p == '0') {
	*b = FALSE;
	return p + 1;
    } else if (strcmp (p, "true") == 0) {
	*b = TRUE;
	return p + 4;
    } else if (strcmp (p, "false") == 0) {
	*b = FALSE;
	return p + 5;
    }

    return NULL;
}

static const char *
parse_int (const char *p, int *i)
{
    int n;

    if (sscanf (p, "%d%n", i, &n) > 0)
	return p + n;

    return NULL;
}

static const char *
parse_float (const char *p, double *d)
{
    int n;
    const char *start = p;
    comac_bool_t has_decimal_point = FALSE;

    while (*p) {
	if (*p == '.' || *p == ']' || _comac_isspace (*p))
	    break;
	p++;
    }

    if (*p == '.')
	has_decimal_point = TRUE;

    if (has_decimal_point) {
	char *end;
	*d = _comac_strtod (start, &end);
	if (end && end != start)
	    return end;

    } else {
	if (sscanf (start, "%lf%n", d, &n) > 0)
	    return start + n;
    }

    return NULL;
}

static const char *
decode_string (const char *p, int *len, char *s)
{
    if (*p != '\'')
	return NULL;

    p++;
    if (! *p)
	return NULL;

    *len = 0;
    while (*p) {
	if (*p == '\\') {
	    p++;
	    if (*p) {
		if (s)
		    *s++ = *p;
		p++;
		(*len)++;
	    }
	} else if (*p == '\'') {
	    return p + 1;
	} else {
	    if (s)
		*s++ = *p;
	    p++;
	    (*len)++;
	}
    }

    return NULL;
}

static const char *
parse_string (const char *p, char **s)
{
    const char *end;
    int len;

    end = decode_string (p, &len, NULL);
    if (! end)
	return NULL;

    *s = _comac_malloc (len + 1);
    decode_string (p, &len, *s);
    (*s)[len] = 0;

    return end;
}

static const char *
parse_scalar (const char *p, attribute_type_t type, attrib_val_t *scalar)
{
    switch (type) {
    case ATTRIBUTE_BOOL:
	return parse_bool (p, &scalar->b);
    case ATTRIBUTE_INT:
	return parse_int (p, &scalar->i);
    case ATTRIBUTE_FLOAT:
	return parse_float (p, &scalar->f);
    case ATTRIBUTE_STRING:
	return parse_string (p, &scalar->s);
    }

    return NULL;
}

static comac_int_status_t
parse_array (const char *attributes,
	     const char *p,
	     attribute_type_t type,
	     comac_array_t *array,
	     const char **end)
{
    attrib_val_t val;
    comac_int_status_t status;

    p = skip_space (p);
    if (! *p)
	goto error;

    if (*p++ != '[')
	goto error;

    while (TRUE) {
	p = skip_space (p);
	if (! *p)
	    goto error;

	if (*p == ']') {
	    *end = p + 1;
	    return COMAC_INT_STATUS_SUCCESS;
	}

	p = parse_scalar (p, type, &val);
	if (! p)
	    goto error;

	status = _comac_array_append (array, &val);
	if (unlikely (status))
	    return status;
    }

error:
    return _comac_tag_error (
	"while parsing attributes: \"%s\". Error parsing array",
	attributes);
}

static comac_int_status_t
parse_name (const char *attributes, const char *p, const char **end, char **s)
{
    const char *p2;
    char *name;
    int len;

    if (! _comac_isalpha (*p))
	return _comac_tag_error (
	    "while parsing attributes: \"%s\". Error parsing name."
	    " \"%s\" does not start with an alphabetic character",
	    attributes,
	    p);

    p2 = p;
    while (_comac_isalpha (*p2) || _comac_isdigit (*p2))
	p2++;

    len = p2 - p;
    name = _comac_malloc (len + 1);
    if (unlikely (name == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    memcpy (name, p, len);
    name[len] = 0;
    *s = name;
    *end = p2;

    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
parse_attributes (const char *attributes,
		  attribute_spec_t *attrib_def,
		  comac_list_t *list)
{
    attribute_spec_t *def;
    attribute_t *attrib;
    char *name = NULL;
    comac_int_status_t status;
    const char *p = attributes;

    if (! p)
	return COMAC_INT_STATUS_SUCCESS;

    while (*p) {
	p = skip_space (p);
	if (! *p)
	    break;

	status = parse_name (attributes, p, &p, &name);
	if (status)
	    return status;

	def = attrib_def;
	while (def->name) {
	    if (strcmp (name, def->name) == 0)
		break;
	    def++;
	}

	if (! def->name) {
	    status = _comac_tag_error ("while parsing attributes: \"%s\". "
				       "Unknown attribute name \"%s\"",
				       attributes,
				       name);
	    goto fail1;
	}

	attrib = calloc (1, sizeof (attribute_t));
	if (unlikely (attrib == NULL)) {
	    status = _comac_error (COMAC_STATUS_NO_MEMORY);
	    goto fail1;
	}

	attrib->name = name;
	attrib->type = def->type;
	_comac_array_init (&attrib->array, sizeof (attrib_val_t));

	p = skip_space (p);
	if (def->type == ATTRIBUTE_BOOL && *p != '=') {
	    attrib->scalar.b = TRUE;
	} else {
	    if (*p++ != '=') {
		status = _comac_tag_error ("while parsing attributes: \"%s\". "
					   "Expected '=' after \"%s\"",
					   attributes,
					   name);
		goto fail2;
	    }

	    if (def->array_size == 0) {
		const char *s = p;
		p = parse_scalar (p, def->type, &attrib->scalar);
		if (! p) {
		    status = _comac_tag_error ("while parsing attributes: "
					       "\"%s\". Error parsing \"%s\"",
					       attributes,
					       s);
		    goto fail2;
		}

		attrib->array_len = 0;
	    } else {
		status =
		    parse_array (attributes, p, def->type, &attrib->array, &p);
		if (unlikely (status))
		    goto fail2;

		attrib->array_len = _comac_array_num_elements (&attrib->array);
		if (def->array_size > 0 &&
		    attrib->array_len != def->array_size) {
		    status = _comac_tag_error (
			"while parsing attributes: \"%s\". Expected %d "
			"elements in array. Found %d",
			attributes,
			def->array_size,
			attrib->array_len);
		    goto fail2;
		}
	    }
	}

	comac_list_add_tail (&attrib->link, list);
    }

    return COMAC_INT_STATUS_SUCCESS;

fail2:
    _comac_array_fini (&attrib->array);
    if (attrib->type == ATTRIBUTE_STRING)
	free (attrib->scalar.s);
    free (attrib);
fail1:
    free (name);

    return status;
}

static void
free_attributes_list (comac_list_t *list)
{
    attribute_t *attr, *next;

    comac_list_foreach_entry_safe (attr, next, attribute_t, list, link)
    {
	comac_list_del (&attr->link);
	free (attr->name);
	_comac_array_fini (&attr->array);
	if (attr->type == ATTRIBUTE_STRING)
	    free (attr->scalar.s);
	free (attr);
    }
}

comac_int_status_t
_comac_tag_parse_link_attributes (const char *attributes,
				  comac_link_attrs_t *link_attrs)
{
    comac_list_t list;
    comac_int_status_t status;
    attribute_t *attr;
    attrib_val_t val;
    comac_bool_t has_rect = FALSE;
    comac_bool_t invalid_combination = FALSE;

    comac_list_init (&list);
    status = parse_attributes (attributes, _link_attrib_spec, &list);
    if (unlikely (status))
	return status;

    memset (link_attrs, 0, sizeof (comac_link_attrs_t));
    _comac_array_init (&link_attrs->rects, sizeof (comac_rectangle_t));

    comac_list_foreach_entry (attr, attribute_t, &list, link)
    {
	if (strcmp (attr->name, "dest") == 0) {
	    link_attrs->dest = strdup (attr->scalar.s);

	} else if (strcmp (attr->name, "page") == 0) {
	    link_attrs->page = attr->scalar.i;
	    if (link_attrs->page < 1) {
		status = _comac_tag_error (
		    "Link attributes: \"%s\" page must be >= 1",
		    attributes);
		goto cleanup;
	    }

	} else if (strcmp (attr->name, "pos") == 0) {
	    _comac_array_copy_element (&attr->array, 0, &val);
	    link_attrs->pos.x = val.f;
	    _comac_array_copy_element (&attr->array, 1, &val);
	    link_attrs->pos.y = val.f;
	    link_attrs->has_pos = TRUE;

	} else if (strcmp (attr->name, "uri") == 0) {
	    link_attrs->uri = strdup (attr->scalar.s);

	} else if (strcmp (attr->name, "file") == 0) {
	    link_attrs->file = strdup (attr->scalar.s);

	} else if (strcmp (attr->name, "rect") == 0) {
	    comac_rectangle_t rect;
	    int i;
	    int num_elem = _comac_array_num_elements (&attr->array);
	    if (num_elem == 0 || num_elem % 4 != 0) {
		status = _comac_tag_error ("Link attributes: \"%s\" rect array "
					   "size must be multiple of 4",
					   attributes);
		goto cleanup;
	    }

	    for (i = 0; i < num_elem; i += 4) {
		_comac_array_copy_element (&attr->array, i, &val);
		rect.x = val.f;
		_comac_array_copy_element (&attr->array, i + 1, &val);
		rect.y = val.f;
		_comac_array_copy_element (&attr->array, i + 2, &val);
		rect.width = val.f;
		_comac_array_copy_element (&attr->array, i + 3, &val);
		rect.height = val.f;
		status = _comac_array_append (&link_attrs->rects, &rect);
		if (unlikely (status))
		    goto cleanup;
	    }
	    has_rect = TRUE;
	}
    }

    if (link_attrs->uri) {
	link_attrs->link_type = TAG_LINK_URI;
	if (link_attrs->dest || link_attrs->page || link_attrs->has_pos ||
	    link_attrs->file)
	    invalid_combination = TRUE;

    } else if (link_attrs->file) {
	link_attrs->link_type = TAG_LINK_FILE;
	if (link_attrs->uri)
	    invalid_combination = TRUE;
	else if (link_attrs->dest && (link_attrs->page || link_attrs->has_pos))
	    invalid_combination = TRUE;

    } else if (link_attrs->dest) {
	link_attrs->link_type = TAG_LINK_DEST;
	if (link_attrs->uri || link_attrs->page || link_attrs->has_pos)
	    invalid_combination = TRUE;

    } else if (link_attrs->page) {
	link_attrs->link_type = TAG_LINK_DEST;
	if (link_attrs->uri || link_attrs->dest)
	    invalid_combination = TRUE;

    } else {
	link_attrs->link_type = TAG_LINK_EMPTY;
	if (link_attrs->has_pos)
	    invalid_combination = TRUE;
    }

    if (invalid_combination) {
	status = _comac_tag_error (
	    "Link attributes: \"%s\" invalid combination of attributes",
	    attributes);
    }

cleanup:
    free_attributes_list (&list);
    if (unlikely (status)) {
	free (link_attrs->dest);
	free (link_attrs->uri);
	free (link_attrs->file);
	_comac_array_fini (&link_attrs->rects);
    }

    return status;
}

comac_int_status_t
_comac_tag_parse_dest_attributes (const char *attributes,
				  comac_dest_attrs_t *dest_attrs)
{
    comac_list_t list;
    comac_int_status_t status;
    attribute_t *attr;

    memset (dest_attrs, 0, sizeof (comac_dest_attrs_t));
    comac_list_init (&list);
    status = parse_attributes (attributes, _dest_attrib_spec, &list);
    if (unlikely (status))
	goto cleanup;

    comac_list_foreach_entry (attr, attribute_t, &list, link)
    {
	if (strcmp (attr->name, "name") == 0) {
	    dest_attrs->name = strdup (attr->scalar.s);
	} else if (strcmp (attr->name, "x") == 0) {
	    dest_attrs->x = attr->scalar.f;
	    dest_attrs->x_valid = TRUE;
	} else if (strcmp (attr->name, "y") == 0) {
	    dest_attrs->y = attr->scalar.f;
	    dest_attrs->y_valid = TRUE;
	} else if (strcmp (attr->name, "internal") == 0) {
	    dest_attrs->internal = attr->scalar.b;
	}
    }

    if (! dest_attrs->name)
	status = _comac_tag_error (
	    "Destination attributes: \"%s\" missing name attribute",
	    attributes);

cleanup:
    free_attributes_list (&list);

    return status;
}

comac_int_status_t
_comac_tag_parse_ccitt_params (const char *attributes,
			       comac_ccitt_params_t *ccitt_params)
{
    comac_list_t list;
    comac_int_status_t status;
    attribute_t *attr;

    ccitt_params->columns = -1;
    ccitt_params->rows = -1;

    /* set defaults */
    ccitt_params->k = 0;
    ccitt_params->end_of_line = FALSE;
    ccitt_params->encoded_byte_align = FALSE;
    ccitt_params->end_of_block = TRUE;
    ccitt_params->black_is_1 = FALSE;
    ccitt_params->damaged_rows_before_error = 0;

    comac_list_init (&list);
    status = parse_attributes (attributes, _ccitt_params_spec, &list);
    if (unlikely (status))
	goto cleanup;

    comac_list_foreach_entry (attr, attribute_t, &list, link)
    {
	if (strcmp (attr->name, "Columns") == 0) {
	    ccitt_params->columns = attr->scalar.i;
	} else if (strcmp (attr->name, "Rows") == 0) {
	    ccitt_params->rows = attr->scalar.i;
	} else if (strcmp (attr->name, "K") == 0) {
	    ccitt_params->k = attr->scalar.i;
	} else if (strcmp (attr->name, "EndOfLine") == 0) {
	    ccitt_params->end_of_line = attr->scalar.b;
	} else if (strcmp (attr->name, "EncodedByteAlign") == 0) {
	    ccitt_params->encoded_byte_align = attr->scalar.b;
	} else if (strcmp (attr->name, "EndOfBlock") == 0) {
	    ccitt_params->end_of_block = attr->scalar.b;
	} else if (strcmp (attr->name, "BlackIs1") == 0) {
	    ccitt_params->black_is_1 = attr->scalar.b;
	} else if (strcmp (attr->name, "DamagedRowsBeforeError") == 0) {
	    ccitt_params->damaged_rows_before_error = attr->scalar.b;
	}
    }

cleanup:
    free_attributes_list (&list);

    return status;
}

comac_int_status_t
_comac_tag_parse_eps_params (const char *attributes,
			     comac_eps_params_t *eps_params)
{
    comac_list_t list;
    comac_int_status_t status;
    attribute_t *attr;
    attrib_val_t val;

    comac_list_init (&list);
    status = parse_attributes (attributes, _eps_params_spec, &list);
    if (unlikely (status))
	goto cleanup;

    comac_list_foreach_entry (attr, attribute_t, &list, link)
    {
	if (strcmp (attr->name, "bbox") == 0) {
	    _comac_array_copy_element (&attr->array, 0, &val);
	    eps_params->bbox.p1.x = val.f;
	    _comac_array_copy_element (&attr->array, 1, &val);
	    eps_params->bbox.p1.y = val.f;
	    _comac_array_copy_element (&attr->array, 2, &val);
	    eps_params->bbox.p2.x = val.f;
	    _comac_array_copy_element (&attr->array, 3, &val);
	    eps_params->bbox.p2.y = val.f;
	}
    }

cleanup:
    free_attributes_list (&list);

    return status;
}
