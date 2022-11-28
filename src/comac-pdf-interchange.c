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

/* PDF Document Interchange features:
 *  - metadata
 *  - document outline
 *  - tagged pdf
 *  - hyperlinks
 *  - page labels
 */

#define _DEFAULT_SOURCE /* for localtime_r(), gmtime_r(), snprintf(), strdup() */
#include "comacint.h"

#include "comac-pdf.h"
#include "comac-pdf-surface-private.h"

#include "comac-array-private.h"
#include "comac-error-private.h"
#include "comac-output-stream-private.h"

#include <time.h>

#ifndef HAVE_LOCALTIME_R
#define localtime_r(T, BUF) (*(BUF) = *localtime (T))
#endif
#ifndef HAVE_GMTIME_R
#define gmtime_r(T, BUF) (*(BUF) = *gmtime (T))
#endif

static void
write_rect_to_pdf_quad_points (comac_output_stream_t *stream,
			       const comac_rectangle_t *rect,
			       double surface_height)
{
    _comac_output_stream_printf (stream,
				 "%f %f %f %f %f %f %f %f",
				 rect->x,
				 surface_height - rect->y,
				 rect->x + rect->width,
				 surface_height - rect->y,
				 rect->x + rect->width,
				 surface_height - (rect->y + rect->height),
				 rect->x,
				 surface_height - (rect->y + rect->height));
}

static void
write_rect_int_to_pdf_bbox (comac_output_stream_t *stream,
			    const comac_rectangle_int_t *rect,
			    double surface_height)
{
    _comac_output_stream_printf (stream,
				 "%d %f %d %f",
				 rect->x,
				 surface_height - (rect->y + rect->height),
				 rect->x + rect->width,
				 surface_height - rect->y);
}

