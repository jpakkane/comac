/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
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
 * The Initial Developer of the Original Code is Chris Wilson.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

/* This surface is intended to produce a verbose, hierarchical, DAG XML file
 * representing a single surface. It is intended to be used by debuggers,
 * such as comac-sphinx, or by application test-suites that want a log of
 * operations.
 */

#include "comacint.h"

#include "comac-xml.h"

#include "comac-clip-private.h"
#include "comac-device-private.h"
#include "comac-default-context-private.h"
#include "comac-image-surface-private.h"
#include "comac-error-private.h"
#include "comac-output-stream-private.h"
#include "comac-recording-surface-inline.h"

#define static comac_warn static

typedef struct _comac_xml_surface comac_xml_surface_t;

typedef struct _comac_xml {
    comac_device_t base;

    comac_output_stream_t *stream;
    int indent;
} comac_xml_t;

struct _comac_xml_surface {
    comac_surface_t base;

    double width, height;
};

static const comac_surface_backend_t _comac_xml_surface_backend;

static const char *
_operator_to_string (comac_operator_t op)
{
    static const char *names[] = {
	"CLEAR", /* COMAC_OPERATOR_CLEAR */

	"SOURCE", /* COMAC_OPERATOR_SOURCE */
	"OVER",	  /* COMAC_OPERATOR_OVER */
	"IN",	  /* COMAC_OPERATOR_IN */
	"OUT",	  /* COMAC_OPERATOR_OUT */
	"ATOP",	  /* COMAC_OPERATOR_ATOP */

	"DEST",	     /* COMAC_OPERATOR_DEST */
	"DEST_OVER", /* COMAC_OPERATOR_DEST_OVER */
	"DEST_IN",   /* COMAC_OPERATOR_DEST_IN */
	"DEST_OUT",  /* COMAC_OPERATOR_DEST_OUT */
	"DEST_ATOP", /* COMAC_OPERATOR_DEST_ATOP */

	"XOR",	    /* COMAC_OPERATOR_XOR */
	"ADD",	    /* COMAC_OPERATOR_ADD */
	"SATURATE", /* COMAC_OPERATOR_SATURATE */

	"MULTIPLY",	  /* COMAC_OPERATOR_MULTIPLY */
	"SCREEN",	  /* COMAC_OPERATOR_SCREEN */
	"OVERLAY",	  /* COMAC_OPERATOR_OVERLAY */
	"DARKEN",	  /* COMAC_OPERATOR_DARKEN */
	"LIGHTEN",	  /* COMAC_OPERATOR_LIGHTEN */
	"DODGE",	  /* COMAC_OPERATOR_COLOR_DODGE */
	"BURN",		  /* COMAC_OPERATOR_COLOR_BURN */
	"HARD_LIGHT",	  /* COMAC_OPERATOR_HARD_LIGHT */
	"SOFT_LIGHT",	  /* COMAC_OPERATOR_SOFT_LIGHT */
	"DIFFERENCE",	  /* COMAC_OPERATOR_DIFFERENCE */
	"EXCLUSION",	  /* COMAC_OPERATOR_EXCLUSION */
	"HSL_HUE",	  /* COMAC_OPERATOR_HSL_HUE */
	"HSL_SATURATION", /* COMAC_OPERATOR_HSL_SATURATION */
	"HSL_COLOR",	  /* COMAC_OPERATOR_HSL_COLOR */
	"HSL_LUMINOSITY"  /* COMAC_OPERATOR_HSL_LUMINOSITY */
    };
    assert (op < ARRAY_LENGTH (names));
    return names[op];
}

static const char *
_extend_to_string (comac_extend_t extend)
{
    static const char *names[] = {
	"EXTEND_NONE",	  /* COMAC_EXTEND_NONE */
	"EXTEND_REPEAT",  /* COMAC_EXTEND_REPEAT */
	"EXTEND_REFLECT", /* COMAC_EXTEND_REFLECT */
	"EXTEND_PAD"	  /* COMAC_EXTEND_PAD */
    };
    assert (extend < ARRAY_LENGTH (names));
    return names[extend];
}

static const char *
_filter_to_string (comac_filter_t filter)
{
    static const char *names[] = {
	"FILTER_FAST",	   /* COMAC_FILTER_FAST */
	"FILTER_GOOD",	   /* COMAC_FILTER_GOOD */
	"FILTER_BEST",	   /* COMAC_FILTER_BEST */
	"FILTER_NEAREST",  /* COMAC_FILTER_NEAREST */
	"FILTER_BILINEAR", /* COMAC_FILTER_BILINEAR */
	"FILTER_GAUSSIAN", /* COMAC_FILTER_GAUSSIAN */
    };
    assert (filter < ARRAY_LENGTH (names));
    return names[filter];
}

