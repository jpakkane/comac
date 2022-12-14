/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright (c) 2008  M Joonas Pihlaja
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "comacint.h"

#include "comac-composite-rectangles-private.h"
#include "comac-clip-private.h"
#include "comac-error-private.h"
#include "comac-fixed-private.h"
#include "comac-types-private.h"

static void
_comac_nil_destroy (void *abstract)
{
    (void) abstract;
}

static comac_status_t
_comac_nil_scan_converter_generate (void *abstract_converter,
				    comac_span_renderer_t *renderer)
{
    (void) abstract_converter;
    (void) renderer;
    return _comac_scan_converter_status (abstract_converter);
}

comac_status_t
_comac_scan_converter_status (void *abstract_converter)
{
    comac_scan_converter_t *converter = abstract_converter;
    return converter->status;
}

comac_status_t
_comac_scan_converter_set_error (void *abstract_converter, comac_status_t error)
{
    comac_scan_converter_t *converter = abstract_converter;
    if (error == COMAC_STATUS_SUCCESS)
	ASSERT_NOT_REACHED;
    if (converter->status == COMAC_STATUS_SUCCESS) {
	converter->generate = _comac_nil_scan_converter_generate;
	converter->status = error;
    }
    return converter->status;
}

static void
_comac_nil_scan_converter_init (comac_scan_converter_t *converter,
				comac_status_t status)
{
    converter->destroy = _comac_nil_destroy;
    converter->status = COMAC_STATUS_SUCCESS;
    status = _comac_scan_converter_set_error (converter, status);
}

comac_scan_converter_t *
_comac_scan_converter_create_in_error (comac_status_t status)
{
#define RETURN_NIL                                                             \
    {                                                                          \
	static comac_scan_converter_t nil;                                     \
	_comac_nil_scan_converter_init (&nil, status);                         \
	return &nil;                                                           \
    }
    switch (status) {
    case COMAC_STATUS_SUCCESS:
    case COMAC_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	break;
    case COMAC_STATUS_INVALID_RESTORE:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_POP_GROUP:
	RETURN_NIL;
    case COMAC_STATUS_NO_CURRENT_POINT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_MATRIX:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_STATUS:
	RETURN_NIL;
    case COMAC_STATUS_NULL_POINTER:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_STRING:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_PATH_DATA:
	RETURN_NIL;
    case COMAC_STATUS_READ_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_WRITE_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_SURFACE_FINISHED:
	RETURN_NIL;
    case COMAC_STATUS_SURFACE_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_PATTERN_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_CONTENT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_FORMAT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_VISUAL:
	RETURN_NIL;
    case COMAC_STATUS_FILE_NOT_FOUND:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_DASH:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_DSC_COMMENT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_INDEX:
	RETURN_NIL;
    case COMAC_STATUS_CLIP_NOT_REPRESENTABLE:
	RETURN_NIL;
    case COMAC_STATUS_TEMP_FILE_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_STRIDE:
	RETURN_NIL;
    case COMAC_STATUS_FONT_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_USER_FONT_IMMUTABLE:
	RETURN_NIL;
    case COMAC_STATUS_USER_FONT_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_NEGATIVE_COUNT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_CLUSTERS:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_SLANT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_WEIGHT:
	RETURN_NIL;
    case COMAC_STATUS_NO_MEMORY:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_SIZE:
	RETURN_NIL;
    case COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED:
	RETURN_NIL;
    case COMAC_STATUS_DEVICE_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_DEVICE_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_MESH_CONSTRUCTION:
	RETURN_NIL;
    case COMAC_STATUS_DEVICE_FINISHED:
	RETURN_NIL;
    case COMAC_STATUS_JBIG2_GLOBAL_MISSING:
    case COMAC_STATUS_PNG_ERROR:
    case COMAC_STATUS_FREETYPE_ERROR:
    case COMAC_STATUS_WIN32_GDI_ERROR:
    case COMAC_STATUS_TAG_ERROR:
    case COMAC_STATUS_DWRITE_ERROR:
    default:
	break;
    }
    status = COMAC_STATUS_NO_MEMORY;
    RETURN_NIL;
#undef RETURN_NIL
}

