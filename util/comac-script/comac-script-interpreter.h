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

#ifndef COMAC_SCRIPT_INTERPRETER_H
#define COMAC_SCRIPT_INTERPRETER_H

#include <comac.h>
#include <stdio.h>

COMAC_BEGIN_DECLS

typedef struct _comac_script_interpreter comac_script_interpreter_t;

/* XXX expose csi_dictionary_t and pass to hooks */
typedef void
(*csi_destroy_func_t) (void *closure,
		       void *ptr);

typedef comac_surface_t *
(*csi_surface_create_func_t) (void *closure,
			      comac_content_t content,
			      double width,
			      double height,
			      long uid);
typedef comac_t *
(*csi_context_create_func_t) (void *closure,
			      comac_surface_t *surface);
typedef void
(*csi_show_page_func_t) (void *closure,
			 comac_t *cr);

typedef void
(*csi_copy_page_func_t) (void *closure,
			 comac_t *cr);

typedef comac_surface_t *
(*csi_create_source_image_t) (void *closure,
			      comac_format_t format,
			      int width, int height,
			      long uid);

typedef struct _comac_script_interpreter_hooks {
    void *closure;
    csi_surface_create_func_t surface_create;
    csi_destroy_func_t surface_destroy;
    csi_context_create_func_t context_create;
    csi_destroy_func_t context_destroy;
    csi_show_page_func_t show_page;
    csi_copy_page_func_t copy_page;
    csi_create_source_image_t create_source_image;
} comac_script_interpreter_hooks_t;

comac_public comac_script_interpreter_t *
comac_script_interpreter_create (void);

comac_public void
comac_script_interpreter_install_hooks (comac_script_interpreter_t *ctx,
					const comac_script_interpreter_hooks_t *hooks);

comac_public comac_status_t
comac_script_interpreter_run (comac_script_interpreter_t *ctx,
			      const char *filename);

comac_public comac_status_t
comac_script_interpreter_feed_stream (comac_script_interpreter_t *ctx,
				      FILE *stream);

comac_public comac_status_t
comac_script_interpreter_feed_string (comac_script_interpreter_t *ctx,
				      const char *line,
				      int len);

comac_public unsigned int
comac_script_interpreter_get_line_number (comac_script_interpreter_t *ctx);

comac_public comac_script_interpreter_t *
comac_script_interpreter_reference (comac_script_interpreter_t *ctx);

comac_public comac_status_t
comac_script_interpreter_finish (comac_script_interpreter_t *ctx);

comac_public comac_status_t
comac_script_interpreter_destroy (comac_script_interpreter_t *ctx);

comac_public comac_status_t
comac_script_interpreter_translate_stream (FILE *stream,
	                                   comac_write_func_t write_func,
					   void *closure);

COMAC_END_DECLS

#endif /*COMAC_SCRIPT_INTERPRETER_H*/