static comac_int_status_t
add_tree_node (comac_pdf_surface_t *surface,
	       comac_pdf_struct_tree_node_t *parent,
	       const char *name,
	       comac_pdf_struct_tree_node_t **new_node)
{
    comac_pdf_struct_tree_node_t *node;

    node = _comac_malloc (sizeof (comac_pdf_struct_tree_node_t));
    if (unlikely (node == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    node->name = strdup (name);
    node->res = _comac_pdf_surface_new_object (surface);
    if (node->res.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    node->parent = parent;
    comac_list_init (&node->children);
    _comac_array_init (&node->mcid, sizeof (struct page_mcid));
    node->annot_res.id = 0;
    node->extents.valid = FALSE;
    comac_list_init (&node->extents.link);

    comac_list_add_tail (&node->link, &parent->children);

    *new_node = node;
    return COMAC_STATUS_SUCCESS;
}

static comac_bool_t
is_leaf_node (comac_pdf_struct_tree_node_t *node)
{
    return node->parent && comac_list_is_empty (&node->children);
}

static void
free_node (comac_pdf_struct_tree_node_t *node)
{
    comac_pdf_struct_tree_node_t *child, *next;

    if (! node)
	return;

    comac_list_foreach_entry_safe (child,
				   next,
				   comac_pdf_struct_tree_node_t,
				   &node->children,
				   link)
    {
	comac_list_del (&child->link);
	free_node (child);
    }
    free (node->name);
    _comac_array_fini (&node->mcid);
    free (node);
}

static comac_status_t
add_mcid_to_node (comac_pdf_surface_t *surface,
		  comac_pdf_struct_tree_node_t *node,
		  int page,
		  int *mcid)
{
    struct page_mcid mcid_elem;
    comac_int_status_t status;
    comac_pdf_interchange_t *ic = &surface->interchange;

    status = _comac_array_append (&ic->mcid_to_tree, &node);
    if (unlikely (status))
	return status;

    mcid_elem.page = page;
    mcid_elem.mcid = _comac_array_num_elements (&ic->mcid_to_tree) - 1;
    *mcid = mcid_elem.mcid;
    return _comac_array_append (&node->mcid, &mcid_elem);
}

static comac_int_status_t
add_annotation (comac_pdf_surface_t *surface,
		comac_pdf_struct_tree_node_t *node,
		const char *name,
		const char *attributes)
{
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_pdf_annotation_t *annot;

    annot = malloc (sizeof (comac_pdf_annotation_t));
    if (unlikely (annot == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = _comac_tag_parse_link_attributes (attributes, &annot->link_attrs);
    if (unlikely (status)) {
	free (annot);
	return status;
    }

    annot->node = node;

    status = _comac_array_append (&ic->annots, &annot);

    return status;
}

static void
free_annotation (comac_pdf_annotation_t *annot)
{
    _comac_array_fini (&annot->link_attrs.rects);
    free (annot->link_attrs.dest);
    free (annot->link_attrs.uri);
    free (annot->link_attrs.file);
    free (annot);
}

static void
comac_pdf_interchange_clear_annotations (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    int num_elems, i;

    num_elems = _comac_array_num_elements (&ic->annots);
    for (i = 0; i < num_elems; i++) {
	comac_pdf_annotation_t *annot;

	_comac_array_copy_element (&ic->annots, i, &annot);
	free_annotation (annot);
    }
    _comac_array_truncate (&ic->annots, 0);
}

static comac_int_status_t
comac_pdf_interchange_write_node_object (comac_pdf_surface_t *surface,
					 comac_pdf_struct_tree_node_t *node)
{
    struct page_mcid *mcid_elem;
    int i, num_mcid, first_page;
    comac_pdf_resource_t *page_res;
    comac_pdf_struct_tree_node_t *child;
    comac_int_status_t status;

    status = _comac_pdf_surface_object_begin (surface, node->res);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (surface->object_stream.stream,
				 "<< /Type /StructElem\n"
				 "   /S /%s\n"
				 "   /P %d 0 R\n",
				 node->name,
				 node->parent->res.id);

    if (! comac_list_is_empty (&node->children)) {
	if (comac_list_is_singular (&node->children) &&
	    node->annot_res.id == 0) {
	    child = comac_list_first_entry (&node->children,
					    comac_pdf_struct_tree_node_t,
					    link);
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /K %d 0 R\n",
					 child->res.id);
	} else {
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /K [ ");
	    if (node->annot_res.id != 0) {
		_comac_output_stream_printf (surface->object_stream.stream,
					     "<< /Type /OBJR /Obj %d 0 R >> ",
					     node->annot_res.id);
	    }
	    comac_list_foreach_entry (child,
				      comac_pdf_struct_tree_node_t,
				      &node->children,
				      link)
	    {
		_comac_output_stream_printf (surface->object_stream.stream,
					     "%d 0 R ",
					     child->res.id);
	    }
	    _comac_output_stream_printf (surface->object_stream.stream, "]\n");
	}
    } else {
	num_mcid = _comac_array_num_elements (&node->mcid);
	if (num_mcid > 0) {
	    mcid_elem = _comac_array_index (&node->mcid, 0);
	    first_page = mcid_elem->page;
	    page_res = _comac_array_index (&surface->pages, first_page - 1);
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /Pg %d 0 R\n",
					 page_res->id);

	    if (num_mcid == 1 && node->annot_res.id == 0) {
		_comac_output_stream_printf (surface->object_stream.stream,
					     "   /K %d\n",
					     mcid_elem->mcid);
	    } else {
		_comac_output_stream_printf (surface->object_stream.stream,
					     "   /K [ ");
		if (node->annot_res.id != 0) {
		    _comac_output_stream_printf (
			surface->object_stream.stream,
			"<< /Type /OBJR /Obj %d 0 R >> ",
			node->annot_res.id);
		}
		for (i = 0; i < num_mcid; i++) {
		    mcid_elem = _comac_array_index (&node->mcid, i);
		    page_res = _comac_array_index (&surface->pages,
						   mcid_elem->page - 1);
		    if (mcid_elem->page == first_page) {
			_comac_output_stream_printf (
			    surface->object_stream.stream,
			    "%d ",
			    mcid_elem->mcid);
		    } else {
			_comac_output_stream_printf (
			    surface->object_stream.stream,
			    "\n       << /Type /MCR /Pg %d 0 R /MCID %d >> ",
			    page_res->id,
			    mcid_elem->mcid);
		    }
		}
		_comac_output_stream_printf (surface->object_stream.stream,
					     "]\n");
	    }
	}
    }
    _comac_output_stream_printf (surface->object_stream.stream, ">>\n");

    _comac_pdf_surface_object_end (surface);

    return _comac_output_stream_get_status (surface->object_stream.stream);
}

static void
init_named_dest_key (comac_pdf_named_dest_t *dest)
{
    dest->base.hash = _comac_hash_bytes (_COMAC_HASH_INIT_VALUE,
					 dest->attrs.name,
					 strlen (dest->attrs.name));
}

static comac_bool_t
_named_dest_equal (const void *key_a, const void *key_b)
{
    const comac_pdf_named_dest_t *a = key_a;
    const comac_pdf_named_dest_t *b = key_b;

    return strcmp (a->attrs.name, b->attrs.name) == 0;
}

static void
_named_dest_pluck (void *entry, void *closure)
{
    comac_pdf_named_dest_t *dest = entry;
    comac_hash_table_t *table = closure;

    _comac_hash_table_remove (table, &dest->base);
    free (dest->attrs.name);
    free (dest);
}

static comac_int_status_t
comac_pdf_interchange_write_explicit_dest (comac_pdf_surface_t *surface,
					   int page,
					   comac_bool_t has_pos,
					   double x,
					   double y)
{
    comac_pdf_resource_t res;
    double height;

    _comac_array_copy_element (&surface->page_heights, page - 1, &height);
    _comac_array_copy_element (&surface->pages, page - 1, &res);
    if (has_pos) {
	_comac_output_stream_printf (surface->object_stream.stream,
				     "[%d 0 R /XYZ %f %f 0]\n",
				     res.id,
				     x,
				     height - y);
    } else {
	_comac_output_stream_printf (surface->object_stream.stream,
				     "[%d 0 R /XYZ null null 0]\n",
				     res.id);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_dest (comac_pdf_surface_t *surface,
				  comac_link_attrs_t *link_attrs)
{
    comac_int_status_t status;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_pdf_forward_link_t *link;
    comac_pdf_resource_t link_res;

    /* If the dest is known, emit an explicit dest */
    if (link_attrs->dest) {
	comac_pdf_named_dest_t key;
	comac_pdf_named_dest_t *named_dest;

	/* check if we already have this dest */
	key.attrs.name = link_attrs->dest;
	init_named_dest_key (&key);
	named_dest = _comac_hash_table_lookup (ic->named_dests, &key.base);
	if (named_dest) {
	    double x = 0;
	    double y = 0;

	    if (named_dest->extents.valid) {
		x = named_dest->extents.extents.x;
		y = named_dest->extents.extents.y;
	    }

	    if (named_dest->attrs.x_valid)
		x = named_dest->attrs.x;

	    if (named_dest->attrs.y_valid)
		y = named_dest->attrs.y;

	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /Dest ");
	    status =
		comac_pdf_interchange_write_explicit_dest (surface,
							   named_dest->page,
							   TRUE,
							   x,
							   y);
	    return status;
	}
    }

    /* If the page is known, emit an explicit dest */
    if (! link_attrs->dest) {
	if (link_attrs->page < 1)
	    return _comac_tag_error (
		"Link attribute: \"page=%d\" page must be >= 1",
		link_attrs->page);

	if (link_attrs->page <=
	    (int) _comac_array_num_elements (&surface->pages)) {
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /Dest ");
	    return comac_pdf_interchange_write_explicit_dest (
		surface,
		link_attrs->page,
		link_attrs->has_pos,
		link_attrs->pos.x,
		link_attrs->pos.y);
	}
    }

    /* Link refers to a future or unknown page. Use an indirect object
     * and write the link at the end of the document */

    link = _comac_malloc (sizeof (comac_pdf_forward_link_t));
    if (unlikely (link == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    link_res = _comac_pdf_surface_new_object (surface);
    if (link_res.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    _comac_output_stream_printf (surface->object_stream.stream,
				 "   /Dest %d 0 R\n",
				 link_res.id);

    link->res = link_res;
    link->dest = link_attrs->dest ? strdup (link_attrs->dest) : NULL;
    link->page = link_attrs->page;
    link->has_pos = link_attrs->has_pos;
    link->pos = link_attrs->pos;
    status = _comac_array_append (&surface->forward_links, link);

    return status;
}

static comac_int_status_t
_comac_utf8_to_pdf_utf8_hexstring (const char *utf8, char **str_out)
{
    int i;
    int len;
    unsigned char *p;
    comac_bool_t ascii;
    char *str;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    ascii = TRUE;
    p = (unsigned char *) utf8;
    len = 0;
    while (*p) {
	if (*p < 32 || *p > 126) {
	    ascii = FALSE;
	}
	if (*p == '(' || *p == ')' || *p == '\\')
	    len += 2;
	else
	    len++;
	p++;
    }

    if (ascii) {
	str = _comac_malloc (len + 3);
	if (str == NULL)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	str[0] = '(';
	p = (unsigned char *) utf8;
	i = 1;
	while (*p) {
	    if (*p == '(' || *p == ')' || *p == '\\')
		str[i++] = '\\';
	    str[i++] = *p;
	    p++;
	}
	str[i++] = ')';
	str[i++] = 0;
    } else {
	str = _comac_malloc (len * 2 + 3);
	if (str == NULL)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	str[0] = '<';
	p = (unsigned char *) utf8;
	i = 1;
	while (*p) {
	    if (*p == '\\') {
		snprintf (str + i, 3, "%02x", '\\');
		i += 2;
	    }
	    snprintf (str + i, 3, "%02x", *p);
	    i += 2;
	    p++;
	}
	str[i++] = '>';
	str[i++] = 0;
    }
    *str_out = str;

    return status;
}

static comac_int_status_t
comac_pdf_interchange_write_link_action (comac_pdf_surface_t *surface,
					 comac_link_attrs_t *link_attrs)
{
    comac_int_status_t status;
    char *dest = NULL;

    if (link_attrs->link_type == TAG_LINK_DEST) {
	status = comac_pdf_interchange_write_dest (surface, link_attrs);
	if (unlikely (status))
	    return status;

    } else if (link_attrs->link_type == TAG_LINK_URI) {
	status = _comac_utf8_to_pdf_string (link_attrs->uri, &dest);
	if (unlikely (status))
	    return status;

	if (dest[0] != '(') {
	    free (dest);
	    return _comac_tag_error ("Link attribute: \"url=%s\" URI may only "
				     "contain ASCII characters",
				     link_attrs->uri);
	}

	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /A <<\n"
				     "      /Type /Action\n"
				     "      /S /URI\n"
				     "      /URI %s\n"
				     "   >>\n",
				     dest);
	free (dest);
    } else if (link_attrs->link_type == TAG_LINK_FILE) {
	/* According to "Developing with PDF", Leonard Rosenthol, 2013,
	 * The F key is encoded in the "standard encoding for the
	 * platform on which the document is being viewed. For most
	 * modern operating systems, that's UTF-8"
	 *
	 * As we don't know the target platform, we assume UTF-8. The
	 * F key may contain multi-byte encodings using the hex
	 * encoding.
	 *
	 * For PDF 1.7 we also include the UF key which uses the
	 * standard PDF UTF-16BE strings.
	 */
	status = _comac_utf8_to_pdf_utf8_hexstring (link_attrs->file, &dest);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /A <<\n"
				     "      /Type /Action\n"
				     "      /S /GoToR\n"
				     "      /F %s\n",
				     dest);
	free (dest);

	if (surface->pdf_version >= COMAC_PDF_VERSION_1_7) {
	    status = _comac_utf8_to_pdf_string (link_attrs->file, &dest);
	    if (unlikely (status))
		return status;

	    _comac_output_stream_printf (surface->object_stream.stream,
					 "      /UF %s\n",
					 dest);
	    free (dest);
	}

	if (link_attrs->dest) {
	    status = _comac_utf8_to_pdf_string (link_attrs->dest, &dest);
	    if (unlikely (status))
		return status;

	    _comac_output_stream_printf (surface->object_stream.stream,
					 "      /D %s\n",
					 dest);
	    free (dest);
	} else {
	    if (link_attrs->has_pos) {
		_comac_output_stream_printf (surface->object_stream.stream,
					     "      /D [%d /XYZ %f %f 0]\n",
					     link_attrs->page,
					     link_attrs->pos.x,
					     link_attrs->pos.y);
	    } else {
		_comac_output_stream_printf (surface->object_stream.stream,
					     "      /D [%d /XYZ null null 0]\n",
					     link_attrs->page);
	    }
	}
	_comac_output_stream_printf (surface->object_stream.stream, "   >>\n");
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_annot (comac_pdf_surface_t *surface,
				   comac_pdf_annotation_t *annot)
{
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_pdf_struct_tree_node_t *node = annot->node;
    int sp;
    int i, num_rects;
    double height;

    num_rects = _comac_array_num_elements (&annot->link_attrs.rects);
    if (strcmp (node->name, COMAC_TAG_LINK) == 0 &&
	annot->link_attrs.link_type != TAG_LINK_EMPTY &&
	(node->extents.valid || num_rects > 0)) {
	status = _comac_array_append (&ic->parent_tree, &node->res);
	if (unlikely (status))
	    return status;

	sp = _comac_array_num_elements (&ic->parent_tree) - 1;

	node->annot_res = _comac_pdf_surface_new_object (surface);
	if (node->annot_res.id == 0)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	status = _comac_array_append (&surface->page_annots, &node->annot_res);
	if (unlikely (status))
	    return status;

	status = _comac_pdf_surface_object_begin (surface, node->annot_res);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream,
				     "<< /Type /Annot\n"
				     "   /Subtype /Link\n"
				     "   /StructParent %d\n",
				     sp);

	height = surface->height;
	if (num_rects > 0) {
	    comac_rectangle_int_t bbox_rect;

	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /QuadPoints [ ");
	    for (i = 0; i < num_rects; i++) {
		comac_rectangle_t rectf;
		comac_rectangle_int_t recti;

		_comac_array_copy_element (&annot->link_attrs.rects, i, &rectf);
		_comac_rectangle_int_from_double (&recti, &rectf);
		if (i == 0)
		    bbox_rect = recti;
		else
		    _comac_rectangle_union (&bbox_rect, &recti);

		write_rect_to_pdf_quad_points (surface->object_stream.stream,
					       &rectf,
					       height);
		_comac_output_stream_printf (surface->object_stream.stream,
					     " ");
	    }
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "]\n"
					 "   /Rect [ ");
	    write_rect_int_to_pdf_bbox (surface->object_stream.stream,
					&bbox_rect,
					height);
	    _comac_output_stream_printf (surface->object_stream.stream, " ]\n");
	} else {
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /Rect [ ");
	    write_rect_int_to_pdf_bbox (surface->object_stream.stream,
					&node->extents.extents,
					height);
	    _comac_output_stream_printf (surface->object_stream.stream, " ]\n");
	}

	status = comac_pdf_interchange_write_link_action (surface,
							  &annot->link_attrs);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /BS << /W 0 >>"
				     ">>\n");

	_comac_pdf_surface_object_end (surface);
	status =
	    _comac_output_stream_get_status (surface->object_stream.stream);
    }

    return status;
}

static comac_int_status_t
comac_pdf_interchange_walk_struct_tree (
    comac_pdf_surface_t *surface,
    comac_pdf_struct_tree_node_t *node,
    comac_int_status_t (*func) (comac_pdf_surface_t *surface,
				comac_pdf_struct_tree_node_t *node))
{
    comac_int_status_t status;
    comac_pdf_struct_tree_node_t *child;

    if (node->parent) {
	status = func (surface, node);
	if (unlikely (status))
	    return status;
    }

    comac_list_foreach_entry (child,
			      comac_pdf_struct_tree_node_t,
			      &node->children,
			      link)
    {
	status = comac_pdf_interchange_walk_struct_tree (surface, child, func);
	if (unlikely (status))
	    return status;
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_struct_tree (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_pdf_struct_tree_node_t *child;
    comac_int_status_t status;

    if (comac_list_is_empty (&ic->struct_root->children))
	return COMAC_STATUS_SUCCESS;

    surface->struct_tree_root = _comac_pdf_surface_new_object (surface);
    if (surface->struct_tree_root.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    ic->struct_root->res = surface->struct_tree_root;

    comac_pdf_interchange_walk_struct_tree (
	surface,
	ic->struct_root,
	comac_pdf_interchange_write_node_object);

    child = comac_list_first_entry (&ic->struct_root->children,
				    comac_pdf_struct_tree_node_t,
				    link);

    status =
	_comac_pdf_surface_object_begin (surface, surface->struct_tree_root);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (surface->object_stream.stream,
				 "<< /Type /StructTreeRoot\n"
				 "   /ParentTree %d 0 R\n",
				 ic->parent_tree_res.id);

    if (comac_list_is_singular (&ic->struct_root->children)) {
	child = comac_list_first_entry (&ic->struct_root->children,
					comac_pdf_struct_tree_node_t,
					link);
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /K [ %d 0 R ]\n",
				     child->res.id);
    } else {
	_comac_output_stream_printf (surface->object_stream.stream, "   /K [ ");

	comac_list_foreach_entry (child,
				  comac_pdf_struct_tree_node_t,
				  &ic->struct_root->children,
				  link)
	{
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "%d 0 R ",
					 child->res.id);
	}
	_comac_output_stream_printf (surface->object_stream.stream, "]\n");
    }

    _comac_output_stream_printf (surface->object_stream.stream, ">>\n");
    _comac_pdf_surface_object_end (surface);

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_page_annots (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    int num_elems, i;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    num_elems = _comac_array_num_elements (&ic->annots);
    for (i = 0; i < num_elems; i++) {
	comac_pdf_annotation_t *annot;

	_comac_array_copy_element (&ic->annots, i, &annot);
	status = comac_pdf_interchange_write_annot (surface, annot);
	if (unlikely (status))
	    return status;
    }

    return status;
}

static comac_int_status_t
comac_pdf_interchange_write_page_parent_elems (comac_pdf_surface_t *surface)
{
    int num_elems, i;
    comac_pdf_struct_tree_node_t *node;
    comac_pdf_resource_t res;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    surface->page_parent_tree = -1;
    num_elems = _comac_array_num_elements (&ic->mcid_to_tree);
    if (num_elems > 0) {
	res = _comac_pdf_surface_new_object (surface);
	if (res.id == 0)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	status = _comac_pdf_surface_object_begin (surface, res);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream, "[\n");
	for (i = 0; i < num_elems; i++) {
	    _comac_array_copy_element (&ic->mcid_to_tree, i, &node);
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "  %d 0 R\n",
					 node->res.id);
	}
	_comac_output_stream_printf (surface->object_stream.stream, "]\n");
	_comac_pdf_surface_object_end (surface);

	status = _comac_array_append (&ic->parent_tree, &res);
	surface->page_parent_tree =
	    _comac_array_num_elements (&ic->parent_tree) - 1;
    }

    return status;
}

static comac_int_status_t
comac_pdf_interchange_write_parent_tree (comac_pdf_surface_t *surface)
{
    int num_elems, i;
    comac_pdf_resource_t *res;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status;

    num_elems = _comac_array_num_elements (&ic->parent_tree);
    if (num_elems > 0) {
	ic->parent_tree_res = _comac_pdf_surface_new_object (surface);
	if (ic->parent_tree_res.id == 0)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	status = _comac_pdf_surface_object_begin (surface, ic->parent_tree_res);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream,
				     "<< /Nums [\n");
	for (i = 0; i < num_elems; i++) {
	    res = _comac_array_index (&ic->parent_tree, i);
	    if (res->id) {
		_comac_output_stream_printf (surface->object_stream.stream,
					     "   %d %d 0 R\n",
					     i,
					     res->id);
	    }
	}
	_comac_output_stream_printf (surface->object_stream.stream,
				     "  ]\n"
				     ">>\n");
	_comac_pdf_surface_object_end (surface);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_outline (comac_pdf_surface_t *surface)
{
    int num_elems, i;
    comac_pdf_outline_entry_t *outline;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status;
    char *name = NULL;

    num_elems = _comac_array_num_elements (&ic->outline);
    if (num_elems < 2)
	return COMAC_INT_STATUS_SUCCESS;

    _comac_array_copy_element (&ic->outline, 0, &outline);
    outline->res = _comac_pdf_surface_new_object (surface);
    if (outline->res.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    surface->outlines_dict_res = outline->res;
    status = _comac_pdf_surface_object_begin (surface, outline->res);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (surface->object_stream.stream,
				 "<< /Type /Outlines\n"
				 "   /First %d 0 R\n"
				 "   /Last %d 0 R\n"
				 "   /Count %d\n"
				 ">>\n",
				 outline->first_child->res.id,
				 outline->last_child->res.id,
				 outline->count);
    _comac_pdf_surface_object_end (surface);

    for (i = 1; i < num_elems; i++) {
	_comac_array_copy_element (&ic->outline, i, &outline);
	_comac_pdf_surface_update_object (surface, outline->res);

	status = _comac_utf8_to_pdf_string (outline->name, &name);
	if (unlikely (status))
	    return status;

	status = _comac_pdf_surface_object_begin (surface, outline->res);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream,
				     "<< /Title %s\n"
				     "   /Parent %d 0 R\n",
				     name,
				     outline->parent->res.id);
	free (name);

	if (outline->prev) {
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /Prev %d 0 R\n",
					 outline->prev->res.id);
	}

	if (outline->next) {
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /Next %d 0 R\n",
					 outline->next->res.id);
	}

	if (outline->first_child) {
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /First %d 0 R\n"
					 "   /Last %d 0 R\n"
					 "   /Count %d\n",
					 outline->first_child->res.id,
					 outline->last_child->res.id,
					 outline->count);
	}

	if (outline->flags) {
	    int flags = 0;
	    if (outline->flags & COMAC_PDF_OUTLINE_FLAG_ITALIC)
		flags |= 1;
	    if (outline->flags & COMAC_PDF_OUTLINE_FLAG_BOLD)
		flags |= 2;
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   /F %d\n",
					 flags);
	}

	status = comac_pdf_interchange_write_link_action (surface,
							  &outline->link_attrs);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream, ">>\n");
	_comac_pdf_surface_object_end (surface);
    }

    return status;
}

/*
 * Split a page label into a text prefix and numeric suffix. Leading '0's are
 * included in the prefix. eg
 *  "3"     => NULL,    3
 *  "cover" => "cover", 0
 *  "A-2"   => "A-",    2
 *  "A-002" => "A-00",  2
 */
static char *
split_label (const char *label, int *num)
{
    int len, i;

    *num = 0;
    len = strlen (label);
    if (len == 0)
	return NULL;

    i = len;
    while (i > 0 && _comac_isdigit (label[i - 1]))
	i--;

    while (i < len && label[i] == '0')
	i++;

    if (i < len)
	sscanf (label + i, "%d", num);

    if (i > 0) {
	char *s;
	s = _comac_malloc (i + 1);
	if (! s)
	    return NULL;

	memcpy (s, label, i);
	s[i] = 0;
	return s;
    }

    return NULL;
}

/* strcmp that handles NULL arguments */
static comac_bool_t
strcmp_null (const char *s1, const char *s2)
{
    if (s1 && s2)
	return strcmp (s1, s2) == 0;

    if (! s1 && ! s2)
	return TRUE;

    return FALSE;
}

static comac_int_status_t
comac_pdf_interchange_write_forward_links (comac_pdf_surface_t *surface)
{
    int num_elems, i;
    comac_pdf_forward_link_t *link;
    comac_int_status_t status;
    comac_pdf_named_dest_t key;
    comac_pdf_named_dest_t *named_dest;
    comac_pdf_interchange_t *ic = &surface->interchange;

    num_elems = _comac_array_num_elements (&surface->forward_links);
    for (i = 0; i < num_elems; i++) {
	link = _comac_array_index (&surface->forward_links, i);
	if (link->page > (int) _comac_array_num_elements (&surface->pages))
	    return _comac_tag_error (
		"Link attribute: \"page=%d\" page exceeds page count (%d)",
		link->page,
		_comac_array_num_elements (&surface->pages));

	status = _comac_pdf_surface_object_begin (surface, link->res);
	if (unlikely (status))
	    return status;

	if (link->dest) {
	    key.attrs.name = link->dest;
	    init_named_dest_key (&key);
	    named_dest = _comac_hash_table_lookup (ic->named_dests, &key.base);
	    if (named_dest) {
		double x = 0;
		double y = 0;

		if (named_dest->extents.valid) {
		    x = named_dest->extents.extents.x;
		    y = named_dest->extents.extents.y;
		}

		if (named_dest->attrs.x_valid)
		    x = named_dest->attrs.x;

		if (named_dest->attrs.y_valid)
		    y = named_dest->attrs.y;

		status =
		    comac_pdf_interchange_write_explicit_dest (surface,
							       named_dest->page,
							       TRUE,
							       x,
							       y);
	    } else {
		return _comac_tag_error ("Link to dest=\"%s\" not found",
					 link->dest);
	    }
	} else {
	    comac_pdf_interchange_write_explicit_dest (surface,
						       link->page,
						       link->has_pos,
						       link->pos.x,
						       link->pos.y);
	}
	_comac_pdf_surface_object_end (surface);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_page_labels (comac_pdf_surface_t *surface)
{
    int num_elems, i;
    char *label;
    char *prefix;
    char *prev_prefix;
    int num, prev_num;
    comac_int_status_t status;
    comac_bool_t has_labels;

    /* Check if any labels defined */
    num_elems = _comac_array_num_elements (&surface->page_labels);
    has_labels = FALSE;
    for (i = 0; i < num_elems; i++) {
	_comac_array_copy_element (&surface->page_labels, i, &label);
	if (label) {
	    has_labels = TRUE;
	    break;
	}
    }

    if (! has_labels)
	return COMAC_STATUS_SUCCESS;

    surface->page_labels_res = _comac_pdf_surface_new_object (surface);
    if (surface->page_labels_res.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status =
	_comac_pdf_surface_object_begin (surface, surface->page_labels_res);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (surface->object_stream.stream, "<< /Nums [\n");
    prefix = NULL;
    prev_prefix = NULL;
    num = 0;
    prev_num = 0;
    for (i = 0; i < num_elems; i++) {
	_comac_array_copy_element (&surface->page_labels, i, &label);
	if (label) {
	    prefix = split_label (label, &num);
	} else {
	    prefix = NULL;
	    num = i + 1;
	}

	if (! strcmp_null (prefix, prev_prefix) || num != prev_num + 1) {
	    _comac_output_stream_printf (surface->object_stream.stream,
					 "   %d << ",
					 i);

	    if (num)
		_comac_output_stream_printf (surface->object_stream.stream,
					     "/S /D /St %d ",
					     num);

	    if (prefix) {
		char *s;
		status = _comac_utf8_to_pdf_string (prefix, &s);
		if (unlikely (status))
		    return status;

		_comac_output_stream_printf (surface->object_stream.stream,
					     "/P %s ",
					     s);
		free (s);
	    }

	    _comac_output_stream_printf (surface->object_stream.stream, ">>\n");
	}
	free (prev_prefix);
	prev_prefix = prefix;
	prefix = NULL;
	prev_num = num;
    }
    free (prefix);
    free (prev_prefix);
    _comac_output_stream_printf (surface->object_stream.stream,
				 "  ]\n"
				 ">>\n");
    _comac_pdf_surface_object_end (surface);

    return COMAC_STATUS_SUCCESS;
}

static void
_collect_external_dest (void *entry, void *closure)
{
    comac_pdf_named_dest_t *dest = entry;
    comac_pdf_surface_t *surface = closure;
    comac_pdf_interchange_t *ic = &surface->interchange;

    if (! dest->attrs.internal)
	ic->sorted_dests[ic->num_dests++] = dest;
}

static int
_dest_compare (const void *a, const void *b)
{
    const comac_pdf_named_dest_t *const *dest_a = a;
    const comac_pdf_named_dest_t *const *dest_b = b;

    return strcmp ((*dest_a)->attrs.name, (*dest_b)->attrs.name);
}

static comac_int_status_t
_comac_pdf_interchange_write_document_dests (comac_pdf_surface_t *surface)
{
    int i;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status;

    if (ic->num_dests == 0) {
	ic->dests_res.id = 0;
	return COMAC_STATUS_SUCCESS;
    }

    ic->sorted_dests =
	calloc (ic->num_dests, sizeof (comac_pdf_named_dest_t *));
    if (unlikely (ic->sorted_dests == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    ic->num_dests = 0;
    _comac_hash_table_foreach (ic->named_dests,
			       _collect_external_dest,
			       surface);

    qsort (ic->sorted_dests,
	   ic->num_dests,
	   sizeof (comac_pdf_named_dest_t *),
	   _dest_compare);

    ic->dests_res = _comac_pdf_surface_new_object (surface);
    if (ic->dests_res.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = _comac_pdf_surface_object_begin (surface, ic->dests_res);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (surface->object_stream.stream,
				 "<< /Names [\n");
    for (i = 0; i < ic->num_dests; i++) {
	comac_pdf_named_dest_t *dest = ic->sorted_dests[i];
	comac_pdf_resource_t page_res;
	double x = 0;
	double y = 0;
	double height;

	if (dest->attrs.internal)
	    continue;

	if (dest->extents.valid) {
	    x = dest->extents.extents.x;
	    y = dest->extents.extents.y;
	}

	if (dest->attrs.x_valid)
	    x = dest->attrs.x;

	if (dest->attrs.y_valid)
	    y = dest->attrs.y;

	_comac_array_copy_element (&surface->pages, dest->page - 1, &page_res);
	_comac_array_copy_element (&surface->page_heights,
				   dest->page - 1,
				   &height);
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   (%s) [%d 0 R /XYZ %f %f 0]\n",
				     dest->attrs.name,
				     page_res.id,
				     x,
				     height - y);
    }
    _comac_output_stream_printf (surface->object_stream.stream,
				 "  ]\n"
				 ">>\n");
    _comac_pdf_surface_object_end (surface);

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_names_dict (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status;

    status = _comac_pdf_interchange_write_document_dests (surface);
    if (unlikely (status))
	return status;

    surface->names_dict_res.id = 0;
    if (ic->dests_res.id != 0) {
	surface->names_dict_res = _comac_pdf_surface_new_object (surface);
	if (surface->names_dict_res.id == 0)
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	status =
	    _comac_pdf_surface_object_begin (surface, surface->names_dict_res);
	if (unlikely (status))
	    return status;

	_comac_output_stream_printf (surface->object_stream.stream,
				     "<< /Dests %d 0 R >>\n",
				     ic->dests_res.id);
	_comac_pdf_surface_object_end (surface);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
comac_pdf_interchange_write_docinfo (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status;
    unsigned int i, num_elems;
    struct metadata *data;
    unsigned char *p;

    surface->docinfo_res = _comac_pdf_surface_new_object (surface);
    if (surface->docinfo_res.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = _comac_pdf_surface_object_begin (surface, surface->docinfo_res);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (
	surface->object_stream.stream,
	"<< /Producer (comac %s (https://github.com/jpakkane/comac/))\n",
	comac_version_string ());

    if (ic->docinfo.title)
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /Title %s\n",
				     ic->docinfo.title);

    if (ic->docinfo.author)
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /Author %s\n",
				     ic->docinfo.author);

    if (ic->docinfo.subject)
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /Subject %s\n",
				     ic->docinfo.subject);

    if (ic->docinfo.keywords)
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /Keywords %s\n",
				     ic->docinfo.keywords);

    if (ic->docinfo.creator)
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /Creator %s\n",
				     ic->docinfo.creator);

    if (ic->docinfo.create_date)
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /CreationDate %s\n",
				     ic->docinfo.create_date);

    if (ic->docinfo.mod_date)
	_comac_output_stream_printf (surface->object_stream.stream,
				     "   /ModDate %s\n",
				     ic->docinfo.mod_date);

    num_elems = _comac_array_num_elements (&ic->custom_metadata);
    for (i = 0; i < num_elems; i++) {
	data = _comac_array_index (&ic->custom_metadata, i);
	if (data->value) {
	    _comac_output_stream_printf (surface->object_stream.stream, "   /");
	    /* The name can be any utf8 string. Use hex codes as
	     * specified in section 7.3.5 of PDF reference
	     */
	    p = (unsigned char *) data->name;
	    while (*p) {
		if (*p < 0x21 || *p > 0x7e || *p == '#' || *p == '/')
		    _comac_output_stream_printf (surface->object_stream.stream,
						 "#%02x",
						 *p);
		else
		    _comac_output_stream_printf (surface->object_stream.stream,
						 "%c",
						 *p);
		p++;
	    }
	    _comac_output_stream_printf (surface->object_stream.stream,
					 " %s\n",
					 data->value);
	}
    }

    _comac_output_stream_printf (surface->object_stream.stream, ">>\n");
    _comac_pdf_surface_object_end (surface);

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_comac_pdf_interchange_begin_structure_tag (comac_pdf_surface_t *surface,
					    comac_tag_type_t tag_type,
					    const char *name,
					    const char *attributes)
{
    int page_num, mcid;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_pdf_interchange_t *ic = &surface->interchange;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	status =
	    add_tree_node (surface, ic->current_node, name, &ic->current_node);
	if (unlikely (status))
	    return status;

	_comac_tag_stack_set_top_data (&ic->analysis_tag_stack,
				       ic->current_node);

	if (tag_type & TAG_TYPE_LINK) {
	    status =
		add_annotation (surface, ic->current_node, name, attributes);
	    if (unlikely (status))
		return status;

	    comac_list_add_tail (&ic->current_node->extents.link,
				 &ic->extents_list);
	}

    } else if (surface->paginated_mode == COMAC_PAGINATED_MODE_RENDER) {
	ic->current_node =
	    _comac_tag_stack_top_elem (&ic->render_tag_stack)->data;
	assert (ic->current_node != NULL);
	if (is_leaf_node (ic->current_node)) {
	    page_num = _comac_array_num_elements (&surface->pages);
	    add_mcid_to_node (surface, ic->current_node, page_num, &mcid);
	    status = _comac_pdf_operators_tag_begin (&surface->pdf_operators,
						     name,
						     mcid);
	}
    }

    return status;
}

static comac_int_status_t
_comac_pdf_interchange_begin_dest_tag (comac_pdf_surface_t *surface,
				       comac_tag_type_t tag_type,
				       const char *name,
				       const char *attributes)
{
    comac_pdf_named_dest_t *dest;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	dest = calloc (1, sizeof (comac_pdf_named_dest_t));
	if (unlikely (dest == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	status = _comac_tag_parse_dest_attributes (attributes, &dest->attrs);
	if (unlikely (status)) {
	    free (dest);
	    return status;
	}

	dest->page = _comac_array_num_elements (&surface->pages);
	init_named_dest_key (dest);
	status = _comac_hash_table_insert (ic->named_dests, &dest->base);
	if (unlikely (status)) {
	    free (dest->attrs.name);
	    free (dest);
	    return status;
	}

	_comac_tag_stack_set_top_data (&ic->analysis_tag_stack, dest);
	comac_list_add_tail (&dest->extents.link, &ic->extents_list);
	ic->num_dests++;
    }

    return status;
}

comac_int_status_t
_comac_pdf_interchange_tag_begin (comac_pdf_surface_t *surface,
				  const char *name,
				  const char *attributes)
{
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_tag_type_t tag_type;
    comac_pdf_interchange_t *ic = &surface->interchange;
    void *ptr;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	status =
	    _comac_tag_stack_push (&ic->analysis_tag_stack, name, attributes);

    } else if (surface->paginated_mode == COMAC_PAGINATED_MODE_RENDER) {
	status =
	    _comac_tag_stack_push (&ic->render_tag_stack, name, attributes);
	_comac_array_copy_element (&ic->push_data, ic->push_data_index++, &ptr);
	_comac_tag_stack_set_top_data (&ic->render_tag_stack, ptr);
    }

    if (unlikely (status))
	return status;

    tag_type = _comac_tag_get_type (name);
    if (tag_type & TAG_TYPE_STRUCTURE) {
	status = _comac_pdf_interchange_begin_structure_tag (surface,
							     tag_type,
							     name,
							     attributes);
	if (unlikely (status))
	    return status;
    }

    if (tag_type & TAG_TYPE_DEST) {
	status = _comac_pdf_interchange_begin_dest_tag (surface,
							tag_type,
							name,
							attributes);
	if (unlikely (status))
	    return status;
    }

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	ptr = _comac_tag_stack_top_elem (&ic->analysis_tag_stack)->data;
	status = _comac_array_append (&ic->push_data, &ptr);
    }

    return status;
}

static comac_int_status_t
_comac_pdf_interchange_end_structure_tag (comac_pdf_surface_t *surface,
					  comac_tag_type_t tag_type,
					  comac_tag_stack_elem_t *elem)
{
    const comac_pdf_struct_tree_node_t *node;
    struct tag_extents *tag, *next;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    assert (elem->data != NULL);
    node = elem->data;
    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	if (tag_type & TAG_TYPE_LINK) {
	    comac_list_foreach_entry_safe (tag,
					   next,
					   struct tag_extents,
					   &ic->extents_list,
					   link)
	    {
		if (tag == &node->extents) {
		    comac_list_del (&tag->link);
		    break;
		}
	    }
	}
    } else if (surface->paginated_mode == COMAC_PAGINATED_MODE_RENDER) {
	if (is_leaf_node (ic->current_node)) {
	    status = _comac_pdf_operators_tag_end (&surface->pdf_operators);
	    if (unlikely (status))
		return status;
	}
    }

    ic->current_node = ic->current_node->parent;
    assert (ic->current_node != NULL);

    return status;
}

static comac_int_status_t
_comac_pdf_interchange_end_dest_tag (comac_pdf_surface_t *surface,
				     comac_tag_type_t tag_type,
				     comac_tag_stack_elem_t *elem)
{
    struct tag_extents *tag, *next;
    comac_pdf_named_dest_t *dest;
    comac_pdf_interchange_t *ic = &surface->interchange;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	assert (elem->data != NULL);
	dest = (comac_pdf_named_dest_t *) elem->data;
	comac_list_foreach_entry_safe (tag,
				       next,
				       struct tag_extents,
				       &ic->extents_list,
				       link)
	{
	    if (tag == &dest->extents) {
		comac_list_del (&tag->link);
		break;
	    }
	}
    }

    return COMAC_STATUS_SUCCESS;
}

comac_int_status_t
_comac_pdf_interchange_tag_end (comac_pdf_surface_t *surface, const char *name)
{
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_tag_type_t tag_type;
    comac_tag_stack_elem_t *elem;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE)
	status = _comac_tag_stack_pop (&ic->analysis_tag_stack, name, &elem);
    else if (surface->paginated_mode == COMAC_PAGINATED_MODE_RENDER)
	status = _comac_tag_stack_pop (&ic->render_tag_stack, name, &elem);

    if (unlikely (status))
	return status;

    tag_type = _comac_tag_get_type (name);
    if (tag_type & TAG_TYPE_STRUCTURE) {
	status =
	    _comac_pdf_interchange_end_structure_tag (surface, tag_type, elem);
	if (unlikely (status))
	    goto cleanup;
    }

    if (tag_type & TAG_TYPE_DEST) {
	status = _comac_pdf_interchange_end_dest_tag (surface, tag_type, elem);
	if (unlikely (status))
	    goto cleanup;
    }

cleanup:
    _comac_tag_stack_free_elem (elem);

    return status;
}

comac_int_status_t
_comac_pdf_interchange_add_operation_extents (
    comac_pdf_surface_t *surface, const comac_rectangle_int_t *extents)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    struct tag_extents *tag;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	comac_list_foreach_entry (tag,
				  struct tag_extents,
				  &ic->extents_list,
				  link)
	{
	    if (tag->valid) {
		_comac_rectangle_union (&tag->extents, extents);
	    } else {
		tag->extents = *extents;
		tag->valid = TRUE;
	    }
	}
    }

    return COMAC_STATUS_SUCCESS;
}

comac_int_status_t
_comac_pdf_interchange_begin_page_content (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    int page_num, mcid;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_ANALYZE) {
	_comac_array_truncate (&ic->mcid_to_tree, 0);
	_comac_array_truncate (&ic->push_data, 0);
	ic->begin_page_node = ic->current_node;
    } else if (surface->paginated_mode == COMAC_PAGINATED_MODE_RENDER) {
	ic->push_data_index = 0;
	ic->current_node = ic->begin_page_node;
	if (ic->end_page_node && is_leaf_node (ic->end_page_node)) {
	    page_num = _comac_array_num_elements (&surface->pages);
	    add_mcid_to_node (surface, ic->end_page_node, page_num, &mcid);
	    status = _comac_pdf_operators_tag_begin (&surface->pdf_operators,
						     ic->end_page_node->name,
						     mcid);
	}
    }

    return status;
}

