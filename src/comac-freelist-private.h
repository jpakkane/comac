/*
 * Copyright © 2006 Joonas Pihlaja
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
#ifndef COMAC_FREELIST_H
#define COMAC_FREELIST_H

#include "comac-types-private.h"
#include "comac-compiler-private.h"
#include "comac-freelist-type-private.h"

/* for stand-alone compilation*/
#ifndef VG
#define VG(x)
#endif

#ifndef NULL
#define NULL (void *) 0
#endif

/* Initialise a freelist that will be responsible for allocating
 * nodes of size nodesize. */
comac_private void
_comac_freelist_init (comac_freelist_t *freelist, unsigned nodesize);

/* Deallocate any nodes in the freelist. */
comac_private void
_comac_freelist_fini (comac_freelist_t *freelist);

/* Allocate a new node from the freelist.  If the freelist contains no
 * nodes, a new one will be allocated using malloc().  The caller is
 * responsible for calling _comac_freelist_free() or free() on the
 * returned node.  Returns %NULL on memory allocation error. */
comac_private void *
_comac_freelist_alloc (comac_freelist_t *freelist);

/* Allocate a new node from the freelist.  If the freelist contains no
 * nodes, a new one will be allocated using calloc().  The caller is
 * responsible for calling _comac_freelist_free() or free() on the
 * returned node.  Returns %NULL on memory allocation error. */
comac_private void *
_comac_freelist_calloc (comac_freelist_t *freelist);

/* Return a node to the freelist. This does not deallocate the memory,
 * but makes it available for later reuse by
 * _comac_freelist_alloc(). */
comac_private void
_comac_freelist_free (comac_freelist_t *freelist, void *node);


comac_private void
_comac_freepool_init (comac_freepool_t *freepool, unsigned nodesize);

comac_private void
_comac_freepool_fini (comac_freepool_t *freepool);

static inline void
_comac_freepool_reset (comac_freepool_t *freepool)
{
    while (freepool->pools != &freepool->embedded_pool) {
	comac_freelist_pool_t *pool = freepool->pools;
	freepool->pools = pool->next;
	pool->next = freepool->freepools;
	freepool->freepools = pool;
    }

    freepool->embedded_pool.rem = sizeof (freepool->embedded_data);
    freepool->embedded_pool.data = freepool->embedded_data;
}

comac_private void *
_comac_freepool_alloc_from_new_pool (comac_freepool_t *freepool);

static inline void *
_comac_freepool_alloc_from_pool (comac_freepool_t *freepool)
{
    comac_freelist_pool_t *pool;
    uint8_t *ptr;

    pool = freepool->pools;
    if (unlikely (freepool->nodesize > pool->rem))
	return _comac_freepool_alloc_from_new_pool (freepool);

    ptr = pool->data;
    pool->data += freepool->nodesize;
    pool->rem -= freepool->nodesize;
    VG (VALGRIND_MAKE_MEM_UNDEFINED (ptr, freepool->nodesize));
    return ptr;
}

static inline void *
_comac_freepool_alloc (comac_freepool_t *freepool)
{
    comac_freelist_node_t *node;

    node = freepool->first_free_node;
    if (node == NULL)
	return _comac_freepool_alloc_from_pool (freepool);

    VG (VALGRIND_MAKE_MEM_DEFINED (node, sizeof (node->next)));
    freepool->first_free_node = node->next;
    VG (VALGRIND_MAKE_MEM_UNDEFINED (node, freepool->nodesize));

    return node;
}

comac_private comac_status_t
_comac_freepool_alloc_array (comac_freepool_t *freepool,
			     int count,
			     void **array);

static inline void
_comac_freepool_free (comac_freepool_t *freepool, void *ptr)
{
    comac_freelist_node_t *node = ptr;

    node->next = freepool->first_free_node;
    freepool->first_free_node = node;
    VG (VALGRIND_MAKE_MEM_UNDEFINED (node, freepool->nodesize));
}

#endif /* COMAC_FREELIST_H */
