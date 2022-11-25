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

#include "comacint.h"

#include "comac-error-private.h"
#include "comac-freelist-private.h"

void
_comac_freelist_init (comac_freelist_t *freelist, unsigned nodesize)
{
    memset (freelist, 0, sizeof (comac_freelist_t));
    freelist->nodesize = nodesize;
}

void
_comac_freelist_fini (comac_freelist_t *freelist)
{
    comac_freelist_node_t *node = freelist->first_free_node;
    while (node) {
	comac_freelist_node_t *next;

	VG (VALGRIND_MAKE_MEM_DEFINED (node, sizeof (node->next)));
	next = node->next;

	free (node);
	node = next;
    }
}

void *
_comac_freelist_alloc (comac_freelist_t *freelist)
{
    if (freelist->first_free_node) {
	comac_freelist_node_t *node;

	node = freelist->first_free_node;
	VG (VALGRIND_MAKE_MEM_DEFINED (node, sizeof (node->next)));
	freelist->first_free_node = node->next;
	VG (VALGRIND_MAKE_MEM_UNDEFINED (node, freelist->nodesize));

	return node;
    }

    return _comac_malloc (freelist->nodesize);
}

void *
_comac_freelist_calloc (comac_freelist_t *freelist)
{
    void *node = _comac_freelist_alloc (freelist);
    if (node)
	memset (node, 0, freelist->nodesize);
    return node;
}

void
_comac_freelist_free (comac_freelist_t *freelist, void *voidnode)
{
    comac_freelist_node_t *node = voidnode;
    if (node) {
	node->next = freelist->first_free_node;
	freelist->first_free_node = node;
	VG (VALGRIND_MAKE_MEM_UNDEFINED (node, freelist->nodesize));
    }
}

void
_comac_freepool_init (comac_freepool_t *freepool, unsigned nodesize)
{
    freepool->first_free_node = NULL;
    freepool->pools = &freepool->embedded_pool;
    freepool->freepools = NULL;
    freepool->nodesize = nodesize;

    freepool->embedded_pool.next = NULL;
    freepool->embedded_pool.size = sizeof (freepool->embedded_data);
    freepool->embedded_pool.rem = sizeof (freepool->embedded_data);
    freepool->embedded_pool.data = freepool->embedded_data;

    VG (VALGRIND_MAKE_MEM_UNDEFINED (freepool->embedded_data,
				     sizeof (freepool->embedded_data)));
}

void
_comac_freepool_fini (comac_freepool_t *freepool)
{
    comac_freelist_pool_t *pool;

    pool = freepool->pools;
    while (pool != &freepool->embedded_pool) {
	comac_freelist_pool_t *next = pool->next;
	free (pool);
	pool = next;
    }

    pool = freepool->freepools;
    while (pool != NULL) {
	comac_freelist_pool_t *next = pool->next;
	free (pool);
	pool = next;
    }

    VG (VALGRIND_MAKE_MEM_UNDEFINED (freepool, sizeof (freepool)));
}

void *
_comac_freepool_alloc_from_new_pool (comac_freepool_t *freepool)
{
    comac_freelist_pool_t *pool;
    int poolsize;

    if (freepool->freepools != NULL) {
	pool = freepool->freepools;
	freepool->freepools = pool->next;

	poolsize = pool->size;
    } else {
	if (freepool->pools != &freepool->embedded_pool)
	    poolsize = 2 * freepool->pools->size;
	else
	    poolsize = (128 * freepool->nodesize + 8191) & -8192;

	pool = _comac_malloc (sizeof (comac_freelist_pool_t) + poolsize);
	if (unlikely (pool == NULL))
	    return pool;

	pool->size = poolsize;
    }

    pool->next = freepool->pools;
    freepool->pools = pool;

    pool->rem = poolsize - freepool->nodesize;
    pool->data = (uint8_t *) (pool + 1) + freepool->nodesize;

    VG (VALGRIND_MAKE_MEM_UNDEFINED (pool->data, pool->rem));

    return pool + 1;
}

comac_status_t
_comac_freepool_alloc_array (comac_freepool_t *freepool,
			     int count,
			     void **array)
{
    int i;

    for (i = 0; i < count; i++) {
	comac_freelist_node_t *node;

	node = freepool->first_free_node;
	if (likely (node != NULL)) {
	    VG (VALGRIND_MAKE_MEM_DEFINED (node, sizeof (node->next)));
	    freepool->first_free_node = node->next;
	    VG (VALGRIND_MAKE_MEM_UNDEFINED (node, freepool->nodesize));
	} else {
	    node = _comac_freepool_alloc_from_pool (freepool);
	    if (unlikely (node == NULL))
		goto CLEANUP;
	}

	array[i] = node;
    }

    return COMAC_STATUS_SUCCESS;

CLEANUP:
    while (i--)
	_comac_freepool_free (freepool, array[i]);

    return _comac_error (COMAC_STATUS_NO_MEMORY);
}
