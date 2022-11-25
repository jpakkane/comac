/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc
 * Copyright © 2006 Red Hat, Inc
 * Copyright © 2007, 2008 Adrian Johnson
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
 *	Kristian Høgsberg <krh@redhat.com>
 *	Carl Worth <cworth@cworth.org>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#include "comacint.h"

#if COMAC_HAS_PDF_OPERATORS

#include "comac-error-private.h"
#include "comac-pdf-operators-private.h"
#include "comac-path-fixed-private.h"
#include "comac-output-stream-private.h"
#include "comac-scaled-font-subsets-private.h"

static comac_status_t
_comac_pdf_operators_end_text (comac_pdf_operators_t *pdf_operators);

void
_comac_pdf_operators_init (comac_pdf_operators_t *pdf_operators,
			   comac_output_stream_t *stream,
			   comac_matrix_t *comac_to_pdf,
			   comac_scaled_font_subsets_t *font_subsets,
			   comac_bool_t ps)
{
    pdf_operators->stream = stream;
    pdf_operators->comac_to_pdf = *comac_to_pdf;
    pdf_operators->font_subsets = font_subsets;
    pdf_operators->ps_output = ps;
    pdf_operators->use_font_subset = NULL;
    pdf_operators->use_font_subset_closure = NULL;
    pdf_operators->in_text_object = FALSE;
    pdf_operators->num_glyphs = 0;
    pdf_operators->has_line_style = FALSE;
    pdf_operators->use_actual_text = FALSE;
}

comac_status_t
_comac_pdf_operators_fini (comac_pdf_operators_t *pdf_operators)
{
    return _comac_pdf_operators_flush (pdf_operators);
}

void
_comac_pdf_operators_set_font_subsets_callback (
    comac_pdf_operators_t *pdf_operators,
    comac_pdf_operators_use_font_subset_t use_font_subset,
    void *closure)
{
    pdf_operators->use_font_subset = use_font_subset;
    pdf_operators->use_font_subset_closure = closure;
}

/* Change the output stream to a different stream.
 * _comac_pdf_operators_flush() should always be called before calling
 * this function.
 */
void
_comac_pdf_operators_set_stream (comac_pdf_operators_t *pdf_operators,
				 comac_output_stream_t *stream)
{
    pdf_operators->stream = stream;
    pdf_operators->has_line_style = FALSE;
}

void
_comac_pdf_operators_set_comac_to_pdf_matrix (
    comac_pdf_operators_t *pdf_operators, comac_matrix_t *comac_to_pdf)
{
    pdf_operators->comac_to_pdf = *comac_to_pdf;
    pdf_operators->has_line_style = FALSE;
}

comac_private void
_comac_pdf_operators_enable_actual_text (comac_pdf_operators_t *pdf_operators,
					 comac_bool_t enable)
{
    pdf_operators->use_actual_text = enable;
}

/* Finish writing out any pending commands to the stream. This
 * function must be called by the surface before emitting anything
 * into the PDF stream.
 *
 * pdf_operators may leave the emitted PDF for some operations
 * unfinished in case subsequent operations can be merged. This
 * function will finish off any incomplete operation so the stream
 * will be in a state where the surface may emit its own PDF
 * operations (eg changing patterns).
 *
 */
comac_status_t
_comac_pdf_operators_flush (comac_pdf_operators_t *pdf_operators)
{
    comac_status_t status = COMAC_STATUS_SUCCESS;

    if (pdf_operators->in_text_object)
	status = _comac_pdf_operators_end_text (pdf_operators);

    return status;
}

/* Reset the known graphics state of the PDF consumer. ie no
 * assumptions will be made about the state. The next time a
 * particular graphics state is required (eg line width) the state
 * operator is always emitted and then remembered for subsequent
 * operations.
 *
 * This should be called when starting a new stream or after emitting
 * the 'Q' operator (where pdf-operators functions were called inside
 * the q/Q pair).
 */
void
_comac_pdf_operators_reset (comac_pdf_operators_t *pdf_operators)
{
    pdf_operators->has_line_style = FALSE;
}

/* A word wrap stream can be used as a filter to do word wrapping on
 * top of an existing output stream. The word wrapping is quite
 * simple, using isspace to determine characters that separate
 * words. Any word that will cause the column count exceed the given
 * max_column will have a '\n' character emitted before it.
 *
 * The stream is careful to maintain integrity for words that cross
 * the boundary from one call to write to the next.
 *
 * Note: This stream does not guarantee that the output will never
 * exceed max_column. In particular, if a single word is larger than
 * max_column it will not be broken up.
 */

typedef enum _comac_word_wrap_state {
    WRAP_STATE_DELIMITER,
    WRAP_STATE_WORD,
    WRAP_STATE_STRING,
    WRAP_STATE_HEXSTRING
} comac_word_wrap_state_t;

typedef struct _word_wrap_stream {
    comac_output_stream_t base;
    comac_output_stream_t *output;
    int max_column;
    comac_bool_t ps_output;
    int column;
    comac_word_wrap_state_t state;
    comac_bool_t in_escape;
    int escape_digits;
} word_wrap_stream_t;

/* Emit word bytes up to the next delimiter character */
static int
_word_wrap_stream_count_word_up_to (word_wrap_stream_t *stream,
				    const unsigned char *data,
				    int length)
{
    const unsigned char *s = data;
    int count = 0;

    while (length--) {
	if (_comac_isspace (*s) || *s == '<' || *s == '(') {
	    stream->state = WRAP_STATE_DELIMITER;
	    break;
	}

	count++;
	stream->column++;
	s++;
    }

    if (count)
	_comac_output_stream_write (stream->output, data, count);

    return count;
}

/* Emit hexstring bytes up to either the end of the ASCII hexstring or the number
 * of columns remaining.
 */
static int
_word_wrap_stream_count_hexstring_up_to (word_wrap_stream_t *stream,
					 const unsigned char *data,
					 int length)
{
    const unsigned char *s = data;
    int count = 0;
    comac_bool_t newline = FALSE;

    while (length--) {
	count++;
	stream->column++;
	if (*s == '>') {
	    stream->state = WRAP_STATE_DELIMITER;
	    break;
	}

	if (stream->column > stream->max_column) {
	    newline = TRUE;
	    break;
	}
	s++;
    }

    if (count)
	_comac_output_stream_write (stream->output, data, count);

    if (newline) {
	_comac_output_stream_printf (stream->output, "\n");
	stream->column = 0;
    }

    return count;
}

/* Count up to either the end of the string or the number of columns
 * remaining.
 */
