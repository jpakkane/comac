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

#ifndef COMAC_ARRAY_PRIVATE_H
#define COMAC_ARRAY_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-types-private.h"

COMAC_BEGIN_DECLS

/* comac-array.c structures and functions */

comac_private void
_comac_array_init (comac_array_t *array, unsigned int element_size);

comac_private void
_comac_array_fini (comac_array_t *array);

comac_private comac_status_t
_comac_array_grow_by (comac_array_t *array, unsigned int additional);

comac_private void
_comac_array_truncate (comac_array_t *array, unsigned int num_elements);

comac_private comac_status_t
_comac_array_append (comac_array_t *array, const void *element);

comac_private comac_status_t
_comac_array_append_multiple (comac_array_t *array,
			      const void *elements,
			      unsigned int num_elements);

comac_private comac_status_t
_comac_array_allocate (comac_array_t *array,
		       unsigned int num_elements,
		       void **elements);

comac_private void *
_comac_array_index (comac_array_t *array, unsigned int index);

comac_private const void *
_comac_array_index_const (const comac_array_t *array, unsigned int index);

comac_private void
_comac_array_copy_element (const comac_array_t *array,
			   unsigned int index,
			   void *dst);

comac_private unsigned int
_comac_array_num_elements (const comac_array_t *array);

comac_private unsigned int
_comac_array_size (const comac_array_t *array);

comac_private void
_comac_array_sort (const comac_array_t *array,
		   int (*compar) (const void *, const void *));

COMAC_END_DECLS

#endif /* COMAC_ARRAY_PRIVATE_H */
