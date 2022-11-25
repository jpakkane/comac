/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2006 Red Hat, Inc
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
 * Author(s):
 *	Kristian Høgsberg <krh@redhat.com>
 */

#ifndef COMAC_OUTPUT_STREAM_PRIVATE_H
#define COMAC_OUTPUT_STREAM_PRIVATE_H

#include "comac-compiler-private.h"
#include "comac-types-private.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

typedef comac_status_t
(*comac_output_stream_write_func_t) (comac_output_stream_t *output_stream,
				     const unsigned char   *data,
				     unsigned int           length);

typedef comac_status_t
(*comac_output_stream_flush_func_t) (comac_output_stream_t *output_stream);

typedef comac_status_t
(*comac_output_stream_close_func_t) (comac_output_stream_t *output_stream);

struct _comac_output_stream {
    comac_output_stream_write_func_t write_func;
    comac_output_stream_flush_func_t flush_func;
    comac_output_stream_close_func_t close_func;
    long long		             position;
    comac_status_t		     status;
    comac_bool_t		     closed;
};

extern const comac_private comac_output_stream_t _comac_output_stream_nil;

comac_private void
_comac_output_stream_init (comac_output_stream_t            *stream,
			   comac_output_stream_write_func_t  write_func,
			   comac_output_stream_flush_func_t  flush_func,
			   comac_output_stream_close_func_t  close_func);

comac_private comac_status_t
_comac_output_stream_fini (comac_output_stream_t *stream);


/* We already have the following declared in comac.h:

typedef comac_status_t (*comac_write_func_t) (void		  *closure,
					      const unsigned char *data,
					      unsigned int	   length);
*/
typedef comac_status_t (*comac_close_func_t) (void *closure);


/* This function never returns %NULL. If an error occurs (NO_MEMORY)
 * while trying to create the output stream this function returns a
 * valid pointer to a nil output stream.
 *
 * Note that even with a nil surface, the close_func callback will be
 * called by a call to _comac_output_stream_close or
 * _comac_output_stream_destroy.
 */
comac_private comac_output_stream_t *
_comac_output_stream_create (comac_write_func_t		write_func,
			     comac_close_func_t		close_func,
			     void			*closure);

comac_private comac_output_stream_t *
_comac_output_stream_create_in_error (comac_status_t status);

/* Tries to flush any buffer maintained by the stream or its delegates. */
comac_private comac_status_t
_comac_output_stream_flush (comac_output_stream_t *stream);

/* Returns the final status value associated with this object, just
 * before its last gasp. This final status value will capture any
 * status failure returned by the stream's close_func as well. */
comac_private comac_status_t
_comac_output_stream_close (comac_output_stream_t *stream);

/* Returns the final status value associated with this object, just
 * before its last gasp. This final status value will capture any
 * status failure returned by the stream's close_func as well. */
comac_private comac_status_t
_comac_output_stream_destroy (comac_output_stream_t *stream);

comac_private void
_comac_output_stream_write (comac_output_stream_t *stream,
			    const void *data, size_t length);

comac_private void
_comac_output_stream_write_hex_string (comac_output_stream_t *stream,
				       const unsigned char *data,
				       size_t length);

comac_private void
_comac_output_stream_vprintf (comac_output_stream_t *stream,
			      const char *fmt,
			      va_list ap) COMAC_PRINTF_FORMAT ( 2, 0);

comac_private void
_comac_output_stream_printf (comac_output_stream_t *stream,
			     const char *fmt,
			     ...) COMAC_PRINTF_FORMAT (2, 3);

/* Print matrix element values with rounding of insignificant digits. */
comac_private void
_comac_output_stream_print_matrix (comac_output_stream_t *stream,
				   const comac_matrix_t  *matrix);

comac_private long long
_comac_output_stream_get_position (comac_output_stream_t *stream);

comac_private comac_status_t
_comac_output_stream_get_status (comac_output_stream_t *stream);

/* This function never returns %NULL. If an error occurs (NO_MEMORY or
 * WRITE_ERROR) while trying to create the output stream this function
 * returns a valid pointer to a nil output stream.
 *
 * Note: Even if a nil surface is returned, the caller should still
 * call _comac_output_stream_destroy (or _comac_output_stream_close at
 * least) in order to ensure that everything is properly cleaned up.
 */
comac_private comac_output_stream_t *
_comac_output_stream_create_for_filename (const char *filename);

/* This function never returns %NULL. If an error occurs (NO_MEMORY or
 * WRITE_ERROR) while trying to create the output stream this function
 * returns a valid pointer to a nil output stream.
 *
 * The caller still "owns" file and is responsible for calling fclose
 * on it when finished. The stream will not do this itself.
 */
comac_private comac_output_stream_t *
_comac_output_stream_create_for_file (FILE *file);

comac_private comac_output_stream_t *
_comac_memory_stream_create (void);

comac_private void
_comac_memory_stream_copy (comac_output_stream_t *base,
			   comac_output_stream_t *dest);

comac_private int
_comac_memory_stream_length (comac_output_stream_t *stream);

comac_private comac_status_t
_comac_memory_stream_destroy (comac_output_stream_t *abstract_stream,
			      unsigned char **data_out,
			      unsigned long *length_out);

comac_private comac_output_stream_t *
_comac_null_stream_create (void);

/* comac-base85-stream.c */
comac_private comac_output_stream_t *
_comac_base85_stream_create (comac_output_stream_t *output);

/* comac-base64-stream.c */
comac_private comac_output_stream_t *
_comac_base64_stream_create (comac_output_stream_t *output);

/* comac-deflate-stream.c */
comac_private comac_output_stream_t *
_comac_deflate_stream_create (comac_output_stream_t *output);


#endif /* COMAC_OUTPUT_STREAM_PRIVATE_H */
