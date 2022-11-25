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

#ifndef COMAC_TAG_STACK_PRIVATE_H
#define COMAC_TAG_STACK_PRIVATE_H

#include "comac-error-private.h"
#include "comac-list-inline.h"

/* The type of a single tag */
typedef enum {
    TAG_TYPE_INVALID = 0,
    TAG_TYPE_STRUCTURE = 1,
    TAG_TYPE_LINK = 2,
    TAG_TYPE_DEST = 4,
} comac_tag_type_t;

/* The type of the structure tree. */
typedef enum _comac_tag_stack_structure_type {
    TAG_TREE_TYPE_TAGGED,    /* compliant with Tagged PDF */
    TAG_TREE_TYPE_STRUCTURE, /* valid structure but not 'Tagged PDF' compliant */
    TAG_TREE_TYPE_LINK_ONLY, /* contains Link tags only */
    TAG_TREE_TYPE_NO_TAGS,   /* no tags used */
    TAG_TREE_TYPE_INVALID,   /* invalid tag structure */
} comac_tag_stack_structure_type_t;

typedef struct _comac_tag_stack_elem {
    char *name;
    char *attributes;
    void *data;
    comac_list_t link;

} comac_tag_stack_elem_t;

typedef struct _comac_tag_stack {
    comac_list_t list;
    comac_tag_stack_structure_type_t type;
    int size;

} comac_tag_stack_t;

comac_private void
_comac_tag_stack_init (comac_tag_stack_t *stack);

comac_private void
_comac_tag_stack_fini (comac_tag_stack_t *stack);

comac_private comac_tag_stack_structure_type_t
_comac_tag_stack_get_structure_type (comac_tag_stack_t *stack);

comac_private comac_int_status_t
_comac_tag_stack_push (comac_tag_stack_t *stack,
		       const char *name,
		       const char *attributes);

comac_private void
_comac_tag_stack_set_top_data (comac_tag_stack_t *stack, void *data);

comac_private comac_int_status_t
_comac_tag_stack_pop (comac_tag_stack_t *stack,
		      const char *name,
		      comac_tag_stack_elem_t **elem);

comac_private comac_tag_stack_elem_t *
_comac_tag_stack_top_elem (comac_tag_stack_t *stack);

comac_private void
_comac_tag_stack_free_elem (comac_tag_stack_elem_t *elem);

comac_private comac_tag_type_t
_comac_tag_get_type (const char *name);

comac_private comac_status_t
_comac_tag_error (const char *fmt, ...) COMAC_PRINTF_FORMAT (1, 2);

#endif /* COMAC_TAG_STACK_PRIVATE_H */