static const char *
_fill_rule_to_string (comac_fill_rule_t rule)
{
    static const char *names[] = {
	"WINDING", /* COMAC_FILL_RULE_WINDING */
	"EVEN_ODD" /* COMAC_FILL_RILE_EVEN_ODD */
    };
    assert (rule < ARRAY_LENGTH (names));
    return names[rule];
}

static const char *
_antialias_to_string (comac_antialias_t antialias)
{
    static const char *names[] = {
	"DEFAULT",  /* COMAC_ANTIALIAS_DEFAULT */
	"NONE",	    /* COMAC_ANTIALIAS_NONE */
	"GRAY",	    /* COMAC_ANTIALIAS_GRAY */
	"SUBPIXEL", /* COMAC_ANTIALIAS_SUBPIXEL */
	"FAST",	    /* COMAC_ANTIALIAS_FAST */
	"GOOD",	    /* COMAC_ANTIALIAS_GOOD */
	"BEST",	    /* COMAC_ANTIALIAS_BEST */
    };
    assert (antialias < ARRAY_LENGTH (names));
    return names[antialias];
}

static const char *
_line_cap_to_string (comac_line_cap_t line_cap)
{
    static const char *names[] = {
	"LINE_CAP_BUTT",  /* COMAC_LINE_CAP_BUTT */
	"LINE_CAP_ROUND", /* COMAC_LINE_CAP_ROUND */
	"LINE_CAP_SQUARE" /* COMAC_LINE_CAP_SQUARE */
    };
    assert (line_cap < ARRAY_LENGTH (names));
    return names[line_cap];
}

static const char *
_line_join_to_string (comac_line_join_t line_join)
{
    static const char *names[] = {
	"LINE_JOIN_MITER", /* COMAC_LINE_JOIN_MITER */
	"LINE_JOIN_ROUND", /* COMAC_LINE_JOIN_ROUND */
	"LINE_JOIN_BEVEL", /* COMAC_LINE_JOIN_BEVEL */
    };
    assert (line_join < ARRAY_LENGTH (names));
    return names[line_join];
}

static const char *
_content_to_string (comac_content_t content)
{
    switch (content) {
    case COMAC_CONTENT_ALPHA:
	return "ALPHA";
    case COMAC_CONTENT_COLOR:
	return "COLOR";
    default:
    case COMAC_CONTENT_COLOR_ALPHA:
	return "COLOR_ALPHA";
    }
}

static const char *
_format_to_string (comac_format_t format)
{
    switch (format) {
    case COMAC_FORMAT_ARGB32:
	return "ARGB32";
    case COMAC_FORMAT_RGB30:
	return "RGB30";
    case COMAC_FORMAT_RGB24:
	return "RGB24";
    case COMAC_FORMAT_RGB16_565:
	return "RGB16_565";
    case COMAC_FORMAT_RGB96F:
	return "RGB96F";
    case COMAC_FORMAT_RGBA128F:
	return "RGBA128F";
    case COMAC_FORMAT_A8:
	return "A8";
    case COMAC_FORMAT_A1:
	return "A1";
    case COMAC_FORMAT_INVALID:
	return "INVALID";
    }
    ASSERT_NOT_REACHED;
    return "INVALID";
}

static comac_status_t
_device_flush (void *abstract_device)
{
    comac_xml_t *xml = abstract_device;
    comac_status_t status;

    status = _comac_output_stream_flush (xml->stream);

    return status;
}

static void
_device_destroy (void *abstract_device)
{
    comac_xml_t *xml = abstract_device;
    comac_status_t status;

    status = _comac_output_stream_destroy (xml->stream);

    free (xml);
}

static const comac_device_backend_t _comac_xml_device_backend = {
    COMAC_DEVICE_TYPE_XML,

    NULL,
    NULL, /* lock, unlock */

    _device_flush,
    NULL, /* finish */
    _device_destroy};

static comac_device_t *
_comac_xml_create_internal (comac_output_stream_t *stream)
{
    comac_xml_t *xml;

    xml = _comac_malloc (sizeof (comac_xml_t));
    if (unlikely (xml == NULL))
	return _comac_device_create_in_error (COMAC_STATUS_NO_MEMORY);

    memset (xml, 0, sizeof (comac_xml_t));

    _comac_device_init (&xml->base, &_comac_xml_device_backend);

    xml->indent = 0;
    xml->stream = stream;

    return &xml->base;
}

