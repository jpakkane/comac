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

#include "comac-tag-stack-private.h"

/* Tagged PDF must have one of these tags at the top level */
static const char *_comac_tag_stack_tagged_pdf_top_level_element_list[] = {
    "Document", "Part", "Art", "Sect", "Div", NULL};

/* List of valid tag names. Table numbers reference PDF 32000 */
static const char *_comac_tag_stack_struct_pdf_list[] = {
    /* Table 333 - Grouping Elements */
    "Document",
    "Part",
    "Art",
    "Sect",
    "Div",
    "BlockQuote",
    "Caption",
    "TOC",
    "TOCI",
    "Index",
    "NonStruct",
    "Private",

    /* Table 335 - Standard structure types for paragraphlike elements */
    "P",
    "H",
    "H1",
    "H2",
    "H3",
    "H4",
    "H5",
    "H6",

    /* Table 336 - Standard structure types for list elements */
    "L",
    "LI",
    "Lbl",
    "LBody",

    /* Table 337 - Standard structure types for table elements */
    "Table",
    "TR",
    "TH",
    "TD",
    "THead",
    "TBody",
    "TFoot",

    /* Table 338 - Standard structure types for inline-level structure elements */
    "Span",
    "Quote",
    "Note",
    "Reference",
    "BibEntry",
    "Code",
    "Link", /* COMAC_TAG_LINK */
    "Annot",
    "Ruby",
    "Warichu",

    /* Table 339 - Standard structure types for Ruby and Warichu elements */
    "RB",
    "RT",
    "RP",
    "WT",
    "WP",

    /* Table 340 - Standard structure types for illustration elements */
    "Figure",
    "Formula",
    "Form",

    NULL};

/* List of comac specific tag names */
static const char *_comac_tag_stack_comac_tag_list[] = {COMAC_TAG_DEST, NULL};

void
_comac_tag_stack_init (comac_tag_stack_t *stack)
{
    comac_list_init (&stack->list);
    stack->type = TAG_TREE_TYPE_NO_TAGS;
    stack->size = 0;
}

void
_comac_tag_stack_fini (comac_tag_stack_t *stack)
{
    while (! comac_list_is_empty (&stack->list)) {
	comac_tag_stack_elem_t *elem;

	elem =
	    comac_list_first_entry (&stack->list, comac_tag_stack_elem_t, link);
	comac_list_del (&elem->link);
	free (elem->name);
	free (elem->attributes);
	free (elem);
    }
}

comac_tag_stack_structure_type_t
_comac_tag_stack_get_structure_type (comac_tag_stack_t *stack)
{
    return stack->type;
}

static comac_bool_t
name_in_list (const char *name, const char **list)
{
    if (! name)
	return FALSE;

    while (*list) {
	if (strcmp (name, *list) == 0)
	    return TRUE;
	list++;
    }

    return FALSE;
}

comac_int_status_t
_comac_tag_stack_push (comac_tag_stack_t *stack,
		       const char *name,
		       const char *attributes)
{
    comac_tag_stack_elem_t *elem;

    if (! name_in_list (name, _comac_tag_stack_struct_pdf_list) &&
	! name_in_list (name, _comac_tag_stack_comac_tag_list)) {
	stack->type = TAG_TYPE_INVALID;
	return _comac_tag_error ("Invalid tag: %s", name);
    }

    if (stack->type == TAG_TREE_TYPE_NO_TAGS) {
	if (name_in_list (name,
			  _comac_tag_stack_tagged_pdf_top_level_element_list))
	    stack->type = TAG_TREE_TYPE_TAGGED;
	else if (strcmp (name, "Link") == 0)
	    stack->type = TAG_TREE_TYPE_LINK_ONLY;
	else if (name_in_list (name, _comac_tag_stack_struct_pdf_list))
	    stack->type = TAG_TREE_TYPE_STRUCTURE;
    } else {
	if (stack->type == TAG_TREE_TYPE_LINK_ONLY &&
	    (strcmp (name, "Link") != 0) &&
	    name_in_list (name, _comac_tag_stack_struct_pdf_list)) {
	    stack->type = TAG_TREE_TYPE_STRUCTURE;
	}
    }

    elem = _comac_malloc (sizeof (comac_tag_stack_elem_t));
    if (unlikely (elem == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    elem->name = strdup (name);
    if (unlikely (elem->name == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    if (attributes) {
	elem->attributes = strdup (attributes);
	if (unlikely (elem->attributes == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);
    } else {
	elem->attributes = NULL;
    }

    elem->data = NULL;

    comac_list_add_tail (&elem->link, &stack->list);
    stack->size++;

    return COMAC_STATUS_SUCCESS;
}

comac_private void
_comac_tag_stack_set_top_data (comac_tag_stack_t *stack, void *data)
{
    comac_tag_stack_elem_t *top;

    top = _comac_tag_stack_top_elem (stack);
    if (top)
	top->data = data;
}

comac_int_status_t
_comac_tag_stack_pop (comac_tag_stack_t *stack,
		      const char *name,
		      comac_tag_stack_elem_t **elem)
{
    comac_tag_stack_elem_t *top;

    top = _comac_tag_stack_top_elem (stack);
    if (! top) {
	stack->type = TAG_TYPE_INVALID;
	return _comac_tag_error ("comac_tag_end(\"%s\") no matching begin tag",
				 name);
    }

    comac_list_del (&top->link);
    stack->size--;
    if (strcmp (top->name, name) != 0) {
	stack->type = TAG_TYPE_INVALID;
	_comac_tag_stack_free_elem (top);
	return _comac_tag_error (
	    "comac_tag_end(\"%s\") does not matching previous begin tag \"%s\"",
	    name,
	    top->name);
    }

    if (elem)
	*elem = top;
    else
	_comac_tag_stack_free_elem (top);

    return COMAC_STATUS_SUCCESS;
}

comac_tag_stack_elem_t *
_comac_tag_stack_top_elem (comac_tag_stack_t *stack)
{
    if (comac_list_is_empty (&stack->list))
	return NULL;

    return comac_list_last_entry (&stack->list, comac_tag_stack_elem_t, link);
}

void
_comac_tag_stack_free_elem (comac_tag_stack_elem_t *elem)
{
    free (elem->name);
    free (elem->attributes);
    free (elem);
}

comac_tag_type_t
_comac_tag_get_type (const char *name)
{
    if (! name_in_list (name, _comac_tag_stack_struct_pdf_list) &&
	! name_in_list (name, _comac_tag_stack_comac_tag_list))
	return TAG_TYPE_INVALID;

    if (strcmp (name, "Link") == 0)
	return (TAG_TYPE_LINK | TAG_TYPE_STRUCTURE);

    if (strcmp (name, "comac.dest") == 0)
	return TAG_TYPE_DEST;

    return TAG_TYPE_STRUCTURE;
}

comac_status_t
_comac_tag_error (const char *fmt, ...)
{
    va_list ap;

    if (getenv ("COMAC_DEBUG_TAG") != NULL) {
	printf ("TAG ERROR: ");
	va_start (ap, fmt);
	vprintf (fmt, ap);
	va_end (ap);
	printf ("\n");
    }
    return _comac_error (COMAC_STATUS_TAG_ERROR);
}
