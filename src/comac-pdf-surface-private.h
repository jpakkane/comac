/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc
 * Copyright © 2006 Red Hat, Inc
 * Copyright © 2007, 2008 Adrian Johnson
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
 *	Kristian Høgsberg <krh@redhat.com>
 *	Carl Worth <cworth@cworth.org>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#ifndef COMAC_PDF_SURFACE_PRIVATE_H
#define COMAC_PDF_SURFACE_PRIVATE_H

#include "comac-pdf.h"

#include "comac-surface-private.h"
#include "comac-surface-clipper-private.h"
#include "comac-pdf-operators-private.h"
#include "comac-path-fixed-private.h"
#include "comac-tag-attributes-private.h"
#include "comac-tag-stack-private.h"

typedef struct _comac_pdf_resource {
    unsigned int id;
} comac_pdf_resource_t;

#define COMAC_NUM_OPERATORS (COMAC_OPERATOR_HSL_LUMINOSITY + 1)

typedef struct _comac_pdf_group_resources {
    comac_bool_t operators[COMAC_NUM_OPERATORS];
    comac_array_t alphas;
    comac_array_t smasks;
    comac_array_t patterns;
    comac_array_t shadings;
    comac_array_t xobjects;
    comac_array_t fonts;
} comac_pdf_group_resources_t;

typedef struct _comac_pdf_source_surface_entry {
    comac_hash_entry_t base;
    unsigned int id;
    unsigned char *unique_id;
    unsigned long unique_id_length;
    comac_operator_t operator;
    comac_bool_t interpolate;
    comac_bool_t stencil_mask;
    comac_bool_t smask;
    comac_bool_t need_transp_group;
    comac_pdf_resource_t surface_res;
    comac_pdf_resource_t smask_res;

    /* True if surface will be emitted as an Image XObject. */
    comac_bool_t emit_image;

    /* Extents of the source surface. */
    comac_bool_t bounded;
    comac_rectangle_int_t extents;

    /* Union of source extents required for all operations using this source */
    comac_rectangle_int_t required_extents;
} comac_pdf_source_surface_entry_t;

typedef struct _comac_pdf_source_surface {
    comac_pattern_type_t type;
    comac_surface_t *surface;
    comac_pattern_t *raster_pattern;
    comac_pdf_source_surface_entry_t *hash_entry;
} comac_pdf_source_surface_t;

typedef struct _comac_pdf_pattern {
    double width;
    double height;
    comac_rectangle_int_t extents;
    comac_pattern_t *pattern;
    comac_pdf_resource_t pattern_res;
    comac_pdf_resource_t gstate_res;
    comac_operator_t operator;
    comac_bool_t is_shading;

    /* PDF pattern space is the pattern matrix concatenated with the
     * initial space of the parent object. If the parent object is the
     * page, the initial space does not include the Y-axis flipping
     * matrix emitted at the start of the page content stream.  If the
     * parent object is not the page content stream, the initial space
     * will have a flipped Y-axis. The inverted_y_axis flag is true
     * when the initial space of the parent object that is drawing
     * this pattern has a flipped Y-axis.
     */
    comac_bool_t inverted_y_axis;
} comac_pdf_pattern_t;

typedef enum _comac_pdf_operation {
    PDF_PAINT,
    PDF_MASK,
    PDF_FILL,
    PDF_STROKE,
    PDF_SHOW_GLYPHS
} comac_pdf_operation_t;

typedef struct _comac_pdf_smask_group {
    double width;
    double height;
    comac_rectangle_int_t extents;
    comac_pdf_resource_t group_res;
    comac_pdf_operation_t operation;
    comac_pattern_t *source;
    comac_pdf_resource_t source_res;
    comac_pattern_t *mask;
    comac_path_fixed_t path;
    comac_fill_rule_t fill_rule;
    comac_stroke_style_t style;
    comac_matrix_t ctm;
    comac_matrix_t ctm_inverse;
    char *utf8;
    int utf8_len;
    comac_glyph_t *glyphs;
    int num_glyphs;
    comac_text_cluster_t *clusters;
    int num_clusters;
    comac_bool_t cluster_flags;
    comac_scaled_font_t *scaled_font;
} comac_pdf_smask_group_t;

typedef struct _comac_pdf_jbig2_global {
    unsigned char *id;
    unsigned long id_length;
    comac_pdf_resource_t res;
    comac_bool_t emitted;
} comac_pdf_jbig2_global_t;

/* comac-pdf-interchange.c types */