comac_int_status_t
_comac_pdf_interchange_end_page_content (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;

    if (surface->paginated_mode == COMAC_PAGINATED_MODE_RENDER) {
	ic->end_page_node = ic->current_node;
	if (is_leaf_node (ic->current_node))
	    status = _comac_pdf_operators_tag_end (&surface->pdf_operators);
    }

    return status;
}

comac_int_status_t
_comac_pdf_interchange_write_page_objects (comac_pdf_surface_t *surface)
{
    comac_int_status_t status;

    status = comac_pdf_interchange_write_page_annots (surface);
    if (unlikely (status))
	return status;

    comac_pdf_interchange_clear_annotations (surface);

    return comac_pdf_interchange_write_page_parent_elems (surface);
}

comac_int_status_t
_comac_pdf_interchange_write_document_objects (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_int_status_t status = COMAC_STATUS_SUCCESS;
    comac_tag_stack_structure_type_t tag_type;

    tag_type = _comac_tag_stack_get_structure_type (&ic->analysis_tag_stack);
    if (tag_type == TAG_TREE_TYPE_TAGGED ||
	tag_type == TAG_TREE_TYPE_STRUCTURE ||
	tag_type == TAG_TREE_TYPE_LINK_ONLY) {

	status = comac_pdf_interchange_write_parent_tree (surface);
	if (unlikely (status))
	    return status;

	status = comac_pdf_interchange_write_struct_tree (surface);
	if (unlikely (status))
	    return status;

	if (tag_type == TAG_TREE_TYPE_TAGGED)
	    surface->tagged = TRUE;
    }

    status = comac_pdf_interchange_write_outline (surface);
    if (unlikely (status))
	return status;

    status = comac_pdf_interchange_write_page_labels (surface);
    if (unlikely (status))
	return status;

    status = comac_pdf_interchange_write_forward_links (surface);
    if (unlikely (status))
	return status;

    status = comac_pdf_interchange_write_names_dict (surface);
    if (unlikely (status))
	return status;

    status = comac_pdf_interchange_write_docinfo (surface);

    return status;
}

