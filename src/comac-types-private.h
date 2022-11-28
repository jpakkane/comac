/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

#ifndef COMAC_TYPES_PRIVATE_H
#define COMAC_TYPES_PRIVATE_H

#include "comac.h"
#include "comac-fixed-type-private.h"
#include "comac-list-private.h"
#include "comac-reference-count-private.h"

COMAC_BEGIN_DECLS

/**
 * SECTION:comac-types
 * @Title: Types
 * @Short_Description: Generic data types
 *
 * This section lists generic data types used in the comac API.
 **/

typedef struct _comac_array comac_array_t;
typedef struct _comac_backend comac_backend_t;
typedef struct _comac_boxes_t comac_boxes_t;
typedef struct _comac_cache comac_cache_t;
typedef struct _comac_composite_rectangles comac_composite_rectangles_t;
typedef struct _comac_clip comac_clip_t;
typedef struct _comac_clip_path comac_clip_path_t;
typedef struct _comac_color comac_color_t;
typedef struct _comac_color_stop comac_color_stop_t;
typedef struct _comac_contour comac_contour_t;
typedef struct _comac_contour_chain comac_contour_chain_t;
typedef struct _comac_contour_iter comac_contour_iter_t;
typedef struct _comac_damage comac_damage_t;
typedef struct _comac_device_backend comac_device_backend_t;
typedef struct _comac_font_face_backend comac_font_face_backend_t;
typedef struct _comac_gstate comac_gstate_t;
typedef struct _comac_gstate_backend comac_gstate_backend_t;
typedef struct _comac_glyph_text_info comac_glyph_text_info_t;
typedef struct _comac_hash_entry comac_hash_entry_t;
typedef struct _comac_hash_table comac_hash_table_t;
typedef struct _comac_image_surface comac_image_surface_t;
typedef struct _comac_mime_data comac_mime_data_t;
typedef struct _comac_observer comac_observer_t;
typedef struct _comac_output_stream comac_output_stream_t;
typedef struct _comac_paginated_surface_backend
    comac_paginated_surface_backend_t;
typedef struct _comac_path_fixed comac_path_fixed_t;
typedef struct _comac_rectangle_int16 comac_glyph_size_t;
typedef struct _comac_scaled_font_subsets comac_scaled_font_subsets_t;
typedef struct _comac_solid_pattern comac_solid_pattern_t;
typedef struct _comac_surface_attributes comac_surface_attributes_t;
typedef struct _comac_surface_backend comac_surface_backend_t;
typedef struct _comac_surface_observer comac_surface_observer_t;
typedef struct _comac_surface_snapshot comac_surface_snapshot_t;
typedef struct _comac_surface_subsurface comac_surface_subsurface_t;
typedef struct _comac_surface_wrapper comac_surface_wrapper_t;
typedef struct _comac_traps comac_traps_t;
typedef struct _comac_tristrip comac_tristrip_t;
typedef struct _comac_unscaled_font_backend comac_unscaled_font_backend_t;
typedef struct _comac_xlib_screen_info comac_xlib_screen_info_t;

typedef comac_array_t comac_user_data_array_t;

typedef struct _comac_scaled_font_private comac_scaled_font_private_t;
typedef struct _comac_scaled_font_backend comac_scaled_font_backend_t;
typedef struct _comac_scaled_glyph comac_scaled_glyph_t;
typedef struct _comac_scaled_glyph_private comac_scaled_glyph_private_t;

typedef struct comac_compositor comac_compositor_t;
typedef struct comac_fallback_compositor comac_fallback_compositor_t;
typedef struct comac_mask_compositor comac_mask_compositor_t;
typedef struct comac_traps_compositor comac_traps_compositor_t;
typedef struct comac_spans_compositor comac_spans_compositor_t;

struct _comac_observer {
    comac_list_t link;
    void (*callback) (comac_observer_t *self, void *arg);
};

/**
 * _comac_hash_entry:
 *
 * A #comac_hash_entry_t contains both a key and a value for
 * #comac_hash_table_t. User-derived types for #comac_hash_entry_t must
 * be type-compatible with this structure (eg. they must have a
 * uintptr_t as the first parameter. The easiest way to get this
 * is to use:
 *
 * 	typedef _my_entry {
 *	    comac_hash_entry_t base;
 *	    ... Remainder of key and value fields here ..
 *	} my_entry_t;
 *
 * which then allows a pointer to my_entry_t to be passed to any of
 * the #comac_hash_table_t functions as follows without requiring a cast:
 *
 *	_comac_hash_table_insert (hash_table, &my_entry->base);
 *
 * IMPORTANT: The caller is responsible for initializing
 * my_entry->base.hash with a hash code derived from the key. The
 * essential property of the hash code is that keys_equal must never
 * return %TRUE for two keys that have different hashes. The best hash
 * code will reduce the frequency of two keys with the same code for
 * which keys_equal returns %FALSE.
 *
 * Which parts of the entry make up the "key" and which part make up
 * the value are entirely up to the caller, (as determined by the
 * computation going into base.hash as well as the keys_equal
 * function). A few of the #comac_hash_table_t functions accept an entry
 * which will be used exclusively as a "key", (indicated by a
 * parameter name of key). In these cases, the value-related fields of
 * the entry need not be initialized if so desired.
 **/