struct page_mcid {
    int page;
    int mcid;
};

struct tag_extents {
    comac_rectangle_int_t extents;
    comac_bool_t valid;
    comac_list_t link;
};

typedef struct _comac_pdf_struct_tree_node {
    char *name;
    comac_pdf_resource_t res;
    struct _comac_pdf_struct_tree_node *parent;
    comac_list_t children;
    comac_array_t mcid;		    /* array of struct page_mcid */
    comac_pdf_resource_t annot_res; /* 0 if no annot */
    struct tag_extents extents;
    comac_list_t link;
} comac_pdf_struct_tree_node_t;

typedef struct _comac_pdf_annotation {
    comac_pdf_struct_tree_node_t *node; /* node containing the annotation */
    comac_link_attrs_t link_attrs;
} comac_pdf_annotation_t;

typedef struct _comac_pdf_named_dest {
    comac_hash_entry_t base;
    struct tag_extents extents;
    comac_dest_attrs_t attrs;
    int page;
} comac_pdf_named_dest_t;

typedef struct _comac_pdf_outline_entry {
    char *name;
    comac_link_attrs_t link_attrs;
    comac_pdf_outline_flags_t flags;
    comac_pdf_resource_t res;
    struct _comac_pdf_outline_entry *parent;
    struct _comac_pdf_outline_entry *first_child;
    struct _comac_pdf_outline_entry *last_child;
    struct _comac_pdf_outline_entry *next;
    struct _comac_pdf_outline_entry *prev;
    int count;
} comac_pdf_outline_entry_t;

typedef struct _comac_pdf_forward_link {
    comac_pdf_resource_t res;
    char *dest;
    int page;
    comac_bool_t has_pos;
    comac_point_double_t pos;
} comac_pdf_forward_link_t;

struct docinfo {
    char *title;
    char *author;
    char *subject;
    char *keywords;
    char *creator;
    char *create_date;
    char *mod_date;
};

struct metadata {
    char *name;
    char *value;
};

typedef struct _comac_pdf_interchange {
    comac_tag_stack_t analysis_tag_stack;
    comac_tag_stack_t render_tag_stack;
    comac_array_t
	push_data; /* records analysis_tag_stack data field for each push */
    int push_data_index;
    comac_pdf_struct_tree_node_t *struct_root;
    comac_pdf_struct_tree_node_t *current_node;
    comac_pdf_struct_tree_node_t *begin_page_node;
    comac_pdf_struct_tree_node_t *end_page_node;
    comac_array_t parent_tree;	/* parent tree resources */
    comac_array_t mcid_to_tree; /* mcid to tree node mapping for current page */
    comac_array_t annots; /* array of pointers to comac_pdf_annotation_t */
    comac_pdf_resource_t parent_tree_res;
    comac_list_t extents_list;
    comac_hash_table_t *named_dests;
    int num_dests;
    comac_pdf_named_dest_t **sorted_dests;
    comac_pdf_resource_t dests_res;
    int annot_page;
    comac_array_t outline; /* array of pointers to comac_pdf_outline_entry_t; */
    struct docinfo docinfo;
    comac_array_t custom_metadata; /* array of struct metadata */

} comac_pdf_interchange_t;

/* pdf surface data */

typedef struct _comac_pdf_surface comac_pdf_surface_t;

struct _comac_pdf_surface {
    comac_surface_t base;

    /* Prefer the name "output" here to avoid confusion over the
     * structure within a PDF document known as a "stream". */
    comac_output_stream_t *output;

    double width;
    double height;
    comac_rectangle_int_t surface_extents;
    comac_bool_t surface_bounded;
    comac_matrix_t comac_to_pdf;
    comac_bool_t in_xobject;

    comac_array_t objects;
    comac_array_t pages;
    comac_array_t rgb_linear_functions;
    comac_array_t alpha_linear_functions;
    comac_array_t page_patterns;
    comac_array_t page_surfaces;
    comac_array_t doc_surfaces;
    comac_hash_table_t *all_surfaces;
    comac_array_t smask_groups;
    comac_array_t knockout_group;
    comac_array_t jbig2_global;
    comac_array_t page_heights;

    comac_scaled_font_subsets_t *font_subsets;
    comac_array_t fonts;

    comac_pdf_resource_t next_available_resource;
    comac_pdf_resource_t pages_resource;
    comac_pdf_resource_t struct_tree_root;

    comac_pdf_version_t pdf_version;
    comac_bool_t compress_streams;

