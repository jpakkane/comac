/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
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
 * The Initial Developer of the Original Code is Chris Wilson.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

/* The script surface is one that records all operations performed on
 * it in the form of a procedural script, similar in fashion to
 * PostScript but using Comac's imaging model. In essence, this is
 * equivalent to the recording-surface, but as there is no impedance mismatch
 * between Comac and ComacScript, we can generate output immediately
 * without having to copy and hold the data in memory.
 */

/**
 * SECTION:comac-script
 * @Title: Script Surfaces
 * @Short_Description: Rendering to replayable scripts
 * @See_Also: #comac_surface_t
 *
 * The script surface provides the ability to render to a native
 * script that matches the comac drawing model. The scripts can
 * be replayed using tools under the util/comac-script directory,
 * or with comac-perf-trace.
 **/

/**
 * COMAC_HAS_SCRIPT_SURFACE:
 *
 * Defined if the script surface backend is available.
 * The script surface backend is always built in since 1.12.
 *
 * Since: 1.12
 **/

#include "comacint.h"

#include "comac-script.h"
#include "comac-script-private.h"

#include "comac-analysis-surface-private.h"
#include "comac-default-context-private.h"
#include "comac-device-private.h"
#include "comac-error-private.h"
#include "comac-list-inline.h"
#include "comac-image-surface-private.h"
#include "comac-output-stream-private.h"
#include "comac-pattern-private.h"
#include "comac-recording-surface-inline.h"
#include "comac-scaled-font-private.h"
#include "comac-surface-clipper-private.h"
#include "comac-surface-snapshot-inline.h"
#include "comac-surface-subsurface-private.h"
#include "comac-surface-wrapper-private.h"

#if COMAC_HAS_FT_FONT
#include "comac-ft-private.h"
#endif

#include <ctype.h>

#ifdef WORDS_BIGENDIAN
#define to_be32(x) x
#else
#define to_be32(x) bswap_32 (x)
#endif

#define _comac_output_stream_puts(S, STR)                                      \
    _comac_output_stream_write ((S), (STR), strlen (STR))

#define static comac_warn static

typedef struct _comac_script_context comac_script_context_t;
typedef struct _comac_script_surface comac_script_surface_t;
typedef struct _comac_script_implicit_context comac_script_implicit_context_t;
typedef struct _comac_script_font comac_script_font_t;

typedef struct _operand {
    enum {
	SURFACE,
	DEFERRED,
    } type;
    comac_list_t link;
} operand_t;

struct deferred_finish {
    comac_list_t link;
    operand_t operand;
};

struct _comac_script_context {
    comac_device_t base;

    int active;
    int attach_snapshots;

    comac_bool_t owns_stream;
    comac_output_stream_t *stream;
    comac_script_mode_t mode;

    struct _bitmap {
	unsigned long min;
	unsigned long count;
	unsigned int map[64];
	struct _bitmap *next;
    } surface_id, font_id;

    comac_list_t operands;
    comac_list_t deferred;

    comac_list_t fonts;
    comac_list_t defines;
};

struct _comac_script_font {
    comac_scaled_font_private_t base;

    comac_bool_t has_sfnt;
    unsigned long id;
    unsigned long subset_glyph_index;
    comac_list_t link;
    comac_scaled_font_t *parent;
};

struct _comac_script_implicit_context {
    comac_operator_t current_operator;
    comac_fill_rule_t current_fill_rule;
    double current_tolerance;
    comac_antialias_t current_antialias;
    comac_stroke_style_t current_style;
    comac_pattern_union_t current_source;
    comac_matrix_t current_ctm;
    comac_matrix_t current_stroke_matrix;
    comac_matrix_t current_font_matrix;
    comac_font_options_t current_font_options;
    comac_scaled_font_t *current_scaled_font;
    comac_path_fixed_t current_path;
    comac_bool_t has_clip;
};

struct _comac_script_surface {
    comac_surface_t base;

    comac_surface_wrapper_t wrapper;

    comac_surface_clipper_t clipper;

    operand_t operand;
    comac_bool_t emitted;
    comac_bool_t defined;
    comac_bool_t active;

    double width, height;

    /* implicit flattened context */
    comac_script_implicit_context_t cr;
};

static const comac_surface_backend_t _comac_script_surface_backend;

static comac_script_surface_t *
_comac_script_surface_create_internal (comac_script_context_t *ctx,
				       comac_content_t content,
				       comac_rectangle_t *extents,
				       comac_surface_t *passthrough);

static void
_comac_script_scaled_font_fini (comac_scaled_font_private_t *abstract_private,
				comac_scaled_font_t *scaled_font);

static void
_comac_script_implicit_context_init (comac_script_implicit_context_t *cr);

static void
_comac_script_implicit_context_reset (comac_script_implicit_context_t *cr);

static void
_bitmap_release_id (struct _bitmap *b, unsigned long token)
{
    struct _bitmap **prev = NULL;

    do {
	if (token < b->min + sizeof (b->map) * CHAR_BIT) {
	    unsigned int bit, elem;

	    token -= b->min;
	    elem = token / (sizeof (b->map[0]) * CHAR_BIT);
	    bit = token % (sizeof (b->map[0]) * CHAR_BIT);
	    b->map[elem] &= ~(1 << bit);
	    if (! --b->count && prev) {
		*prev = b->next;
		free (b);
	    }
	    return;
	}
	prev = &b->next;
	b = b->next;
    } while (b != NULL);
}