static int
_word_wrap_stream_count_string_up_to (word_wrap_stream_t *stream,
				      const unsigned char *data,
				      int length)
{
    const unsigned char *s = data;
    int count = 0;
    comac_bool_t newline = FALSE;

    while (length--) {
	count++;
	stream->column++;
	if (! stream->in_escape) {
	    if (*s == ')') {
		stream->state = WRAP_STATE_DELIMITER;
		break;
	    }
	    if (*s == '\\') {
		stream->in_escape = TRUE;
		stream->escape_digits = 0;
	    } else if (stream->ps_output &&
		       stream->column > stream->max_column) {
		newline = TRUE;
		break;
	    }
	} else {
	    if (! _comac_isdigit (*s) || ++stream->escape_digits == 3)
		stream->in_escape = FALSE;
	}
	s++;
    }

    if (count)
	_comac_output_stream_write (stream->output, data, count);

    if (newline) {
	_comac_output_stream_printf (stream->output, "\\\n");
	stream->column = 0;
    }

    return count;
}

static comac_status_t
_word_wrap_stream_write (comac_output_stream_t *base,
			 const unsigned char *data,
			 unsigned int length)
{
    word_wrap_stream_t *stream = (word_wrap_stream_t *) base;
    int count;

    while (length) {
	switch (stream->state) {
	case WRAP_STATE_WORD:
	    count = _word_wrap_stream_count_word_up_to (stream, data, length);
	    break;
	case WRAP_STATE_HEXSTRING:
	    count =
		_word_wrap_stream_count_hexstring_up_to (stream, data, length);
	    break;
	case WRAP_STATE_STRING:
	    count = _word_wrap_stream_count_string_up_to (stream, data, length);
	    break;
	case WRAP_STATE_DELIMITER:
	    count = 1;
	    stream->column++;
	    if (*data == '\n' || stream->column >= stream->max_column) {
		_comac_output_stream_printf (stream->output, "\n");
		stream->column = 0;
	    }
	    if (*data == '<') {
		stream->state = WRAP_STATE_HEXSTRING;
	    } else if (*data == '(') {
		stream->state = WRAP_STATE_STRING;
	    } else if (! _comac_isspace (*data)) {
		stream->state = WRAP_STATE_WORD;
	    }
	    if (*data != '\n')
		_comac_output_stream_write (stream->output, data, 1);
	    break;

	default:
	    ASSERT_NOT_REACHED;
	    count = length;
	    break;
	}
	data += count;
	length -= count;
    }

    return _comac_output_stream_get_status (stream->output);
}

static comac_status_t
_word_wrap_stream_close (comac_output_stream_t *base)
{
    word_wrap_stream_t *stream = (word_wrap_stream_t *) base;

    return _comac_output_stream_get_status (stream->output);
}

static comac_output_stream_t *
_word_wrap_stream_create (comac_output_stream_t *output,
			  comac_bool_t ps,
			  int max_column)
{
    word_wrap_stream_t *stream;

    if (output->status)
	return _comac_output_stream_create_in_error (output->status);

    stream = _comac_malloc (sizeof (word_wrap_stream_t));
    if (unlikely (stream == NULL)) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_output_stream_t *) &_comac_output_stream_nil;
    }

    _comac_output_stream_init (&stream->base,
			       _word_wrap_stream_write,
			       NULL,
			       _word_wrap_stream_close);
    stream->output = output;
    stream->max_column = max_column;
    stream->ps_output = ps;
    stream->column = 0;
    stream->state = WRAP_STATE_DELIMITER;
    stream->in_escape = FALSE;
    stream->escape_digits = 0;

    return &stream->base;
}

typedef struct _pdf_path_info {
    comac_output_stream_t *output;
    comac_matrix_t *path_transform;
    comac_line_cap_t line_cap;
    comac_point_t last_move_to_point;
    comac_bool_t has_sub_path;
} pdf_path_info_t;

static comac_status_t
_comac_pdf_path_move_to (void *closure, const comac_point_t *point)
{
    pdf_path_info_t *info = closure;
    double x = _comac_fixed_to_double (point->x);
    double y = _comac_fixed_to_double (point->y);

    info->last_move_to_point = *point;
    info->has_sub_path = FALSE;
    comac_matrix_transform_point (info->path_transform, &x, &y);
    _comac_output_stream_printf (info->output, "%g %g m ", x, y);

    return _comac_output_stream_get_status (info->output);
}

static comac_status_t
_comac_pdf_path_line_to (void *closure, const comac_point_t *point)
{
    pdf_path_info_t *info = closure;
    double x = _comac_fixed_to_double (point->x);
    double y = _comac_fixed_to_double (point->y);

    if (info->line_cap != COMAC_LINE_CAP_ROUND && ! info->has_sub_path &&
	point->x == info->last_move_to_point.x &&
	point->y == info->last_move_to_point.y) {
	return COMAC_STATUS_SUCCESS;
    }

    info->has_sub_path = TRUE;
    comac_matrix_transform_point (info->path_transform, &x, &y);
    _comac_output_stream_printf (info->output, "%g %g l ", x, y);

    return _comac_output_stream_get_status (info->output);
}

static comac_status_t
_comac_pdf_path_curve_to (void *closure,
			  const comac_point_t *b,
			  const comac_point_t *c,
			  const comac_point_t *d)
{
    pdf_path_info_t *info = closure;
    double bx = _comac_fixed_to_double (b->x);
    double by = _comac_fixed_to_double (b->y);
    double cx = _comac_fixed_to_double (c->x);
    double cy = _comac_fixed_to_double (c->y);
    double dx = _comac_fixed_to_double (d->x);
    double dy = _comac_fixed_to_double (d->y);

    info->has_sub_path = TRUE;
    comac_matrix_transform_point (info->path_transform, &bx, &by);
    comac_matrix_transform_point (info->path_transform, &cx, &cy);
    comac_matrix_transform_point (info->path_transform, &dx, &dy);
    _comac_output_stream_printf (info->output,
				 "%g %g %g %g %g %g c ",
				 bx,
				 by,
				 cx,
				 cy,
				 dx,
				 dy);
    return _comac_output_stream_get_status (info->output);
}

static comac_status_t
_comac_pdf_path_close_path (void *closure)
{
    pdf_path_info_t *info = closure;

    if (info->line_cap != COMAC_LINE_CAP_ROUND && ! info->has_sub_path) {
	return COMAC_STATUS_SUCCESS;
    }

    _comac_output_stream_printf (info->output, "h\n");

    return _comac_output_stream_get_status (info->output);
}

