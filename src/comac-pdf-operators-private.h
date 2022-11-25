/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc
 * Copyright © 2006 Red Hat, Inc
 * Copyright © 2007 Adrian Johnson
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

#ifndef COMAC_PDF_OPERATORS_H
#define COMAC_PDF_OPERATORS_H

#include "comac-compiler-private.h"
#include "comac-error-private.h"
#include "comac-types-private.h"

/* The glyph buffer size is based on the expected maximum glyphs in a
 * line so that an entire line can be emitted in as one string. If the
 * glyphs in a line exceeds this size the only downside is the slight
 * overhead of emitting two strings.
 */
#define PDF_GLYPH_BUFFER_SIZE 200

typedef comac_int_status_t
(*comac_pdf_operators_use_font_subset_t) (unsigned int  font_id,
					  unsigned int  subset_id,
					  void         *closure);

typedef struct _comac_pdf_glyph {
    unsigned int glyph_index;
    double x_position;
    double x_advance;
} comac_pdf_glyph_t;

typedef struct _comac_pdf_operators {
    comac_output_stream_t *stream;
    comac_matrix_t comac_to_pdf;
    comac_scaled_font_subsets_t *font_subsets;
    comac_pdf_operators_use_font_subset_t use_font_subset;
    void *use_font_subset_closure;
    comac_bool_t ps_output; /* output is for PostScript */
    comac_bool_t use_actual_text;
    comac_bool_t in_text_object; /* inside BT/ET pair */

    /* PDF text state */
    comac_bool_t is_new_text_object; /* text object started but matrix and font not yet selected */
    unsigned int font_id;
    unsigned int subset_id;
    comac_matrix_t text_matrix; /* PDF text matrix (Tlm in the PDF reference) */
    comac_matrix_t comac_to_pdftext; /* translate comac coords to PDF text space */
    comac_matrix_t font_matrix_inverse;
    double cur_x; /* Current position in PDF text space (Tm in the PDF reference) */
    double cur_y;
    int hex_width;
    comac_bool_t is_latin;
    int num_glyphs;
    double glyph_buf_x_pos;
    comac_pdf_glyph_t glyphs[PDF_GLYPH_BUFFER_SIZE];

    /* PDF line style */
    comac_bool_t         has_line_style;
    double		 line_width;
    comac_line_cap_t	 line_cap;
    comac_line_join_t	 line_join;
    double		 miter_limit;
    comac_bool_t         has_dashes;
} comac_pdf_operators_t;

comac_private void
_comac_pdf_operators_init (comac_pdf_operators_t       *pdf_operators,
			   comac_output_stream_t       *stream,
			   comac_matrix_t 	       *comac_to_pdf,
			   comac_scaled_font_subsets_t *font_subsets,
			   comac_bool_t                 ps);

comac_private comac_status_t
_comac_pdf_operators_fini (comac_pdf_operators_t       *pdf_operators);

comac_private void
_comac_pdf_operators_set_font_subsets_callback (comac_pdf_operators_t 		     *pdf_operators,
						comac_pdf_operators_use_font_subset_t use_font_subset,
						void				     *closure);

comac_private void
_comac_pdf_operators_set_stream (comac_pdf_operators_t 	 *pdf_operators,
				 comac_output_stream_t   *stream);


comac_private void
_comac_pdf_operators_set_comac_to_pdf_matrix (comac_pdf_operators_t *pdf_operators,
					      comac_matrix_t 	    *comac_to_pdf);

comac_private void
_comac_pdf_operators_enable_actual_text (comac_pdf_operators_t *pdf_operators,
					 comac_bool_t 	  	enable);

comac_private comac_status_t
_comac_pdf_operators_flush (comac_pdf_operators_t	 *pdf_operators);

comac_private void
_comac_pdf_operators_reset (comac_pdf_operators_t	 *pdf_operators);

comac_private comac_int_status_t
_comac_pdf_operators_clip (comac_pdf_operators_t	*pdf_operators,
			   const comac_path_fixed_t	*path,
			   comac_fill_rule_t		 fill_rule);

comac_private comac_int_status_t
_comac_pdf_operators_emit_stroke_style (comac_pdf_operators_t		*pdf_operators,
					const comac_stroke_style_t	*style,
					double				 scale);

comac_private comac_int_status_t
_comac_pdf_operators_stroke (comac_pdf_operators_t	*pdf_operators,
			     const comac_path_fixed_t	*path,
			     const comac_stroke_style_t	*style,
			     const comac_matrix_t	*ctm,
			     const comac_matrix_t	*ctm_inverse);

comac_private comac_int_status_t
_comac_pdf_operators_fill (comac_pdf_operators_t	*pdf_operators,
			   const comac_path_fixed_t	*path,
			   comac_fill_rule_t		fill_rule);

comac_private comac_int_status_t
_comac_pdf_operators_fill_stroke (comac_pdf_operators_t		*pdf_operators,
				  const comac_path_fixed_t	*path,
				  comac_fill_rule_t		 fill_rule,
				  const comac_stroke_style_t	*style,
				  const comac_matrix_t		*ctm,
				  const comac_matrix_t		*ctm_inverse);

comac_private comac_int_status_t
_comac_pdf_operators_show_text_glyphs (comac_pdf_operators_t	  *pdf_operators,
				       const char                 *utf8,
				       int                         utf8_len,
				       comac_glyph_t              *glyphs,
				       int                         num_glyphs,
				       const comac_text_cluster_t *clusters,
				       int                         num_clusters,
				       comac_text_cluster_flags_t  cluster_flags,
				       comac_scaled_font_t	  *scaled_font);

comac_private comac_int_status_t
_comac_pdf_operators_tag_begin (comac_pdf_operators_t *pdf_operators,
				const char            *tag_name,
				int                    mcid);

comac_private comac_int_status_t
_comac_pdf_operators_tag_end (comac_pdf_operators_t *pdf_operators);

#endif /* COMAC_PDF_OPERATORS_H */
