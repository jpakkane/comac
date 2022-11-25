/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2014 Intel Corporation, Inc
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
 */

#ifndef COMAC_LINE_PRIVATE_H
#define COMAC_LINE_PRIVATE_H

#include "comac-types-private.h"
#include "comac-error-private.h"
#include "comac-compiler-private.h"

COMAC_BEGIN_DECLS

comac_private int
_comac_lines_compare_at_y (const comac_line_t *a,
			 const comac_line_t *b,
			 int y);

COMAC_END_DECLS

#endif /* COMAC_LINE_PRIVATE_H */