static comac_status_t
_comac_pdf_path_rectangle (pdf_path_info_t *info, comac_box_t *box)
{
    double x1 = _comac_fixed_to_double (box->p1.x);
    double y1 = _comac_fixed_to_double (box->p1.y);
    double x2 = _comac_fixed_to_double (box->p2.x);
    double y2 = _comac_fixed_to_double (box->p2.y);

    comac_matrix_transform_point (info->path_transform, &x1, &y1);
    comac_matrix_transform_point (info->path_transform, &x2, &y2);
    _comac_output_stream_printf (info->output,
				 "%g %g %g %g re ",
				 x1,
				 y1,
				 x2 - x1,
				 y2 - y1);

    return _comac_output_stream_get_status (info->output);
}

/* The line cap value is needed to workaround the fact that PostScript
 * and PDF semantics for stroking degenerate sub-paths do not match
 * comac semantics. (PostScript draws something for any line cap
 * value, while comac draws something only for round caps).
 *
 * When using this function to emit a path to be filled, rather than
 * stroked, simply pass %COMAC_LINE_CAP_ROUND which will guarantee that
 * the stroke workaround will not modify the path being emitted.
 */
static comac_status_t
_comac_pdf_operators_emit_path (comac_pdf_operators_t *pdf_operators,
				const comac_path_fixed_t *path,
				comac_matrix_t *path_transform,
				comac_line_cap_t line_cap)
{
    comac_output_stream_t *word_wrap;
    comac_status_t status, status2;
    pdf_path_info_t info;
    comac_box_t box;

    word_wrap = _word_wrap_stream_create (pdf_operators->stream,
					  pdf_operators->ps_output,
					  72);
    status = _comac_output_stream_get_status (word_wrap);
    if (unlikely (status))
	return _comac_output_stream_destroy (word_wrap);

    info.output = word_wrap;
    info.path_transform = path_transform;
    info.line_cap = line_cap;
    if (_comac_path_fixed_is_rectangle (path, &box) &&
	((path_transform->xx == 0 && path_transform->yy == 0) ||
	 (path_transform->xy == 0 && path_transform->yx == 0))) {
	status = _comac_pdf_path_rectangle (&info, &box);
    } else {
	status = _comac_path_fixed_interpret (path,
					      _comac_pdf_path_move_to,
					      _comac_pdf_path_line_to,
					      _comac_pdf_path_curve_to,
					      _comac_pdf_path_close_path,
					      &info);
    }

    status2 = _comac_output_stream_destroy (word_wrap);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;

    return status;
}

comac_int_status_t
_comac_pdf_operators_clip (comac_pdf_operators_t *pdf_operators,
			   const comac_path_fixed_t *path,
			   comac_fill_rule_t fill_rule)
{
    const char *pdf_operator;
    comac_status_t status;

    if (pdf_operators->in_text_object) {
	status = _comac_pdf_operators_end_text (pdf_operators);
	if (unlikely (status))
	    return status;
    }

    if (! path->has_current_point) {
	/* construct an empty path */
	_comac_output_stream_printf (pdf_operators->stream, "0 0 m ");
    } else {
	status = _comac_pdf_operators_emit_path (pdf_operators,
						 path,
						 &pdf_operators->comac_to_pdf,
						 COMAC_LINE_CAP_ROUND);
	if (unlikely (status))
	    return status;
    }

    switch (fill_rule) {
    default:
	ASSERT_NOT_REACHED;
    case COMAC_FILL_RULE_WINDING:
	pdf_operator = "W";
	break;
    case COMAC_FILL_RULE_EVEN_ODD:
	pdf_operator = "W*";
	break;
    }

    _comac_output_stream_printf (pdf_operators->stream, "%s n\n", pdf_operator);

    return _comac_output_stream_get_status (pdf_operators->stream);
}

static int
_comac_pdf_line_cap (comac_line_cap_t cap)
{
    switch (cap) {
    case COMAC_LINE_CAP_BUTT:
	return 0;
    case COMAC_LINE_CAP_ROUND:
	return 1;
    case COMAC_LINE_CAP_SQUARE:
	return 2;
    default:
	ASSERT_NOT_REACHED;
	return 0;
    }
}

static int
_comac_pdf_line_join (comac_line_join_t join)
{
    switch (join) {
    case COMAC_LINE_JOIN_MITER:
	return 0;
    case COMAC_LINE_JOIN_ROUND:
	return 1;
    case COMAC_LINE_JOIN_BEVEL:
	return 2;
    default:
	ASSERT_NOT_REACHED;
	return 0;
    }
}

