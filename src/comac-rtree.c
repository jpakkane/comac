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

#include "comacint.h"

#include "comac-error-private.h"
#include "comac-rtree-private.h"

comac_rtree_node_t *
_comac_rtree_node_create (comac_rtree_t		 *rtree,
		          comac_rtree_node_t	 *parent,
			  int			  x,
			  int			  y,
			  int			  width,
			  int			  height)
{
    comac_rtree_node_t *node;

    node = _comac_freepool_alloc (&rtree->node_freepool);
    if (node == NULL) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return NULL;
    }

    node->children[0] = NULL;
    node->parent = parent;
    node->state  = COMAC_RTREE_NODE_AVAILABLE;
    node->pinned = FALSE;
    node->x	 = x;
    node->y	 = y;
    node->width  = width;
    node->height = height;

    comac_list_add (&node->link, &rtree->available);

    return node;
}

void
_comac_rtree_node_destroy (comac_rtree_t *rtree, comac_rtree_node_t *node)
{
    int i;

    comac_list_del (&node->link);

    if (node->state == COMAC_RTREE_NODE_OCCUPIED) {
	rtree->destroy (node);
    } else {
	for (i = 0; i < 4 && node->children[i] != NULL; i++)
	    _comac_rtree_node_destroy (rtree, node->children[i]);
    }

    _comac_freepool_free (&rtree->node_freepool, node);
}

void
_comac_rtree_node_collapse (comac_rtree_t *rtree, comac_rtree_node_t *node)
{
    int i;

    do {
	assert (node->state == COMAC_RTREE_NODE_DIVIDED);

	for (i = 0;  i < 4 && node->children[i] != NULL; i++)
	    if (node->children[i]->state != COMAC_RTREE_NODE_AVAILABLE)
		return;

	for (i = 0; i < 4 && node->children[i] != NULL; i++)
	    _comac_rtree_node_destroy (rtree, node->children[i]);

	node->children[0] = NULL;
	node->state = COMAC_RTREE_NODE_AVAILABLE;
	comac_list_move (&node->link, &rtree->available);
    } while ((node = node->parent) != NULL);
}

