/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc.
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *      Keith Packard <keithp@keithp.com>
 *	Graydon Hoare <graydon@redhat.com>
 *	Carl Worth <cworth@cworth.org>
 */

#ifndef COMAC_HASH_PRIVATE_H
#define COMAC_HASH_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-types-private.h"

/* XXX: I'd like this file to be self-contained in terms of
 * includeability, but that's not really possible with the current
 * monolithic comacint.h. So, for now, just include comacint.h instead
 * if you want to include this file. */

typedef comac_bool_t (*comac_hash_keys_equal_func_t) (const void *key_a,
						      const void *key_b);

typedef comac_bool_t (*comac_hash_predicate_func_t) (const void *entry);

typedef void (*comac_hash_callback_func_t) (void *entry, void *closure);

comac_private comac_hash_table_t *
_comac_hash_table_create (comac_hash_keys_equal_func_t keys_equal);

comac_private void
_comac_hash_table_destroy (comac_hash_table_t *hash_table);

comac_private void *
_comac_hash_table_lookup (comac_hash_table_t *hash_table,
			  comac_hash_entry_t *key);

comac_private void *
_comac_hash_table_random_entry (comac_hash_table_t *hash_table,
				comac_hash_predicate_func_t predicate);

comac_private comac_status_t
_comac_hash_table_insert (comac_hash_table_t *hash_table,
			  comac_hash_entry_t *entry);

comac_private void
_comac_hash_table_remove (comac_hash_table_t *hash_table,
			  comac_hash_entry_t *key);

comac_private void
_comac_hash_table_foreach (comac_hash_table_t *hash_table,
			   comac_hash_callback_func_t hash_callback,
			   void *closure);

#endif