static void
_comac_xml_indent (comac_xml_t *xml, int indent)
{
    xml->indent += indent;
    assert (xml->indent >= 0);
}

static void COMAC_PRINTF_FORMAT (2, 3)
    _comac_xml_printf (comac_xml_t *xml, const char *fmt, ...)
{
    va_list ap;
    char indent[80];
    int len;

    len = MIN (xml->indent, ARRAY_LENGTH (indent));
    memset (indent, ' ', len);
    _comac_output_stream_write (xml->stream, indent, len);

    va_start (ap, fmt);
    _comac_output_stream_vprintf (xml->stream, fmt, ap);
    va_end (ap);

    _comac_output_stream_write (xml->stream, "\n", 1);
}

static void COMAC_PRINTF_FORMAT (2, 3)
    _comac_xml_printf_start (comac_xml_t *xml, const char *fmt, ...)
{
    char indent[80];
    int len;

    len = MIN (xml->indent, ARRAY_LENGTH (indent));
    memset (indent, ' ', len);
    _comac_output_stream_write (xml->stream, indent, len);

    if (fmt != NULL) {
	va_list ap;

	va_start (ap, fmt);
	_comac_output_stream_vprintf (xml->stream, fmt, ap);
	va_end (ap);
    }
}

static void COMAC_PRINTF_FORMAT (2, 3)
    _comac_xml_printf_continue (comac_xml_t *xml, const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    _comac_output_stream_vprintf (xml->stream, fmt, ap);
    va_end (ap);
}

static void COMAC_PRINTF_FORMAT (2, 3)
    _comac_xml_printf_end (comac_xml_t *xml, const char *fmt, ...)
{
    if (fmt != NULL) {
	va_list ap;

	va_start (ap, fmt);
	_comac_output_stream_vprintf (xml->stream, fmt, ap);
	va_end (ap);
    }

    _comac_output_stream_write (xml->stream, "\n", 1);
}

static comac_surface_t *
_comac_xml_surface_create_similar (void *abstract_surface,
				   comac_content_t content,
				   int width,
				   int height)
{
    comac_rectangle_t extents;

    extents.x = extents.y = 0;
    extents.width = width;
    extents.height = height;

    return comac_recording_surface_create (content, &extents);
}

static comac_bool_t
_comac_xml_surface_get_extents (void *abstract_surface,
				comac_rectangle_int_t *rectangle)
{
    comac_xml_surface_t *surface = abstract_surface;

    if (surface->width < 0 || surface->height < 0)
	return FALSE;

    rectangle->x = 0;
    rectangle->y = 0;
    rectangle->width = surface->width;
    rectangle->height = surface->height;

    return TRUE;
}

