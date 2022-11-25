/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2008 Chris Wilson
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
 * The Initial Developer of the Original Code is Chris Wilson
 *
 * Contributor(s):
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_SCRIPT_H
#define COMAC_SCRIPT_H

#include "comac.h"

#if COMAC_HAS_SCRIPT_SURFACE

COMAC_BEGIN_DECLS

/**
 * comac_script_mode_t:
 * @COMAC_SCRIPT_MODE_ASCII: the output will be in readable text (default). (Since 1.12)
 * @COMAC_SCRIPT_MODE_BINARY: the output will use byte codes. (Since 1.12)
 *
 * A set of script output variants.
 *
 * Since: 1.12
 **/
typedef enum {
    COMAC_SCRIPT_MODE_ASCII,
    COMAC_SCRIPT_MODE_BINARY
} comac_script_mode_t;

comac_public comac_device_t *
comac_script_create (const char *filename);

comac_public comac_device_t *
comac_script_create_for_stream (comac_write_func_t write_func, void *closure);

comac_public void
comac_script_write_comment (comac_device_t *script,
			    const char *comment,
			    int len);

comac_public void
comac_script_set_mode (comac_device_t *script, comac_script_mode_t mode);

comac_public comac_script_mode_t
comac_script_get_mode (comac_device_t *script);

comac_public comac_surface_t *
comac_script_surface_create (comac_device_t *script,
			     comac_content_t content,
			     double width,
			     double height);

comac_public comac_surface_t *
comac_script_surface_create_for_target (comac_device_t *script,
					comac_surface_t *target);

comac_public comac_status_t
comac_script_from_recording_surface (comac_device_t *script,
				     comac_surface_t *recording_surface);

COMAC_END_DECLS

#else /*COMAC_HAS_SCRIPT_SURFACE*/
#error Comac was not compiled with support for the ComacScript backend
#endif /*COMAC_HAS_SCRIPT_SURFACE*/

#endif /*COMAC_SCRIPT_H*/