comac_int_status_t
_comac_pdf_operators_emit_stroke_style (comac_pdf_operators_t *pdf_operators,
					const comac_stroke_style_t *style,
					double scale)
{
    double *dash = style->dash;
    int num_dashes = style->num_dashes;
    double dash_offset = style->dash_offset;
    double line_width = style->line_width * scale;

    /* PostScript has "special needs" when it comes to zero-length
     * dash segments with butt caps. It apparently (at least
     * according to ghostscript) draws hairlines for this
     * case. That's not what the comac semantics want, so we first
     * touch up the array to eliminate any 0.0 values that will
     * result in "on" segments.
     */
    if (num_dashes && style->line_cap == COMAC_LINE_CAP_BUTT) {
	int i;

	/* If there's an odd number of dash values they will each get
	 * interpreted as both on and off. So we first explicitly
	 * expand the array to remove the duplicate usage so that we
	 * can modify some of the values.
	 */
	if (num_dashes % 2) {
	    dash = _comac_malloc_abc (num_dashes, 2, sizeof (double));
	    if (unlikely (dash == NULL))
		return _comac_error (COMAC_STATUS_NO_MEMORY);

	    memcpy (dash, style->dash, num_dashes * sizeof (double));
	    memcpy (dash + num_dashes,
		    style->dash,
		    num_dashes * sizeof (double));

	    num_dashes *= 2;
	}

	for (i = 0; i < num_dashes; i += 2) {
	    if (dash[i] == 0.0) {
		/* Do not modify the dashes in-place, as we may need to also
		 * replay this stroke to an image fallback.
		 */
		if (dash == style->dash) {
		    dash = _comac_malloc_ab (num_dashes, sizeof (double));
		    if (unlikely (dash == NULL))
			return _comac_error (COMAC_STATUS_NO_MEMORY);
		    memcpy (dash, style->dash, num_dashes * sizeof (double));
		}

		/* If we're at the front of the list, we first rotate
		 * two elements from the end of the list to the front
		 * of the list before folding away the 0.0. Or, if
		 * there are only two dash elements, then there is
		 * nothing at all to draw.
		 */
		if (i == 0) {
		    double last_two[2];

		    if (num_dashes == 2) {
			free (dash);
			return COMAC_INT_STATUS_NOTHING_TO_DO;
		    }

		    /* The cases of num_dashes == 0, 1, or 3 elements
		     * cannot exist, so the rotation of 2 elements
		     * will always be safe */
		    memcpy (last_two, dash + num_dashes - 2, sizeof (last_two));
		    memmove (dash + 2,
			     dash,
			     (num_dashes - 2) * sizeof (double));
		    memcpy (dash, last_two, sizeof (last_two));
		    dash_offset += dash[0] + dash[1];
		    i = 2;
		}
		dash[i - 1] += dash[i + 1];
		num_dashes -= 2;
		memmove (dash + i,
			 dash + i + 2,
			 (num_dashes - i) * sizeof (double));
		/* If we might have just rotated, it's possible that
		 * we rotated a 0.0 value to the front of the list.
		 * Set i to -2 so it will get incremented to 0. */
		if (i == 2)
		    i = -2;
	    }
	}
    }

    if (! pdf_operators->has_line_style ||
	pdf_operators->line_width != line_width) {
	_comac_output_stream_printf (pdf_operators->stream,
				     "%f w\n",
				     line_width);
	pdf_operators->line_width = line_width;
    }

    if (! pdf_operators->has_line_style ||
	pdf_operators->line_cap != style->line_cap) {
	_comac_output_stream_printf (pdf_operators->stream,
				     "%d J\n",
				     _comac_pdf_line_cap (style->line_cap));
	pdf_operators->line_cap = style->line_cap;
    }

    if (! pdf_operators->has_line_style ||
	pdf_operators->line_join != style->line_join) {
	_comac_output_stream_printf (pdf_operators->stream,
				     "%d j\n",
				     _comac_pdf_line_join (style->line_join));
	pdf_operators->line_join = style->line_join;
    }

    if (num_dashes) {
	int d;

	_comac_output_stream_printf (pdf_operators->stream, "[");
	for (d = 0; d < num_dashes; d++)
	    _comac_output_stream_printf (pdf_operators->stream,
					 " %f",
					 dash[d] * scale);
	_comac_output_stream_printf (pdf_operators->stream,
				     "] %f d\n",
				     dash_offset * scale);
	pdf_operators->has_dashes = TRUE;
    } else if (! pdf_operators->has_line_style || pdf_operators->has_dashes) {
	_comac_output_stream_printf (pdf_operators->stream, "[] 0.0 d\n");
	pdf_operators->has_dashes = FALSE;
    }
    if (dash != style->dash)
	free (dash);

    if (! pdf_operators->has_line_style ||
	pdf_operators->miter_limit != style->miter_limit) {
	_comac_output_stream_printf (
	    pdf_operators->stream,
	    "%f M ",
	    style->miter_limit < 1.0 ? 1.0 : style->miter_limit);
	pdf_operators->miter_limit = style->miter_limit;
    }
    pdf_operators->has_line_style = TRUE;

    return _comac_output_stream_get_status (pdf_operators->stream);
}

/* Scale the matrix so the largest absolute value of the non
 * translation components is 1.0. Return the scale required to restore
 * the matrix to the original values.
 *
 * eg the matrix  [ 100  0  0  50   20   10  ]
 *
 * is rescaled to [  1   0  0  0.5  0.2  0.1 ]
 * and the scale returned is 100
 */
static void
_comac_matrix_factor_out_scale (comac_matrix_t *m, double *scale)
{
    double s;

    s = fabs (m->xx);
    if (fabs (m->xy) > s)
	s = fabs (m->xy);
    if (fabs (m->yx) > s)
	s = fabs (m->yx);
    if (fabs (m->yy) > s)
	s = fabs (m->yy);
    *scale = s;
    s = 1.0 / s;
    comac_matrix_scale (m, s, s);
}

static comac_int_status_t
_comac_pdf_operators_emit_stroke (comac_pdf_operators_t *pdf_operators,
				  const comac_path_fixed_t *path,
				  const comac_stroke_style_t *style,
				  const comac_matrix_t *ctm,
				  const comac_matrix_t *ctm_inverse,
				  const char *pdf_operator)
{
    comac_int_status_t status;
    comac_matrix_t m, path_transform;
    comac_bool_t has_ctm = TRUE;
    double scale = 1.0;

    if (pdf_operators->in_text_object) {
	status = _comac_pdf_operators_end_text (pdf_operators);
	if (unlikely (status))
	    return status;
    }

    /* Optimize away the stroke ctm when it does not affect the
     * stroke. There are other ctm cases that could be optimized
     * however this is the most common.
     */
    if (fabs (ctm->xx) == 1.0 && fabs (ctm->yy) == 1.0 &&
	fabs (ctm->xy) == 0.0 && fabs (ctm->yx) == 0.0) {
	has_ctm = FALSE;
    }

    /* The PDF CTM is transformed to the user space CTM when stroking
     * so the correct pen shape will be used. This also requires that
     * the path be transformed to user space when emitted. The
     * conversion of path coordinates to user space may cause rounding
     * errors. For example the device space point (1.234, 3.142) when
     * transformed to a user space CTM of [100 0 0 100 0 0] will be
     * emitted as (0.012, 0.031).
     *
     * To avoid the rounding problem we scale the user space CTM
     * matrix so that all the non translation components of the matrix
     * are <= 1. The line width and and dashes are scaled by the
     * inverse of the scale applied to the CTM. This maintains the
     * shape of the stroke pen while keeping the user space CTM within
     * the range that maximizes the precision of the emitted path.
     */
    if (has_ctm) {
	m = *ctm;
	/* Zero out the translation since it does not affect the pen
	 * shape however it may cause unnecessary digits to be emitted.
	 */
	m.x0 = 0.0;
	m.y0 = 0.0;
	_comac_matrix_factor_out_scale (&m, &scale);
	path_transform = m;
	status = comac_matrix_invert (&path_transform);
	if (unlikely (status))
	    return status;

	comac_matrix_multiply (&m, &m, &pdf_operators->comac_to_pdf);
    }

    status =
	_comac_pdf_operators_emit_stroke_style (pdf_operators, style, scale);
    if (status == COMAC_INT_STATUS_NOTHING_TO_DO)
	return COMAC_STATUS_SUCCESS;
    if (unlikely (status))
	return status;

    if (has_ctm) {
	_comac_output_stream_printf (pdf_operators->stream, "q ");
	_comac_output_stream_print_matrix (pdf_operators->stream, &m);
	_comac_output_stream_printf (pdf_operators->stream, " cm\n");
    } else {
	path_transform = pdf_operators->comac_to_pdf;
    }

    status = _comac_pdf_operators_emit_path (pdf_operators,
					     path,
					     &path_transform,
					     style->line_cap);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (pdf_operators->stream, "%s", pdf_operator);
    if (has_ctm)
	_comac_output_stream_printf (pdf_operators->stream, " Q");

    _comac_output_stream_printf (pdf_operators->stream, "\n");

    return _comac_output_stream_get_status (pdf_operators->stream);
}