static comac_status_t
_comac_xml_move_to (void *closure, const comac_point_t *p1)
{
    _comac_xml_printf_continue (closure,
				" %f %f m",
				_comac_fixed_to_double (p1->x),
				_comac_fixed_to_double (p1->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_line_to (void *closure, const comac_point_t *p1)
{
    _comac_xml_printf_continue (closure,
				" %f %f l",
				_comac_fixed_to_double (p1->x),
				_comac_fixed_to_double (p1->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_curve_to (void *closure,
		     const comac_point_t *p1,
		     const comac_point_t *p2,
		     const comac_point_t *p3)
{
    _comac_xml_printf_continue (closure,
				" %f %f %f %f %f %f c",
				_comac_fixed_to_double (p1->x),
				_comac_fixed_to_double (p1->y),
				_comac_fixed_to_double (p2->x),
				_comac_fixed_to_double (p2->y),
				_comac_fixed_to_double (p3->x),
				_comac_fixed_to_double (p3->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_close_path (void *closure)
{
    _comac_xml_printf_continue (closure, " h");

    return COMAC_STATUS_SUCCESS;
}

static void
_comac_xml_emit_path (comac_xml_t *xml, const comac_path_fixed_t *path)
{
    comac_status_t status;

    _comac_xml_printf_start (xml, "<path>");
    status = _comac_path_fixed_interpret (path,
					  _comac_xml_move_to,
					  _comac_xml_line_to,
					  _comac_xml_curve_to,
					  _comac_xml_close_path,
					  xml);
    assert (status == COMAC_STATUS_SUCCESS);
    _comac_xml_printf_end (xml, "</path>");
}

static void
_comac_xml_emit_string (comac_xml_t *xml, const char *node, const char *data)
{
    _comac_xml_printf (xml, "<%s>%s</%s>", node, data, node);
}

static void
_comac_xml_emit_double (comac_xml_t *xml, const char *node, double data)
{
    _comac_xml_printf (xml, "<%s>%f</%s>", node, data, node);
}

static comac_xml_t *
to_xml (comac_xml_surface_t *surface)
{
    return (comac_xml_t *) surface->base.device;
}

static comac_status_t
_comac_xml_surface_emit_clip_boxes (comac_xml_surface_t *surface,
				    const comac_clip_t *clip)
{
    comac_box_t *box;
    comac_xml_t *xml;
    int n;

    if (clip->num_boxes == 0)
	return COMAC_STATUS_SUCCESS;

    /* skip the trivial clip covering the surface extents */
    if (surface->width >= 0 && surface->height >= 0 && clip->num_boxes == 1) {
	box = &clip->boxes[0];
	if (box->p1.x <= 0 && box->p1.y <= 0 &&
	    box->p2.x - box->p1.x >=
		_comac_fixed_from_double (surface->width) &&
	    box->p2.y - box->p1.y >=
		_comac_fixed_from_double (surface->height)) {
	    return COMAC_STATUS_SUCCESS;
	}
    }

    xml = to_xml (surface);

    _comac_xml_printf (xml, "<clip>");
    _comac_xml_indent (xml, 2);

    _comac_xml_printf (xml, "<path>");
    _comac_xml_indent (xml, 2);
    for (n = 0; n < clip->num_boxes; n++) {
	box = &clip->boxes[n];

	_comac_xml_printf_start (xml,
				 "%f %f m",
				 _comac_fixed_to_double (box->p1.x),
				 _comac_fixed_to_double (box->p1.y));
	_comac_xml_printf_continue (xml,
				    " %f %f l",
				    _comac_fixed_to_double (box->p2.x),
				    _comac_fixed_to_double (box->p1.y));
	_comac_xml_printf_continue (xml,
				    " %f %f l",
				    _comac_fixed_to_double (box->p2.x),
				    _comac_fixed_to_double (box->p2.y));
	_comac_xml_printf_continue (xml,
				    " %f %f l",
				    _comac_fixed_to_double (box->p1.x),
				    _comac_fixed_to_double (box->p2.y));
	_comac_xml_printf_end (xml, " h");
    }
    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</path>");
    _comac_xml_emit_double (xml, "tolerance", 1.0);
    _comac_xml_emit_string (xml,
			    "antialias",
			    _antialias_to_string (COMAC_ANTIALIAS_NONE));
    _comac_xml_emit_string (xml,
			    "fill-rule",
			    _fill_rule_to_string (COMAC_FILL_RULE_WINDING));

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</clip>");

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_surface_emit_clip_path (comac_xml_surface_t *surface,
				   const comac_clip_path_t *clip_path)
{
    comac_box_t box;
    comac_status_t status;
    comac_xml_t *xml;

    if (clip_path == NULL)
	return COMAC_STATUS_SUCCESS;

    status = _comac_xml_surface_emit_clip_path (surface, clip_path->prev);
    if (unlikely (status))
	return status;

    /* skip the trivial clip covering the surface extents */
    if (surface->width >= 0 && surface->height >= 0 &&
	_comac_path_fixed_is_box (&clip_path->path, &box)) {
	if (box.p1.x <= 0 && box.p1.y <= 0 &&
	    box.p2.x - box.p1.x >= _comac_fixed_from_double (surface->width) &&
	    box.p2.y - box.p1.y >= _comac_fixed_from_double (surface->height)) {
	    return COMAC_STATUS_SUCCESS;
	}
    }

    xml = to_xml (surface);

    _comac_xml_printf_start (xml, "<clip>");
    _comac_xml_indent (xml, 2);

    _comac_xml_emit_path (xml, &clip_path->path);
    _comac_xml_emit_double (xml, "tolerance", clip_path->tolerance);
    _comac_xml_emit_string (xml,
			    "antialias",
			    _antialias_to_string (clip_path->antialias));
    _comac_xml_emit_string (xml,
			    "fill-rule",
			    _fill_rule_to_string (clip_path->fill_rule));

    _comac_xml_indent (xml, -2);
    _comac_xml_printf_end (xml, "</clip>");

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_surface_emit_clip (comac_xml_surface_t *surface,
			      const comac_clip_t *clip)
{
    comac_status_t status;

    if (clip == NULL)
	return COMAC_STATUS_SUCCESS;

    status = _comac_xml_surface_emit_clip_boxes (surface, clip);
    if (unlikely (status))
	return status;

    return _comac_xml_surface_emit_clip_path (surface, clip->path);
}

static comac_status_t
_comac_xml_emit_solid (comac_xml_t *xml, const comac_solid_pattern_t *solid)
{
    _comac_xml_printf (xml,
		       "<solid>%f %f %f %f</solid>",
		       solid->color.red,
		       solid->color.green,
		       solid->color.blue,
		       solid->color.alpha);
    return COMAC_STATUS_SUCCESS;
}

static void
_comac_xml_emit_matrix (comac_xml_t *xml, const comac_matrix_t *matrix)
{
    if (! _comac_matrix_is_identity (matrix)) {
	_comac_xml_printf (xml,
			   "<matrix>%f %f %f %f %f %f</matrix>",
			   matrix->xx,
			   matrix->yx,
			   matrix->xy,
			   matrix->yy,
			   matrix->x0,
			   matrix->y0);
    }
}

static void
_comac_xml_emit_gradient (comac_xml_t *xml,
			  const comac_gradient_pattern_t *gradient)
{
    unsigned int i;

    for (i = 0; i < gradient->n_stops; i++) {
	_comac_xml_printf (xml,
			   "<color-stop>%f %f %f %f %f</color-stop>",
			   gradient->stops[i].offset,
			   gradient->stops[i].color.red,
			   gradient->stops[i].color.green,
			   gradient->stops[i].color.blue,
			   gradient->stops[i].color.alpha);
    }
}

static comac_status_t
_comac_xml_emit_linear (comac_xml_t *xml, const comac_linear_pattern_t *linear)
{
    _comac_xml_printf (xml,
		       "<linear x1='%f' y1='%f' x2='%f' y2='%f'>",
		       linear->pd1.x,
		       linear->pd1.y,
		       linear->pd2.x,
		       linear->pd2.y);
    _comac_xml_indent (xml, 2);
    _comac_xml_emit_gradient (xml, &linear->base);
    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</linear>");
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_emit_radial (comac_xml_t *xml, const comac_radial_pattern_t *radial)
{
    _comac_xml_printf (
	xml,
	"<radial x1='%f' y1='%f' r1='%f' x2='%f' y2='%f' r2='%f'>",
	radial->cd1.center.x,
	radial->cd1.center.y,
	radial->cd1.radius,
	radial->cd2.center.x,
	radial->cd2.center.y,
	radial->cd2.radius);
    _comac_xml_indent (xml, 2);
    _comac_xml_emit_gradient (xml, &radial->base);
    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</radial>");
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_write_func (void *closure, const unsigned char *data, unsigned len)
{
    _comac_output_stream_write (closure, data, len);
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_emit_image (comac_xml_t *xml, comac_image_surface_t *image)
{
    comac_output_stream_t *stream;
    comac_status_t status;

    _comac_xml_printf_start (xml,
			     "<image width='%d' height='%d' format='%s'>",
			     image->width,
			     image->height,
			     _format_to_string (image->format));

    stream = _comac_base64_stream_create (xml->stream);
    status =
	comac_surface_write_to_png_stream (&image->base, _write_func, stream);
    assert (status == COMAC_STATUS_SUCCESS);
    status = _comac_output_stream_destroy (stream);
    if (unlikely (status))
	return status;

    _comac_xml_printf_end (xml, "</image>");

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_emit_surface (comac_xml_t *xml,
			 const comac_surface_pattern_t *pattern)
{
    comac_surface_t *source = pattern->surface;
    comac_status_t status;

    if (_comac_surface_is_recording (source)) {
	status = comac_xml_for_recording_surface (&xml->base, source);
    } else {
	comac_image_surface_t *image;
	void *image_extra;

	status =
	    _comac_surface_acquire_source_image (source, &image, &image_extra);
	if (unlikely (status))
	    return status;

	status = _comac_xml_emit_image (xml, image);

	_comac_surface_release_source_image (source, image, image_extra);
    }

    return status;
}

static comac_status_t
_comac_xml_emit_pattern (comac_xml_t *xml,
			 const char *source_or_mask,
			 const comac_pattern_t *pattern)
{
    comac_status_t status;

    _comac_xml_printf (xml, "<%s-pattern>", source_or_mask);
    _comac_xml_indent (xml, 2);

    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	status = _comac_xml_emit_solid (xml, (comac_solid_pattern_t *) pattern);
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	status =
	    _comac_xml_emit_linear (xml, (comac_linear_pattern_t *) pattern);
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	status =
	    _comac_xml_emit_radial (xml, (comac_radial_pattern_t *) pattern);
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	status =
	    _comac_xml_emit_surface (xml, (comac_surface_pattern_t *) pattern);
	break;
    default:
	ASSERT_NOT_REACHED;
	status = COMAC_INT_STATUS_UNSUPPORTED;
	break;
    }

    if (pattern->type != COMAC_PATTERN_TYPE_SOLID) {
	_comac_xml_emit_matrix (xml, &pattern->matrix);
	_comac_xml_printf (xml,
			   "<extend>%s</extend>",
			   _extend_to_string (pattern->extend));
	_comac_xml_printf (xml,
			   "<filter>%s</filter>",
			   _filter_to_string (pattern->filter));
    }

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</%s-pattern>", source_or_mask);

    return status;
}

static comac_int_status_t
_comac_xml_surface_paint (void *abstract_surface,
			  comac_operator_t op,
			  const comac_pattern_t *source,
			  const comac_clip_t *clip)
{
    comac_xml_surface_t *surface = abstract_surface;
    comac_xml_t *xml = to_xml (surface);
    comac_status_t status;

    _comac_xml_printf (xml, "<paint>");
    _comac_xml_indent (xml, 2);

    _comac_xml_emit_string (xml, "operator", _operator_to_string (op));

    status = _comac_xml_surface_emit_clip (surface, clip);
    if (unlikely (status))
	return status;

    status = _comac_xml_emit_pattern (xml, "source", source);
    if (unlikely (status))
	return status;

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</paint>");

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_comac_xml_surface_mask (void *abstract_surface,
			 comac_operator_t op,
			 const comac_pattern_t *source,
			 const comac_pattern_t *mask,
			 const comac_clip_t *clip)
{
    comac_xml_surface_t *surface = abstract_surface;
    comac_xml_t *xml = to_xml (surface);
    comac_status_t status;

    _comac_xml_printf (xml, "<mask>");
    _comac_xml_indent (xml, 2);

    _comac_xml_emit_string (xml, "operator", _operator_to_string (op));

    status = _comac_xml_surface_emit_clip (surface, clip);
    if (unlikely (status))
	return status;

    status = _comac_xml_emit_pattern (xml, "source", source);
    if (unlikely (status))
	return status;

    status = _comac_xml_emit_pattern (xml, "mask", mask);
    if (unlikely (status))
	return status;

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</mask>");

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_comac_xml_surface_stroke (void *abstract_surface,
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
    comac_xml_surface_t *surface = abstract_surface;
    comac_xml_t *xml = to_xml (surface);
    comac_status_t status;

    _comac_xml_printf (xml, "<stroke>");
    _comac_xml_indent (xml, 2);

    _comac_xml_emit_string (xml, "operator", _operator_to_string (op));
    _comac_xml_emit_double (xml, "line-width", style->line_width);
    _comac_xml_emit_double (xml, "miter-limit", style->miter_limit);
    _comac_xml_emit_string (xml,
			    "line-cap",
			    _line_cap_to_string (style->line_cap));
    _comac_xml_emit_string (xml,
			    "line-join",
			    _line_join_to_string (style->line_join));

    status = _comac_xml_surface_emit_clip (surface, clip);
    if (unlikely (status))
	return status;

    status = _comac_xml_emit_pattern (xml, "source", source);
    if (unlikely (status))
	return status;

    if (style->num_dashes) {
	unsigned int i;

	_comac_xml_printf_start (xml, "<dash offset='%f'>", style->dash_offset);
	for (i = 0; i < style->num_dashes; i++)
	    _comac_xml_printf_continue (xml, "%f ", style->dash[i]);

	_comac_xml_printf_end (xml, "</dash>");
    }

    _comac_xml_emit_path (xml, path);
    _comac_xml_emit_double (xml, "tolerance", tolerance);
    _comac_xml_emit_string (xml, "antialias", _antialias_to_string (antialias));

    _comac_xml_emit_matrix (xml, ctm);

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</stroke>");

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_comac_xml_surface_fill (void *abstract_surface,
			 comac_operator_t op,
			 const comac_pattern_t *source,
			 const comac_path_fixed_t *path,
			 comac_fill_rule_t fill_rule,
			 double tolerance,
			 comac_antialias_t antialias,
			 const comac_clip_t *clip)
{
    comac_xml_surface_t *surface = abstract_surface;
    comac_xml_t *xml = to_xml (surface);
    comac_status_t status;

    _comac_xml_printf (xml, "<fill>");
    _comac_xml_indent (xml, 2);

    _comac_xml_emit_string (xml, "operator", _operator_to_string (op));

    status = _comac_xml_surface_emit_clip (surface, clip);
    if (unlikely (status))
	return status;

    status = _comac_xml_emit_pattern (xml, "source", source);
    if (unlikely (status))
	return status;

    _comac_xml_emit_path (xml, path);
    _comac_xml_emit_double (xml, "tolerance", tolerance);
    _comac_xml_emit_string (xml, "antialias", _antialias_to_string (antialias));
    _comac_xml_emit_string (xml, "fill-rule", _fill_rule_to_string (fill_rule));

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</fill>");

    return COMAC_STATUS_SUCCESS;
}

#if COMAC_HAS_FT_FONT
#include "comac-ft-private.h"
static comac_status_t
_comac_xml_emit_type42_font (comac_xml_t *xml, comac_scaled_font_t *scaled_font)
{
    const comac_scaled_font_backend_t *backend;
    comac_output_stream_t *base64_stream;
    comac_output_stream_t *zlib_stream;
    comac_status_t status, status2;
    unsigned long size;
    uint32_t len;
    uint8_t *buf;

    backend = scaled_font->backend;
    if (backend->load_truetype_table == NULL)
	return COMAC_INT_STATUS_UNSUPPORTED;

    size = 0;
    status = backend->load_truetype_table (scaled_font, 0, 0, NULL, &size);
    if (unlikely (status))
	return status;

    buf = _comac_malloc (size);
    if (unlikely (buf == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = backend->load_truetype_table (scaled_font, 0, 0, buf, &size);
    if (unlikely (status)) {
	free (buf);
	return status;
    }

    _comac_xml_printf_start (
	xml,
	"<font type='42' flags='%d' index='0'>",
	_comac_ft_scaled_font_get_load_flags (scaled_font));

    base64_stream = _comac_base64_stream_create (xml->stream);
    len = size;
    _comac_output_stream_write (base64_stream, &len, sizeof (len));

    zlib_stream = _comac_deflate_stream_create (base64_stream);

    _comac_output_stream_write (zlib_stream, buf, size);
    free (buf);

    status2 = _comac_output_stream_destroy (zlib_stream);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;

    status2 = _comac_output_stream_destroy (base64_stream);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;

    _comac_xml_printf_end (xml, "</font>");

    return status;
}
#else
static comac_status_t
_comac_xml_emit_type42_font (comac_xml_t *xml, comac_scaled_font_t *scaled_font)
{
    return COMAC_INT_STATUS_UNSUPPORTED;
}
#endif

static comac_status_t
_comac_xml_emit_type3_font (comac_xml_t *xml,
			    comac_scaled_font_t *scaled_font,
			    comac_glyph_t *glyphs,
			    int num_glyphs)
{
    _comac_xml_printf_start (xml, "<font type='3'>");
    _comac_xml_printf_end (xml, "</font>");

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_xml_emit_scaled_font (comac_xml_t *xml,
			     comac_scaled_font_t *scaled_font,
			     comac_glyph_t *glyphs,
			     int num_glyphs)
{
    comac_int_status_t status;

    _comac_xml_printf (xml, "<scaled-font>");
    _comac_xml_indent (xml, 2);

    status = _comac_xml_emit_type42_font (xml, scaled_font);
    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	status =
	    _comac_xml_emit_type3_font (xml, scaled_font, glyphs, num_glyphs);
    }

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</scaled-font>");

    return status;
}

static comac_int_status_t
_comac_xml_surface_glyphs (void *abstract_surface,
			   comac_operator_t op,
			   const comac_pattern_t *source,
			   comac_glyph_t *glyphs,
			   int num_glyphs,
			   comac_scaled_font_t *scaled_font,
			   const comac_clip_t *clip)
{
    comac_xml_surface_t *surface = abstract_surface;
    comac_xml_t *xml = to_xml (surface);
    comac_status_t status;
    int i;

    _comac_xml_printf (xml, "<glyphs>");
    _comac_xml_indent (xml, 2);

    _comac_xml_emit_string (xml, "operator", _operator_to_string (op));

    status = _comac_xml_surface_emit_clip (surface, clip);
    if (unlikely (status))
	return status;

    status = _comac_xml_emit_pattern (xml, "source", source);
    if (unlikely (status))
	return status;

    status = _comac_xml_emit_scaled_font (xml, scaled_font, glyphs, num_glyphs);
    if (unlikely (status))
	return status;

    for (i = 0; i < num_glyphs; i++) {
	_comac_xml_printf (xml,
			   "<glyph index='%lu'>%f %f</glyph>",
			   glyphs[i].index,
			   glyphs[i].x,
			   glyphs[i].y);
    }

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</glyphs>");

    return COMAC_STATUS_SUCCESS;
}

static const comac_surface_backend_t _comac_xml_surface_backend = {
    COMAC_SURFACE_TYPE_XML,
    NULL,

    _comac_default_context_create,

    _comac_xml_surface_create_similar,
    NULL, /* create_similar_image */
    NULL, /* map_to_image */
    NULL, /* unmap_image */

    _comac_surface_default_source,
    NULL, /* acquire source image */
    NULL, /* release source image */
    NULL, /* snapshot */

    NULL, /* copy page */
    NULL, /* show page */

    _comac_xml_surface_get_extents,
    NULL, /* get_font_options */

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    _comac_xml_surface_paint,
    _comac_xml_surface_mask,
    _comac_xml_surface_stroke,
    _comac_xml_surface_fill,
    NULL, /* fill_stroke */
    _comac_xml_surface_glyphs,
};

static comac_surface_t *
_comac_xml_surface_create_internal (comac_device_t *device,
				    comac_content_t content,
				    double width,
				    double height)
{
    comac_xml_surface_t *surface;

    surface = _comac_malloc (sizeof (comac_xml_surface_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_surface_init (&surface->base,
			 &_comac_xml_surface_backend,
			 device,
			 content,
			 TRUE); /* is_vector */

    surface->width = width;
    surface->height = height;

    return &surface->base;
}

comac_device_t *
comac_xml_create (const char *filename)
{
    comac_output_stream_t *stream;
    comac_status_t status;

    stream = _comac_output_stream_create_for_filename (filename);
    if ((status = _comac_output_stream_get_status (stream)))
	return _comac_device_create_in_error (status);

    return _comac_xml_create_internal (stream);
}

comac_device_t *
comac_xml_create_for_stream (comac_write_func_t write_func, void *closure)
{
    comac_output_stream_t *stream;
    comac_status_t status;

    stream = _comac_output_stream_create (write_func, NULL, closure);
    if ((status = _comac_output_stream_get_status (stream)))
	return _comac_device_create_in_error (status);

    return _comac_xml_create_internal (stream);
}

comac_surface_t *
comac_xml_surface_create (comac_device_t *device,
			  comac_content_t content,
			  double width,
			  double height)
{
    if (unlikely (device->backend->type != COMAC_DEVICE_TYPE_XML))
	return _comac_surface_create_in_error (
	    COMAC_STATUS_DEVICE_TYPE_MISMATCH);

    if (unlikely (device->status))
	return _comac_surface_create_in_error (device->status);

    return _comac_xml_surface_create_internal (device, content, width, height);
}

comac_status_t
comac_xml_for_recording_surface (comac_device_t *device,
				 comac_surface_t *recording_surface)
{
    comac_box_t bbox;
    comac_rectangle_int_t extents;
    comac_surface_t *surface;
    comac_xml_t *xml;
    comac_status_t status;

    if (unlikely (device->status))
	return device->status;

    if (unlikely (recording_surface->status))
	return recording_surface->status;

    if (unlikely (device->backend->type != COMAC_DEVICE_TYPE_XML))
	return _comac_error (COMAC_STATUS_DEVICE_TYPE_MISMATCH);

    if (unlikely (! _comac_surface_is_recording (recording_surface)))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    status = _comac_recording_surface_get_bbox (
	(comac_recording_surface_t *) recording_surface,
	&bbox,
	NULL);
    if (unlikely (status))
	return status;

    _comac_box_round_to_rectangle (&bbox, &extents);
    surface = _comac_xml_surface_create_internal (device,
						  recording_surface->content,
						  extents.width,
						  extents.height);
    if (unlikely (surface->status))
	return surface->status;

    xml = (comac_xml_t *) device;

    _comac_xml_printf (xml,
		       "<surface content='%s' width='%d' height='%d'>",
		       _content_to_string (recording_surface->content),
		       extents.width,
		       extents.height);
    _comac_xml_indent (xml, 2);

    comac_surface_set_device_offset (surface, -extents.x, -extents.y);
    status = _comac_recording_surface_replay (recording_surface, surface);
    comac_surface_destroy (surface);

    _comac_xml_indent (xml, -2);
    _comac_xml_printf (xml, "</surface>");

    return status;
}