comac_status_t
_comac_rtree_node_insert (comac_rtree_t *rtree,
	                  comac_rtree_node_t *node,
			  int width,
			  int height,
			  comac_rtree_node_t **out)
{
    int w, h, i;

    assert (node->state == COMAC_RTREE_NODE_AVAILABLE);
    assert (node->pinned == FALSE);

    if (node->width  - width  > rtree->min_size ||
	node->height - height > rtree->min_size)
    {
	w = node->width  - width;
	h = node->height - height;

	i = 0;
	node->children[i] = _comac_rtree_node_create (rtree, node,
						      node->x, node->y,
						      width, height);
	if (unlikely (node->children[i] == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);
	i++;

	if (w > rtree->min_size) {
	    node->children[i] = _comac_rtree_node_create (rtree, node,
							  node->x + width,
							  node->y,
							  w, height);
	    if (unlikely (node->children[i] == NULL))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	    i++;
	}

	if (h > rtree->min_size) {
	    node->children[i] = _comac_rtree_node_create (rtree, node,
							  node->x,
							  node->y + height,
							  width, h);
	    if (unlikely (node->children[i] == NULL))
		return _comac_error (COMAC_STATUS_NO_MEMORY);
	    i++;

	    if (w > rtree->min_size) {
		node->children[i] = _comac_rtree_node_create (rtree, node,
							      node->x + width,
							      node->y + height,
							      w, h);
		if (unlikely (node->children[i] == NULL))
		    return _comac_error (COMAC_STATUS_NO_MEMORY);
		i++;
	    }
	}

	if (i < 4)
	    node->children[i] = NULL;

	node->state = COMAC_RTREE_NODE_DIVIDED;
	comac_list_move (&node->link, &rtree->evictable);
	node = node->children[0];
    }

    node->state = COMAC_RTREE_NODE_OCCUPIED;
    comac_list_move (&node->link, &rtree->evictable);
    *out = node;

    return COMAC_STATUS_SUCCESS;
}

void
_comac_rtree_node_remove (comac_rtree_t *rtree, comac_rtree_node_t *node)
{
    assert (node->state == COMAC_RTREE_NODE_OCCUPIED);
    assert (node->pinned == FALSE);

    rtree->destroy (node);

    node->state = COMAC_RTREE_NODE_AVAILABLE;
    comac_list_move (&node->link, &rtree->available);

    _comac_rtree_node_collapse (rtree, node->parent);
}

comac_int_status_t
_comac_rtree_insert (comac_rtree_t	     *rtree,
		     int		      width,
	             int		      height,
	             comac_rtree_node_t	    **out)
{
    comac_rtree_node_t *node;

    comac_list_foreach_entry (node, comac_rtree_node_t,
			      &rtree->available, link)
    {
	if (node->width >= width && node->height >= height)
	    return _comac_rtree_node_insert (rtree, node, width, height, out);
    }

    return COMAC_INT_STATUS_UNSUPPORTED;
}

static uint32_t
hars_petruska_f54_1_random (void)
{
#define rol(x,k) ((x << k) | (x >> (32-k)))
    static uint32_t x;
    return x = (x ^ rol (x, 5) ^ rol (x, 24)) + 0x37798849;
#undef rol
}

comac_int_status_t
_comac_rtree_evict_random (comac_rtree_t	 *rtree,
		           int			  width,
		           int			  height,
		           comac_rtree_node_t		**out)
{
    comac_int_status_t ret = COMAC_INT_STATUS_UNSUPPORTED;
    comac_rtree_node_t *node, *next;
    comac_list_t tmp_pinned;
    int i, cnt;

    comac_list_init (&tmp_pinned);

    /* propagate pinned from children to root */
    comac_list_foreach_entry_safe (node, next,
				   comac_rtree_node_t, &rtree->pinned, link) {
	node = node->parent;
	while (node && ! node->pinned) {
	    node->pinned = 1;
	    comac_list_move (&node->link, &tmp_pinned);
	    node = node->parent;
	}
    }

    cnt = 0;
    comac_list_foreach_entry (node, comac_rtree_node_t,
			      &rtree->evictable, link)
    {
	if (node->width >= width && node->height >= height)
	    cnt++;
    }

    if (cnt == 0)
	goto out;

    cnt = hars_petruska_f54_1_random () % cnt;
    comac_list_foreach_entry (node, comac_rtree_node_t,
			      &rtree->evictable, link)
    {
	if (node->width >= width && node->height >= height && cnt-- == 0) {
	    if (node->state == COMAC_RTREE_NODE_OCCUPIED) {
		rtree->destroy (node);
	    } else {
		for (i = 0; i < 4 && node->children[i] != NULL; i++)
		    _comac_rtree_node_destroy (rtree, node->children[i]);
		node->children[0] = NULL;
	    }

	    node->state = COMAC_RTREE_NODE_AVAILABLE;
	    comac_list_move (&node->link, &rtree->available);

	    *out = node;
	    ret = COMAC_STATUS_SUCCESS;
	    break;
	}
    }

out:
    while (! comac_list_is_empty (&tmp_pinned)) {
	node = comac_list_first_entry (&tmp_pinned, comac_rtree_node_t, link);
	node->pinned = 0;
	comac_list_move (&node->link, &rtree->evictable);
    }
    return ret;
}

void
_comac_rtree_unpin (comac_rtree_t *rtree)
{
    while (! comac_list_is_empty (&rtree->pinned)) {
	comac_rtree_node_t *node = comac_list_first_entry (&rtree->pinned,
							   comac_rtree_node_t,
							   link);
	node->pinned = 0;
	comac_list_move (&node->link, &rtree->evictable);
    }
}

void
_comac_rtree_init (comac_rtree_t	*rtree,
	           int			 width,
		   int			 height,
		   int			 min_size,
		   int			 node_size,
		   void (*destroy) (comac_rtree_node_t *))
{
    assert (node_size >= (int) sizeof (comac_rtree_node_t));
    _comac_freepool_init (&rtree->node_freepool, node_size);

    comac_list_init (&rtree->available);
    comac_list_init (&rtree->pinned);
    comac_list_init (&rtree->evictable);

    rtree->min_size = min_size;
    rtree->destroy = destroy;

    memset (&rtree->root, 0, sizeof (rtree->root));
    rtree->root.width = width;
    rtree->root.height = height;
    rtree->root.state = COMAC_RTREE_NODE_AVAILABLE;
    comac_list_add (&rtree->root.link, &rtree->available);
}

void
_comac_rtree_reset (comac_rtree_t *rtree)
{
    int i;

    if (rtree->root.state == COMAC_RTREE_NODE_OCCUPIED) {
	rtree->destroy (&rtree->root);
    } else {
	for (i = 0; i < 4 && rtree->root.children[i] != NULL; i++)
	    _comac_rtree_node_destroy (rtree, rtree->root.children[i]);
	rtree->root.children[0] = NULL;
    }

    comac_list_init (&rtree->available);
    comac_list_init (&rtree->evictable);
    comac_list_init (&rtree->pinned);

    rtree->root.state = COMAC_RTREE_NODE_AVAILABLE;
    rtree->root.pinned = FALSE;
    comac_list_add (&rtree->root.link, &rtree->available);
}

static void
_comac_rtree_node_foreach (comac_rtree_node_t *node,
			   void (*func)(comac_rtree_node_t *, void *data),
			   void *data)
{
    int i;

    for (i = 0; i < 4 && node->children[i] != NULL; i++)
	_comac_rtree_node_foreach(node->children[i], func, data);

    func(node, data);
}

void
_comac_rtree_foreach (comac_rtree_t *rtree,
		      void (*func)(comac_rtree_node_t *, void *data),
		      void *data)
{
    int i;

    if (rtree->root.state == COMAC_RTREE_NODE_OCCUPIED) {
	func(&rtree->root, data);
    } else {
	for (i = 0; i < 4 && rtree->root.children[i] != NULL; i++)
	    _comac_rtree_node_foreach (rtree->root.children[i], func, data);
    }
}

void
_comac_rtree_fini (comac_rtree_t *rtree)
{
    int i;

    if (rtree->root.state == COMAC_RTREE_NODE_OCCUPIED) {
	rtree->destroy (&rtree->root);
    } else {
	for (i = 0; i < 4 && rtree->root.children[i] != NULL; i++)
	    _comac_rtree_node_destroy (rtree, rtree->root.children[i]);
    }

    _comac_freepool_fini (&rtree->node_freepool);
}