struct _comac_hash_entry {
    uintptr_t hash;
};

struct _comac_array {
    unsigned int size;
    unsigned int num_elements;
    unsigned int element_size;
    char *elements;
};

/**
 * _comac_lcd_filter:
 * @COMAC_LCD_FILTER_DEFAULT: Use the default LCD filter for
 *   font backend and target device
 * @COMAC_LCD_FILTER_NONE: Do not perform LCD filtering
 * @COMAC_LCD_FILTER_INTRA_PIXEL: Intra-pixel filter
 * @COMAC_LCD_FILTER_FIR3: FIR filter with a 3x3 kernel
 * @COMAC_LCD_FILTER_FIR5: FIR filter with a 5x5 kernel
 *
 * The LCD filter specifies the low-pass filter applied to LCD-optimized
 * bitmaps generated with an antialiasing mode of %COMAC_ANTIALIAS_SUBPIXEL.
 *
 * Note: This API was temporarily made available in the public
 * interface during the 1.7.x development series, but was made private
 * before 1.8.
 **/
typedef enum _comac_lcd_filter {
    COMAC_LCD_FILTER_DEFAULT,
    COMAC_LCD_FILTER_NONE,
    COMAC_LCD_FILTER_INTRA_PIXEL,
    COMAC_LCD_FILTER_FIR3,
    COMAC_LCD_FILTER_FIR5
} comac_lcd_filter_t;

typedef enum _comac_round_glyph_positions {
    COMAC_ROUND_GLYPH_POS_DEFAULT,
    COMAC_ROUND_GLYPH_POS_ON,
    COMAC_ROUND_GLYPH_POS_OFF
} comac_round_glyph_positions_t;

struct _comac_font_options {
    comac_antialias_t antialias;
    comac_subpixel_order_t subpixel_order;
    comac_lcd_filter_t lcd_filter;
    comac_hint_style_t hint_style;
    comac_hint_metrics_t hint_metrics;
    comac_round_glyph_positions_t round_glyph_positions;
    char *variations;
    comac_color_mode_t color_mode;
    unsigned int palette_index;
};

struct _comac_glyph_text_info {
    const char *utf8;
    int utf8_len;

    const comac_text_cluster_t *clusters;
    int num_clusters;
    comac_text_cluster_flags_t cluster_flags;
};

/* XXX: Right now, the _comac_color structure puts unpremultiplied
   color in the doubles and premultiplied color in the shorts. Yes,
   this is crazy insane, (but at least we don't export this
   madness). I'm still working on a cleaner API, but in the meantime,
   at least this does prevent precision loss in color when changing
   alpha. */
struct _comac_rgb_color {
    double red;
    double green;
    double blue;
    double alpha;

    unsigned short red_short;
    unsigned short green_short;
    unsigned short blue_short;
    unsigned short alpha_short;
};

struct _comac_fake_color {
    int do_not_use;
};

struct _comac_color {
    comac_colorspace_t colorspace;
    union {
	struct _comac_rgb_color rgb;
	struct _comac_fake_color do_not_use;
    } c;
};

struct _comac_color_stop {
    /* unpremultiplied */
    double red;
    double green;
    double blue;
    double alpha;

    /* unpremultipled, for convenience */
    uint16_t red_short;
    uint16_t green_short;
    uint16_t blue_short;
    uint16_t alpha_short;
};

typedef enum _comac_paginated_mode {
    COMAC_PAGINATED_MODE_ANALYZE, /* analyze page regions */
    COMAC_PAGINATED_MODE_RENDER,  /* render page contents */
    COMAC_PAGINATED_MODE_FALLBACK /* paint fallback images */
} comac_paginated_mode_t;

typedef enum _comac_internal_surface_type {
    COMAC_INTERNAL_SURFACE_TYPE_SNAPSHOT = 0x1000,
    COMAC_INTERNAL_SURFACE_TYPE_PAGINATED,
    COMAC_INTERNAL_SURFACE_TYPE_ANALYSIS,
    COMAC_INTERNAL_SURFACE_TYPE_OBSERVER,
    COMAC_INTERNAL_SURFACE_TYPE_TEST_FALLBACK,
    COMAC_INTERNAL_SURFACE_TYPE_TEST_PAGINATED,
    COMAC_INTERNAL_SURFACE_TYPE_TEST_WRAPPING,
    COMAC_INTERNAL_SURFACE_TYPE_NULL,
    COMAC_INTERNAL_SURFACE_TYPE_TYPE3_GLYPH,
    COMAC_INTERNAL_SURFACE_TYPE_QUARTZ_SNAPSHOT
} comac_internal_surface_type_t;