comac_int_status_t
_comac_pdf_operators_stroke (comac_pdf_operators_t *pdf_operators,
			     const comac_path_fixed_t *path,
			     const comac_stroke_style_t *style,
			     const comac_matrix_t *ctm,
			     const comac_matrix_t *ctm_inverse)
{
    return _comac_pdf_operators_emit_stroke (pdf_operators,
					     path,
					     style,
					     ctm,
					     ctm_inverse,
					     "S");
}

comac_int_status_t
_comac_pdf_operators_fill (comac_pdf_operators_t *pdf_operators,
			   const comac_path_fixed_t *path,
			   comac_fill_rule_t fill_rule)
{
    const char *pdf_operator;
    comac_status_t status;

    if (pdf_operators->in_text_object) {
	status = _comac_pdf_operators_end_text (pdf_operators);
	if (unlikely (status))
	    return status;
    }

    status = _comac_pdf_operators_emit_path (pdf_operators,
					     path,
					     &pdf_operators->comac_to_pdf,
					     COMAC_LINE_CAP_ROUND);
    if (unlikely (status))
	return status;

    switch (fill_rule) {
    default:
	ASSERT_NOT_REACHED;
    case COMAC_FILL_RULE_WINDING:
	pdf_operator = "f";
	break;
    case COMAC_FILL_RULE_EVEN_ODD:
	pdf_operator = "f*";
	break;
    }

    _comac_output_stream_printf (pdf_operators->stream, "%s\n", pdf_operator);

    return _comac_output_stream_get_status (pdf_operators->stream);
}

comac_int_status_t
_comac_pdf_operators_fill_stroke (comac_pdf_operators_t *pdf_operators,
				  const comac_path_fixed_t *path,
				  comac_fill_rule_t fill_rule,
				  const comac_stroke_style_t *style,
				  const comac_matrix_t *ctm,
				  const comac_matrix_t *ctm_inverse)
{
    const char *operator;

    switch (fill_rule) {
    default:
	ASSERT_NOT_REACHED;
    case COMAC_FILL_RULE_WINDING:
	operator= "B";
	break;
    case COMAC_FILL_RULE_EVEN_ODD:
	operator= "B*";
	break;
    }

    return _comac_pdf_operators_emit_stroke (pdf_operators,
					     path,
					     style,
					     ctm,
					     ctm_inverse,
					     operator);
}

static void
_comac_pdf_operators_emit_glyph_index (comac_pdf_operators_t *pdf_operators,
				       comac_output_stream_t *stream,
				       unsigned int glyph)
{
    if (pdf_operators->is_latin) {
	if (glyph == '(' || glyph == ')' || glyph == '\\')
	    _comac_output_stream_printf (stream, "\\%c", glyph);
	else if (glyph >= 0x20 && glyph <= 0x7e)
	    _comac_output_stream_printf (stream, "%c", glyph);
	else
	    _comac_output_stream_printf (stream, "\\%03o", glyph);
    } else {
	_comac_output_stream_printf (stream,
				     "%0*x",
				     pdf_operators->hex_width,
				     glyph);
    }
}

#define GLYPH_POSITION_TOLERANCE 0.001

/* Emit the string of glyphs using the 'Tj' operator. This requires
 * that the glyphs are positioned at their natural glyph advances. */
static comac_status_t
_comac_pdf_operators_emit_glyph_string (comac_pdf_operators_t *pdf_operators,
					comac_output_stream_t *stream)
{
    int i;

    _comac_output_stream_printf (stream,
				 "%s",
				 pdf_operators->is_latin ? "(" : "<");
    for (i = 0; i < pdf_operators->num_glyphs; i++) {
	_comac_pdf_operators_emit_glyph_index (
	    pdf_operators,
	    stream,
	    pdf_operators->glyphs[i].glyph_index);
	pdf_operators->cur_x += pdf_operators->glyphs[i].x_advance;
    }
    _comac_output_stream_printf (stream,
				 "%sTj\n",
				 pdf_operators->is_latin ? ")" : ">");

    return _comac_output_stream_get_status (stream);
}

/* Emit the string of glyphs using the 'TJ' operator.
 *
 * The TJ operator takes an array of strings of glyphs. Each string of
 * glyphs is displayed using the glyph advances of each glyph to
 * position the glyphs. A relative adjustment to the glyph advance may
 * be specified by including the adjustment between two strings. The
 * adjustment is in units of text space * -1000.
 */
static comac_status_t
_comac_pdf_operators_emit_glyph_string_with_positioning (
    comac_pdf_operators_t *pdf_operators, comac_output_stream_t *stream)
{
    int i;

    _comac_output_stream_printf (stream,
				 "[%s",
				 pdf_operators->is_latin ? "(" : "<");
    for (i = 0; i < pdf_operators->num_glyphs; i++) {
	if (pdf_operators->glyphs[i].x_position != pdf_operators->cur_x) {
	    double delta =
		pdf_operators->glyphs[i].x_position - pdf_operators->cur_x;
	    int rounded_delta;

	    delta = -1000.0 * delta;
	    /* As the delta is in 1/1000 of a unit of text space,
	     * rounding to an integer should still provide sufficient
	     * precision. We round the delta before adding to Tm_x so
	     * that we keep track of the accumulated rounding error in
	     * the PDF interpreter and compensate for it when
	     * calculating subsequent deltas.
	     */
	    rounded_delta = _comac_lround (delta);
	    if (abs (rounded_delta) < 3)
		rounded_delta = 0;
	    if (rounded_delta != 0) {
		if (pdf_operators->is_latin) {
		    _comac_output_stream_printf (stream, ")%d(", rounded_delta);
		} else {
		    _comac_output_stream_printf (stream, ">%d<", rounded_delta);
		}
	    }

	    /* Convert the rounded delta back to text
	     * space before adding to the current text
	     * position. */
	    delta = rounded_delta / -1000.0;
	    pdf_operators->cur_x += delta;
	}

	_comac_pdf_operators_emit_glyph_index (
	    pdf_operators,
	    stream,
	    pdf_operators->glyphs[i].glyph_index);
	pdf_operators->cur_x += pdf_operators->glyphs[i].x_advance;
    }
    _comac_output_stream_printf (stream,
				 "%s]TJ\n",
				 pdf_operators->is_latin ? ")" : ">");

    return _comac_output_stream_get_status (stream);
}