static void
_comac_pdf_interchange_set_create_date (comac_pdf_surface_t *surface)
{
    time_t utc, local, offset;
    struct tm tm_local, tm_utc;
    char buf[50];
    int buf_size;
    char *p;
    comac_pdf_interchange_t *ic = &surface->interchange;

    utc = time (NULL);
    localtime_r (&utc, &tm_local);
    strftime (buf, sizeof (buf), "(D:%Y%m%d%H%M%S", &tm_local);

    /* strftime "%z" is non standard and does not work on windows (it prints zone name, not offset).
     * Calculate time zone offset by comparing local and utc time_t values for the same time.
     */
    gmtime_r (&utc, &tm_utc);
    tm_utc.tm_isdst = tm_local.tm_isdst;
    local = mktime (&tm_utc);
    offset = difftime (utc, local);

    if (offset == 0) {
	strcat (buf, "Z");
    } else {
	if (offset > 0) {
	    strcat (buf, "+");
	} else {
	    strcat (buf, "-");
	    offset = -offset;
	}
	p = buf + strlen (buf);
	buf_size = sizeof (buf) - strlen (buf);
	snprintf (p,
		  buf_size,
		  "%02d'%02d",
		  (int) (offset / 3600),
		  (int) (offset % 3600) / 60);
    }
    strcat (buf, ")");
    ic->docinfo.create_date = strdup (buf);
}

