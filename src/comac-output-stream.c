/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac-output-stream.c: Output stream abstraction
 *
 * Copyright © 2005 Red Hat, Inc
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

#define _DEFAULT_SOURCE /* for snprintf() */
#include "comacint.h"

#include "comac-output-stream-private.h"

#include "comac-array-private.h"
#include "comac-error-private.h"
#include "comac-compiler-private.h"

#include <stdio.h>
#include <errno.h>

/* Numbers printed with %f are printed with this number of significant
 * digits after the decimal.
 */
#define SIGNIFICANT_DIGITS_AFTER_DECIMAL 6

/* Numbers printed with %g are assumed to only have %COMAC_FIXED_FRAC_BITS
 * bits of precision available after the decimal point.
 *
 * FIXED_POINT_DECIMAL_DIGITS specifies the minimum number of decimal
 * digits after the decimal point required to preserve the available
 * precision.
 *
 * The conversion is:
 *
 * <programlisting>
 * FIXED_POINT_DECIMAL_DIGITS = ceil( COMAC_FIXED_FRAC_BITS * ln(2)/ln(10) )
 * </programlisting>
 *
 * We can replace ceil(x) with (int)(x+1) since x will never be an
 * integer for any likely value of %COMAC_FIXED_FRAC_BITS.
 */
#define FIXED_POINT_DECIMAL_DIGITS ((int)(COMAC_FIXED_FRAC_BITS*0.301029996 + 1))

void
_comac_output_stream_init (comac_output_stream_t            *stream,
			   comac_output_stream_write_func_t  write_func,
			   comac_output_stream_flush_func_t  flush_func,
			   comac_output_stream_close_func_t  close_func)
{
    stream->write_func = write_func;
    stream->flush_func = flush_func;
    stream->close_func = close_func;
    stream->position = 0;
    stream->status = COMAC_STATUS_SUCCESS;
    stream->closed = FALSE;
}

comac_status_t
_comac_output_stream_fini (comac_output_stream_t *stream)
{
    return _comac_output_stream_close (stream);
}

const comac_output_stream_t _comac_output_stream_nil = {
    NULL, /* write_func */
    NULL, /* flush_func */
    NULL, /* close_func */
    0,    /* position */
    COMAC_STATUS_NO_MEMORY,
    FALSE /* closed */
};

static const comac_output_stream_t _comac_output_stream_nil_write_error = {
    NULL, /* write_func */
    NULL, /* flush_func */
    NULL, /* close_func */
    0,    /* position */
    COMAC_STATUS_WRITE_ERROR,
    FALSE /* closed */
};

typedef struct _comac_output_stream_with_closure {
    comac_output_stream_t	 base;
    comac_write_func_t		 write_func;
    comac_close_func_t		 close_func;
    void			*closure;
} comac_output_stream_with_closure_t;


static comac_status_t
closure_write (comac_output_stream_t *stream,
	       const unsigned char *data, unsigned int length)
{
    comac_output_stream_with_closure_t *stream_with_closure =
	(comac_output_stream_with_closure_t *) stream;

    if (stream_with_closure->write_func == NULL)
	return COMAC_STATUS_SUCCESS;

    return stream_with_closure->write_func (stream_with_closure->closure,
					    data, length);
}

static comac_status_t
closure_close (comac_output_stream_t *stream)
{
    comac_output_stream_with_closure_t *stream_with_closure =
	(comac_output_stream_with_closure_t *) stream;

    if (stream_with_closure->close_func != NULL)
	return stream_with_closure->close_func (stream_with_closure->closure);
    else
	return COMAC_STATUS_SUCCESS;
}