static comac_status_t
_comac_pdf_operators_flush_glyphs (comac_pdf_operators_t *pdf_operators)
{
    comac_output_stream_t *word_wrap_stream;
    comac_status_t status, status2;
    int i;
    double x;

    if (pdf_operators->num_glyphs == 0)
	return COMAC_STATUS_SUCCESS;

    word_wrap_stream = _word_wrap_stream_create (pdf_operators->stream,
						 pdf_operators->ps_output,
						 72);
    status = _comac_output_stream_get_status (word_wrap_stream);
    if (unlikely (status))
	return _comac_output_stream_destroy (word_wrap_stream);

    /* Check if glyph advance used to position every glyph */
    x = pdf_operators->cur_x;
    for (i = 0; i < pdf_operators->num_glyphs; i++) {
	if (fabs (pdf_operators->glyphs[i].x_position - x) >
	    GLYPH_POSITION_TOLERANCE)
	    break;
	x += pdf_operators->glyphs[i].x_advance;
    }
    if (i == pdf_operators->num_glyphs) {
	status = _comac_pdf_operators_emit_glyph_string (pdf_operators,
							 word_wrap_stream);
    } else {
	status = _comac_pdf_operators_emit_glyph_string_with_positioning (
	    pdf_operators,
	    word_wrap_stream);
    }

    pdf_operators->num_glyphs = 0;
    pdf_operators->glyph_buf_x_pos = pdf_operators->cur_x;
    status2 = _comac_output_stream_destroy (word_wrap_stream);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;

    return status;
}

static comac_status_t
_comac_pdf_operators_add_glyph (comac_pdf_operators_t *pdf_operators,
				comac_scaled_font_subsets_glyph_t *glyph,
				double x_position)
{
    double x, y;

    x = glyph->x_advance;
    y = glyph->y_advance;
    if (glyph->is_scaled)
	comac_matrix_transform_distance (&pdf_operators->font_matrix_inverse,
					 &x,
					 &y);

    pdf_operators->glyphs[pdf_operators->num_glyphs].x_position = x_position;
    pdf_operators->glyphs[pdf_operators->num_glyphs].glyph_index =
	glyph->subset_glyph_index;
    pdf_operators->glyphs[pdf_operators->num_glyphs].x_advance = x;
    pdf_operators->glyph_buf_x_pos += x;
    pdf_operators->num_glyphs++;
    if (pdf_operators->num_glyphs == PDF_GLYPH_BUFFER_SIZE)
	return _comac_pdf_operators_flush_glyphs (pdf_operators);

    return COMAC_STATUS_SUCCESS;
}

/* Use 'Tm' operator to set the PDF text matrix. */
static comac_status_t
_comac_pdf_operators_set_text_matrix (comac_pdf_operators_t *pdf_operators,
				      comac_matrix_t *matrix)
{
    comac_matrix_t inverse;
    comac_status_t status;

    /* We require the matrix to be invertable. */
    inverse = *matrix;
    status = comac_matrix_invert (&inverse);
    if (unlikely (status))
	return status;

    pdf_operators->text_matrix = *matrix;
    pdf_operators->cur_x = 0;
    pdf_operators->cur_y = 0;
    pdf_operators->glyph_buf_x_pos = 0;
    _comac_output_stream_print_matrix (pdf_operators->stream,
				       &pdf_operators->text_matrix);
    _comac_output_stream_printf (pdf_operators->stream, " Tm\n");

    pdf_operators->comac_to_pdftext = *matrix;
    status = comac_matrix_invert (&pdf_operators->comac_to_pdftext);
    assert (status == COMAC_STATUS_SUCCESS);
    comac_matrix_multiply (&pdf_operators->comac_to_pdftext,
			   &pdf_operators->comac_to_pdf,
			   &pdf_operators->comac_to_pdftext);

    return _comac_output_stream_get_status (pdf_operators->stream);
}

#define TEXT_MATRIX_TOLERANCE 1e-6

/* Set the translation components of the PDF text matrix to x, y. The
 * 'Td' operator is used to transform the text matrix.
 */
static comac_status_t
_comac_pdf_operators_set_text_position (comac_pdf_operators_t *pdf_operators,
					double x,
					double y)
{
    comac_matrix_t translate, inverse;
    comac_status_t status;

    /* The Td operator transforms the text_matrix with:
     *
     *   text_matrix' = T x text_matrix
     *
     * where T is a translation matrix with the translation components
     * set to the Td operands tx and ty.
     */
    inverse = pdf_operators->text_matrix;
    status = comac_matrix_invert (&inverse);
    assert (status == COMAC_STATUS_SUCCESS);
    pdf_operators->text_matrix.x0 = x;
    pdf_operators->text_matrix.y0 = y;
    comac_matrix_multiply (&translate, &pdf_operators->text_matrix, &inverse);
    if (fabs (translate.x0) < TEXT_MATRIX_TOLERANCE)
	translate.x0 = 0.0;
    if (fabs (translate.y0) < TEXT_MATRIX_TOLERANCE)
	translate.y0 = 0.0;
    _comac_output_stream_printf (pdf_operators->stream,
				 "%f %f Td\n",
				 translate.x0,
				 translate.y0);
    pdf_operators->cur_x = 0;
    pdf_operators->cur_y = 0;
    pdf_operators->glyph_buf_x_pos = 0;

    pdf_operators->comac_to_pdftext = pdf_operators->text_matrix;
    status = comac_matrix_invert (&pdf_operators->comac_to_pdftext);
    assert (status == COMAC_STATUS_SUCCESS);
    comac_matrix_multiply (&pdf_operators->comac_to_pdftext,
			   &pdf_operators->comac_to_pdf,
			   &pdf_operators->comac_to_pdftext);

    return _comac_output_stream_get_status (pdf_operators->stream);
}

/* Select the font using the 'Tf' operator. The font size is set to 1
 * as we use the 'Tm' operator to set the font scale.
 */