static comac_status_t
_comac_nil_span_renderer_render_rows (void *abstract_renderer,
				      int y,
				      int height,
				      const comac_half_open_span_t *coverages,
				      unsigned num_coverages)
{
    (void) y;
    (void) height;
    (void) coverages;
    (void) num_coverages;
    return _comac_span_renderer_status (abstract_renderer);
}

static comac_status_t
_comac_nil_span_renderer_finish (void *abstract_renderer)
{
    return _comac_span_renderer_status (abstract_renderer);
}

comac_status_t
_comac_span_renderer_status (void *abstract_renderer)
{
    comac_span_renderer_t *renderer = abstract_renderer;
    return renderer->status;
}

comac_status_t
_comac_span_renderer_set_error (void *abstract_renderer, comac_status_t error)
{
    comac_span_renderer_t *renderer = abstract_renderer;
    if (error == COMAC_STATUS_SUCCESS) {
	ASSERT_NOT_REACHED;
    }
    if (renderer->status == COMAC_STATUS_SUCCESS) {
	renderer->render_rows = _comac_nil_span_renderer_render_rows;
	renderer->finish = _comac_nil_span_renderer_finish;
	renderer->status = error;
    }
    return renderer->status;
}

static void
_comac_nil_span_renderer_init (comac_span_renderer_t *renderer,
			       comac_status_t status)
{
    renderer->destroy = _comac_nil_destroy;
    renderer->status = COMAC_STATUS_SUCCESS;
    status = _comac_span_renderer_set_error (renderer, status);
}

comac_span_renderer_t *
_comac_span_renderer_create_in_error (comac_status_t status)
{
#define RETURN_NIL                                                             \
    {                                                                          \
	static comac_span_renderer_t nil;                                      \
	_comac_nil_span_renderer_init (&nil, status);                          \
	return &nil;                                                           \
    }
    switch (status) {
    case COMAC_STATUS_SUCCESS:
    case COMAC_STATUS_LAST_STATUS:
	ASSERT_NOT_REACHED;
	break;
    case COMAC_STATUS_INVALID_RESTORE:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_POP_GROUP:
	RETURN_NIL;
    case COMAC_STATUS_NO_CURRENT_POINT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_MATRIX:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_STATUS:
	RETURN_NIL;
    case COMAC_STATUS_NULL_POINTER:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_STRING:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_PATH_DATA:
	RETURN_NIL;
    case COMAC_STATUS_READ_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_WRITE_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_SURFACE_FINISHED:
	RETURN_NIL;
    case COMAC_STATUS_SURFACE_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_PATTERN_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_CONTENT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_FORMAT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_VISUAL:
	RETURN_NIL;
    case COMAC_STATUS_FILE_NOT_FOUND:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_DASH:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_DSC_COMMENT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_INDEX:
	RETURN_NIL;
    case COMAC_STATUS_CLIP_NOT_REPRESENTABLE:
	RETURN_NIL;
    case COMAC_STATUS_TEMP_FILE_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_STRIDE:
	RETURN_NIL;
    case COMAC_STATUS_FONT_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_USER_FONT_IMMUTABLE:
	RETURN_NIL;
    case COMAC_STATUS_USER_FONT_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_NEGATIVE_COUNT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_CLUSTERS:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_SLANT:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_WEIGHT:
	RETURN_NIL;
    case COMAC_STATUS_NO_MEMORY:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_SIZE:
	RETURN_NIL;
    case COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED:
	RETURN_NIL;
    case COMAC_STATUS_DEVICE_TYPE_MISMATCH:
	RETURN_NIL;
    case COMAC_STATUS_DEVICE_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_INVALID_MESH_CONSTRUCTION:
	RETURN_NIL;
    case COMAC_STATUS_DEVICE_FINISHED:
	RETURN_NIL;
    case COMAC_STATUS_JBIG2_GLOBAL_MISSING:
	RETURN_NIL;
    case COMAC_STATUS_PNG_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_FREETYPE_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_WIN32_GDI_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_TAG_ERROR:
	RETURN_NIL;
    case COMAC_STATUS_DWRITE_ERROR:
	RETURN_NIL;
    default:
	break;
    }
    status = COMAC_STATUS_NO_MEMORY;
    RETURN_NIL;
#undef RETURN_NIL
}