static comac_status_t
_bitmap_next_id (struct _bitmap *b, unsigned long *id)
{
    struct _bitmap *bb, **prev = NULL;
    unsigned long min = 0;

    do {
	if (b->min != min)
	    break;

	if (b->count < sizeof (b->map) * CHAR_BIT) {
	    unsigned int n, m, bit;
	    for (n = 0; n < ARRAY_LENGTH (b->map); n++) {
		if (b->map[n] == (unsigned int) -1)
		    continue;

		for (m = 0, bit = 1; m < sizeof (b->map[0]) * CHAR_BIT;
		     m++, bit <<= 1) {
		    if ((b->map[n] & bit) == 0) {
			b->map[n] |= bit;
			b->count++;
			*id = n * sizeof (b->map[0]) * CHAR_BIT + m + b->min;
			return COMAC_STATUS_SUCCESS;
		    }
		}
	    }
	}
	min += sizeof (b->map) * CHAR_BIT;

	prev = &b->next;
	b = b->next;
    } while (b != NULL);
    assert (prev != NULL);

    bb = _comac_malloc (sizeof (struct _bitmap));
    if (unlikely (bb == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    *prev = bb;
    bb->next = b;
    bb->min = min;
    bb->count = 1;
    bb->map[0] = 0x1;
    memset (bb->map + 1, 0, sizeof (bb->map) - sizeof (bb->map[0]));
    *id = min;

    return COMAC_STATUS_SUCCESS;
}

static void
_bitmap_fini (struct _bitmap *b)
{
    while (b != NULL) {
	struct _bitmap *next = b->next;
	free (b);
	b = next;
    }
}

static const char *
_direction_to_string (comac_bool_t backward)
{
    static const char *names[] = {"FORWARD", "BACKWARD"};
    assert (backward < ARRAY_LENGTH (names));
    return names[backward];
}

static const char *
_operator_to_string (comac_operator_t op)
{
    static const char *names[] = {
	"CLEAR", /* COMAC_OPERATOR_CLEAR */

	"SOURCE", /* COMAC_OPERATOR_SOURCE */
	"OVER",	  /* COMAC_OPERATOR_OVER */
	"IN",	  /* COMAC_OPERATOR_IN */
	"OUT",	  /* COMAC_OPERATOR_OUT */
	"ATOP",	  /* COMAC_OPERATOR_ATOP */

	"DEST",	     /* COMAC_OPERATOR_DEST */
	"DEST_OVER", /* COMAC_OPERATOR_DEST_OVER */
	"DEST_IN",   /* COMAC_OPERATOR_DEST_IN */
	"DEST_OUT",  /* COMAC_OPERATOR_DEST_OUT */
	"DEST_ATOP", /* COMAC_OPERATOR_DEST_ATOP */

	"XOR",	    /* COMAC_OPERATOR_XOR */
	"ADD",	    /* COMAC_OPERATOR_ADD */
	"SATURATE", /* COMAC_OPERATOR_SATURATE */

	"MULTIPLY",	  /* COMAC_OPERATOR_MULTIPLY */
	"SCREEN",	  /* COMAC_OPERATOR_SCREEN */
	"OVERLAY",	  /* COMAC_OPERATOR_OVERLAY */
	"DARKEN",	  /* COMAC_OPERATOR_DARKEN */
	"LIGHTEN",	  /* COMAC_OPERATOR_LIGHTEN */
	"DODGE",	  /* COMAC_OPERATOR_COLOR_DODGE */
	"BURN",		  /* COMAC_OPERATOR_COLOR_BURN */
	"HARD_LIGHT",	  /* COMAC_OPERATOR_HARD_LIGHT */
	"SOFT_LIGHT",	  /* COMAC_OPERATOR_SOFT_LIGHT */
	"DIFFERENCE",	  /* COMAC_OPERATOR_DIFFERENCE */
	"EXCLUSION",	  /* COMAC_OPERATOR_EXCLUSION */
	"HSL_HUE",	  /* COMAC_OPERATOR_HSL_HUE */
	"HSL_SATURATION", /* COMAC_OPERATOR_HSL_SATURATION */
	"HSL_COLOR",	  /* COMAC_OPERATOR_HSL_COLOR */
	"HSL_LUMINOSITY"  /* COMAC_OPERATOR_HSL_LUMINOSITY */
    };
    assert (op < ARRAY_LENGTH (names));
    return names[op];
}

static const char *
_extend_to_string (comac_extend_t extend)
{
    static const char *names[] = {
	"EXTEND_NONE",	  /* COMAC_EXTEND_NONE */
	"EXTEND_REPEAT",  /* COMAC_EXTEND_REPEAT */
	"EXTEND_REFLECT", /* COMAC_EXTEND_REFLECT */
	"EXTEND_PAD"	  /* COMAC_EXTEND_PAD */
    };
    assert (extend < ARRAY_LENGTH (names));
    return names[extend];
}

static const char *
_filter_to_string (comac_filter_t filter)
{
    static const char *names[] = {
	"FILTER_FAST",	   /* COMAC_FILTER_FAST */
	"FILTER_GOOD",	   /* COMAC_FILTER_GOOD */
	"FILTER_BEST",	   /* COMAC_FILTER_BEST */
	"FILTER_NEAREST",  /* COMAC_FILTER_NEAREST */
	"FILTER_BILINEAR", /* COMAC_FILTER_BILINEAR */
	"FILTER_GAUSSIAN", /* COMAC_FILTER_GAUSSIAN */
    };
    assert (filter < ARRAY_LENGTH (names));
    return names[filter];
}

static const char *
_fill_rule_to_string (comac_fill_rule_t rule)
{
    static const char *names[] = {
	"WINDING", /* COMAC_FILL_RULE_WINDING */
	"EVEN_ODD" /* COMAC_FILL_RILE_EVEN_ODD */
    };
    assert (rule < ARRAY_LENGTH (names));
    return names[rule];
}

static const char *
_antialias_to_string (comac_antialias_t antialias)
{
    static const char *names[] = {
	"ANTIALIAS_DEFAULT",  /* COMAC_ANTIALIAS_DEFAULT */
	"ANTIALIAS_NONE",     /* COMAC_ANTIALIAS_NONE */
	"ANTIALIAS_GRAY",     /* COMAC_ANTIALIAS_GRAY */
	"ANTIALIAS_SUBPIXEL", /* COMAC_ANTIALIAS_SUBPIXEL */
	"ANTIALIAS_FAST",     /* COMAC_ANTIALIAS_FAST */
	"ANTIALIAS_GOOD",     /* COMAC_ANTIALIAS_GOOD */
	"ANTIALIAS_BEST"      /* COMAC_ANTIALIAS_BEST */
    };
    assert (antialias < ARRAY_LENGTH (names));
    return names[antialias];
}

static const char *
_line_cap_to_string (comac_line_cap_t line_cap)
{
    static const char *names[] = {
	"LINE_CAP_BUTT",  /* COMAC_LINE_CAP_BUTT */
	"LINE_CAP_ROUND", /* COMAC_LINE_CAP_ROUND */
	"LINE_CAP_SQUARE" /* COMAC_LINE_CAP_SQUARE */
    };
    assert (line_cap < ARRAY_LENGTH (names));
    return names[line_cap];
}

static const char *
_line_join_to_string (comac_line_join_t line_join)
{
    static const char *names[] = {
	"LINE_JOIN_MITER", /* COMAC_LINE_JOIN_MITER */
	"LINE_JOIN_ROUND", /* COMAC_LINE_JOIN_ROUND */
	"LINE_JOIN_BEVEL", /* COMAC_LINE_JOIN_BEVEL */
    };
    assert (line_join < ARRAY_LENGTH (names));
    return names[line_join];
}

static inline comac_script_context_t *
to_context (comac_script_surface_t *surface)
{
    return (comac_script_context_t *) surface->base.device;
}

static comac_bool_t
target_is_active (comac_script_surface_t *surface)
{
    return comac_list_is_first (&surface->operand.link,
				&to_context (surface)->operands);
}

static void
target_push (comac_script_surface_t *surface)
{
    comac_list_move (&surface->operand.link, &to_context (surface)->operands);
}

static int
target_depth (comac_script_surface_t *surface)
{
    comac_list_t *link;
    int depth = 0;

    comac_list_foreach (link, &to_context (surface)->operands)
    {
	if (link == &surface->operand.link)
	    break;
	depth++;
    }

    return depth;
}

static void
_get_target (comac_script_surface_t *surface)
{
    comac_script_context_t *ctx = to_context (surface);

    if (target_is_active (surface)) {
	_comac_output_stream_puts (ctx->stream, "dup ");
	return;
    }

    if (surface->defined) {
	_comac_output_stream_printf (ctx->stream,
				     "s%u ",
				     surface->base.unique_id);
    } else {
	int depth = target_depth (surface);

	assert (! comac_list_is_empty (&surface->operand.link));
	assert (! target_is_active (surface));

	if (ctx->active) {
	    _comac_output_stream_printf (ctx->stream, "%d index ", depth);
	    _comac_output_stream_puts (ctx->stream, "/target get exch pop ");
	} else {
	    if (depth == 1) {
		_comac_output_stream_puts (ctx->stream, "exch ");
	    } else {
		_comac_output_stream_printf (ctx->stream, "%d -1 roll ", depth);
	    }
	    target_push (surface);
	    _comac_output_stream_puts (ctx->stream, "dup ");
	}
    }
}

static const char *
_content_to_string (comac_content_t content)
{
    switch (content) {
    case COMAC_CONTENT_ALPHA:
	return "ALPHA";
    case COMAC_CONTENT_COLOR:
	return "COLOR";
    default:
    case COMAC_CONTENT_COLOR_ALPHA:
	return "COLOR_ALPHA";
    }
}

static comac_status_t
_emit_surface (comac_script_surface_t *surface)
{
    comac_script_context_t *ctx = to_context (surface);

    _comac_output_stream_printf (ctx->stream,
				 "<< /content //%s",
				 _content_to_string (surface->base.content));
    if (surface->width != -1 && surface->height != -1) {
	_comac_output_stream_printf (ctx->stream,
				     " /width %f /height %f",
				     surface->width,
				     surface->height);
    }

    if (surface->base.x_fallback_resolution !=
	    COMAC_SURFACE_FALLBACK_RESOLUTION_DEFAULT ||
	surface->base.y_fallback_resolution !=
	    COMAC_SURFACE_FALLBACK_RESOLUTION_DEFAULT) {
	_comac_output_stream_printf (ctx->stream,
				     " /fallback-resolution [%f %f]",
				     surface->base.x_fallback_resolution,
				     surface->base.y_fallback_resolution);
    }

    if (surface->base.device_transform.x0 != 0. ||
	surface->base.device_transform.y0 != 0.) {
	/* XXX device offset is encoded into the pattern matrices etc. */
	if (0) {
	    _comac_output_stream_printf (ctx->stream,
					 " /device-offset [%f %f]",
					 surface->base.device_transform.x0,
					 surface->base.device_transform.y0);
	}
    }

    _comac_output_stream_puts (ctx->stream, " >> surface context\n");
    surface->emitted = TRUE;
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_context (comac_script_surface_t *surface)
{
    comac_script_context_t *ctx = to_context (surface);

    if (target_is_active (surface))
	return COMAC_STATUS_SUCCESS;

    while (! comac_list_is_empty (&ctx->operands)) {
	operand_t *op;
	comac_script_surface_t *old;

	op = comac_list_first_entry (&ctx->operands, operand_t, link);
	if (op->type == DEFERRED)
	    break;

	old = comac_container_of (op, comac_script_surface_t, operand);
	if (old == surface)
	    break;
	if (old->active)
	    break;

	if (! old->defined) {
	    assert (old->emitted);
	    _comac_output_stream_printf (ctx->stream,
					 "/target get /s%u exch def pop\n",
					 old->base.unique_id);
	    old->defined = TRUE;
	} else {
	    _comac_output_stream_puts (ctx->stream, "pop\n");
	}

	comac_list_del (&old->operand.link);
    }

    if (target_is_active (surface))
	return COMAC_STATUS_SUCCESS;

    if (! surface->emitted) {
	comac_status_t status;

	status = _emit_surface (surface);
	if (unlikely (status))
	    return status;
    } else if (comac_list_is_empty (&surface->operand.link)) {
	assert (surface->defined);
	_comac_output_stream_printf (ctx->stream,
				     "s%u context\n",
				     surface->base.unique_id);
	_comac_script_implicit_context_reset (&surface->cr);
	_comac_surface_clipper_reset (&surface->clipper);
    } else {
	int depth = target_depth (surface);
	if (depth == 1) {
	    _comac_output_stream_puts (ctx->stream, "exch\n");
	} else {
	    _comac_output_stream_printf (ctx->stream, "%d -1 roll\n", depth);
	}
    }
    target_push (surface);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_operator (comac_script_surface_t *surface, comac_operator_t op)
{
    assert (target_is_active (surface));

    if (surface->cr.current_operator == op)
	return COMAC_STATUS_SUCCESS;

    surface->cr.current_operator = op;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "//%s set-operator\n",
				 _operator_to_string (op));
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_fill_rule (comac_script_surface_t *surface, comac_fill_rule_t fill_rule)
{
    assert (target_is_active (surface));

    if (surface->cr.current_fill_rule == fill_rule)
	return COMAC_STATUS_SUCCESS;

    surface->cr.current_fill_rule = fill_rule;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "//%s set-fill-rule\n",
				 _fill_rule_to_string (fill_rule));
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_tolerance (comac_script_surface_t *surface,
		 double tolerance,
		 comac_bool_t force)
{
    assert (target_is_active (surface));

    if ((! force || fabs (tolerance - COMAC_GSTATE_TOLERANCE_DEFAULT) < 1e-5) &&
	surface->cr.current_tolerance == tolerance) {
	return COMAC_STATUS_SUCCESS;
    }

    surface->cr.current_tolerance = tolerance;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "%f set-tolerance\n",
				 tolerance);
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_antialias (comac_script_surface_t *surface, comac_antialias_t antialias)
{
    assert (target_is_active (surface));

    if (surface->cr.current_antialias == antialias)
	return COMAC_STATUS_SUCCESS;

    surface->cr.current_antialias = antialias;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "//%s set-antialias\n",
				 _antialias_to_string (antialias));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_line_width (comac_script_surface_t *surface,
		  double line_width,
		  comac_bool_t force)
{
    assert (target_is_active (surface));

    if ((! force ||
	 fabs (line_width - COMAC_GSTATE_LINE_WIDTH_DEFAULT) < 1e-5) &&
	surface->cr.current_style.line_width == line_width) {
	return COMAC_STATUS_SUCCESS;
    }

    surface->cr.current_style.line_width = line_width;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "%f set-line-width\n",
				 line_width);
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_hairline (comac_script_surface_t *surface, comac_bool_t set_hairline)
{
    assert (target_is_active (surface));

    if (surface->cr.current_style.is_hairline == set_hairline) {
	return COMAC_STATUS_SUCCESS;
    }

    surface->cr.current_style.is_hairline = set_hairline;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "%d set-hairline\n",
				 set_hairline);
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_line_cap (comac_script_surface_t *surface, comac_line_cap_t line_cap)
{
    assert (target_is_active (surface));

    if (surface->cr.current_style.line_cap == line_cap)
	return COMAC_STATUS_SUCCESS;

    surface->cr.current_style.line_cap = line_cap;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "//%s set-line-cap\n",
				 _line_cap_to_string (line_cap));
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_line_join (comac_script_surface_t *surface, comac_line_join_t line_join)
{
    assert (target_is_active (surface));

    if (surface->cr.current_style.line_join == line_join)
	return COMAC_STATUS_SUCCESS;

    surface->cr.current_style.line_join = line_join;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "//%s set-line-join\n",
				 _line_join_to_string (line_join));
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_miter_limit (comac_script_surface_t *surface,
		   double miter_limit,
		   comac_bool_t force)
{
    assert (target_is_active (surface));

    if ((! force ||
	 fabs (miter_limit - COMAC_GSTATE_MITER_LIMIT_DEFAULT) < 1e-5) &&
	surface->cr.current_style.miter_limit == miter_limit) {
	return COMAC_STATUS_SUCCESS;
    }

    surface->cr.current_style.miter_limit = miter_limit;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "%f set-miter-limit\n",
				 miter_limit);
    return COMAC_STATUS_SUCCESS;
}

static comac_bool_t
_dashes_equal (const double *a, const double *b, int num_dashes)
{
    while (num_dashes--) {
	if (fabs (*a - *b) > 1e-5)
	    return FALSE;
	a++, b++;
    }

    return TRUE;
}

static comac_status_t
_emit_dash (comac_script_surface_t *surface,
	    const double *dash,
	    unsigned int num_dashes,
	    double offset,
	    comac_bool_t force)
{
    unsigned int n;

    assert (target_is_active (surface));

    if (force && num_dashes == 0 && surface->cr.current_style.num_dashes == 0) {
	return COMAC_STATUS_SUCCESS;
    }

    if (! force &&
	(surface->cr.current_style.num_dashes == num_dashes &&
	 (num_dashes == 0 ||
	  (fabs (surface->cr.current_style.dash_offset - offset) < 1e-5 &&
	   _dashes_equal (surface->cr.current_style.dash,
			  dash,
			  num_dashes))))) {
	return COMAC_STATUS_SUCCESS;
    }

    if (num_dashes) {
	surface->cr.current_style.dash =
	    _comac_realloc_ab (surface->cr.current_style.dash,
			       num_dashes,
			       sizeof (double));
	if (unlikely (surface->cr.current_style.dash == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);

	memcpy (surface->cr.current_style.dash,
		dash,
		sizeof (double) * num_dashes);
    } else {
	free (surface->cr.current_style.dash);
	surface->cr.current_style.dash = NULL;
    }

    surface->cr.current_style.num_dashes = num_dashes;
    surface->cr.current_style.dash_offset = offset;

    _comac_output_stream_puts (to_context (surface)->stream, "[");
    for (n = 0; n < num_dashes; n++) {
	_comac_output_stream_printf (to_context (surface)->stream,
				     "%f",
				     dash[n]);
	if (n < num_dashes - 1)
	    _comac_output_stream_puts (to_context (surface)->stream, " ");
    }
    _comac_output_stream_printf (to_context (surface)->stream,
				 "] %f set-dash\n",
				 offset);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_stroke_style (comac_script_surface_t *surface,
		    const comac_stroke_style_t *style,
		    comac_bool_t force)
{
    comac_status_t status;

    assert (target_is_active (surface));

    status = _emit_line_width (surface, style->line_width, force);
    if (unlikely (status))
	return status;

    status = _emit_line_cap (surface, style->line_cap);
    if (unlikely (status))
	return status;

    status = _emit_line_join (surface, style->line_join);
    if (unlikely (status))
	return status;

    status = _emit_miter_limit (surface, style->miter_limit, force);
    if (unlikely (status))
	return status;

    status = _emit_hairline (surface, style->is_hairline);
    if (unlikely (status))
	return status;

    status = _emit_dash (surface,
			 style->dash,
			 style->num_dashes,
			 style->dash_offset,
			 force);
    if (unlikely (status))
	return status;

    return COMAC_STATUS_SUCCESS;
}

static const char *
_format_to_string (comac_format_t format)
{
    switch (format) {
    case COMAC_FORMAT_RGBA128F:
	return "RGBA128F";
    case COMAC_FORMAT_RGB96F:
	return "RGB96F";
    case COMAC_FORMAT_ARGB32:
	return "ARGB32";
    case COMAC_FORMAT_RGB30:
	return "RGB30";
    case COMAC_FORMAT_RGB24:
	return "RGB24";
    case COMAC_FORMAT_RGB16_565:
	return "RGB16_565";
    case COMAC_FORMAT_A8:
	return "A8";
    case COMAC_FORMAT_A1:
	return "A1";
    case COMAC_FORMAT_INVALID:
	return "INVALID";
    }
    ASSERT_NOT_REACHED;
    return "INVALID";
}

static comac_status_t
_emit_solid_pattern (comac_script_surface_t *surface,
		     const comac_pattern_t *pattern)
{
    comac_solid_pattern_t *solid = (comac_solid_pattern_t *) pattern;
    comac_script_context_t *ctx = to_context (surface);

    if (! COMAC_COLOR_IS_OPAQUE (&solid->color)) {
	if (! (surface->base.content & COMAC_CONTENT_COLOR) ||
	    ((solid->color.red_short == 0 ||
	      solid->color.red_short == 0xffff) &&
	     (solid->color.green_short == 0 ||
	      solid->color.green_short == 0xffff) &&
	     (solid->color.blue_short == 0 ||
	      solid->color.blue_short == 0xffff))) {
	    _comac_output_stream_printf (ctx->stream,
					 "%f a",
					 solid->color.alpha);
	} else {
	    _comac_output_stream_printf (ctx->stream,
					 "%f %f %f %f rgba",
					 solid->color.red,
					 solid->color.green,
					 solid->color.blue,
					 solid->color.alpha);
	}
    } else {
	if (solid->color.red_short == solid->color.green_short &&
	    solid->color.red_short == solid->color.blue_short) {
	    _comac_output_stream_printf (ctx->stream, "%f g", solid->color.red);
	} else {
	    _comac_output_stream_printf (ctx->stream,
					 "%f %f %f rgb",
					 solid->color.red,
					 solid->color.green,
					 solid->color.blue);
	}
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_gradient_color_stops (comac_gradient_pattern_t *gradient,
			    comac_output_stream_t *output)
{
    unsigned int n;

    for (n = 0; n < gradient->n_stops; n++) {
	_comac_output_stream_printf (output,
				     "\n  %f %f %f %f %f add-color-stop",
				     gradient->stops[n].offset,
				     gradient->stops[n].color.red,
				     gradient->stops[n].color.green,
				     gradient->stops[n].color.blue,
				     gradient->stops[n].color.alpha);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_linear_pattern (comac_script_surface_t *surface,
		      const comac_pattern_t *pattern)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_linear_pattern_t *linear;

    linear = (comac_linear_pattern_t *) pattern;

    _comac_output_stream_printf (ctx->stream,
				 "%f %f %f %f linear",
				 linear->pd1.x,
				 linear->pd1.y,
				 linear->pd2.x,
				 linear->pd2.y);
    return _emit_gradient_color_stops (&linear->base, ctx->stream);
}

static comac_status_t
_emit_radial_pattern (comac_script_surface_t *surface,
		      const comac_pattern_t *pattern)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_radial_pattern_t *radial;

    radial = (comac_radial_pattern_t *) pattern;

    _comac_output_stream_printf (ctx->stream,
				 "%f %f %f %f %f %f radial",
				 radial->cd1.center.x,
				 radial->cd1.center.y,
				 radial->cd1.radius,
				 radial->cd2.center.x,
				 radial->cd2.center.y,
				 radial->cd2.radius);
    return _emit_gradient_color_stops (&radial->base, ctx->stream);
}

static comac_status_t
_emit_mesh_pattern (comac_script_surface_t *surface,
		    const comac_pattern_t *pattern)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_pattern_t *mesh;
    comac_status_t status;
    unsigned int i, n;

    mesh = (comac_pattern_t *) pattern;
    status = comac_mesh_pattern_get_patch_count (mesh, &n);
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (ctx->stream, "mesh");
    for (i = 0; i < n; i++) {
	comac_path_t *path;
	comac_path_data_t *data;
	int j;

	_comac_output_stream_printf (ctx->stream, "\n  begin-patch");

	path = comac_mesh_pattern_get_path (mesh, i);
	if (unlikely (path->status))
	    return path->status;

	for (j = 0; j < path->num_data; j += data[0].header.length) {
	    data = &path->data[j];
	    switch (data->header.type) {
	    case COMAC_PATH_MOVE_TO:
		_comac_output_stream_printf (ctx->stream,
					     "\n  %f %f m",
					     data[1].point.x,
					     data[1].point.y);
		break;
	    case COMAC_PATH_LINE_TO:
		_comac_output_stream_printf (ctx->stream,
					     "\n  %f %f l",
					     data[1].point.x,
					     data[1].point.y);
		break;
	    case COMAC_PATH_CURVE_TO:
		_comac_output_stream_printf (ctx->stream,
					     "\n  %f %f %f %f %f %f c",
					     data[1].point.x,
					     data[1].point.y,
					     data[2].point.x,
					     data[2].point.y,
					     data[3].point.x,
					     data[3].point.y);
		break;
	    case COMAC_PATH_CLOSE_PATH:
		break;
	    }
	}
	comac_path_destroy (path);

	for (j = 0; j < 4; j++) {
	    double x, y;

	    status = comac_mesh_pattern_get_control_point (mesh, i, j, &x, &y);
	    if (unlikely (status))
		return status;
	    _comac_output_stream_printf (ctx->stream,
					 "\n  %d %f %f set-control-point",
					 j,
					 x,
					 y);
	}

	for (j = 0; j < 4; j++) {
	    double r, g, b, a;

	    status = comac_mesh_pattern_get_corner_color_rgba (mesh,
							       i,
							       j,
							       &r,
							       &g,
							       &b,
							       &a);
	    if (unlikely (status))
		return status;

	    _comac_output_stream_printf (ctx->stream,
					 "\n  %d %f %f %f %f set-corner-color",
					 j,
					 r,
					 g,
					 b,
					 a);
	}

	_comac_output_stream_printf (ctx->stream, "\n  end-patch");
    }

    return COMAC_STATUS_SUCCESS;
}

struct script_snapshot {
    comac_surface_t base;
};

static comac_status_t
script_snapshot_finish (void *abstract_surface)
{
    return COMAC_STATUS_SUCCESS;
}

static const comac_surface_backend_t script_snapshot_backend = {
    COMAC_SURFACE_TYPE_SCRIPT,
    script_snapshot_finish,
};

static void
detach_snapshot (comac_surface_t *abstract_surface)
{
    comac_script_surface_t *surface =
	(comac_script_surface_t *) abstract_surface;
    comac_script_context_t *ctx = to_context (surface);

    _comac_output_stream_printf (ctx->stream,
				 "/s%d undef\n",
				 surface->base.unique_id);
}

static void
attach_snapshot (comac_script_context_t *ctx, comac_surface_t *source)
{
    struct script_snapshot *surface;

    if (! ctx->attach_snapshots)
	return;

    surface = _comac_malloc (sizeof (*surface));
    if (unlikely (surface == NULL))
	return;

    _comac_surface_init (&surface->base,
			 &script_snapshot_backend,
			 &ctx->base,
			 source->content,
			 source->is_vector,
			 source->colorspace);

    _comac_output_stream_printf (ctx->stream,
				 "dup /s%d exch def ",
				 surface->base.unique_id);

    _comac_surface_attach_snapshot (source, &surface->base, detach_snapshot);
    comac_surface_destroy (&surface->base);
}

static comac_status_t
_emit_recording_surface_pattern (comac_script_surface_t *surface,
				 comac_recording_surface_t *source)
{
    comac_script_implicit_context_t old_cr;
    comac_script_context_t *ctx = to_context (surface);
    comac_script_surface_t *similar;
    comac_surface_t *snapshot;
    comac_rectangle_t r, *extents;
    comac_status_t status;

    snapshot =
	_comac_surface_has_snapshot (&source->base, &script_snapshot_backend);
    if (snapshot) {
	_comac_output_stream_printf (ctx->stream, "s%d", snapshot->unique_id);
	return COMAC_INT_STATUS_SUCCESS;
    }

    extents = NULL;
    if (_comac_recording_surface_get_bounds (&source->base, &r))
	extents = &r;

    similar = _comac_script_surface_create_internal (ctx,
						     source->base.content,
						     extents,
						     NULL);
    if (unlikely (similar->base.status))
	return similar->base.status;

    similar->base.is_clear = TRUE;

    _comac_output_stream_printf (ctx->stream,
				 "//%s ",
				 _content_to_string (source->base.content));
    if (extents) {
	_comac_output_stream_printf (ctx->stream,
				     "[%f %f %f %f]",
				     extents->x,
				     extents->y,
				     extents->width,
				     extents->height);
    } else
	_comac_output_stream_puts (ctx->stream, "[]");
    _comac_output_stream_puts (ctx->stream, " record\n");

    attach_snapshot (ctx, &source->base);

    _comac_output_stream_puts (ctx->stream, "dup context\n");

    target_push (similar);
    similar->emitted = TRUE;

    old_cr = surface->cr;
    _comac_script_implicit_context_init (&surface->cr);
    status = _comac_recording_surface_replay (&source->base, &similar->base);
    surface->cr = old_cr;

    if (unlikely (status)) {
	comac_surface_destroy (&similar->base);
	return status;
    }

    comac_list_del (&similar->operand.link);
    assert (target_is_active (surface));

    _comac_output_stream_puts (ctx->stream, "pop ");
    comac_surface_destroy (&similar->base);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_script_surface_pattern (comac_script_surface_t *surface,
			      comac_script_surface_t *source)
{
    _get_target (source);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_write_image_surface (comac_output_stream_t *output,
		      const comac_image_surface_t *image)
{
    int row, width;
    ptrdiff_t stride;
    uint8_t row_stack[COMAC_STACK_BUFFER_SIZE];
    uint8_t *rowdata;
    uint8_t *data;

    stride = image->stride;
    width = image->width;
    data = image->data;
#if WORDS_BIGENDIAN
    switch (image->format) {
    case COMAC_FORMAT_A1:
	for (row = image->height; row--;) {
	    _comac_output_stream_write (output, data, (width + 7) / 8);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_A8:
	for (row = image->height; row--;) {
	    _comac_output_stream_write (output, data, width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_RGB16_565:
	for (row = image->height; row--;) {
	    _comac_output_stream_write (output, data, 2 * width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_RGB24:
	for (row = image->height; row--;) {
	    int col;
	    rowdata = data;
	    for (col = width; col--;) {
		_comac_output_stream_write (output, rowdata, 3);
		rowdata += 4;
	    }
	    data += stride;
	}
	break;
    case COMAC_FORMAT_ARGB32:
	for (row = image->height; row--;) {
	    _comac_output_stream_write (output, data, 4 * width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_INVALID:
    default:
	ASSERT_NOT_REACHED;
	break;
    }
#else
    if (stride > ARRAY_LENGTH (row_stack)) {
	rowdata = _comac_malloc (stride);
	if (unlikely (rowdata == NULL))
	    return _comac_error (COMAC_STATUS_NO_MEMORY);
    } else
	rowdata = row_stack;

    switch (image->format) {
    case COMAC_FORMAT_A1:
	for (row = image->height; row--;) {
	    int col;
	    for (col = 0; col < (width + 7) / 8; col++)
		rowdata[col] = COMAC_BITSWAP8 (data[col]);
	    _comac_output_stream_write (output, rowdata, (width + 7) / 8);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_A8:
	for (row = image->height; row--;) {
	    _comac_output_stream_write (output, data, width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_RGB16_565:
	for (row = image->height; row--;) {
	    uint16_t *src = (uint16_t *) data;
	    uint16_t *dst = (uint16_t *) rowdata;
	    int col;
	    for (col = 0; col < width; col++)
		dst[col] = bswap_16 (src[col]);
	    _comac_output_stream_write (output, rowdata, 2 * width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_RGB24:
	for (row = image->height; row--;) {
	    uint8_t *src = data;
	    int col;
	    for (col = 0; col < width; col++) {
		rowdata[3 * col + 2] = *src++;
		rowdata[3 * col + 1] = *src++;
		rowdata[3 * col + 0] = *src++;
		src++;
	    }
	    _comac_output_stream_write (output, rowdata, 3 * width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_RGB30:
    case COMAC_FORMAT_ARGB32:
	for (row = image->height; row--;) {
	    uint32_t *src = (uint32_t *) data;
	    uint32_t *dst = (uint32_t *) rowdata;
	    int col;
	    for (col = 0; col < width; col++)
		dst[col] = bswap_32 (src[col]);
	    _comac_output_stream_write (output, rowdata, 4 * width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_RGB96F:
	for (row = image->height; row--;) {
	    _comac_output_stream_write (output, data, 12 * width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_RGBA128F:
	for (row = image->height; row--;) {
	    _comac_output_stream_write (output, data, 16 * width);
	    data += stride;
	}
	break;
    case COMAC_FORMAT_INVALID:
    default:
	ASSERT_NOT_REACHED;
	break;
    }
    if (rowdata != row_stack)
	free (rowdata);
#endif

    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_png_surface (comac_script_surface_t *surface,
		   comac_image_surface_t *image)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_output_stream_t *base85_stream;
    comac_status_t status;
    const uint8_t *mime_data;
    unsigned long mime_data_length;

    comac_surface_get_mime_data (&image->base,
				 COMAC_MIME_TYPE_PNG,
				 &mime_data,
				 &mime_data_length);
    if (mime_data == NULL)
	return COMAC_INT_STATUS_UNSUPPORTED;

    _comac_output_stream_printf (ctx->stream,
				 "<< "
				 "/width %d "
				 "/height %d "
				 "/format //%s "
				 "/mime-type (image/png) "
				 "/source <~",
				 image->width,
				 image->height,
				 _format_to_string (image->format));

    base85_stream = _comac_base85_stream_create (ctx->stream);
    _comac_output_stream_write (base85_stream, mime_data, mime_data_length);
    status = _comac_output_stream_destroy (base85_stream);
    if (unlikely (status))
	return status;

    _comac_output_stream_puts (ctx->stream, "~> >> image ");
    return COMAC_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_image_surface (comac_script_surface_t *surface,
		     comac_image_surface_t *image)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_output_stream_t *base85_stream;
    comac_output_stream_t *zlib_stream;
    comac_int_status_t status, status2;
    comac_surface_t *snapshot;
    const uint8_t *mime_data;
    unsigned long mime_data_length;

    snapshot =
	_comac_surface_has_snapshot (&image->base, &script_snapshot_backend);
    if (snapshot) {
	_comac_output_stream_printf (ctx->stream, "s%u ", snapshot->unique_id);
	return COMAC_INT_STATUS_SUCCESS;
    }

    status = _emit_png_surface (surface, image);
    if (_comac_int_status_is_error (status)) {
	return status;
    } else if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	comac_image_surface_t *clone;
	uint32_t len;

	if (image->format == COMAC_FORMAT_INVALID) {
	    clone = _comac_image_surface_coerce (image);
	} else {
	    clone = (comac_image_surface_t *) comac_surface_reference (
		&image->base);
	}

	_comac_output_stream_printf (ctx->stream,
				     "<< "
				     "/width %d "
				     "/height %d "
				     "/format //%s "
				     "/source ",
				     clone->width,
				     clone->height,
				     _format_to_string (clone->format));

	switch (clone->format) {
	case COMAC_FORMAT_A1:
	    len = (clone->width + 7) / 8;
	    break;
	case COMAC_FORMAT_A8:
	    len = clone->width;
	    break;
	case COMAC_FORMAT_RGB16_565:
	    len = clone->width * 2;
	    break;
	case COMAC_FORMAT_RGB24:
	    len = clone->width * 3;
	    break;
	case COMAC_FORMAT_RGB30:
	case COMAC_FORMAT_ARGB32:
	    len = clone->width * 4;
	    break;
	case COMAC_FORMAT_RGB96F:
	    len = clone->width * 12;
	    break;
	case COMAC_FORMAT_RGBA128F:
	    len = clone->width * 16;
	    break;
	case COMAC_FORMAT_INVALID:
	default:
	    ASSERT_NOT_REACHED;
	    len = 0;
	    break;
	}
	len *= clone->height;

	if (len > 24) {
	    _comac_output_stream_puts (ctx->stream, "<|");

	    base85_stream = _comac_base85_stream_create (ctx->stream);

	    len = to_be32 (len);
	    _comac_output_stream_write (base85_stream, &len, sizeof (len));

	    zlib_stream = _comac_deflate_stream_create (base85_stream);
	    status = _write_image_surface (zlib_stream, clone);

	    status2 = _comac_output_stream_destroy (zlib_stream);
	    if (status == COMAC_INT_STATUS_SUCCESS)
		status = status2;
	    status2 = _comac_output_stream_destroy (base85_stream);
	    if (status == COMAC_INT_STATUS_SUCCESS)
		status = status2;
	    if (unlikely (status))
		return status;
	} else {
	    _comac_output_stream_puts (ctx->stream, "<~");

	    base85_stream = _comac_base85_stream_create (ctx->stream);
	    status = _write_image_surface (base85_stream, clone);
	    status2 = _comac_output_stream_destroy (base85_stream);
	    if (status == COMAC_INT_STATUS_SUCCESS)
		status = status2;
	    if (unlikely (status))
		return status;
	}
	_comac_output_stream_puts (ctx->stream, "~> >> image ");

	comac_surface_destroy (&clone->base);
    }

    comac_surface_get_mime_data (&image->base,
				 COMAC_MIME_TYPE_JPEG,
				 &mime_data,
				 &mime_data_length);
    if (mime_data != NULL) {
	_comac_output_stream_printf (ctx->stream,
				     "\n  (%s) <~",
				     COMAC_MIME_TYPE_JPEG);

	base85_stream = _comac_base85_stream_create (ctx->stream);
	_comac_output_stream_write (base85_stream, mime_data, mime_data_length);
	status = _comac_output_stream_destroy (base85_stream);
	if (unlikely (status))
	    return status;

	_comac_output_stream_puts (ctx->stream, "~> set-mime-data\n");
    }

    comac_surface_get_mime_data (&image->base,
				 COMAC_MIME_TYPE_JP2,
				 &mime_data,
				 &mime_data_length);
    if (mime_data != NULL) {
	_comac_output_stream_printf (ctx->stream,
				     "\n  (%s) <~",
				     COMAC_MIME_TYPE_JP2);

	base85_stream = _comac_base85_stream_create (ctx->stream);
	_comac_output_stream_write (base85_stream, mime_data, mime_data_length);
	status = _comac_output_stream_destroy (base85_stream);
	if (unlikely (status))
	    return status;

	_comac_output_stream_puts (ctx->stream, "~> set-mime-data\n");
    }

    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_image_surface_pattern (comac_script_surface_t *surface,
			     comac_surface_t *source)
{
    comac_image_surface_t *image;
    comac_status_t status;
    void *extra;

    status = _comac_surface_acquire_source_image (source, &image, &extra);
    if (likely (status == COMAC_STATUS_SUCCESS)) {
	status = _emit_image_surface (surface, image);
	_comac_surface_release_source_image (source, image, extra);
    }

    return status;
}

static comac_int_status_t
_emit_subsurface_pattern (comac_script_surface_t *surface,
			  comac_surface_subsurface_t *sub)
{
    comac_surface_t *source = sub->target;
    comac_int_status_t status;

    switch ((int) source->backend->type) {
    case COMAC_SURFACE_TYPE_RECORDING:
	status = _emit_recording_surface_pattern (
	    surface,
	    (comac_recording_surface_t *) source);
	break;
    case COMAC_SURFACE_TYPE_SCRIPT:
	status =
	    _emit_script_surface_pattern (surface,
					  (comac_script_surface_t *) source);
	break;
    default:
	status = _emit_image_surface_pattern (surface, source);
	break;
    }
    if (unlikely (status))
	return status;

    _comac_output_stream_printf (to_context (surface)->stream,
				 "%d %d %d %d subsurface ",
				 sub->extents.x,
				 sub->extents.y,
				 sub->extents.width,
				 sub->extents.height);
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_surface_pattern (comac_script_surface_t *surface,
		       const comac_pattern_t *pattern)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_surface_pattern_t *surface_pattern;
    comac_surface_t *source, *snapshot, *free_me = NULL;
    comac_surface_t *take_snapshot = NULL;
    comac_int_status_t status;

    surface_pattern = (comac_surface_pattern_t *) pattern;
    source = surface_pattern->surface;

    if (_comac_surface_is_snapshot (source)) {
	snapshot =
	    _comac_surface_has_snapshot (source, &script_snapshot_backend);
	if (snapshot) {
	    _comac_output_stream_printf (ctx->stream,
					 "s%d pattern ",
					 snapshot->unique_id);
	    return COMAC_INT_STATUS_SUCCESS;
	}

	if (_comac_surface_snapshot_is_reused (source))
	    take_snapshot = source;

	free_me = source = _comac_surface_snapshot_get_target (source);
    }

    switch ((int) source->backend->type) {
    case COMAC_SURFACE_TYPE_RECORDING:
	status = _emit_recording_surface_pattern (
	    surface,
	    (comac_recording_surface_t *) source);
	break;
    case COMAC_SURFACE_TYPE_SCRIPT:
	status =
	    _emit_script_surface_pattern (surface,
					  (comac_script_surface_t *) source);
	break;
    case COMAC_SURFACE_TYPE_SUBSURFACE:
	status =
	    _emit_subsurface_pattern (surface,
				      (comac_surface_subsurface_t *) source);
	break;
    default:
	status = _emit_image_surface_pattern (surface, source);
	break;
    }
    comac_surface_destroy (free_me);
    if (unlikely (status))
	return status;

    if (take_snapshot)
	attach_snapshot (ctx, take_snapshot);

    _comac_output_stream_puts (ctx->stream, "pattern");
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_raster_pattern (comac_script_surface_t *surface,
		      const comac_pattern_t *pattern)
{
    comac_surface_t *source;
    comac_int_status_t status;

    source =
	_comac_raster_source_pattern_acquire (pattern, &surface->base, NULL);
    if (unlikely (source == NULL)) {
	ASSERT_NOT_REACHED;
	return COMAC_INT_STATUS_UNSUPPORTED;
    }
    if (unlikely (source->status))
	return source->status;

    status = _emit_image_surface_pattern (surface, source);
    _comac_raster_source_pattern_release (pattern, source);
    if (unlikely (status))
	return status;

    _comac_output_stream_puts (to_context (surface)->stream, "pattern");
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_pattern (comac_script_surface_t *surface, const comac_pattern_t *pattern)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_int_status_t status;
    comac_bool_t is_default_extend;
    comac_bool_t need_newline = TRUE;

    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SOLID:
	/* solid colors do not need filter/extend/matrix */
	return _emit_solid_pattern (surface, pattern);

    case COMAC_PATTERN_TYPE_LINEAR:
	status = _emit_linear_pattern (surface, pattern);
	is_default_extend = pattern->extend == COMAC_EXTEND_GRADIENT_DEFAULT;
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	status = _emit_radial_pattern (surface, pattern);
	is_default_extend = pattern->extend == COMAC_EXTEND_GRADIENT_DEFAULT;
	break;
    case COMAC_PATTERN_TYPE_MESH:
	status = _emit_mesh_pattern (surface, pattern);
	is_default_extend = TRUE;
	break;
    case COMAC_PATTERN_TYPE_SURFACE:
	status = _emit_surface_pattern (surface, pattern);
	is_default_extend = pattern->extend == COMAC_EXTEND_SURFACE_DEFAULT;
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	status = _emit_raster_pattern (surface, pattern);
	is_default_extend = pattern->extend == COMAC_EXTEND_SURFACE_DEFAULT;
	break;

    default:
	ASSERT_NOT_REACHED;
	status = COMAC_INT_STATUS_UNSUPPORTED;
    }
    if (unlikely (status))
	return status;

    if (! _comac_matrix_is_identity (&pattern->matrix)) {
	if (need_newline) {
	    _comac_output_stream_puts (ctx->stream, "\n ");
	    need_newline = FALSE;
	}

	_comac_output_stream_printf (ctx->stream,
				     " [%f %f %f %f %f %f] set-matrix\n ",
				     pattern->matrix.xx,
				     pattern->matrix.yx,
				     pattern->matrix.xy,
				     pattern->matrix.yy,
				     pattern->matrix.x0,
				     pattern->matrix.y0);
    }

    /* XXX need to discriminate the user explicitly setting the default */
    if (pattern->filter != COMAC_FILTER_DEFAULT) {
	if (need_newline) {
	    _comac_output_stream_puts (ctx->stream, "\n ");
	    need_newline = FALSE;
	}

	_comac_output_stream_printf (ctx->stream,
				     " //%s set-filter\n ",
				     _filter_to_string (pattern->filter));
    }
    if (! is_default_extend) {
	if (need_newline) {
	    _comac_output_stream_puts (ctx->stream, "\n ");
	    need_newline = FALSE;
	}

	_comac_output_stream_printf (ctx->stream,
				     " //%s set-extend\n ",
				     _extend_to_string (pattern->extend));
    }

    if (need_newline)
	_comac_output_stream_puts (ctx->stream, "\n ");

    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_identity (comac_script_surface_t *surface, comac_bool_t *matrix_updated)
{
    assert (target_is_active (surface));

    if (_comac_matrix_is_identity (&surface->cr.current_ctm))
	return COMAC_INT_STATUS_SUCCESS;

    _comac_output_stream_puts (to_context (surface)->stream,
			       "identity set-matrix\n");

    *matrix_updated = TRUE;
    comac_matrix_init_identity (&surface->cr.current_ctm);

    return COMAC_INT_STATUS_SUCCESS;
}

static comac_int_status_t
_emit_source (comac_script_surface_t *surface,
	      comac_operator_t op,
	      const comac_pattern_t *source)
{
    comac_bool_t matrix_updated = FALSE;
    comac_int_status_t status;

    assert (target_is_active (surface));

    if (op == COMAC_OPERATOR_CLEAR) {
	/* the source is ignored, so don't change it */
	return COMAC_INT_STATUS_SUCCESS;
    }

    if (_comac_pattern_equal (&surface->cr.current_source.base, source))
	return COMAC_INT_STATUS_SUCCESS;

    _comac_pattern_fini (&surface->cr.current_source.base);
    status =
	_comac_pattern_init_copy (&surface->cr.current_source.base, source);
    if (unlikely (status))
	return status;

    status = _emit_identity (surface, &matrix_updated);
    if (unlikely (status))
	return status;

    status = _emit_pattern (surface, source);
    if (unlikely (status))
	return status;

    assert (target_is_active (surface));
    _comac_output_stream_puts (to_context (surface)->stream, " set-source\n");
    return COMAC_INT_STATUS_SUCCESS;
}

static comac_status_t
_path_move_to (void *closure, const comac_point_t *point)
{
    _comac_output_stream_printf (closure,
				 " %f %f m",
				 _comac_fixed_to_double (point->x),
				 _comac_fixed_to_double (point->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_path_line_to (void *closure, const comac_point_t *point)
{
    _comac_output_stream_printf (closure,
				 " %f %f l",
				 _comac_fixed_to_double (point->x),
				 _comac_fixed_to_double (point->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_path_curve_to (void *closure,
		const comac_point_t *p1,
		const comac_point_t *p2,
		const comac_point_t *p3)
{
    _comac_output_stream_printf (closure,
				 " %f %f %f %f %f %f c",
				 _comac_fixed_to_double (p1->x),
				 _comac_fixed_to_double (p1->y),
				 _comac_fixed_to_double (p2->x),
				 _comac_fixed_to_double (p2->y),
				 _comac_fixed_to_double (p3->x),
				 _comac_fixed_to_double (p3->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_path_close (void *closure)
{
    _comac_output_stream_printf (closure, " h");

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_path_boxes (comac_script_surface_t *surface,
		  const comac_path_fixed_t *path)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_path_fixed_iter_t iter;
    comac_status_t status = COMAC_STATUS_SUCCESS;
    struct _comac_boxes_chunk *chunk;
    comac_boxes_t boxes;
    comac_box_t box;
    int i;

    _comac_boxes_init (&boxes);
    _comac_path_fixed_iter_init (&iter, path);
    while (_comac_path_fixed_iter_is_fill_box (&iter, &box)) {
	if (box.p1.y == box.p2.y || box.p1.x == box.p2.x)
	    continue;

	status = _comac_boxes_add (&boxes, COMAC_ANTIALIAS_DEFAULT, &box);
	if (unlikely (status)) {
	    _comac_boxes_fini (&boxes);
	    return status;
	}
    }

    if (! _comac_path_fixed_iter_at_end (&iter)) {
	_comac_boxes_fini (&boxes);
	return COMAC_STATUS_INVALID_PATH_DATA;
    }

    for (chunk = &boxes.chunks; chunk; chunk = chunk->next) {
	for (i = 0; i < chunk->count; i++) {
	    const comac_box_t *b = &chunk->base[i];
	    double x1 = _comac_fixed_to_double (b->p1.x);
	    double y1 = _comac_fixed_to_double (b->p1.y);
	    double x2 = _comac_fixed_to_double (b->p2.x);
	    double y2 = _comac_fixed_to_double (b->p2.y);

	    _comac_output_stream_printf (ctx->stream,
					 "\n  %f %f %f %f rectangle",
					 x1,
					 y1,
					 x2 - x1,
					 y2 - y1);
	}
    }

    _comac_boxes_fini (&boxes);
    return status;
}

static comac_status_t
_emit_path (comac_script_surface_t *surface,
	    const comac_path_fixed_t *path,
	    comac_bool_t is_fill)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_box_t box;
    comac_int_status_t status;

    assert (target_is_active (surface));
    assert (_comac_matrix_is_identity (&surface->cr.current_ctm));

    if (_comac_path_fixed_equal (&surface->cr.current_path, path))
	return COMAC_STATUS_SUCCESS;

    _comac_path_fixed_fini (&surface->cr.current_path);

    _comac_output_stream_puts (ctx->stream, "n");

    if (path == NULL) {
	_comac_path_fixed_init (&surface->cr.current_path);
	_comac_output_stream_puts (ctx->stream, "\n");
	return COMAC_STATUS_SUCCESS;
    }

    status = _comac_path_fixed_init_copy (&surface->cr.current_path, path);
    if (unlikely (status))
	return status;

    status = COMAC_INT_STATUS_UNSUPPORTED;
    if (_comac_path_fixed_is_rectangle (path, &box)) {
	double x1 = _comac_fixed_to_double (box.p1.x);
	double y1 = _comac_fixed_to_double (box.p1.y);
	double x2 = _comac_fixed_to_double (box.p2.x);
	double y2 = _comac_fixed_to_double (box.p2.y);

	assert (x1 > -9999);

	_comac_output_stream_printf (ctx->stream,
				     " %f %f %f %f rectangle",
				     x1,
				     y1,
				     x2 - x1,
				     y2 - y1);
	status = COMAC_INT_STATUS_SUCCESS;
    } else if (is_fill && _comac_path_fixed_fill_is_rectilinear (path)) {
	status = _emit_path_boxes (surface, path);
    }

    if (status == COMAC_INT_STATUS_UNSUPPORTED) {
	status = _comac_path_fixed_interpret (path,
					      _path_move_to,
					      _path_line_to,
					      _path_curve_to,
					      _path_close,
					      ctx->stream);
    }

    _comac_output_stream_puts (ctx->stream, "\n");

    return status;
}
static comac_bool_t
_scaling_matrix_equal (const comac_matrix_t *a, const comac_matrix_t *b)
{
    return fabs (a->xx - b->xx) < 1e-5 && fabs (a->xy - b->xy) < 1e-5 &&
	   fabs (a->yx - b->yx) < 1e-5 && fabs (a->yy - b->yy) < 1e-5;
}

static comac_status_t
_emit_scaling_matrix (comac_script_surface_t *surface,
		      const comac_matrix_t *ctm,
		      comac_bool_t *matrix_updated)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_bool_t was_identity;
    assert (target_is_active (surface));

    if (_scaling_matrix_equal (&surface->cr.current_ctm, ctm))
	return COMAC_STATUS_SUCCESS;

    was_identity = _comac_matrix_is_identity (&surface->cr.current_ctm);

    *matrix_updated = TRUE;
    surface->cr.current_ctm = *ctm;
    surface->cr.current_ctm.x0 = 0.;
    surface->cr.current_ctm.y0 = 0.;

    if (_comac_matrix_is_identity (&surface->cr.current_ctm)) {
	_comac_output_stream_puts (ctx->stream, "identity set-matrix\n");
    } else if (was_identity && fabs (ctm->yx) < 1e-5 && fabs (ctm->xy) < 1e-5) {
	_comac_output_stream_printf (ctx->stream,
				     "%f %f scale\n",
				     ctm->xx,
				     ctm->yy);
    } else {
	_comac_output_stream_printf (ctx->stream,
				     "[%f %f %f %f 0 0] set-matrix\n",
				     ctm->xx,
				     ctm->yx,
				     ctm->xy,
				     ctm->yy);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_font_matrix (comac_script_surface_t *surface,
		   const comac_matrix_t *font_matrix)
{
    comac_script_context_t *ctx = to_context (surface);
    assert (target_is_active (surface));

    if (memcmp (&surface->cr.current_font_matrix,
		font_matrix,
		sizeof (comac_matrix_t)) == 0) {
	return COMAC_STATUS_SUCCESS;
    }

    surface->cr.current_font_matrix = *font_matrix;

    if (_comac_matrix_is_identity (font_matrix)) {
	_comac_output_stream_puts (ctx->stream, "identity set-font-matrix\n");
    } else {
	_comac_output_stream_printf (ctx->stream,
				     "[%f %f %f %f %f %f] set-font-matrix\n",
				     font_matrix->xx,
				     font_matrix->yx,
				     font_matrix->xy,
				     font_matrix->yy,
				     font_matrix->x0,
				     font_matrix->y0);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
_comac_script_surface_create_similar (void *abstract_surface,
				      comac_content_t content,
				      int width,
				      int height)
{
    comac_script_surface_t *surface, *other = abstract_surface;
    comac_surface_t *passthrough = NULL;
    comac_script_context_t *ctx;
    comac_rectangle_t extents;
    comac_status_t status;

    ctx = to_context (other);

    status = comac_device_acquire (&ctx->base);
    if (unlikely (status))
	return _comac_surface_create_in_error (status);

    if (! other->emitted) {
	status = _emit_surface (other);
	if (unlikely (status)) {
	    comac_device_release (&ctx->base);
	    return _comac_surface_create_in_error (status);
	}

	target_push (other);
    }

    if (_comac_surface_wrapper_is_active (&other->wrapper)) {
	passthrough = _comac_surface_wrapper_create_similar (&other->wrapper,
							     content,
							     width,
							     height);
	if (unlikely (passthrough->status)) {
	    comac_device_release (&ctx->base);
	    return passthrough;
	}
    }

    extents.x = extents.y = 0;
    extents.width = width;
    extents.height = height;
    surface = _comac_script_surface_create_internal (ctx,
						     content,
						     &extents,
						     passthrough);
    comac_surface_destroy (passthrough);

    if (unlikely (surface->base.status)) {
	comac_device_release (&ctx->base);
	return &surface->base;
    }

    _get_target (other);
    _comac_output_stream_printf (
	ctx->stream,
	"%u %u //%s similar dup /s%u exch def context\n",
	width,
	height,
	_content_to_string (content),
	surface->base.unique_id);

    surface->emitted = TRUE;
    surface->defined = TRUE;
    surface->base.is_clear = TRUE;
    target_push (surface);

    comac_device_release (&ctx->base);
    return &surface->base;
}

static comac_status_t
_device_flush (void *abstract_device)
{
    comac_script_context_t *ctx = abstract_device;

    return _comac_output_stream_flush (ctx->stream);
}

static void
_device_destroy (void *abstract_device)
{
    comac_script_context_t *ctx = abstract_device;
    comac_status_t status;

    while (! comac_list_is_empty (&ctx->fonts)) {
	comac_script_font_t *font;

	font = comac_list_first_entry (&ctx->fonts, comac_script_font_t, link);
	comac_list_del (&font->base.link);
	comac_list_del (&font->link);
	free (font);
    }

    _bitmap_fini (ctx->surface_id.next);
    _bitmap_fini (ctx->font_id.next);

    if (ctx->owns_stream)
	status = _comac_output_stream_destroy (ctx->stream);

    free (ctx);
}

static comac_surface_t *
_comac_script_surface_source (void *abstract_surface,
			      comac_rectangle_int_t *extents)
{
    comac_script_surface_t *surface = abstract_surface;

    if (extents) {
	extents->x = extents->y = 0;
	extents->width = surface->width;
	extents->height = surface->height;
    }

    return &surface->base;
}

static comac_status_t
_comac_script_surface_acquire_source_image (void *abstract_surface,
					    comac_image_surface_t **image_out,
					    void **image_extra)
{
    comac_script_surface_t *surface = abstract_surface;

    if (_comac_surface_wrapper_is_active (&surface->wrapper)) {
	return _comac_surface_wrapper_acquire_source_image (&surface->wrapper,
							    image_out,
							    image_extra);
    }

    return COMAC_INT_STATUS_UNSUPPORTED;
}

static void
_comac_script_surface_release_source_image (void *abstract_surface,
					    comac_image_surface_t *image,
					    void *image_extra)
{
    comac_script_surface_t *surface = abstract_surface;

    assert (_comac_surface_wrapper_is_active (&surface->wrapper));
    _comac_surface_wrapper_release_source_image (&surface->wrapper,
						 image,
						 image_extra);
}

static comac_status_t
_comac_script_surface_finish (void *abstract_surface)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_script_context_t *ctx = to_context (surface);
    comac_status_t status = COMAC_STATUS_SUCCESS, status2;

    _comac_surface_wrapper_fini (&surface->wrapper);

    free (surface->cr.current_style.dash);
    surface->cr.current_style.dash = NULL;

    _comac_pattern_fini (&surface->cr.current_source.base);
    _comac_path_fixed_fini (&surface->cr.current_path);
    _comac_surface_clipper_reset (&surface->clipper);

    status = comac_device_acquire (&ctx->base);
    if (unlikely (status))
	return status;

    if (surface->emitted) {
	assert (! surface->active);

	if (! comac_list_is_empty (&surface->operand.link)) {
	    if (! ctx->active) {
		if (target_is_active (surface)) {
		    _comac_output_stream_printf (ctx->stream, "pop\n");
		} else {
		    int depth = target_depth (surface);
		    if (depth == 1) {
			_comac_output_stream_printf (ctx->stream, "exch pop\n");
		    } else {
			_comac_output_stream_printf (ctx->stream,
						     "%d -1 roll pop\n",
						     depth);
		    }
		}
		comac_list_del (&surface->operand.link);
	    } else {
		struct deferred_finish *link = _comac_malloc (sizeof (*link));
		if (link == NULL) {
		    status2 = _comac_error (COMAC_STATUS_NO_MEMORY);
		    if (status == COMAC_STATUS_SUCCESS)
			status = status2;
		    comac_list_del (&surface->operand.link);
		} else {
		    link->operand.type = DEFERRED;
		    comac_list_swap (&link->operand.link,
				     &surface->operand.link);
		    comac_list_add (&link->link, &ctx->deferred);
		}
	    }
	}

	if (surface->defined) {
	    _comac_output_stream_printf (ctx->stream,
					 "/s%u undef\n",
					 surface->base.unique_id);
	}
    }

    if (status == COMAC_STATUS_SUCCESS)
	status = _comac_output_stream_flush (to_context (surface)->stream);

    comac_device_release (&ctx->base);

    return status;
}

static comac_int_status_t
_comac_script_surface_copy_page (void *abstract_surface)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_status_t status;

    status = comac_device_acquire (surface->base.device);
    if (unlikely (status))
	return status;

    status = _emit_context (surface);
    if (unlikely (status))
	goto BAIL;

    _comac_output_stream_puts (to_context (surface)->stream, "copy-page\n");

BAIL:
    comac_device_release (surface->base.device);
    return status;
}

static comac_int_status_t
_comac_script_surface_show_page (void *abstract_surface)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_status_t status;

    status = comac_device_acquire (surface->base.device);
    if (unlikely (status))
	return status;

    status = _emit_context (surface);
    if (unlikely (status))
	goto BAIL;

    _comac_output_stream_puts (to_context (surface)->stream, "show-page\n");

BAIL:
    comac_device_release (surface->base.device);
    return status;
}

static comac_status_t
_comac_script_surface_clipper_intersect_clip_path (
    comac_surface_clipper_t *clipper,
    comac_path_fixed_t *path,
    comac_fill_rule_t fill_rule,
    double tolerance,
    comac_antialias_t antialias)
{
    comac_script_surface_t *surface =
	comac_container_of (clipper, comac_script_surface_t, clipper);
    comac_script_context_t *ctx = to_context (surface);
    comac_bool_t matrix_updated = FALSE;
    comac_status_t status;
    comac_box_t box;

    status = _emit_context (surface);
    if (unlikely (status))
	return status;

    if (path == NULL) {
	if (surface->cr.has_clip) {
	    _comac_output_stream_puts (ctx->stream, "reset-clip\n");
	    surface->cr.has_clip = FALSE;
	}
	return COMAC_STATUS_SUCCESS;
    }

    /* skip the trivial clip covering the surface extents */
    if (surface->width >= 0 && surface->height >= 0 &&
	_comac_path_fixed_is_box (path, &box)) {
	if (box.p1.x <= 0 && box.p1.y <= 0 &&
	    box.p2.x >= _comac_fixed_from_double (surface->width) &&
	    box.p2.y >= _comac_fixed_from_double (surface->height)) {
	    return COMAC_STATUS_SUCCESS;
	}
    }

    status = _emit_identity (surface, &matrix_updated);
    if (unlikely (status))
	return status;

    status = _emit_fill_rule (surface, fill_rule);
    if (unlikely (status))
	return status;

    if (path->has_curve_to) {
	status = _emit_tolerance (surface, tolerance, matrix_updated);
	if (unlikely (status))
	    return status;
    }

    if (! _comac_path_fixed_fill_maybe_region (path)) {
	status = _emit_antialias (surface, antialias);
	if (unlikely (status))
	    return status;
    }

    status = _emit_path (surface, path, TRUE);
    if (unlikely (status))
	return status;

    _comac_output_stream_puts (ctx->stream, "clip+\n");
    surface->cr.has_clip = TRUE;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
active (comac_script_surface_t *surface)
{
    comac_status_t status;

    status = comac_device_acquire (surface->base.device);
    if (unlikely (status))
	return status;

    if (surface->active++ == 0)
	to_context (surface)->active++;

    return COMAC_STATUS_SUCCESS;
}

static void
inactive (comac_script_surface_t *surface)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_list_t sorted;

    assert (surface->active > 0);
    if (--surface->active)
	goto DONE;

    assert (ctx->active > 0);
    if (--ctx->active)
	goto DONE;

    comac_list_init (&sorted);
    while (! comac_list_is_empty (&ctx->deferred)) {
	struct deferred_finish *df;
	comac_list_t *operand;
	int depth;

	df = comac_list_first_entry (&ctx->deferred,
				     struct deferred_finish,
				     link);

	depth = 0;
	comac_list_foreach (operand, &ctx->operands)
	{
	    if (operand == &df->operand.link)
		break;
	    depth++;
	}

	df->operand.type = depth;

	if (comac_list_is_empty (&sorted)) {
	    comac_list_move (&df->link, &sorted);
	} else {
	    struct deferred_finish *pos;

	    comac_list_foreach_entry (pos,
				      struct deferred_finish,
				      &sorted,
				      link)
	    {
		if (df->operand.type < pos->operand.type)
		    break;
	    }
	    comac_list_move_tail (&df->link, &pos->link);
	}
    }

    while (! comac_list_is_empty (&sorted)) {
	struct deferred_finish *df;
	comac_list_t *operand;
	int depth;

	df = comac_list_first_entry (&sorted, struct deferred_finish, link);

	depth = 0;
	comac_list_foreach (operand, &ctx->operands)
	{
	    if (operand == &df->operand.link)
		break;
	    depth++;
	}

	if (depth == 0) {
	    _comac_output_stream_printf (ctx->stream, "pop\n");
	} else if (depth == 1) {
	    _comac_output_stream_printf (ctx->stream, "exch pop\n");
	} else {
	    _comac_output_stream_printf (ctx->stream,
					 "%d -1 roll pop\n",
					 depth);
	}

	comac_list_del (&df->operand.link);
	comac_list_del (&df->link);
	free (df);
    }

DONE:
    comac_device_release (surface->base.device);
}

static comac_int_status_t
_comac_script_surface_paint (void *abstract_surface,
			     comac_operator_t op,
			     const comac_pattern_t *source,
			     const comac_clip_t *clip)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_status_t status;

    status = active (surface);
    if (unlikely (status))
	return status;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	goto BAIL;

    status = _emit_context (surface);
    if (unlikely (status))
	goto BAIL;

    status = _emit_source (surface, op, source);
    if (unlikely (status))
	goto BAIL;

    status = _emit_operator (surface, op);
    if (unlikely (status))
	goto BAIL;

    _comac_output_stream_puts (to_context (surface)->stream, "paint\n");

    inactive (surface);

    if (_comac_surface_wrapper_is_active (&surface->wrapper)) {
	return _comac_surface_wrapper_paint (&surface->wrapper,
					     op,
					     source,
					     clip);
    }

    return COMAC_STATUS_SUCCESS;

BAIL:
    inactive (surface);
    return status;
}

static comac_int_status_t
_comac_script_surface_mask (void *abstract_surface,
			    comac_operator_t op,
			    const comac_pattern_t *source,
			    const comac_pattern_t *mask,
			    const comac_clip_t *clip)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_status_t status;

    status = active (surface);
    if (unlikely (status))
	return status;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	goto BAIL;

    status = _emit_context (surface);
    if (unlikely (status))
	goto BAIL;

    status = _emit_source (surface, op, source);
    if (unlikely (status))
	goto BAIL;

    status = _emit_operator (surface, op);
    if (unlikely (status))
	goto BAIL;

    if (_comac_pattern_equal (source, mask)) {
	_comac_output_stream_puts (to_context (surface)->stream, "/source get");
    } else {
	status = _emit_pattern (surface, mask);
	if (unlikely (status))
	    goto BAIL;
    }

    assert (surface->cr.current_operator == op);

    _comac_output_stream_puts (to_context (surface)->stream, " mask\n");

    inactive (surface);

    if (_comac_surface_wrapper_is_active (&surface->wrapper)) {
	return _comac_surface_wrapper_mask (&surface->wrapper,
					    op,
					    source,
					    mask,
					    clip);
    }

    return COMAC_STATUS_SUCCESS;

BAIL:
    inactive (surface);
    return status;
}

static comac_int_status_t
_comac_script_surface_stroke (void *abstract_surface,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_path_fixed_t *path,
			      const comac_stroke_style_t *style,
			      const comac_matrix_t *ctm,
			      const comac_matrix_t *ctm_inverse,
			      double tolerance,
			      comac_antialias_t antialias,
			      const comac_clip_t *clip)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_bool_t matrix_updated = FALSE;
    comac_status_t status;

    status = active (surface);
    if (unlikely (status))
	return status;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	goto BAIL;

    status = _emit_context (surface);
    if (unlikely (status))
	goto BAIL;

    status = _emit_identity (surface, &matrix_updated);
    if (unlikely (status))
	goto BAIL;

    status = _emit_path (surface, path, FALSE);
    if (unlikely (status))
	goto BAIL;

    status = _emit_source (surface, op, source);
    if (unlikely (status))
	goto BAIL;

    status = _emit_scaling_matrix (surface, ctm, &matrix_updated);
    if (unlikely (status))
	goto BAIL;

    status = _emit_operator (surface, op);
    if (unlikely (status))
	goto BAIL;

    if (_scaling_matrix_equal (&surface->cr.current_ctm,
			       &surface->cr.current_stroke_matrix)) {
	matrix_updated = FALSE;
    } else {
	matrix_updated = TRUE;
	surface->cr.current_stroke_matrix = surface->cr.current_ctm;
    }

    status = _emit_stroke_style (surface, style, matrix_updated);
    if (unlikely (status))
	goto BAIL;

    status = _emit_tolerance (surface, tolerance, matrix_updated);
    if (unlikely (status))
	goto BAIL;

    status = _emit_antialias (surface, antialias);
    if (unlikely (status))
	goto BAIL;

    _comac_output_stream_puts (to_context (surface)->stream, "stroke+\n");

    inactive (surface);

    if (_comac_surface_wrapper_is_active (&surface->wrapper)) {
	return _comac_surface_wrapper_stroke (&surface->wrapper,
					      op,
					      source,
					      path,
					      style,
					      ctm,
					      ctm_inverse,
					      tolerance,
					      antialias,
					      clip);
    }

    return COMAC_STATUS_SUCCESS;

BAIL:
    inactive (surface);
    return status;
}

static comac_int_status_t
_comac_script_surface_fill (void *abstract_surface,
			    comac_operator_t op,
			    const comac_pattern_t *source,
			    const comac_path_fixed_t *path,
			    comac_fill_rule_t fill_rule,
			    double tolerance,
			    comac_antialias_t antialias,
			    const comac_clip_t *clip)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_bool_t matrix_updated = FALSE;
    comac_status_t status;
    comac_box_t box;

    status = active (surface);
    if (unlikely (status))
	return status;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	goto BAIL;

    status = _emit_context (surface);
    if (unlikely (status))
	goto BAIL;

    status = _emit_identity (surface, &matrix_updated);
    if (unlikely (status))
	goto BAIL;

    status = _emit_source (surface, op, source);
    if (unlikely (status))
	goto BAIL;

    if (! _comac_path_fixed_is_box (path, &box)) {
	status = _emit_fill_rule (surface, fill_rule);
	if (unlikely (status))
	    goto BAIL;
    }

    if (path->has_curve_to) {
	status = _emit_tolerance (surface, tolerance, matrix_updated);
	if (unlikely (status))
	    goto BAIL;
    }

    if (! _comac_path_fixed_fill_maybe_region (path)) {
	status = _emit_antialias (surface, antialias);
	if (unlikely (status))
	    goto BAIL;
    }

    status = _emit_path (surface, path, TRUE);
    if (unlikely (status))
	goto BAIL;

    status = _emit_operator (surface, op);
    if (unlikely (status))
	goto BAIL;

    _comac_output_stream_puts (to_context (surface)->stream, "fill+\n");

    inactive (surface);

    if (_comac_surface_wrapper_is_active (&surface->wrapper)) {
	return _comac_surface_wrapper_fill (&surface->wrapper,
					    op,
					    source,
					    path,
					    fill_rule,
					    tolerance,
					    antialias,
					    clip);
    }

    return COMAC_STATUS_SUCCESS;

BAIL:
    inactive (surface);
    return status;
}

static comac_surface_t *
_comac_script_surface_snapshot (void *abstract_surface)
{
    comac_script_surface_t *surface = abstract_surface;

    if (_comac_surface_wrapper_is_active (&surface->wrapper))
	return _comac_surface_wrapper_snapshot (&surface->wrapper);

    return NULL;
}

static comac_bool_t
_comac_script_surface_has_show_text_glyphs (void *abstract_surface)
{
    return TRUE;
}

static const char *
_subpixel_order_to_string (comac_subpixel_order_t subpixel_order)
{
    static const char *names[] = {
	"SUBPIXEL_ORDER_DEFAULT", /* COMAC_SUBPIXEL_ORDER_DEFAULT */
	"SUBPIXEL_ORDER_RGB",	  /* COMAC_SUBPIXEL_ORDER_RGB */
	"SUBPIXEL_ORDER_BGR",	  /* COMAC_SUBPIXEL_ORDER_BGR */
	"SUBPIXEL_ORDER_VRGB",	  /* COMAC_SUBPIXEL_ORDER_VRGB */
	"SUBPIXEL_ORDER_VBGR"	  /* COMAC_SUBPIXEL_ORDER_VBGR */
    };
    return names[subpixel_order];
}
static const char *
_hint_style_to_string (comac_hint_style_t hint_style)
{
    static const char *names[] = {
	"HINT_STYLE_DEFAULT", /* COMAC_HINT_STYLE_DEFAULT */
	"HINT_STYLE_NONE",    /* COMAC_HINT_STYLE_NONE */
	"HINT_STYLE_SLIGHT",  /* COMAC_HINT_STYLE_SLIGHT */
	"HINT_STYLE_MEDIUM",  /* COMAC_HINT_STYLE_MEDIUM */
	"HINT_STYLE_FULL"     /* COMAC_HINT_STYLE_FULL */
    };
    return names[hint_style];
}
static const char *
_hint_metrics_to_string (comac_hint_metrics_t hint_metrics)
{
    static const char *names[] = {
	"HINT_METRICS_DEFAULT", /* COMAC_HINT_METRICS_DEFAULT */
	"HINT_METRICS_OFF",	/* COMAC_HINT_METRICS_OFF */
	"HINT_METRICS_ON"	/* COMAC_HINT_METRICS_ON */
    };
    return names[hint_metrics];
}

static comac_status_t
_emit_font_options (comac_script_surface_t *surface,
		    comac_font_options_t *font_options)
{
    comac_script_context_t *ctx = to_context (surface);

    if (comac_font_options_equal (&surface->cr.current_font_options,
				  font_options)) {
	return COMAC_STATUS_SUCCESS;
    }

    _comac_output_stream_printf (ctx->stream, "<<");

    if (font_options->antialias != surface->cr.current_font_options.antialias) {
	_comac_output_stream_printf (
	    ctx->stream,
	    " /antialias //%s",
	    _antialias_to_string (font_options->antialias));
    }

    if (font_options->subpixel_order !=
	surface->cr.current_font_options.subpixel_order) {
	_comac_output_stream_printf (
	    ctx->stream,
	    " /subpixel-order //%s",
	    _subpixel_order_to_string (font_options->subpixel_order));
    }

    if (font_options->hint_style !=
	surface->cr.current_font_options.hint_style) {
	_comac_output_stream_printf (
	    ctx->stream,
	    " /hint-style //%s",
	    _hint_style_to_string (font_options->hint_style));
    }

    if (font_options->hint_metrics !=
	surface->cr.current_font_options.hint_metrics) {
	_comac_output_stream_printf (
	    ctx->stream,
	    " /hint-metrics //%s",
	    _hint_metrics_to_string (font_options->hint_metrics));
    }

    _comac_output_stream_printf (ctx->stream, " >> set-font-options\n");

    surface->cr.current_font_options = *font_options;
    return COMAC_STATUS_SUCCESS;
}

static void
_comac_script_scaled_font_fini (comac_scaled_font_private_t *abstract_private,
				comac_scaled_font_t *scaled_font)
{
    comac_script_font_t *priv = (comac_script_font_t *) abstract_private;
    comac_script_context_t *ctx =
	(comac_script_context_t *) abstract_private->key;
    comac_status_t status;

    status = comac_device_acquire (&ctx->base);
    if (likely (status == COMAC_STATUS_SUCCESS)) {
	_comac_output_stream_printf (ctx->stream,
				     "/f%lu undef /sf%lu undef\n",
				     priv->id,
				     priv->id);

	_bitmap_release_id (&ctx->font_id, priv->id);
	comac_device_release (&ctx->base);
    }

    comac_list_del (&priv->link);
    comac_list_del (&priv->base.link);
    free (priv);
}

static comac_script_font_t *
_comac_script_font_get (comac_script_context_t *ctx, comac_scaled_font_t *font)
{
    return (comac_script_font_t *) _comac_scaled_font_find_private (font, ctx);
}

static long unsigned
_comac_script_font_id (comac_script_context_t *ctx, comac_scaled_font_t *font)
{
    return _comac_script_font_get (ctx, font)->id;
}

static comac_status_t
_emit_type42_font (comac_script_surface_t *surface,
		   comac_scaled_font_t *scaled_font)
{
    comac_script_context_t *ctx = to_context (surface);
    const comac_scaled_font_backend_t *backend;
    comac_output_stream_t *base85_stream;
    comac_output_stream_t *zlib_stream;
    comac_status_t status, status2;
    unsigned long size;
    unsigned int load_flags;
    uint32_t len;
    uint8_t *buf;

    backend = scaled_font->backend;
    if (backend->load_truetype_table == NULL)
	return COMAC_INT_STATUS_UNSUPPORTED;

    size = 0;
    status = backend->load_truetype_table (scaled_font, 0, 0, NULL, &size);
    if (unlikely (status))
	return status;

    buf = _comac_malloc (size);
    if (unlikely (buf == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    status = backend->load_truetype_table (scaled_font, 0, 0, buf, &size);
    if (unlikely (status)) {
	free (buf);
	return status;
    }

#if COMAC_HAS_FT_FONT
    load_flags = _comac_ft_scaled_font_get_load_flags (scaled_font);
#else
    load_flags = 0;
#endif
    _comac_output_stream_printf (ctx->stream,
				 "<< "
				 "/type 42 "
				 "/index 0 "
				 "/flags %d "
				 "/source <|",
				 load_flags);

    base85_stream = _comac_base85_stream_create (ctx->stream);
    len = to_be32 (size);
    _comac_output_stream_write (base85_stream, &len, sizeof (len));

    zlib_stream = _comac_deflate_stream_create (base85_stream);

    _comac_output_stream_write (zlib_stream, buf, size);
    free (buf);

    status2 = _comac_output_stream_destroy (zlib_stream);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;

    status2 = _comac_output_stream_destroy (base85_stream);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;

    _comac_output_stream_printf (ctx->stream,
				 "~> >> font dup /f%lu exch def set-font-face",
				 _comac_script_font_id (ctx, scaled_font));

    return status;
}

static comac_status_t
_emit_scaled_font_init (comac_script_surface_t *surface,
			comac_scaled_font_t *scaled_font,
			comac_script_font_t **font_out)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_script_font_t *font_private;
    comac_int_status_t status;

    font_private = _comac_malloc (sizeof (comac_script_font_t));
    if (unlikely (font_private == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    _comac_scaled_font_attach_private (scaled_font,
				       &font_private->base,
				       ctx,
				       _comac_script_scaled_font_fini);

    font_private->parent = scaled_font;
    font_private->subset_glyph_index = 0;
    font_private->has_sfnt = TRUE;

    comac_list_add (&font_private->link, &ctx->fonts);

    status = _bitmap_next_id (&ctx->font_id, &font_private->id);
    if (unlikely (status)) {
	free (font_private);
	return status;
    }

    status = _emit_context (surface);
    if (unlikely (status)) {
	free (font_private);
	return status;
    }

    status = _emit_type42_font (surface, scaled_font);
    if (status != COMAC_INT_STATUS_UNSUPPORTED) {
	*font_out = font_private;
	return status;
    }

    font_private->has_sfnt = FALSE;
    _comac_output_stream_printf (ctx->stream,
				 "dict\n"
				 "  /type 3 set\n"
				 "  /metrics [%f %f %f %f %f] set\n"
				 "  /glyphs array set\n"
				 "  font dup /f%lu exch def set-font-face",
				 scaled_font->fs_extents.ascent,
				 scaled_font->fs_extents.descent,
				 scaled_font->fs_extents.height,
				 scaled_font->fs_extents.max_x_advance,
				 scaled_font->fs_extents.max_y_advance,
				 font_private->id);

    *font_out = font_private;
    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_scaled_font (comac_script_surface_t *surface,
		   comac_scaled_font_t *scaled_font)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_matrix_t matrix;
    comac_font_options_t options;
    comac_bool_t matrix_updated = FALSE;
    comac_status_t status;
    comac_script_font_t *font_private;

    comac_scaled_font_get_ctm (scaled_font, &matrix);
    status = _emit_scaling_matrix (surface, &matrix, &matrix_updated);
    if (unlikely (status))
	return status;

    if (! matrix_updated && surface->cr.current_scaled_font == scaled_font)
	return COMAC_STATUS_SUCCESS;

    surface->cr.current_scaled_font = scaled_font;

    font_private = _comac_script_font_get (ctx, scaled_font);
    if (font_private == NULL) {
	comac_scaled_font_get_font_matrix (scaled_font, &matrix);
	status = _emit_font_matrix (surface, &matrix);
	if (unlikely (status))
	    return status;

	comac_scaled_font_get_font_options (scaled_font, &options);
	status = _emit_font_options (surface, &options);
	if (unlikely (status))
	    return status;

	status = _emit_scaled_font_init (surface, scaled_font, &font_private);
	if (unlikely (status))
	    return status;

	assert (target_is_active (surface));
	_comac_output_stream_printf (ctx->stream,
				     " /scaled-font get /sf%lu exch def\n",
				     font_private->id);
    } else {
	_comac_output_stream_printf (ctx->stream,
				     "sf%lu set-scaled-font\n",
				     font_private->id);
    }

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_scaled_glyph_vector (comac_script_surface_t *surface,
			   comac_scaled_font_t *scaled_font,
			   comac_script_font_t *font_private,
			   comac_scaled_glyph_t *scaled_glyph)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_script_implicit_context_t old_cr;
    comac_status_t status;
    unsigned long index;

    index = ++font_private->subset_glyph_index;
    scaled_glyph->dev_private_key = ctx;
    scaled_glyph->dev_private = (void *) (uintptr_t) index;

    _comac_output_stream_printf (ctx->stream,
				 "%lu <<\n"
				 "  /metrics [%f %f %f %f %f %f]\n"
				 "  /render {\n",
				 index,
				 scaled_glyph->fs_metrics.x_bearing,
				 scaled_glyph->fs_metrics.y_bearing,
				 scaled_glyph->fs_metrics.width,
				 scaled_glyph->fs_metrics.height,
				 scaled_glyph->fs_metrics.x_advance,
				 scaled_glyph->fs_metrics.y_advance);

    if (! _comac_matrix_is_identity (&scaled_font->scale_inverse)) {
	_comac_output_stream_printf (ctx->stream,
				     "[%f %f %f %f %f %f] transform\n",
				     scaled_font->scale_inverse.xx,
				     scaled_font->scale_inverse.yx,
				     scaled_font->scale_inverse.xy,
				     scaled_font->scale_inverse.yy,
				     scaled_font->scale_inverse.x0,
				     scaled_font->scale_inverse.y0);
    }

    old_cr = surface->cr;
    _comac_script_implicit_context_init (&surface->cr);
    status = _comac_recording_surface_replay (scaled_glyph->recording_surface,
					      &surface->base);
    surface->cr = old_cr;

    _comac_output_stream_puts (ctx->stream, "} >> set\n");

    return status;
}

static comac_status_t
_emit_scaled_glyph_bitmap (comac_script_surface_t *surface,
			   comac_scaled_font_t *scaled_font,
			   comac_script_font_t *font_private,
			   comac_scaled_glyph_t *scaled_glyph)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_status_t status;
    unsigned long index;

    index = ++font_private->subset_glyph_index;
    scaled_glyph->dev_private_key = ctx;
    scaled_glyph->dev_private = (void *) (uintptr_t) index;

    _comac_output_stream_printf (ctx->stream,
				 "%lu <<\n"
				 "  /metrics [%f %f %f %f %f %f]\n"
				 "  /render {\n"
				 "%f %f translate\n",
				 index,
				 scaled_glyph->fs_metrics.x_bearing,
				 scaled_glyph->fs_metrics.y_bearing,
				 scaled_glyph->fs_metrics.width,
				 scaled_glyph->fs_metrics.height,
				 scaled_glyph->fs_metrics.x_advance,
				 scaled_glyph->fs_metrics.y_advance,
				 scaled_glyph->fs_metrics.x_bearing,
				 scaled_glyph->fs_metrics.y_bearing);

    status = _emit_image_surface (surface, scaled_glyph->surface);
    if (unlikely (status))
	return status;

    _comac_output_stream_puts (ctx->stream, "pattern ");

    if (! _comac_matrix_is_identity (&scaled_font->font_matrix)) {
	_comac_output_stream_printf (ctx->stream,
				     "\n  [%f %f %f %f %f %f] set-matrix\n",
				     scaled_font->font_matrix.xx,
				     scaled_font->font_matrix.yx,
				     scaled_font->font_matrix.xy,
				     scaled_font->font_matrix.yy,
				     scaled_font->font_matrix.x0,
				     scaled_font->font_matrix.y0);
    }
    _comac_output_stream_puts (ctx->stream, "mask\n} >> set\n");

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_scaled_glyph_prologue (comac_script_surface_t *surface,
			     comac_scaled_font_t *scaled_font)
{
    comac_script_context_t *ctx = to_context (surface);

    _comac_output_stream_printf (ctx->stream,
				 "f%lu /glyphs get\n",
				 _comac_script_font_id (ctx, scaled_font));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_emit_scaled_glyphs (comac_script_surface_t *surface,
		     comac_scaled_font_t *scaled_font,
		     comac_glyph_t *glyphs,
		     unsigned int num_glyphs)
{
    comac_script_context_t *ctx = to_context (surface);
    comac_script_font_t *font_private;
    comac_status_t status;
    unsigned int n;
    comac_bool_t have_glyph_prologue = FALSE;

    if (num_glyphs == 0)
	return COMAC_STATUS_SUCCESS;

    font_private = _comac_script_font_get (ctx, scaled_font);
    if (font_private->has_sfnt)
	return COMAC_STATUS_SUCCESS;

    _comac_scaled_font_freeze_cache (scaled_font);
    for (n = 0; n < num_glyphs; n++) {
	comac_scaled_glyph_t *scaled_glyph;

	status = _comac_scaled_glyph_lookup (scaled_font,
					     glyphs[n].index,
					     COMAC_SCALED_GLYPH_INFO_METRICS,
					     NULL, /* foreground color */
					     &scaled_glyph);
	if (unlikely (status))
	    break;

	if (scaled_glyph->dev_private_key == ctx)
	    continue;

	status = _comac_scaled_glyph_lookup (
	    scaled_font,
	    glyphs[n].index,
	    COMAC_SCALED_GLYPH_INFO_RECORDING_SURFACE,
	    NULL, /* foreground color */
	    &scaled_glyph);
	if (_comac_status_is_error (status))
	    break;

	if (status == COMAC_STATUS_SUCCESS) {
	    if (! have_glyph_prologue) {
		status = _emit_scaled_glyph_prologue (surface, scaled_font);
		if (unlikely (status))
		    break;

		have_glyph_prologue = TRUE;
	    }

	    status = _emit_scaled_glyph_vector (surface,
						scaled_font,
						font_private,
						scaled_glyph);
	    if (unlikely (status))
		break;

	    continue;
	}

	status = _comac_scaled_glyph_lookup (scaled_font,
					     glyphs[n].index,
					     COMAC_SCALED_GLYPH_INFO_SURFACE,
					     NULL, /* foreground color */
					     &scaled_glyph);
	if (_comac_status_is_error (status))
	    break;

	if (status == COMAC_STATUS_SUCCESS) {
	    if (! have_glyph_prologue) {
		status = _emit_scaled_glyph_prologue (surface, scaled_font);
		if (unlikely (status))
		    break;

		have_glyph_prologue = TRUE;
	    }

	    status = _emit_scaled_glyph_bitmap (surface,
						scaled_font,
						font_private,
						scaled_glyph);
	    if (unlikely (status))
		break;

	    continue;
	}
    }
    _comac_scaled_font_thaw_cache (scaled_font);

    if (have_glyph_prologue) {
	_comac_output_stream_puts (to_context (surface)->stream, "pop pop\n");
    }

    return status;
}

static void
to_octal (int value, char *buf, size_t size)
{
    do {
	buf[--size] = '0' + (value & 7);
	value >>= 3;
    } while (size);
}

static void
_emit_string_literal (comac_script_surface_t *surface,
		      const char *utf8,
		      int len)
{
    comac_script_context_t *ctx = to_context (surface);
    char c;
    const char *end;

    _comac_output_stream_puts (ctx->stream, "(");

    if (utf8 == NULL) {
	end = utf8;
    } else {
	if (len < 0)
	    len = strlen (utf8);
	end = utf8 + len;
    }

    while (utf8 < end) {
	switch ((c = *utf8++)) {
	case '\n':
	    c = 'n';
	    goto ESCAPED_CHAR;
	case '\r':
	    c = 'r';
	    goto ESCAPED_CHAR;
	case '\t':
	    c = 't';
	    goto ESCAPED_CHAR;
	case '\b':
	    c = 'b';
	    goto ESCAPED_CHAR;
	case '\f':
	    c = 'f';
	    goto ESCAPED_CHAR;
	case '\\':
	case '(':
	case ')':
	ESCAPED_CHAR:
	    _comac_output_stream_printf (ctx->stream, "\\%c", c);
	    break;
	default:
	    if (_comac_isprint (c)) {
		_comac_output_stream_printf (ctx->stream, "%c", c);
	    } else {
		char buf[4] = {'\\'};

		to_octal (c, buf + 1, 3);
		_comac_output_stream_write (ctx->stream, buf, 4);
	    }
	    break;
	}
    }
    _comac_output_stream_puts (ctx->stream, ")");
}

static comac_int_status_t
_comac_script_surface_show_text_glyphs (void *abstract_surface,
					comac_operator_t op,
					const comac_pattern_t *source,
					const char *utf8,
					int utf8_len,
					comac_glyph_t *glyphs,
					int num_glyphs,
					const comac_text_cluster_t *clusters,
					int num_clusters,
					comac_text_cluster_flags_t backward,
					comac_scaled_font_t *scaled_font,
					const comac_clip_t *clip)
{
    comac_script_surface_t *surface = abstract_surface;
    comac_script_context_t *ctx = to_context (surface);
    comac_script_font_t *font_private;
    comac_scaled_glyph_t *scaled_glyph;
    comac_matrix_t matrix;
    comac_status_t status;
    double x, y, ix, iy;
    int n;
    comac_output_stream_t *base85_stream = NULL;

    status = active (surface);
    if (unlikely (status))
	return status;

    status = _comac_surface_clipper_set_clip (&surface->clipper, clip);
    if (unlikely (status))
	goto BAIL;

    status = _emit_context (surface);
    if (unlikely (status))
	goto BAIL;

    status = _emit_source (surface, op, source);
    if (unlikely (status))
	goto BAIL;

    status = _emit_scaled_font (surface, scaled_font);
    if (unlikely (status))
	goto BAIL;

    status = _emit_operator (surface, op);
    if (unlikely (status))
	goto BAIL;

    status = _emit_scaled_glyphs (surface, scaled_font, glyphs, num_glyphs);
    if (unlikely (status))
	goto BAIL;

    /* (utf8) [cx cy [glyphs]] [clusters] backward show_text_glyphs */
    /* [cx cy [glyphs]] show_glyphs */

    if (utf8 != NULL && clusters != NULL) {
	_emit_string_literal (surface, utf8, utf8_len);
	_comac_output_stream_puts (ctx->stream, " ");
    }

    matrix = surface->cr.current_ctm;
    status = comac_matrix_invert (&matrix);
    assert (status == COMAC_STATUS_SUCCESS);

    ix = x = glyphs[0].x;
    iy = y = glyphs[0].y;
    comac_matrix_transform_point (&matrix, &ix, &iy);
    ix -= scaled_font->font_matrix.x0;
    iy -= scaled_font->font_matrix.y0;

    _comac_scaled_font_freeze_cache (scaled_font);
    font_private = _comac_script_font_get (ctx, scaled_font);

    _comac_output_stream_printf (ctx->stream, "[%f %f ", ix, iy);

    for (n = 0; n < num_glyphs; n++) {
	if (font_private->has_sfnt) {
	    if (glyphs[n].index > 256)
		break;
	} else {
	    status =
		_comac_scaled_glyph_lookup (scaled_font,
					    glyphs[n].index,
					    COMAC_SCALED_GLYPH_INFO_METRICS,
					    NULL, /* foreground color */
					    &scaled_glyph);
	    if (unlikely (status)) {
		_comac_scaled_font_thaw_cache (scaled_font);
		goto BAIL;
	    }

	    if ((uintptr_t) scaled_glyph->dev_private > 256)
		break;
	}
    }

    if (n == num_glyphs) {
	_comac_output_stream_puts (ctx->stream, "<~");
	base85_stream = _comac_base85_stream_create (ctx->stream);
    } else
	_comac_output_stream_puts (ctx->stream, "[");

    for (n = 0; n < num_glyphs; n++) {
	double dx, dy;

	status = _comac_scaled_glyph_lookup (scaled_font,
					     glyphs[n].index,
					     COMAC_SCALED_GLYPH_INFO_METRICS,
					     NULL, /* foreground color */
					     &scaled_glyph);
	if (unlikely (status)) {
	    _comac_scaled_font_thaw_cache (scaled_font);
	    goto BAIL;
	}

	if (fabs (glyphs[n].x - x) > 1e-5 || fabs (glyphs[n].y - y) > 1e-5) {
	    if (fabs (glyphs[n].y - y) < 1e-5) {
		if (base85_stream != NULL) {
		    status = _comac_output_stream_destroy (base85_stream);
		    if (unlikely (status)) {
			base85_stream = NULL;
			break;
		    }

		    _comac_output_stream_printf (ctx->stream,
						 "~> %f <~",
						 glyphs[n].x - x);
		    base85_stream = _comac_base85_stream_create (ctx->stream);
		} else {
		    _comac_output_stream_printf (ctx->stream,
						 " ] %f [ ",
						 glyphs[n].x - x);
		}

		x = glyphs[n].x;
	    } else {
		ix = x = glyphs[n].x;
		iy = y = glyphs[n].y;
		comac_matrix_transform_point (&matrix, &ix, &iy);
		ix -= scaled_font->font_matrix.x0;
		iy -= scaled_font->font_matrix.y0;
		if (base85_stream != NULL) {
		    status = _comac_output_stream_destroy (base85_stream);
		    if (unlikely (status)) {
			base85_stream = NULL;
			break;
		    }

		    _comac_output_stream_printf (ctx->stream,
						 "~> %f %f <~",
						 ix,
						 iy);
		    base85_stream = _comac_base85_stream_create (ctx->stream);
		} else {
		    _comac_output_stream_printf (ctx->stream,
						 " ] %f %f [ ",
						 ix,
						 iy);
		}
	    }
	}
	if (base85_stream != NULL) {
	    uint8_t c;

	    if (font_private->has_sfnt)
		c = glyphs[n].index;
	    else
		c = (uint8_t) (uintptr_t) scaled_glyph->dev_private;

	    _comac_output_stream_write (base85_stream, &c, 1);
	} else {
	    if (font_private->has_sfnt)
		_comac_output_stream_printf (ctx->stream,
					     " %lu",
					     glyphs[n].index);
	    else
		_comac_output_stream_printf (
		    ctx->stream,
		    " %lu",
		    (long unsigned) (uintptr_t) scaled_glyph->dev_private);
	}

	dx = scaled_glyph->metrics.x_advance;
	dy = scaled_glyph->metrics.y_advance;
	comac_matrix_transform_distance (&scaled_font->ctm, &dx, &dy);
	x += dx;
	y += dy;
    }
    _comac_scaled_font_thaw_cache (scaled_font);

    if (base85_stream != NULL) {
	comac_status_t status2;

	status2 = _comac_output_stream_destroy (base85_stream);
	if (status == COMAC_STATUS_SUCCESS)
	    status = status2;

	_comac_output_stream_printf (ctx->stream, "~>");
    } else {
	_comac_output_stream_puts (ctx->stream, " ]");
    }
    if (unlikely (status))
	return status;

    if (utf8 != NULL && clusters != NULL) {
	for (n = 0; n < num_clusters; n++) {
	    if (clusters[n].num_bytes > UCHAR_MAX ||
		clusters[n].num_glyphs > UCHAR_MAX) {
		break;
	    }
	}

	if (n < num_clusters) {
	    _comac_output_stream_puts (ctx->stream, "] [ ");
	    for (n = 0; n < num_clusters; n++) {
		_comac_output_stream_printf (ctx->stream,
					     "%d %d ",
					     clusters[n].num_bytes,
					     clusters[n].num_glyphs);
	    }
	    _comac_output_stream_puts (ctx->stream, "]");
	} else {
	    _comac_output_stream_puts (ctx->stream, "] <~");
	    base85_stream = _comac_base85_stream_create (ctx->stream);
	    for (n = 0; n < num_clusters; n++) {
		uint8_t c[2];
		c[0] = clusters[n].num_bytes;
		c[1] = clusters[n].num_glyphs;
		_comac_output_stream_write (base85_stream, c, 2);
	    }
	    status = _comac_output_stream_destroy (base85_stream);
	    if (unlikely (status))
		goto BAIL;

	    _comac_output_stream_puts (ctx->stream, "~>");
	}

	_comac_output_stream_printf (ctx->stream,
				     " //%s show-text-glyphs\n",
				     _direction_to_string (backward));
    } else {
	_comac_output_stream_puts (ctx->stream, "] show-glyphs\n");
    }

    inactive (surface);

    if (_comac_surface_wrapper_is_active (&surface->wrapper)) {
	return _comac_surface_wrapper_show_text_glyphs (&surface->wrapper,
							op,
							source,
							utf8,
							utf8_len,
							glyphs,
							num_glyphs,
							clusters,
							num_clusters,
							backward,
							scaled_font,
							clip);
    }

    return COMAC_STATUS_SUCCESS;

BAIL:
    inactive (surface);
    return status;
}

static comac_bool_t
_comac_script_surface_get_extents (void *abstract_surface,
				   comac_rectangle_int_t *rectangle)
{
    comac_script_surface_t *surface = abstract_surface;

    if (_comac_surface_wrapper_is_active (&surface->wrapper)) {
	return _comac_surface_wrapper_get_extents (&surface->wrapper,
						   rectangle);
    }

    if (surface->width < 0 || surface->height < 0)
	return FALSE;

    rectangle->x = 0;
    rectangle->y = 0;
    rectangle->width = surface->width;
    rectangle->height = surface->height;

    return TRUE;
}

static const comac_surface_backend_t _comac_script_surface_backend = {
    COMAC_SURFACE_TYPE_SCRIPT,
    _comac_script_surface_finish,

    _comac_default_context_create,

    _comac_script_surface_create_similar,
    NULL, /* create similar image */
    NULL, /* map to image */
    NULL, /* unmap image */

    _comac_script_surface_source,
    _comac_script_surface_acquire_source_image,
    _comac_script_surface_release_source_image,
    _comac_script_surface_snapshot,

    _comac_script_surface_copy_page,
    _comac_script_surface_show_page,

    _comac_script_surface_get_extents,
    NULL, /* get_font_options */

    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */

    _comac_script_surface_paint,
    _comac_script_surface_mask,
    _comac_script_surface_stroke,
    _comac_script_surface_fill,
    NULL, /* fill/stroke */
    NULL, /* glyphs */
    _comac_script_surface_has_show_text_glyphs,
    _comac_script_surface_show_text_glyphs};

static void
_comac_script_implicit_context_init (comac_script_implicit_context_t *cr)
{
    cr->current_operator = COMAC_GSTATE_OPERATOR_DEFAULT;
    cr->current_fill_rule = COMAC_GSTATE_FILL_RULE_DEFAULT;
    cr->current_tolerance = COMAC_GSTATE_TOLERANCE_DEFAULT;
    cr->current_antialias = COMAC_ANTIALIAS_DEFAULT;
    _comac_stroke_style_init (&cr->current_style);
    _comac_pattern_init_solid (&cr->current_source.solid, COMAC_COLOR_BLACK);
    _comac_path_fixed_init (&cr->current_path);
    comac_matrix_init_identity (&cr->current_ctm);
    comac_matrix_init_identity (&cr->current_stroke_matrix);
    comac_matrix_init_identity (&cr->current_font_matrix);
    _comac_font_options_init_default (&cr->current_font_options);
    cr->current_scaled_font = NULL;
    cr->has_clip = FALSE;
}

static void
_comac_script_implicit_context_reset (comac_script_implicit_context_t *cr)
{
    free (cr->current_style.dash);
    cr->current_style.dash = NULL;

    _comac_pattern_fini (&cr->current_source.base);
    _comac_path_fixed_fini (&cr->current_path);

    _comac_script_implicit_context_init (cr);
}

static comac_script_surface_t *
_comac_script_surface_create_internal (comac_script_context_t *ctx,
				       comac_content_t content,
				       comac_rectangle_t *extents,
				       comac_surface_t *passthrough)
{
    comac_script_surface_t *surface;

    if (unlikely (ctx == NULL))
	return (comac_script_surface_t *) _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NULL_POINTER));

    surface = _comac_malloc (sizeof (comac_script_surface_t));
    if (unlikely (surface == NULL))
	return (comac_script_surface_t *) _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_surface_init (&surface->base,
			 &_comac_script_surface_backend,
			 &ctx->base,
			 content,
			 TRUE, /* is_vector */
			 COMAC_COLORSPACE_RGB);

    _comac_surface_wrapper_init (&surface->wrapper, passthrough);

    _comac_surface_clipper_init (
	&surface->clipper,
	_comac_script_surface_clipper_intersect_clip_path);

    surface->width = surface->height = -1;
    if (extents) {
	surface->width = extents->width;
	surface->height = extents->height;
	comac_surface_set_device_offset (&surface->base,
					 -extents->x,
					 -extents->y);
    }

    surface->emitted = FALSE;
    surface->defined = FALSE;
    surface->active = FALSE;
    surface->operand.type = SURFACE;
    comac_list_init (&surface->operand.link);

    _comac_script_implicit_context_init (&surface->cr);

    return surface;
}

static const comac_device_backend_t _comac_script_device_backend = {
    COMAC_DEVICE_TYPE_SCRIPT,

    NULL,
    NULL, /* lock, unlock */

    _device_flush, /* flush */
    NULL,	   /* finish */
    _device_destroy};

comac_device_t *
_comac_script_context_create_internal (comac_output_stream_t *stream)
{
    comac_script_context_t *ctx;

    ctx = _comac_malloc (sizeof (comac_script_context_t));
    if (unlikely (ctx == NULL))
	return _comac_device_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    memset (ctx, 0, sizeof (comac_script_context_t));

    _comac_device_init (&ctx->base, &_comac_script_device_backend);

    comac_list_init (&ctx->operands);
    comac_list_init (&ctx->deferred);
    ctx->stream = stream;
    ctx->mode = COMAC_SCRIPT_MODE_ASCII;

    comac_list_init (&ctx->fonts);
    comac_list_init (&ctx->defines);

    ctx->attach_snapshots = TRUE;

    return &ctx->base;
}

void
_comac_script_context_attach_snapshots (comac_device_t *device,
					comac_bool_t enable)
{
    comac_script_context_t *ctx;

    ctx = (comac_script_context_t *) device;
    ctx->attach_snapshots = enable;
}

static comac_device_t *
_comac_script_context_create (comac_output_stream_t *stream)
{
    comac_script_context_t *ctx;

    ctx = (comac_script_context_t *) _comac_script_context_create_internal (
	stream);
    if (unlikely (ctx->base.status))
	return &ctx->base;

    ctx->owns_stream = TRUE;
    _comac_output_stream_puts (ctx->stream, "%!ComacScript\n");
    return &ctx->base;
}

/**
 * comac_script_create:
 * @filename: the name (path) of the file to write the script to
 *
 * Creates a output device for emitting the script, used when
 * creating the individual surfaces.
 *
 * Return value: a pointer to the newly created device. The caller
 * owns the surface and should call comac_device_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" device if an error such as out of memory
 * occurs. You can use comac_device_status() to check for this.
 *
 * Since: 1.12
 **/
comac_device_t *
comac_script_create (const char *filename)
{
    comac_output_stream_t *stream;
    comac_status_t status;

    stream = _comac_output_stream_create_for_filename (filename);
    if ((status = _comac_output_stream_get_status (stream)))
	return _comac_device_create_in_error (status);

    return _comac_script_context_create (stream);
}

/**
 * comac_script_create_for_stream:
 * @write_func: callback function passed the bytes written to the script
 * @closure: user data to be passed to the callback
 *
 * Creates a output device for emitting the script, used when
 * creating the individual surfaces.
 *
 * Return value: a pointer to the newly created device. The caller
 * owns the surface and should call comac_device_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" device if an error such as out of memory
 * occurs. You can use comac_device_status() to check for this.
 *
 * Since: 1.12
 **/
comac_device_t *
comac_script_create_for_stream (comac_write_func_t write_func, void *closure)
{
    comac_output_stream_t *stream;
    comac_status_t status;

    stream = _comac_output_stream_create (write_func, NULL, closure);
    if ((status = _comac_output_stream_get_status (stream)))
	return _comac_device_create_in_error (status);

    return _comac_script_context_create (stream);
}

/**
 * comac_script_write_comment:
 * @script: the script (output device)
 * @comment: the string to emit
 * @len:the length of the string to write, or -1 to use strlen()
 *
 * Emit a string verbatim into the script.
 *
 * Since: 1.12
 **/
void
comac_script_write_comment (comac_device_t *script,
			    const char *comment,
			    int len)
{
    comac_script_context_t *context = (comac_script_context_t *) script;

    if (len < 0)
	len = strlen (comment);

    _comac_output_stream_puts (context->stream, "% ");
    _comac_output_stream_write (context->stream, comment, len);
    _comac_output_stream_puts (context->stream, "\n");
}

/**
 * comac_script_set_mode:
 * @script: The script (output device)
 * @mode: the new mode
 *
 * Change the output mode of the script
 *
 * Since: 1.12
 **/
void
comac_script_set_mode (comac_device_t *script, comac_script_mode_t mode)
{
    comac_script_context_t *context = (comac_script_context_t *) script;

    context->mode = mode;
}

/**
 * comac_script_get_mode:
 * @script: The script (output device) to query
 *
 * Queries the script for its current output mode.
 *
 * Return value: the current output mode of the script
 *
 * Since: 1.12
 **/
comac_script_mode_t
comac_script_get_mode (comac_device_t *script)
{
    comac_script_context_t *context = (comac_script_context_t *) script;

    return context->mode;
}

/**
 * comac_script_surface_create:
 * @script: the script (output device)
 * @content: the content of the surface
 * @width: width in pixels
 * @height: height in pixels
 *
 * Create a new surface that will emit its rendering through @script
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if an error such as out of memory
 * occurs. You can use comac_surface_status() to check for this.
 *
 * Since: 1.12
 **/
comac_surface_t *
comac_script_surface_create (comac_device_t *script,
			     comac_content_t content,
			     double width,
			     double height)
{
    comac_rectangle_t *extents, r;

    if (unlikely (script->backend->type != COMAC_DEVICE_TYPE_SCRIPT))
	return _comac_surface_create_in_error (
	    COMAC_STATUS_DEVICE_TYPE_MISMATCH);

    if (unlikely (script->status))
	return _comac_surface_create_in_error (script->status);

    extents = NULL;
    if (width > 0 && height > 0) {
	r.x = r.y = 0;
	r.width = width;
	r.height = height;
	extents = &r;
    }
    return &_comac_script_surface_create_internal (
		(comac_script_context_t *) script,
		content,
		extents,
		NULL)
		->base;
}

/**
 * comac_script_surface_create_for_target:
 * @script: the script (output device)
 * @target: a target surface to wrap
 *
 * Create a pxoy surface that will render to @target and record
 * the operations to @device.
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if an error such as out of memory
 * occurs. You can use comac_surface_status() to check for this.
 *
 * Since: 1.12
 **/
comac_surface_t *
comac_script_surface_create_for_target (comac_device_t *script,
					comac_surface_t *target)
{
    comac_rectangle_int_t extents;
    comac_rectangle_t rect, *r;

    if (unlikely (script->backend->type != COMAC_DEVICE_TYPE_SCRIPT))
	return _comac_surface_create_in_error (
	    COMAC_STATUS_DEVICE_TYPE_MISMATCH);

    if (unlikely (script->status))
	return _comac_surface_create_in_error (script->status);

    if (unlikely (target->status))
	return _comac_surface_create_in_error (target->status);

    r = NULL;
    if (_comac_surface_get_extents (target, &extents)) {
	rect.x = rect.y = 0;
	rect.width = extents.width;
	rect.height = extents.height;
	r = &rect;
    }
    return &_comac_script_surface_create_internal (
		(comac_script_context_t *) script,
		target->content,
		r,
		target)
		->base;
}

/**
 * comac_script_from_recording_surface:
 * @script: the script (output device)
 * @recording_surface: the recording surface to replay
 *
 * Converts the record operations in @recording_surface into a script.
 *
 * Return value: #COMAC_STATUS_SUCCESS on successful completion or an error code.
 *
 * Since: 1.12
 **/
comac_status_t
comac_script_from_recording_surface (comac_device_t *script,
				     comac_surface_t *recording_surface)
{
    comac_rectangle_t r, *extents;
    comac_surface_t *surface;
    comac_status_t status;

    if (unlikely (script->backend->type != COMAC_DEVICE_TYPE_SCRIPT))
	return _comac_error (COMAC_STATUS_DEVICE_TYPE_MISMATCH);

    if (unlikely (script->status))
	return _comac_error (script->status);

    if (unlikely (recording_surface->status))
	return recording_surface->status;

    if (unlikely (! _comac_surface_is_recording (recording_surface)))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    extents = NULL;
    if (_comac_recording_surface_get_bounds (recording_surface, &r))
	extents = &r;

    surface = &_comac_script_surface_create_internal (
		   (comac_script_context_t *) script,
		   recording_surface->content,
		   extents,
		   NULL)
		   ->base;
    if (unlikely (surface->status))
	return surface->status;

    status = _comac_recording_surface_replay (recording_surface, surface);
    comac_surface_destroy (surface);

    return status;
}