static comac_status_t
_comac_pdf_operators_set_font_subset (
    comac_pdf_operators_t *pdf_operators,
    comac_scaled_font_subsets_glyph_t *subset_glyph)
{
    comac_status_t status;

    _comac_output_stream_printf (pdf_operators->stream,
				 "/f-%d-%d 1 Tf\n",
				 subset_glyph->font_id,
				 subset_glyph->subset_id);
    if (pdf_operators->use_font_subset) {
	status = pdf_operators->use_font_subset (
	    subset_glyph->font_id,
	    subset_glyph->subset_id,
	    pdf_operators->use_font_subset_closure);
	if (unlikely (status))
	    return status;
    }
    pdf_operators->font_id = subset_glyph->font_id;
    pdf_operators->subset_id = subset_glyph->subset_id;
    pdf_operators->is_latin = subset_glyph->is_latin;

    if (subset_glyph->is_composite)
	pdf_operators->hex_width = 4;
    else
	pdf_operators->hex_width = 2;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_pdf_operators_begin_text (comac_pdf_operators_t *pdf_operators)
{
    _comac_output_stream_printf (pdf_operators->stream, "BT\n");

    pdf_operators->in_text_object = TRUE;
    pdf_operators->num_glyphs = 0;
    pdf_operators->glyph_buf_x_pos = 0;

    return _comac_output_stream_get_status (pdf_operators->stream);
}

static comac_status_t
_comac_pdf_operators_end_text (comac_pdf_operators_t *pdf_operators)
{
    comac_status_t status;

    status = _comac_pdf_operators_flush_glyphs (pdf_operators);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (pdf_operators->stream, "ET\n");

    pdf_operators->in_text_object = FALSE;

    return _comac_output_stream_get_status (pdf_operators->stream);
}

/* Compare the scale components of two matrices. The translation
 * components are ignored. */
static comac_bool_t
_comac_matrix_scale_equal (comac_matrix_t *a, comac_matrix_t *b)
{
    return (a->xx == b->xx && a->xy == b->xy && a->yx == b->yx &&
	    a->yy == b->yy);
}

static comac_status_t
_comac_pdf_operators_begin_actualtext (comac_pdf_operators_t *pdf_operators,
				       const char *utf8,
				       int utf8_len)
{
    uint16_t *utf16;
    int utf16_len;
    comac_status_t status;
    int i;

    _comac_output_stream_printf (pdf_operators->stream,
				 "/Span << /ActualText <feff");
    if (utf8_len) {
	status = _comac_utf8_to_utf16 (utf8, utf8_len, &utf16, &utf16_len);
	if (unlikely (status))
	    return status;

	for (i = 0; i < utf16_len; i++) {
	    _comac_output_stream_printf (pdf_operators->stream,
					 "%04x",
					 (int) (utf16[i]));
	}
	free (utf16);
    }
    _comac_output_stream_printf (pdf_operators->stream, "> >> BDC\n");

    return _comac_output_stream_get_status (pdf_operators->stream);
}

static comac_status_t
_comac_pdf_operators_end_actualtext (comac_pdf_operators_t *pdf_operators)
{
    _comac_output_stream_printf (pdf_operators->stream, "EMC\n");

    return _comac_output_stream_get_status (pdf_operators->stream);
}

static comac_status_t
_comac_pdf_operators_emit_glyph (
    comac_pdf_operators_t *pdf_operators,
    comac_glyph_t *glyph,
    comac_scaled_font_subsets_glyph_t *subset_glyph)
{
    double x, y;
    comac_status_t status;

    if (pdf_operators->is_new_text_object ||
	pdf_operators->font_id != subset_glyph->font_id ||
	pdf_operators->subset_id != subset_glyph->subset_id) {
	status = _comac_pdf_operators_flush_glyphs (pdf_operators);
	if (unlikely (status))
	    return status;

	status =
	    _comac_pdf_operators_set_font_subset (pdf_operators, subset_glyph);
	if (unlikely (status))
	    return status;

	pdf_operators->is_new_text_object = FALSE;
    }

    x = glyph->x;
    y = glyph->y;
    comac_matrix_transform_point (&pdf_operators->comac_to_pdftext, &x, &y);

    /* The TJ operator for displaying text strings can only set
     * the horizontal position of the glyphs. If the y position
     * (in text space) changes, use the Td operator to change the
     * current position to the next glyph. We also use the Td
     * operator to move the current position if the horizontal
     * position changes by more than 10 (in text space
     * units). This is because the horizontal glyph positioning
     * in the TJ operator is intended for kerning and there may be
     * PDF consumers that do not handle very large position
     * adjustments in TJ.
     */
    if (fabs (x - pdf_operators->glyph_buf_x_pos) > 10 ||
	fabs (y - pdf_operators->cur_y) > GLYPH_POSITION_TOLERANCE) {
	status = _comac_pdf_operators_flush_glyphs (pdf_operators);
	if (unlikely (status))
	    return status;

	x = glyph->x;
	y = glyph->y;
	comac_matrix_transform_point (&pdf_operators->comac_to_pdf, &x, &y);
	status = _comac_pdf_operators_set_text_position (pdf_operators, x, y);
	if (unlikely (status))
	    return status;

	x = 0.0;
	y = 0.0;
    }

    status = _comac_pdf_operators_add_glyph (pdf_operators, subset_glyph, x);
    return status;
}

/* A utf8_len of -1 indicates no unicode text. A utf8_len = 0 is an
 * empty string.
 */
static comac_int_status_t
_comac_pdf_operators_emit_cluster (comac_pdf_operators_t *pdf_operators,
				   const char *utf8,
				   int utf8_len,
				   comac_glyph_t *glyphs,
				   int num_glyphs,
				   comac_text_cluster_flags_t cluster_flags,
				   comac_scaled_font_t *scaled_font)
{
    comac_scaled_font_subsets_glyph_t subset_glyph;
    comac_glyph_t *cur_glyph;
    comac_status_t status = COMAC_STATUS_SUCCESS;
    int i;

    /* If the cluster maps 1 glyph to 1 or more unicode characters, we
     * first try _map_glyph() with the unicode string to see if it can
     * use toUnicode to map our glyph to the unicode. This will fail
     * if the glyph is already mapped to a different unicode string.
     *
     * We also go through this path if no unicode mapping was
     * supplied (utf8_len < 0).
     *
     * Mapping a glyph to a zero length unicode string requires the
     * use of ActualText.
     */
    if (num_glyphs == 1 && utf8_len != 0) {
	status =
	    _comac_scaled_font_subsets_map_glyph (pdf_operators->font_subsets,
						  scaled_font,
						  glyphs->index,
						  utf8,
						  utf8_len,
						  &subset_glyph);
	if (unlikely (status))
	    return status;

	if (subset_glyph.utf8_is_mapped || utf8_len < 0) {
	    status = _comac_pdf_operators_emit_glyph (pdf_operators,
						      glyphs,
						      &subset_glyph);
	    if (unlikely (status))
		return status;

	    return COMAC_STATUS_SUCCESS;
	}
    }

    if (pdf_operators->use_actual_text) {
	/* Fallback to using ActualText to map zero or more glyphs to a
	 * unicode string. */
	status = _comac_pdf_operators_flush_glyphs (pdf_operators);
	if (unlikely (status))
	    return status;

	status = _comac_pdf_operators_begin_actualtext (pdf_operators,
							utf8,
							utf8_len);
	if (unlikely (status))
	    return status;
    }

    if (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD)
	cur_glyph = glyphs + num_glyphs - 1;
    else
	cur_glyph = glyphs;

    /* XXX
     * If no glyphs, we should put *something* here for the text to be selectable. */
    for (i = 0; i < num_glyphs; i++) {
	status =
	    _comac_scaled_font_subsets_map_glyph (pdf_operators->font_subsets,
						  scaled_font,
						  cur_glyph->index,
						  NULL,
						  -1,
						  &subset_glyph);
	if (unlikely (status))
	    return status;

	status = _comac_pdf_operators_emit_glyph (pdf_operators,
						  cur_glyph,
						  &subset_glyph);
	if (unlikely (status))
	    return status;

	if ((cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD))
	    cur_glyph--;
	else
	    cur_glyph++;
    }

    if (pdf_operators->use_actual_text) {
	status = _comac_pdf_operators_flush_glyphs (pdf_operators);
	if (unlikely (status))
	    return status;

	status = _comac_pdf_operators_end_actualtext (pdf_operators);
    }

    return status;
}

comac_int_status_t
_comac_pdf_operators_show_text_glyphs (comac_pdf_operators_t *pdf_operators,
				       const char *utf8,
				       int utf8_len,
				       comac_glyph_t *glyphs,
				       int num_glyphs,
				       const comac_text_cluster_t *clusters,
				       int num_clusters,
				       comac_text_cluster_flags_t cluster_flags,
				       comac_scaled_font_t *scaled_font)
{
    comac_status_t status;
    int i;
    comac_matrix_t text_matrix, invert_y_axis;
    double x, y;
    const char *cur_text;
    comac_glyph_t *cur_glyph;

    pdf_operators->font_matrix_inverse = scaled_font->font_matrix;
    status = comac_matrix_invert (&pdf_operators->font_matrix_inverse);
    if (status == COMAC_STATUS_INVALID_MATRIX)
	return COMAC_STATUS_SUCCESS;
    assert (status == COMAC_STATUS_SUCCESS);

    pdf_operators->is_new_text_object = FALSE;
    if (pdf_operators->in_text_object == FALSE) {
	status = _comac_pdf_operators_begin_text (pdf_operators);
	if (unlikely (status))
	    return status;

	/* Force Tm and Tf to be emitted when starting a new text
	 * object.*/
	pdf_operators->is_new_text_object = TRUE;
    }

    comac_matrix_init_scale (&invert_y_axis, 1, -1);
    text_matrix = scaled_font->scale;

    /* Invert y axis in device space  */
    comac_matrix_multiply (&text_matrix, &invert_y_axis, &text_matrix);

    if (pdf_operators->is_new_text_object ||
	! _comac_matrix_scale_equal (&pdf_operators->text_matrix,
				     &text_matrix)) {
	status = _comac_pdf_operators_flush_glyphs (pdf_operators);
	if (unlikely (status))
	    return status;

	x = glyphs[0].x;
	y = glyphs[0].y;
	comac_matrix_transform_point (&pdf_operators->comac_to_pdf, &x, &y);
	text_matrix.x0 = x;
	text_matrix.y0 = y;
	status =
	    _comac_pdf_operators_set_text_matrix (pdf_operators, &text_matrix);
	if (status == COMAC_STATUS_INVALID_MATRIX)
	    return COMAC_STATUS_SUCCESS;
	if (unlikely (status))
	    return status;
    }

    if (num_clusters > 0) {
	cur_text = utf8;
	if ((cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD))
	    cur_glyph = glyphs + num_glyphs;
	else
	    cur_glyph = glyphs;
	for (i = 0; i < num_clusters; i++) {
	    if ((cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD))
		cur_glyph -= clusters[i].num_glyphs;
	    status = _comac_pdf_operators_emit_cluster (pdf_operators,
							cur_text,
							clusters[i].num_bytes,
							cur_glyph,
							clusters[i].num_glyphs,
							cluster_flags,
							scaled_font);
	    if (unlikely (status))
		return status;

	    cur_text += clusters[i].num_bytes;
	    if (! (cluster_flags & COMAC_TEXT_CLUSTER_FLAG_BACKWARD))
		cur_glyph += clusters[i].num_glyphs;
	}
    } else {
	for (i = 0; i < num_glyphs; i++) {
	    status = _comac_pdf_operators_emit_cluster (
		pdf_operators,
		NULL,
		-1, /* no unicode string available */
		&glyphs[i],
		1,
		FALSE,
		scaled_font);
	    if (unlikely (status))
		return status;
	}
    }

    return _comac_output_stream_get_status (pdf_operators->stream);
}

comac_int_status_t
_comac_pdf_operators_tag_begin (comac_pdf_operators_t *pdf_operators,
				const char *tag_name,
				int mcid)
{
    comac_status_t status;

    if (pdf_operators->in_text_object) {
	status = _comac_pdf_operators_end_text (pdf_operators);
	if (unlikely (status))
	    return status;
    }

    _comac_output_stream_printf (pdf_operators->stream,
				 "/%s << /MCID %d >> BDC\n",
				 tag_name,
				 mcid);

    return _comac_output_stream_get_status (pdf_operators->stream);
}

comac_int_status_t
_comac_pdf_operators_tag_end (comac_pdf_operators_t *pdf_operators)
{
    comac_status_t status;

    if (pdf_operators->in_text_object) {
	status = _comac_pdf_operators_end_text (pdf_operators);
	if (unlikely (status))
	    return status;
    }

    _comac_output_stream_printf (pdf_operators->stream, "EMC\n");

    return _comac_output_stream_get_status (pdf_operators->stream);
}

#endif /* COMAC_HAS_PDF_OPERATORS */