    comac_pdf_resource_t content;
    comac_pdf_resource_t content_resources;
    comac_pdf_group_resources_t resources;
    comac_bool_t has_fallback_images;
    comac_bool_t header_emitted;

    struct {
	comac_bool_t active;
	comac_pdf_resource_t self;
	comac_pdf_resource_t length;
	long long start_offset;
	comac_bool_t compressed;
	comac_output_stream_t *old_output;
    } pdf_stream;

    struct {
	comac_bool_t active;
	comac_output_stream_t *stream;
	comac_output_stream_t *mem_stream;
	comac_output_stream_t *old_output;
	comac_pdf_resource_t resource;
	comac_box_double_t bbox;
	comac_bool_t is_knockout;
    } group_stream;

    struct {
	comac_bool_t active;
	comac_output_stream_t *stream;
	comac_pdf_resource_t resource;
	comac_array_t objects;
    } object_stream;

    comac_surface_clipper_t clipper;

    comac_pdf_operators_t pdf_operators;
    comac_paginated_mode_t paginated_mode;
    comac_bool_t select_pattern_gstate_saved;

    comac_bool_t force_fallbacks;

    comac_operator_t current_operator;
    comac_bool_t current_pattern_is_solid_color;
    comac_bool_t current_color_is_stroke;
    double current_color_red;
    double current_color_green;
    double current_color_blue;
    double current_color_alpha;

    comac_pdf_interchange_t interchange;
    int page_parent_tree; /* -1 if not used */
    comac_array_t page_annots;
    comac_array_t forward_links;
    comac_bool_t tagged;
    char *current_page_label;
    comac_array_t page_labels;
    comac_pdf_resource_t outlines_dict_res;
    comac_pdf_resource_t names_dict_res;
    comac_pdf_resource_t docinfo_res;
    comac_pdf_resource_t page_labels_res;

    int thumbnail_width;
    int thumbnail_height;
    comac_image_surface_t *thumbnail_image;

    comac_surface_t *paginated_surface;
};

comac_private comac_pdf_resource_t
_comac_pdf_surface_new_object (comac_pdf_surface_t *surface);

comac_private void
_comac_pdf_surface_update_object (comac_pdf_surface_t *surface,
				  comac_pdf_resource_t resource);

comac_private comac_int_status_t
_comac_utf8_to_pdf_string (const char *utf8, char **str_out);

comac_private comac_int_status_t
_comac_pdf_interchange_init (comac_pdf_surface_t *surface);

comac_private void
_comac_pdf_interchange_fini (comac_pdf_surface_t *surface);

comac_private comac_int_status_t
_comac_pdf_interchange_begin_page_content (comac_pdf_surface_t *surface);

comac_private comac_int_status_t
_comac_pdf_interchange_end_page_content (comac_pdf_surface_t *surface);

comac_private comac_int_status_t
_comac_pdf_interchange_tag_begin (comac_pdf_surface_t *surface,
				  const char *name,
				  const char *attributes);

comac_private comac_int_status_t
_comac_pdf_surface_object_begin (comac_pdf_surface_t *surface,
				 comac_pdf_resource_t resource);

comac_private void
_comac_pdf_surface_object_end (comac_pdf_surface_t *surface);

comac_private comac_int_status_t
_comac_pdf_interchange_tag_end (comac_pdf_surface_t *surface, const char *name);

comac_private comac_int_status_t
_comac_pdf_interchange_add_operation_extents (
    comac_pdf_surface_t *surface, const comac_rectangle_int_t *extents);

comac_private comac_int_status_t
_comac_pdf_interchange_write_page_objects (comac_pdf_surface_t *surface);

comac_private comac_int_status_t
_comac_pdf_interchange_write_document_objects (comac_pdf_surface_t *surface);

comac_private comac_int_status_t
_comac_pdf_interchange_add_outline (comac_pdf_surface_t *surface,
				    int parent_id,
				    const char *name,
				    const char *dest,
				    comac_pdf_outline_flags_t flags,
				    int *id);

comac_private comac_int_status_t
_comac_pdf_interchange_set_metadata (comac_pdf_surface_t *surface,
				     comac_pdf_metadata_t metadata,
				     const char *utf8);

comac_private comac_int_status_t
_comac_pdf_interchange_set_custom_metadata (comac_pdf_surface_t *surface,
					    const char *name,
					    const char *value);

#endif /* COMAC_PDF_SURFACE_PRIVATE_H */