typedef enum _comac_internal_device_type {
    COMAC_INTERNAL_DEVICE_TYPE_OBSERVER = 0x1000,
} comac_device_surface_type_t;

#define COMAC_HAS_TEST_PAGINATED_SURFACE 1

typedef struct _comac_slope {
    comac_fixed_t dx;
    comac_fixed_t dy;
} comac_slope_t, comac_distance_t;

typedef struct _comac_point_double {
    double x;
    double y;
} comac_point_double_t;

typedef struct _comac_circle_double {
    comac_point_double_t center;
    double radius;
} comac_circle_double_t;

typedef struct _comac_distance_double {
    double dx;
    double dy;
} comac_distance_double_t;

typedef struct _comac_box_double {
    comac_point_double_t p1;
    comac_point_double_t p2;
} comac_box_double_t;

typedef struct _comac_line {
    comac_point_t p1;
    comac_point_t p2;
} comac_line_t, comac_box_t;

typedef struct _comac_trapezoid {
    comac_fixed_t top, bottom;
    comac_line_t left, right;
} comac_trapezoid_t;

typedef struct _comac_point_int {
    int x, y;
} comac_point_int_t;

#define COMAC_RECT_INT_MIN (INT_MIN >> COMAC_FIXED_FRAC_BITS)
#define COMAC_RECT_INT_MAX (INT_MAX >> COMAC_FIXED_FRAC_BITS)

typedef enum _comac_direction {
    COMAC_DIRECTION_FORWARD,
    COMAC_DIRECTION_REVERSE
} comac_direction_t;

typedef struct _comac_edge {
    comac_line_t line;
    int top, bottom;
    int dir;
} comac_edge_t;

typedef struct _comac_polygon {
    comac_status_t status;

    comac_box_t extents;
    comac_box_t limit;
    const comac_box_t *limits;
    int num_limits;

    int num_edges;
    int edges_size;
    comac_edge_t *edges;
    comac_edge_t edges_embedded[32];
} comac_polygon_t;

typedef comac_warn comac_status_t (*comac_spline_add_point_func_t) (
    void *closure, const comac_point_t *point, const comac_slope_t *tangent);

typedef struct _comac_spline_knots {
    comac_point_t a, b, c, d;
} comac_spline_knots_t;

typedef struct _comac_spline {
    comac_spline_add_point_func_t add_point_func;
    void *closure;

    comac_spline_knots_t knots;

    comac_slope_t initial_slope;
    comac_slope_t final_slope;

    comac_bool_t has_point;
    comac_point_t last_point;
} comac_spline_t;

typedef struct _comac_pen_vertex {
    comac_point_t point;

    comac_slope_t slope_ccw;
    comac_slope_t slope_cw;
} comac_pen_vertex_t;

typedef struct _comac_pen {
    double radius;
    double tolerance;

    int num_vertices;
    comac_pen_vertex_t *vertices;
    comac_pen_vertex_t vertices_embedded[32];
} comac_pen_t;

typedef struct _comac_stroke_style {
    double line_width;
    comac_line_cap_t line_cap;
    comac_line_join_t line_join;
    double miter_limit;
    double *dash;
    unsigned int num_dashes;
    double dash_offset;
    comac_bool_t is_hairline;
    double pre_hairline_line_width;
} comac_stroke_style_t;

typedef struct _comac_format_masks {
    int bpp;
    unsigned long alpha_mask;
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
} comac_format_masks_t;

typedef enum {
    COMAC_STOCK_WHITE,
    COMAC_STOCK_BLACK,
    COMAC_STOCK_TRANSPARENT,
    COMAC_STOCK_NUM_COLORS,
} comac_stock_t;

typedef enum _comac_image_transparency {
    COMAC_IMAGE_IS_OPAQUE,
    COMAC_IMAGE_HAS_BILEVEL_ALPHA,
    COMAC_IMAGE_HAS_ALPHA,
    COMAC_IMAGE_UNKNOWN
} comac_image_transparency_t;

typedef enum _comac_image_color {
    COMAC_IMAGE_IS_COLOR,
    COMAC_IMAGE_IS_GRAYSCALE,
    COMAC_IMAGE_IS_MONOCHROME,
    COMAC_IMAGE_UNKNOWN_COLOR
} comac_image_color_t;

struct _comac_mime_data {
    comac_reference_count_t ref_count;
    unsigned char *data;
    unsigned long length;
    comac_destroy_func_t destroy;
    void *closure;
};

/*
 * A #comac_unscaled_font_t is just an opaque handle we use in the
 * glyph cache.
 */
typedef struct _comac_unscaled_font {
    comac_hash_entry_t hash_entry;
    comac_reference_count_t ref_count;
    const comac_unscaled_font_backend_t *backend;
} comac_unscaled_font_t;
COMAC_END_DECLS

#endif /* COMAC_TYPES_PRIVATE_H */