comac_int_status_t
_comac_pdf_interchange_init (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_pdf_outline_entry_t *outline_root;
    comac_int_status_t status;

    _comac_tag_stack_init (&ic->analysis_tag_stack);
    _comac_tag_stack_init (&ic->render_tag_stack);
    _comac_array_init (&ic->push_data, sizeof (void *));
    ic->struct_root = calloc (1, sizeof (comac_pdf_struct_tree_node_t));
    if (unlikely (ic->struct_root == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    comac_list_init (&ic->struct_root->children);
    _comac_array_init (&ic->struct_root->mcid, sizeof (struct page_mcid));
    ic->current_node = ic->struct_root;
    ic->begin_page_node = NULL;
    ic->end_page_node = NULL;
    _comac_array_init (&ic->parent_tree, sizeof (comac_pdf_resource_t));
    _comac_array_init (&ic->mcid_to_tree,
		       sizeof (comac_pdf_struct_tree_node_t *));
    _comac_array_init (&ic->annots, sizeof (comac_pdf_annotation_t *));
    ic->parent_tree_res.id = 0;
    comac_list_init (&ic->extents_list);
    ic->named_dests = _comac_hash_table_create (_named_dest_equal);
    if (unlikely (ic->named_dests == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    ic->num_dests = 0;
    ic->sorted_dests = NULL;
    ic->dests_res.id = 0;

    _comac_array_init (&ic->outline, sizeof (comac_pdf_outline_entry_t *));
    outline_root = calloc (1, sizeof (comac_pdf_outline_entry_t));
    if (unlikely (outline_root == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    memset (&ic->docinfo, 0, sizeof (ic->docinfo));
    _comac_array_init (&ic->custom_metadata, sizeof (struct metadata));
    _comac_pdf_interchange_set_create_date (surface);
    status = _comac_array_append (&ic->outline, &outline_root);

    return status;
}

static void
_comac_pdf_interchange_free_outlines (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    int num_elems, i;

    num_elems = _comac_array_num_elements (&ic->outline);
    for (i = 0; i < num_elems; i++) {
	comac_pdf_outline_entry_t *outline;

	_comac_array_copy_element (&ic->outline, i, &outline);
	free (outline->name);
	free (outline->link_attrs.dest);
	free (outline->link_attrs.uri);
	free (outline->link_attrs.file);
	free (outline);
    }
    _comac_array_fini (&ic->outline);
}

void
_comac_pdf_interchange_fini (comac_pdf_surface_t *surface)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    unsigned int i, num_elems;
    struct metadata *data;

    _comac_tag_stack_fini (&ic->analysis_tag_stack);
    _comac_tag_stack_fini (&ic->render_tag_stack);
    _comac_array_fini (&ic->push_data);
    free_node (ic->struct_root);
    _comac_array_fini (&ic->mcid_to_tree);
    comac_pdf_interchange_clear_annotations (surface);
    _comac_array_fini (&ic->annots);
    _comac_array_fini (&ic->parent_tree);
    _comac_hash_table_foreach (ic->named_dests,
			       _named_dest_pluck,
			       ic->named_dests);
    _comac_hash_table_destroy (ic->named_dests);
    free (ic->sorted_dests);
    _comac_pdf_interchange_free_outlines (surface);
    free (ic->docinfo.title);
    free (ic->docinfo.author);
    free (ic->docinfo.subject);
    free (ic->docinfo.keywords);
    free (ic->docinfo.creator);
    free (ic->docinfo.create_date);
    free (ic->docinfo.mod_date);

    num_elems = _comac_array_num_elements (&ic->custom_metadata);
    for (i = 0; i < num_elems; i++) {
	data = _comac_array_index (&ic->custom_metadata, i);
	free (data->name);
	free (data->value);
    }
    _comac_array_fini (&ic->custom_metadata);
}

comac_int_status_t
_comac_pdf_interchange_add_outline (comac_pdf_surface_t *surface,
				    int parent_id,
				    const char *name,
				    const char *link_attribs,
				    comac_pdf_outline_flags_t flags,
				    int *id)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_pdf_outline_entry_t *outline;
    comac_pdf_outline_entry_t *parent;
    comac_int_status_t status;

    if (parent_id < 0 ||
	parent_id >= (int) _comac_array_num_elements (&ic->outline))
	return COMAC_STATUS_SUCCESS;

    outline = _comac_malloc (sizeof (comac_pdf_outline_entry_t));
    if (unlikely (outline == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status =
	_comac_tag_parse_link_attributes (link_attribs, &outline->link_attrs);
    if (unlikely (status)) {
	free (outline);
	return status;
    }

    outline->res = _comac_pdf_surface_new_object (surface);
    if (outline->res.id == 0)
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    outline->name = strdup (name);
    outline->flags = flags;
    outline->count = 0;

    _comac_array_copy_element (&ic->outline, parent_id, &parent);

    outline->parent = parent;
    outline->first_child = NULL;
    outline->last_child = NULL;
    outline->next = NULL;
    if (parent->last_child) {
	parent->last_child->next = outline;
	outline->prev = parent->last_child;
	parent->last_child = outline;
    } else {
	parent->first_child = outline;
	parent->last_child = outline;
	outline->prev = NULL;
    }

    *id = _comac_array_num_elements (&ic->outline);
    status = _comac_array_append (&ic->outline, &outline);
    if (unlikely (status))
	return status;

    /* Update Count */
    outline = outline->parent;
    while (outline) {
	if (outline->flags & COMAC_PDF_OUTLINE_FLAG_OPEN) {
	    outline->count++;
	} else {
	    outline->count--;
	    break;
	}
	outline = outline->parent;
    }

    return COMAC_STATUS_SUCCESS;
}

/*
 * Date must be in the following format:
 *
 *     YYYY-MM-DDThh:mm:ss[Z+-]hh:mm
 *
 * Only the year is required. If a field is included all preceding
 * fields must be included.
 */
static char *
iso8601_to_pdf_date_string (const char *iso)
{
    char buf[40];
    const char *p;
    int i;

    /* Check that utf8 contains only the characters "0123456789-T:Z+" */
    p = iso;
    while (*p) {
	if (! _comac_isdigit (*p) && *p != '-' && *p != 'T' && *p != ':' &&
	    *p != 'Z' && *p != '+')
	    return NULL;
	p++;
    }

    p = iso;
    strcpy (buf, "(");

    /* YYYY (required) */
    if (strlen (p) < 4)
	return NULL;

    strncat (buf, p, 4);
    p += 4;

    /* -MM, -DD, Thh, :mm, :ss */
    for (i = 0; i < 5; i++) {
	if (strlen (p) < 3)
	    goto finish;

	strncat (buf, p + 1, 2);
	p += 3;
    }

    /* Z, +, - */
    if (strlen (p) < 1)
	goto finish;
    strncat (buf, p, 1);
    p += 1;

    /* hh */
    if (strlen (p) < 2)
	goto finish;

    strncat (buf, p, 2);
    strcat (buf, "'");
    p += 2;

    /* :mm */
    if (strlen (p) < 3)
	goto finish;

    strncat (buf, p + 1, 2);
    strcat (buf, "'");

finish:
    strcat (buf, ")");
    return strdup (buf);
}

comac_int_status_t
_comac_pdf_interchange_set_metadata (comac_pdf_surface_t *surface,
				     comac_pdf_metadata_t metadata,
				     const char *utf8)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    comac_status_t status;
    char *s = NULL;

    if (utf8) {
	if (metadata == COMAC_PDF_METADATA_CREATE_DATE ||
	    metadata == COMAC_PDF_METADATA_MOD_DATE) {
	    s = iso8601_to_pdf_date_string (utf8);
	} else {
	    status = _comac_utf8_to_pdf_string (utf8, &s);
	    if (unlikely (status))
		return status;
	}
    }

    switch (metadata) {
    case COMAC_PDF_METADATA_TITLE:
	free (ic->docinfo.title);
	ic->docinfo.title = s;
	break;
    case COMAC_PDF_METADATA_AUTHOR:
	free (ic->docinfo.author);
	ic->docinfo.author = s;
	break;
    case COMAC_PDF_METADATA_SUBJECT:
	free (ic->docinfo.subject);
	ic->docinfo.subject = s;
	break;
    case COMAC_PDF_METADATA_KEYWORDS:
	free (ic->docinfo.keywords);
	ic->docinfo.keywords = s;
	break;
    case COMAC_PDF_METADATA_CREATOR:
	free (ic->docinfo.creator);
	ic->docinfo.creator = s;
	break;
    case COMAC_PDF_METADATA_CREATE_DATE:
	free (ic->docinfo.create_date);
	ic->docinfo.create_date = s;
	break;
    case COMAC_PDF_METADATA_MOD_DATE:
	free (ic->docinfo.mod_date);
	ic->docinfo.mod_date = s;
	break;
    }

    return COMAC_STATUS_SUCCESS;
}

static const char *reserved_metadata_names[] = {
    "",
    "Title",
    "Author",
    "Subject",
    "Keywords",
    "Creator",
    "Producer",
    "CreationDate",
    "ModDate",
    "Trapped",
};

comac_int_status_t
_comac_pdf_interchange_set_custom_metadata (comac_pdf_surface_t *surface,
					    const char *name,
					    const char *value)
{
    comac_pdf_interchange_t *ic = &surface->interchange;
    struct metadata *data;
    struct metadata new_data;
    int i, num_elems;
    comac_int_status_t status;
    char *s = NULL;

    if (name == NULL)
	return COMAC_STATUS_NULL_POINTER;

    for (i = 0; i < ARRAY_LENGTH (reserved_metadata_names); i++) {
	if (strcmp (name, reserved_metadata_names[i]) == 0)
	    return COMAC_STATUS_INVALID_STRING;
    }

    /* First check if we already have an entry for this name. If so,
     * update the value. A NULL value means the entry has been removed
     * and will not be emitted. */
    num_elems = _comac_array_num_elements (&ic->custom_metadata);
    for (i = 0; i < num_elems; i++) {
	data = _comac_array_index (&ic->custom_metadata, i);
	if (strcmp (name, data->name) == 0) {
	    free (data->value);
	    data->value = NULL;
	    if (value && strlen (value)) {
		status = _comac_utf8_to_pdf_string (value, &s);
		if (unlikely (status))
		    return status;
		data->value = s;
	    }
	    return COMAC_STATUS_SUCCESS;
	}
    }

    /* Add new entry */
    status = COMAC_STATUS_SUCCESS;
    if (value && strlen (value)) {
	new_data.name = strdup (name);
	status = _comac_utf8_to_pdf_string (value, &s);
	if (unlikely (status))
	    return status;
	new_data.value = s;
	status = _comac_array_append (&ic->custom_metadata, &new_data);
    }

    return status;
}