comac_output_stream_t *
_comac_output_stream_create (comac_write_func_t		write_func,
			     comac_close_func_t		close_func,
			     void			*closure)
{
    comac_output_stream_with_closure_t *stream;

    stream = _comac_malloc (sizeof (comac_output_stream_with_closure_t));
    if (unlikely (stream == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    }

    _comac_output_stream_init (&stream->base,
			       closure_write, NULL, closure_close);
    stream->write_func = write_func;
    stream->close_func = close_func;
    stream->closure = closure;

    return &stream->base;
}

comac_output_stream_t *
_comac_output_stream_create_in_error (comac_status_t status)
{
    comac_output_stream_t *stream;

    /* check for the common ones */
    if (status == COMAC_STATUS_NO_MEMORY)
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    if (status == COMAC_STATUS_WRITE_ERROR)
	return (comac_output_stream_t *) &_comac_output_stream_nil_write_error;

    stream = _comac_malloc (sizeof (comac_output_stream_t));
    if (unlikely (stream == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    }

    _comac_output_stream_init (stream, NULL, NULL, NULL);
    stream->status = status;

    return stream;
}

comac_status_t
_comac_output_stream_flush (comac_output_stream_t *stream)
{
    comac_status_t status;

    if (stream->closed)
	return stream->status;

    if (stream == &_comac_output_stream_nil ||
	stream == &_comac_output_stream_nil_write_error)
    {
	return stream->status;
    }

    if (stream->flush_func) {
	status = stream->flush_func (stream);
	/* Don't overwrite a pre-existing status failure. */
	if (stream->status == COMAC_STATUS_SUCCESS)
	    stream->status = status;
    }

    return stream->status;
}

comac_status_t
_comac_output_stream_close (comac_output_stream_t *stream)
{
    comac_status_t status;

    if (stream->closed)
	return stream->status;

    if (stream == &_comac_output_stream_nil ||
	stream == &_comac_output_stream_nil_write_error)
    {
	return stream->status;
    }

    if (stream->close_func) {
	status = stream->close_func (stream);
	/* Don't overwrite a pre-existing status failure. */
	if (stream->status == COMAC_STATUS_SUCCESS)
	    stream->status = status;
    }

    stream->closed = TRUE;

    return stream->status;
}

comac_status_t
_comac_output_stream_destroy (comac_output_stream_t *stream)
{
    comac_status_t status;

    assert (stream != NULL);

    if (stream == &_comac_output_stream_nil ||
	stream == &_comac_output_stream_nil_write_error)
    {
	return stream->status;
    }

    status = _comac_output_stream_fini (stream);
    free (stream);

    return status;
}

void
_comac_output_stream_write (comac_output_stream_t *stream,
			    const void *data, size_t length)
{
    if (length == 0)
	return;

    if (stream->status)
	return;

    stream->status = stream->write_func (stream, data, length);
    stream->position += length;
}

void
_comac_output_stream_write_hex_string (comac_output_stream_t *stream,
				       const unsigned char *data,
				       size_t length)
{
    const char hex_chars[] = "0123456789abcdef";
    char buffer[2];
    unsigned int i, column;

    if (stream->status)
	return;

    for (i = 0, column = 0; i < length; i++, column++) {
	if (column == 38) {
	    _comac_output_stream_write (stream, "\n", 1);
	    column = 0;
	}
	buffer[0] = hex_chars[(data[i] >> 4) & 0x0f];
	buffer[1] = hex_chars[data[i] & 0x0f];
	_comac_output_stream_write (stream, buffer, 2);
    }
}

/* Format a double in a locale independent way and trim trailing
 * zeros.  Based on code from Alex Larson <alexl@redhat.com>.
 * https://mail.gnome.org/archives/gtk-devel-list/2001-October/msg00087.html
 *
 * The code in the patch is copyright Red Hat, Inc under the LGPL, but
 * has been relicensed under the LGPL/MPL dual license for inclusion
 * into comac (see COPYING). -- Kristian Høgsberg <krh@redhat.com>
 */
static void
_comac_dtostr (char *buffer, size_t size, double d, comac_bool_t limited_precision)
{
    const char *decimal_point;
    int decimal_point_len;
    char *p;
    int decimal_len;
    int num_zeros, decimal_digits;

    /* Omit the minus sign from negative zero. */
    if (d == 0.0)
	d = 0.0;

    decimal_point = _comac_get_locale_decimal_point ();
    decimal_point_len = strlen (decimal_point);

    assert (decimal_point_len != 0);

    if (limited_precision) {
	snprintf (buffer, size, "%.*f", FIXED_POINT_DECIMAL_DIGITS, d);
    } else {
	/* Using "%f" to print numbers less than 0.1 will result in
	 * reduced precision due to the default 6 digits after the
	 * decimal point.
	 *
	 * For numbers is < 0.1, we print with maximum precision and count
	 * the number of zeros between the decimal point and the first
	 * significant digit. We then print the number again with the
	 * number of decimal places that gives us the required number of
	 * significant digits. This ensures the number is correctly
	 * rounded.
	 */
	if (fabs (d) >= 0.1) {
	    snprintf (buffer, size, "%f", d);
	} else {
	    snprintf (buffer, size, "%.18f", d);
	    p = buffer;

	    if (*p == '+' || *p == '-')
		p++;

	    while (_comac_isdigit (*p))
		p++;

	    if (strncmp (p, decimal_point, decimal_point_len) == 0)
		p += decimal_point_len;

	    num_zeros = 0;
	    while (*p++ == '0')
		num_zeros++;

	    decimal_digits = num_zeros + SIGNIFICANT_DIGITS_AFTER_DECIMAL;

	    if (decimal_digits < 18)
		snprintf (buffer, size, "%.*f", decimal_digits, d);
	}
    }
    p = buffer;

    if (*p == '+' || *p == '-')
	p++;

    while (_comac_isdigit (*p))
	p++;

    if (strncmp (p, decimal_point, decimal_point_len) == 0) {
	*p = '.';
	decimal_len = strlen (p + decimal_point_len);
	memmove (p + 1, p + decimal_point_len, decimal_len);
	p[1 + decimal_len] = 0;

	/* Remove trailing zeros and decimal point if possible. */
	for (p = p + decimal_len; *p == '0'; p--)
	    *p = 0;

	if (*p == '.') {
	    *p = 0;
	    p--;
	}
    }
}

enum {
    LENGTH_MODIFIER_LONG = 0x100,
    LENGTH_MODIFIER_LONG_LONG = 0x200
};

/* Here's a limited reimplementation of printf.  The reason for doing
 * this is primarily to special case handling of doubles.  We want
 * locale independent formatting of doubles and we want to trim
 * trailing zeros.  This is handled by dtostr() above, and the code
 * below handles everything else by calling snprintf() to do the
 * formatting.  This functionality is only for internal use and we
 * only implement the formats we actually use.
 */
void
_comac_output_stream_vprintf (comac_output_stream_t *stream,
			      const char *fmt, va_list ap)
{
#define SINGLE_FMT_BUFFER_SIZE 32
    char buffer[512], single_fmt[SINGLE_FMT_BUFFER_SIZE];
    int single_fmt_length;
    char *p;
    const char *f, *start;
    int length_modifier, width;
    comac_bool_t var_width;

    if (stream->status)
	return;

    f = fmt;
    p = buffer;
    while (*f != '\0') {
	if (p == buffer + sizeof (buffer)) {
	    _comac_output_stream_write (stream, buffer, sizeof (buffer));
	    p = buffer;
	}

	if (*f != '%') {
	    *p++ = *f++;
	    continue;
	}

	start = f;
	f++;

	if (*f == '0')
	    f++;

        var_width = FALSE;
        if (*f == '*') {
            var_width = TRUE;
	    f++;
        }

	while (_comac_isdigit (*f))
	    f++;

	length_modifier = 0;
	if (*f == 'l') {
	    length_modifier = LENGTH_MODIFIER_LONG;
	    f++;
	    if (*f == 'l') {
		length_modifier = LENGTH_MODIFIER_LONG_LONG;
		f++;
	    }
	}

	/* The only format strings exist in the comac implementation
	 * itself. So there's an internal consistency problem if any
	 * of them is larger than our format buffer size. */
	single_fmt_length = f - start + 1;
	assert (single_fmt_length + 1 <= SINGLE_FMT_BUFFER_SIZE);

	/* Reuse the format string for this conversion. */
	memcpy (single_fmt, start, single_fmt_length);
	single_fmt[single_fmt_length] = '\0';

	/* Flush contents of buffer before snprintf()'ing into it. */
	_comac_output_stream_write (stream, buffer, p - buffer);

	/* We group signed and unsigned together in this switch, the
	 * only thing that matters here is the size of the arguments,
	 * since we're just passing the data through to sprintf(). */
	switch (*f | length_modifier) {
	case '%':
	    buffer[0] = *f;
	    buffer[1] = 0;
	    break;
	case 'd':
	case 'u':
	case 'o':
	case 'x':
	case 'X':
            if (var_width) {
                width = va_arg (ap, int);
                snprintf (buffer, sizeof buffer,
                          single_fmt, width, va_arg (ap, int));
            } else {
                snprintf (buffer, sizeof buffer, single_fmt, va_arg (ap, int));
            }
	    break;
	case 'd' | LENGTH_MODIFIER_LONG:
	case 'u' | LENGTH_MODIFIER_LONG:
	case 'o' | LENGTH_MODIFIER_LONG:
	case 'x' | LENGTH_MODIFIER_LONG:
	case 'X' | LENGTH_MODIFIER_LONG:
            if (var_width) {
                width = va_arg (ap, int);
                snprintf (buffer, sizeof buffer,
                          single_fmt, width, va_arg (ap, long int));
            } else {
                snprintf (buffer, sizeof buffer,
                          single_fmt, va_arg (ap, long int));
            }
	    break;
	case 'd' | LENGTH_MODIFIER_LONG_LONG:
	case 'u' | LENGTH_MODIFIER_LONG_LONG:
	case 'o' | LENGTH_MODIFIER_LONG_LONG:
	case 'x' | LENGTH_MODIFIER_LONG_LONG:
	case 'X' | LENGTH_MODIFIER_LONG_LONG:
	    if (var_width) {
		width = va_arg (ap, int);
		snprintf (buffer, sizeof buffer,
			  single_fmt, width, va_arg (ap, long long int));
	    } else {
		snprintf (buffer, sizeof buffer,
			  single_fmt, va_arg (ap, long long int));
	    }
	    break;
	case 's': {
	    /* Write out strings as they may be larger than the buffer. */
	    const char *s = va_arg (ap, const char *);
	    int len = strlen(s);
	    _comac_output_stream_write (stream, s, len);
	    buffer[0] = 0;
	    }
	    break;
	case 'f':
	    _comac_dtostr (buffer, sizeof buffer, va_arg (ap, double), FALSE);
	    break;
	case 'g':
	    _comac_dtostr (buffer, sizeof buffer, va_arg (ap, double), TRUE);
	    break;
	case 'c':
	    buffer[0] = va_arg (ap, int);
	    buffer[1] = 0;
	    break;
	default:
	    ASSERT_NOT_REACHED;
	}
	p = buffer + strlen (buffer);
	f++;
    }

    _comac_output_stream_write (stream, buffer, p - buffer);
}

void
_comac_output_stream_printf (comac_output_stream_t *stream,
			     const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);

    _comac_output_stream_vprintf (stream, fmt, ap);

    va_end (ap);
}

/* Matrix elements that are smaller than the value of the largest element * MATRIX_ROUNDING_TOLERANCE
 * are rounded down to zero. */
#define MATRIX_ROUNDING_TOLERANCE 1e-12

void
_comac_output_stream_print_matrix (comac_output_stream_t *stream,
				   const comac_matrix_t  *matrix)
{
    comac_matrix_t m;
    double s, e;

    m = *matrix;
    s = fabs (m.xx);
    if (fabs (m.xy) > s)
	s = fabs (m.xy);
    if (fabs (m.yx) > s)
	s = fabs (m.yx);
    if (fabs (m.yy) > s)
	s = fabs (m.yy);

    e = s * MATRIX_ROUNDING_TOLERANCE;
    if (fabs(m.xx) < e)
	m.xx = 0;
    if (fabs(m.xy) < e)
	m.xy = 0;
    if (fabs(m.yx) < e)
	m.yx = 0;
    if (fabs(m.yy) < e)
	m.yy = 0;
    if (fabs(m.x0) < e)
	m.x0 = 0;
    if (fabs(m.y0) < e)
	m.y0 = 0;

    _comac_output_stream_printf (stream,
				 "%f %f %f %f %f %f",
				 m.xx, m.yx, m.xy, m.yy, m.x0, m.y0);
}

long long
_comac_output_stream_get_position (comac_output_stream_t *stream)
{
    return stream->position;
}

comac_status_t
_comac_output_stream_get_status (comac_output_stream_t *stream)
{
    return stream->status;
}

/* Maybe this should be a configure time option, so embedded targets
 * don't have to pull in stdio. */


typedef struct _stdio_stream {
    comac_output_stream_t	 base;
    FILE			*file;
} stdio_stream_t;

static comac_status_t
stdio_write (comac_output_stream_t *base,
	     const unsigned char *data, unsigned int length)
{
    stdio_stream_t *stream = (stdio_stream_t *) base;

    if (fwrite (data, 1, length, stream->file) != length)
	return _comac_error (COMAC_STATUS_WRITE_ERROR);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
stdio_flush (comac_output_stream_t *base)
{
    stdio_stream_t *stream = (stdio_stream_t *) base;

    fflush (stream->file);

    if (ferror (stream->file))
	return _comac_error (COMAC_STATUS_WRITE_ERROR);
    else
	return COMAC_STATUS_SUCCESS;
}

static comac_status_t
stdio_close (comac_output_stream_t *base)
{
    comac_status_t status;
    stdio_stream_t *stream = (stdio_stream_t *) base;

    status = stdio_flush (base);

    fclose (stream->file);

    return status;
}

comac_output_stream_t *
_comac_output_stream_create_for_file (FILE *file)
{
    stdio_stream_t *stream;

    if (file == NULL) {
	_comac_error_throw (COMAC_STATUS_WRITE_ERROR);
	return (comac_output_stream_t *) &_comac_output_stream_nil_write_error;
    }

    stream = _comac_malloc (sizeof *stream);
    if (unlikely (stream == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    }

    _comac_output_stream_init (&stream->base,
			       stdio_write, stdio_flush, stdio_flush);
    stream->file = file;

    return &stream->base;
}

comac_output_stream_t *
_comac_output_stream_create_for_filename (const char *filename)
{
    stdio_stream_t *stream;
    FILE *file;
    comac_status_t status;

    if (filename == NULL)
	return _comac_null_stream_create ();

    status = _comac_fopen (filename, "wb", &file);

    if (status != COMAC_STATUS_SUCCESS)
	return _comac_output_stream_create_in_error (status);

    if (file == NULL) {
	switch (errno) {
	case ENOMEM:
	    _comac_error_throw (COMAC_STATUS_NO_MEMORY);
	    return (comac_output_stream_t *) &_comac_output_stream_nil;
	default:
	    _comac_error_throw (COMAC_STATUS_WRITE_ERROR);
	    return (comac_output_stream_t *) &_comac_output_stream_nil_write_error;
	}
    }

    stream = _comac_malloc (sizeof *stream);
    if (unlikely (stream == NULL)) {
	fclose (file);
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    }

    _comac_output_stream_init (&stream->base,
			       stdio_write, stdio_flush, stdio_close);
    stream->file = file;

    return &stream->base;
}


typedef struct _memory_stream {
    comac_output_stream_t	base;
    comac_array_t		array;
} memory_stream_t;

static comac_status_t
memory_write (comac_output_stream_t *base,
	      const unsigned char *data, unsigned int length)
{
    memory_stream_t *stream = (memory_stream_t *) base;

    return _comac_array_append_multiple (&stream->array, data, length);
}

static comac_status_t
memory_close (comac_output_stream_t *base)
{
    memory_stream_t *stream = (memory_stream_t *) base;

    _comac_array_fini (&stream->array);

    return COMAC_STATUS_SUCCESS;
}

comac_output_stream_t *
_comac_memory_stream_create (void)
{
    memory_stream_t *stream;

    stream = _comac_malloc (sizeof *stream);
    if (unlikely (stream == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    }

    _comac_output_stream_init (&stream->base, memory_write, NULL, memory_close);
    _comac_array_init (&stream->array, 1);

    return &stream->base;
}

comac_status_t
_comac_memory_stream_destroy (comac_output_stream_t *abstract_stream,
			      unsigned char **data_out,
			      unsigned long *length_out)
{
    memory_stream_t *stream;
    comac_status_t status;

    status = abstract_stream->status;
    if (unlikely (status))
	return _comac_output_stream_destroy (abstract_stream);

    stream = (memory_stream_t *) abstract_stream;

    *length_out = _comac_array_num_elements (&stream->array);
    *data_out = _comac_malloc (*length_out);
    if (unlikely (*data_out == NULL)) {
	status = _comac_output_stream_destroy (abstract_stream);
	assert (status == COMAC_STATUS_SUCCESS);
	return _comac_error (COMAC_STATUS_NO_MEMORY);
    }
    memcpy (*data_out, _comac_array_index (&stream->array, 0), *length_out);

    return _comac_output_stream_destroy (abstract_stream);
}

void
_comac_memory_stream_copy (comac_output_stream_t *base,
			   comac_output_stream_t *dest)
{
    memory_stream_t *stream = (memory_stream_t *) base;

    if (dest->status)
	return;

    if (base->status) {
	dest->status = base->status;
	return;
    }

    _comac_output_stream_write (dest,
				_comac_array_index (&stream->array, 0),
				_comac_array_num_elements (&stream->array));
}

int
_comac_memory_stream_length (comac_output_stream_t *base)
{
    memory_stream_t *stream = (memory_stream_t *) base;

    return _comac_array_num_elements (&stream->array);
}

static comac_status_t
null_write (comac_output_stream_t *base,
	    const unsigned char *data, unsigned int length)
{
    return COMAC_STATUS_SUCCESS;
}

comac_output_stream_t *
_comac_null_stream_create (void)
{
    comac_output_stream_t *stream;

    stream = _comac_malloc (sizeof *stream);
    if (unlikely (stream == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    }

    _comac_output_stream_init (stream, null_write, NULL, NULL);

    return stream;
}
