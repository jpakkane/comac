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
 *
 */

#ifndef COMAC_RTREE_PRIVATE_H
#define COMAC_RTREE_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-error-private.h"
#include "comac-types-private.h"

#include "comac-freelist-private.h"
#include "comac-list-inline.h"

enum {
    COMAC_RTREE_NODE_AVAILABLE,
    COMAC_RTREE_NODE_DIVIDED,
    COMAC_RTREE_NODE_OCCUPIED,
};

typedef struct _comac_rtree_node {
    struct _comac_rtree_node *children[4], *parent;
    comac_list_t link;
    uint16_t pinned;
    uint16_t state;
    uint16_t x, y;
    uint16_t width, height;
} comac_rtree_node_t;

typedef struct _comac_rtree {
    comac_rtree_node_t root;
    int min_size;
    comac_list_t pinned;
    comac_list_t available;
    comac_list_t evictable;
    void (*destroy) (comac_rtree_node_t *);
    comac_freepool_t node_freepool;
} comac_rtree_t;

comac_private comac_rtree_node_t *
_comac_rtree_node_create (comac_rtree_t *rtree,
			  comac_rtree_node_t *parent,
			  int x,
			  int y,
			  int width,
			  int height);

comac_private comac_status_t
_comac_rtree_node_insert (comac_rtree_t *rtree,
			  comac_rtree_node_t *node,
			  int width,
			  int height,
			  comac_rtree_node_t **out);

comac_private void
_comac_rtree_node_collapse (comac_rtree_t *rtree, comac_rtree_node_t *node);

comac_private void
_comac_rtree_node_remove (comac_rtree_t *rtree, comac_rtree_node_t *node);

comac_private void
_comac_rtree_node_destroy (comac_rtree_t *rtree, comac_rtree_node_t *node);

comac_private void
_comac_rtree_init (comac_rtree_t *rtree,
		   int width,
		   int height,
		   int min_size,
		   int node_size,
		   void (*destroy) (comac_rtree_node_t *));

comac_private comac_int_status_t
_comac_rtree_insert (comac_rtree_t *rtree,
		     int width,
		     int height,
		     comac_rtree_node_t **out);

comac_private comac_int_status_t
_comac_rtree_evict_random (comac_rtree_t *rtree,
			   int width,
			   int height,
			   comac_rtree_node_t **out);

comac_private void
_comac_rtree_foreach (comac_rtree_t *rtree,
		      void (*func) (comac_rtree_node_t *, void *data),
		      void *data);

static inline void *
_comac_rtree_pin (comac_rtree_t *rtree, comac_rtree_node_t *node)
{
    assert (node->state == COMAC_RTREE_NODE_OCCUPIED);
    if (! node->pinned) {
	comac_list_move (&node->link, &rtree->pinned);
	node->pinned = 1;
    }

    return node;
}

comac_private void
_comac_rtree_unpin (comac_rtree_t *rtree);

comac_private void
_comac_rtree_reset (comac_rtree_t *rtree);

comac_private void
_comac_rtree_fini (comac_rtree_t *rtree);

#endif /* COMAC_RTREE_PRIVATE_H */
