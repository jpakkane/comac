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
#ifndef COMAC_SPANS_PRIVATE_H
#define COMAC_SPANS_PRIVATE_H
#include "comac-types-private.h"
#include "comac-compiler-private.h"

/* Number of bits of precision used for alpha. */
#define COMAC_SPANS_UNIT_COVERAGE_BITS 8
#define COMAC_SPANS_UNIT_COVERAGE ((1 << COMAC_SPANS_UNIT_COVERAGE_BITS) - 1)

/* A structure representing an open-ended horizontal span of constant
 * pixel coverage. */
typedef struct _comac_half_open_span {
    int32_t x;	      /* The inclusive x-coordinate of the start of the span. */
    uint8_t coverage; /* The pixel coverage for the pixels to the right. */
    uint8_t inverse;  /* between regular mask and clip */
} comac_half_open_span_t;

/* Span renderer interface. Instances of renderers are provided by
 * surfaces if they want to composite spans instead of trapezoids. */
typedef struct _comac_span_renderer comac_span_renderer_t;
struct _comac_span_renderer {
    /* Private status variable. */
    comac_status_t status;

    /* Called to destroy the renderer. */
    comac_destroy_func_t destroy;

    /* Render the spans on row y of the destination by whatever compositing
     * method is required. */
    comac_status_t (*render_rows) (void *abstract_renderer,
				   int y,
				   int height,
				   const comac_half_open_span_t *coverages,
				   unsigned num_coverages);

    /* Called after all rows have been rendered to perform whatever
     * final rendering step is required.  This function is called just
     * once before the renderer is destroyed. */
    comac_status_t (*finish) (void *abstract_renderer);
};

/* Scan converter interface. */
typedef struct _comac_scan_converter comac_scan_converter_t;
struct _comac_scan_converter {
    /* Destroy this scan converter. */
    comac_destroy_func_t destroy;

    /* Generates coverage spans for rows for the added edges and calls
     * the renderer function for each row. After generating spans the
     * only valid thing to do with the converter is to destroy it. */
    comac_status_t (*generate) (void *abstract_converter,
				comac_span_renderer_t *renderer);

    /* Private status. Read with _comac_scan_converter_status(). */
    comac_status_t status;
};

/* Scan converter constructors. */

comac_private comac_scan_converter_t *
_comac_tor_scan_converter_create (int xmin,
				  int ymin,
				  int xmax,
				  int ymax,
				  comac_fill_rule_t fill_rule,
				  comac_antialias_t antialias);
comac_private comac_status_t
_comac_tor_scan_converter_add_polygon (void *converter,
				       const comac_polygon_t *polygon);

comac_private comac_scan_converter_t *
_comac_tor22_scan_converter_create (int xmin,
				    int ymin,
				    int xmax,
				    int ymax,
				    comac_fill_rule_t fill_rule,
				    comac_antialias_t antialias);
comac_private comac_status_t
_comac_tor22_scan_converter_add_polygon (void *converter,
					 const comac_polygon_t *polygon);

comac_private comac_scan_converter_t *
_comac_mono_scan_converter_create (
    int xmin, int ymin, int xmax, int ymax, comac_fill_rule_t fill_rule);
comac_private comac_status_t
_comac_mono_scan_converter_add_polygon (void *converter,
					const comac_polygon_t *polygon);

comac_private comac_scan_converter_t *
_comac_clip_tor_scan_converter_create (comac_clip_t *clip,
				       comac_polygon_t *polygon,
				       comac_fill_rule_t fill_rule,
				       comac_antialias_t antialias);

typedef struct _comac_rectangular_scan_converter {
    comac_scan_converter_t base;

    comac_box_t extents;

    struct _comac_rectangular_scan_converter_chunk {
	struct _comac_rectangular_scan_converter_chunk *next;
	void *base;
	int count;
	int size;
    } chunks, *tail;
    char buf[COMAC_STACK_BUFFER_SIZE];
    int num_rectangles;
} comac_rectangular_scan_converter_t;

comac_private void
_comac_rectangular_scan_converter_init (
    comac_rectangular_scan_converter_t *self,
    const comac_rectangle_int_t *extents);

comac_private comac_status_t
_comac_rectangular_scan_converter_add_box (
    comac_rectangular_scan_converter_t *self, const comac_box_t *box, int dir);

typedef struct _comac_botor_scan_converter {
    comac_scan_converter_t base;

    comac_box_t extents;
    comac_fill_rule_t fill_rule;

    int xmin, xmax;

    struct _comac_botor_scan_converter_chunk {
	struct _comac_botor_scan_converter_chunk *next;
	void *base;
	int count;
	int size;
    } chunks, *tail;
    char buf[COMAC_STACK_BUFFER_SIZE];
    int num_edges;
} comac_botor_scan_converter_t;

comac_private void
_comac_botor_scan_converter_init (comac_botor_scan_converter_t *self,
				  const comac_box_t *extents,
				  comac_fill_rule_t fill_rule);

comac_private comac_status_t
_comac_botor_scan_converter_add_polygon (
    comac_botor_scan_converter_t *converter, const comac_polygon_t *polygon);

/* comac-spans.c: */

comac_private comac_scan_converter_t *
_comac_scan_converter_create_in_error (comac_status_t error);

comac_private comac_status_t
_comac_scan_converter_status (void *abstract_converter);

comac_private comac_status_t
_comac_scan_converter_set_error (void *abstract_converter,
				 comac_status_t error);

comac_private comac_span_renderer_t *
_comac_span_renderer_create_in_error (comac_status_t error);

comac_private comac_status_t
_comac_span_renderer_status (void *abstract_renderer);

/* Set the renderer into an error state.  This sets all the method
 * pointers except ->destroy() of the renderer to no-op
 * implementations that just return the error status. */
comac_private comac_status_t
_comac_span_renderer_set_error (void *abstract_renderer, comac_status_t error);

comac_private comac_status_t
_comac_surface_composite_polygon (comac_surface_t *surface,
				  comac_operator_t op,
				  const comac_pattern_t *pattern,
				  comac_fill_rule_t fill_rule,
				  comac_antialias_t antialias,
				  const comac_composite_rectangles_t *rects,
				  comac_polygon_t *polygon,
				  comac_region_t *clip_region);

#endif /* COMAC_SPANS_PRIVATE_H */
