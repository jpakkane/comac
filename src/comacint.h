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

/*
 * These definitions are solely for use by the implementation of comac
 * and constitute no kind of standard.  If you need any of these
 * functions, please drop me a note.  Either the library needs new
 * functionality, or there's a way to do what you need using the
 * existing published interfaces. cworth@cworth.org
 */

#ifndef _COMACINT_H_
#define _COMACINT_H_

#include "config.h"

#ifdef _MSC_VER
#define comac_public __declspec(dllexport)
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <limits.h>
#include <stdio.h>

#include "comac.h"
#include <pixman.h>

#include "comac-compiler-private.h"
#include "comac-error-private.h"

#if COMAC_HAS_PDF_SURFACE    || \
    COMAC_HAS_PS_SURFACE     || \
    COMAC_HAS_SCRIPT_SURFACE || \
    COMAC_HAS_XML_SURFACE
#define COMAC_HAS_DEFLATE_STREAM 1
#endif

#if COMAC_HAS_PS_SURFACE  || \
    COMAC_HAS_PDF_SURFACE || \
    COMAC_HAS_SVG_SURFACE || \
    COMAC_HAS_WIN32_SURFACE
#define COMAC_HAS_FONT_SUBSET 1
#endif

#if COMAC_HAS_PS_SURFACE  || \
    COMAC_HAS_PDF_SURFACE || \
    COMAC_HAS_FONT_SUBSET
#define COMAC_HAS_PDF_OPERATORS 1
#endif

COMAC_BEGIN_DECLS

#if _WIN32 && !_WIN32_WCE /* Permissions on WinCE? No worries! */
comac_private FILE *
_comac_win32_tmpfile (void);
#define tmpfile() _comac_win32_tmpfile()
#endif

#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#if _XOPEN_SOURCE >= 600 || defined (_ISOC99_SOURCE)
#define ISFINITE(x) isfinite (x)
#else
#define ISFINITE(x) ((x) * (x) >= 0.) /* check for NaNs */
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524400844362104849039
#endif

#undef  ARRAY_LENGTH
#define ARRAY_LENGTH(__array) ((int) (sizeof (__array) / sizeof (__array[0])))

#undef STRINGIFY
#undef STRINGIFY_ARG
#define STRINGIFY(macro_or_string)    STRINGIFY_ARG (macro_or_string)
#define STRINGIFY_ARG(contents)       #contents

#if defined (__GNUC__)
#define comac_container_of(ptr, type, member) ({ \
    const __typeof__ (((type *) 0)->member) *mptr__ = (ptr); \
    (type *) ((char *) mptr__ - offsetof (type, member)); \
})
#else
#define comac_container_of(ptr, type, member) \
    ((type *)((char *) (ptr) - (char *) &((type *)0)->member))
#endif


#define ASSERT_NOT_REACHED		\
do {					\
    assert (!"reached");		\
} while (0)
#define COMPILE_TIME_ASSERT1(condition, line)		\
    typedef int compile_time_assertion_at_line_##line##_failed [(condition)?1:-1]
#define COMPILE_TIME_ASSERT0(condition, line)	COMPILE_TIME_ASSERT1(condition, line)
#define COMPILE_TIME_ASSERT(condition)		COMPILE_TIME_ASSERT0(condition, __LINE__)

#define COMAC_ALPHA_IS_CLEAR(alpha) ((alpha) <= ((double)0x00ff / (double)0xffff))
#define COMAC_ALPHA_SHORT_IS_CLEAR(alpha) ((alpha) <= 0x00ff)

#define COMAC_ALPHA_IS_OPAQUE(alpha) ((alpha) >= ((double)0xff00 / (double)0xffff))
#define COMAC_ALPHA_SHORT_IS_OPAQUE(alpha) ((alpha) >= 0xff00)
#define COMAC_ALPHA_IS_ZERO(alpha) ((alpha) <= 0.0)

#define COMAC_COLOR_IS_CLEAR(color) COMAC_ALPHA_SHORT_IS_CLEAR ((color)->alpha_short)
#define COMAC_COLOR_IS_OPAQUE(color) COMAC_ALPHA_SHORT_IS_OPAQUE ((color)->alpha_short)

/* Reverse the bits in a byte with 7 operations (no 64-bit):
 * Devised by Sean Anderson, July 13, 2001.
 * Source: http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
 */
#define COMAC_BITSWAP8(c) ((((c) * 0x0802LU & 0x22110LU) | ((c) * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16)

/* Return the number of 1 bits in mask.
 *
 * GCC 3.4 supports a "population count" builtin, which on many targets is
 * implemented with a single instruction. There is a fallback definition
 * in libgcc in case a target does not have one, which should be just as
 * good as the open-coded solution below, (which is "HACKMEM 169").
 */
static inline int comac_const
_comac_popcount (uint32_t mask)
{
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
    return __builtin_popcount (mask);
#else
    register int y;

    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >>1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
#endif
}

static comac_always_inline comac_bool_t
_comac_is_little_endian (void)
{
    static const int i = 1;
    return *((char *) &i) == 0x01;
}

#ifdef WORDS_BIGENDIAN
#define COMAC_BITSWAP8_IF_LITTLE_ENDIAN(c) (c)
#else
#define COMAC_BITSWAP8_IF_LITTLE_ENDIAN(c) COMAC_BITSWAP8(c)
#endif

#ifdef WORDS_BIGENDIAN

#define cpu_to_be16(v) (v)
#define be16_to_cpu(v) (v)
#define cpu_to_be32(v) (v)
#define be32_to_cpu(v) (v)

#else

static inline uint16_t comac_const
cpu_to_be16(uint16_t v)
{
    return (v << 8) | (v >> 8);
}

static inline uint16_t comac_const
be16_to_cpu(uint16_t v)
{
    return cpu_to_be16 (v);
}

static inline uint32_t comac_const
cpu_to_be32(uint32_t v)
{
    return (v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24);
}

static inline uint32_t comac_const
be32_to_cpu(uint32_t v)
{
    return cpu_to_be32 (v);
}

#endif

/* Unaligned big endian access
 */

static inline uint16_t get_unaligned_be16 (const unsigned char *p)
{
    return p[0] << 8 | p[1];
}

static inline uint32_t get_unaligned_be32 (const unsigned char *p)
{
    return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline void put_unaligned_be16 (uint16_t v, unsigned char *p)
{
    p[0] = (v >> 8) & 0xff;
    p[1] = v & 0xff;
}

static inline void put_unaligned_be32 (uint32_t v, unsigned char *p)
{
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8)  & 0xff;
    p[3] = v & 0xff;
}

#include "comac-ctype-inline.h"
#include "comac-types-private.h"
#include "comac-cache-private.h"
#include "comac-reference-count-private.h"
#include "comac-spans-private.h"
#include "comac-surface-private.h"

comac_private void
_comac_box_from_doubles (comac_box_t *box,
			 double *x1, double *y1,
			 double *x2, double *y2);

comac_private void
_comac_box_to_doubles (const comac_box_t *box,
		       double *x1, double *y1,
		       double *x2, double *y2);

comac_private void
_comac_box_from_rectangle (comac_box_t                 *box,
			   const comac_rectangle_int_t *rectangle);

comac_private void
_comac_box_round_to_rectangle (const comac_box_t     *box,
			       comac_rectangle_int_t *rectangle);

comac_private void
_comac_box_add_curve_to (comac_box_t         *extents,
			 const comac_point_t *a,
			 const comac_point_t *b,
			 const comac_point_t *c,
			 const comac_point_t *d);

comac_private void
_comac_boxes_get_extents (const comac_box_t *boxes,
			  int num_boxes,
			  comac_box_t *extents);

comac_private extern const comac_rectangle_int_t _comac_empty_rectangle;
comac_private extern const comac_rectangle_int_t _comac_unbounded_rectangle;

static inline void
_comac_unbounded_rectangle_init (comac_rectangle_int_t *rect)
{
    *rect = _comac_unbounded_rectangle;
}

comac_private_no_warn comac_bool_t
_comac_rectangle_intersect (comac_rectangle_int_t *dst,
			    const comac_rectangle_int_t *src);

static inline comac_bool_t
_comac_rectangle_intersects (const comac_rectangle_int_t *dst,
			     const comac_rectangle_int_t *src)
{
    return !(src->x >= dst->x + dst->width  ||
	     src->x + src->width <= dst->x  ||
	     src->y >= dst->y + dst->height ||
	     src->y + src->height <= dst->y);
}

static inline comac_bool_t
_comac_rectangle_contains_rectangle (const comac_rectangle_int_t *a,
				     const comac_rectangle_int_t *b)
{
    return (a->x <= b->x &&
	    a->x + a->width >= b->x + b->width &&
	    a->y <= b->y &&
	    a->y + a->height >= b->y + b->height);
}

comac_private void
_comac_rectangle_int_from_double (comac_rectangle_int_t *recti,
				  const comac_rectangle_t *rectf);

/* Extends the dst rectangle to also contain src.
 * If one of the rectangles is empty, the result is undefined
 */
comac_private void
_comac_rectangle_union (comac_rectangle_int_t *dst,
			const comac_rectangle_int_t *src);

comac_private comac_bool_t
_comac_box_intersects_line_segment (const comac_box_t *box,
	                            comac_line_t *line) comac_pure;

comac_private comac_bool_t
_comac_spline_intersects (const comac_point_t *a,
			  const comac_point_t *b,
			  const comac_point_t *c,
			  const comac_point_t *d,
			  const comac_box_t *box) comac_pure;

typedef struct {
    const comac_user_data_key_t *key;
    void *user_data;
    comac_destroy_func_t destroy;
} comac_user_data_slot_t;

comac_private void
_comac_user_data_array_init (comac_user_data_array_t *array);

comac_private void
_comac_user_data_array_fini (comac_user_data_array_t *array);

comac_private void *
_comac_user_data_array_get_data (comac_user_data_array_t     *array,
				 const comac_user_data_key_t *key);

comac_private comac_status_t
_comac_user_data_array_set_data (comac_user_data_array_t     *array,
				 const comac_user_data_key_t *key,
				 void			     *user_data,
				 comac_destroy_func_t	      destroy);

comac_private comac_status_t
_comac_user_data_array_copy (comac_user_data_array_t		*dst,
			     const comac_user_data_array_t	*src);

comac_private void
_comac_user_data_array_foreach (comac_user_data_array_t     *array,
				void (*func) (const void *key,
					      void *elt,
					      void *closure),
				void *closure);

#define _COMAC_HASH_INIT_VALUE 5381

comac_private uintptr_t
_comac_hash_string (const char *c);

comac_private uintptr_t
_comac_hash_bytes (uintptr_t hash,
		   const void *bytes,
		   unsigned int length);

/* We use bits 24-27 to store phases for subpixel positions */
#define _comac_scaled_glyph_index(g) ((unsigned long)((g)->hash_entry.hash & 0xffffff))
#define _comac_scaled_glyph_xphase(g) (int)(((g)->hash_entry.hash >> 24) & 3)
#define _comac_scaled_glyph_yphase(g) (int)(((g)->hash_entry.hash >> 26) & 3)
#define _comac_scaled_glyph_set_index(g, i)  ((g)->hash_entry.hash = (i))

#include "comac-scaled-font-private.h"

struct _comac_font_face {
    /* hash_entry must be first */
    comac_hash_entry_t hash_entry;
    comac_status_t status;
    comac_reference_count_t ref_count;
    comac_user_data_array_t user_data;
    const comac_font_face_backend_t *backend;
};

comac_private void
_comac_default_context_reset_static_data (void);

comac_private void
_comac_toy_font_face_reset_static_data (void);

comac_private void
_comac_ft_font_reset_static_data (void);

comac_private void
_comac_win32_font_reset_static_data (void);

/* the font backend interface */

struct _comac_unscaled_font_backend {
    comac_bool_t (*destroy) (void	*unscaled_font);
};

/* #comac_toy_font_face_t - simple family/slant/weight font faces used for
 * the built-in font API
 */

typedef struct _comac_toy_font_face {
    comac_font_face_t base;
    const char *family;
    comac_bool_t owns_family;
    comac_font_slant_t slant;
    comac_font_weight_t weight;

    comac_font_face_t *impl_face; /* The non-toy font face this actually uses */
} comac_toy_font_face_t;

typedef enum _comac_scaled_glyph_info {
    COMAC_SCALED_GLYPH_INFO_METRICS	 = (1 << 0),
    COMAC_SCALED_GLYPH_INFO_SURFACE	 = (1 << 1),
    COMAC_SCALED_GLYPH_INFO_PATH	 = (1 << 2),
    COMAC_SCALED_GLYPH_INFO_RECORDING_SURFACE = (1 << 3),
    COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE = (1 << 4)
} comac_scaled_glyph_info_t;

typedef struct _comac_scaled_font_subset {
    comac_scaled_font_t *scaled_font;
    unsigned int font_id;
    unsigned int subset_id;

    /* Index of glyphs array is subset_glyph_index.
     * Value of glyphs array is scaled_font_glyph_index.
     */
    unsigned long *glyphs;
    char          **utf8;
    char          **glyph_names;
    int           *to_latin_char;
    unsigned long *latin_to_subset_glyph_index;
    unsigned int num_glyphs;
    comac_bool_t is_composite;
    comac_bool_t is_scaled;
    comac_bool_t is_latin;
} comac_scaled_font_subset_t;

struct _comac_scaled_font_backend {
    comac_font_type_t type;

    void
    (*fini)		(void			*scaled_font);

    /*
     * Get the requested glyph info.
     * @scaled_font: a #comac_scaled_font_t
     * @scaled_glyph: a #comac_scaled_glyph_t the glyph
     * @info: a #comac_scaled_glyph_info_t which information to retrieve
     *  %COMAC_SCALED_GLYPH_INFO_METRICS - glyph metrics and bounding box
     *  %COMAC_SCALED_GLYPH_INFO_SURFACE - surface holding glyph image
     *  %COMAC_SCALED_GLYPH_INFO_PATH - path holding glyph outline in device space
     *  %COMAC_SCALED_GLYPH_INFO_RECORDING_SURFACE - surface holding recording of glyph
     *  %COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE - surface holding color glyph image
     * @foreground_color - foreground color to use when rendering color fonts. Use NULL
     * if not requesting COMAC_SCALED_GLYPH_INFO_COLOR_SURFACE or foreground color is unknown.
     */
    comac_warn comac_int_status_t
    (*scaled_glyph_init)	(void			     *scaled_font,
				 comac_scaled_glyph_t	     *scaled_glyph,
				 comac_scaled_glyph_info_t    info,
                                 const comac_color_t         *foreground_color);

    /* A backend only needs to implement this or ucs4_to_index(), not
     * both. This allows the backend to do something more sophisticated
     * then just converting characters one by one.
     */
    comac_warn comac_int_status_t
    (*text_to_glyphs) (void                       *scaled_font,
		       double		           x,
		       double		           y,
		       const char	          *utf8,
		       int		           utf8_len,
		       comac_glyph_t	         **glyphs,
		       int		          *num_glyphs,
		       comac_text_cluster_t      **clusters,
		       int		          *num_clusters,
		       comac_text_cluster_flags_t *cluster_flags);

    /* Get the glyph index for the given unicode code point.
     * @scaled_font: a #comac_scaled_font_t
     * @ucs4: unicode code point
     * Returns glyph index or 0 if not found.
     */
    unsigned long
    (*ucs4_to_index)		(void			     *scaled_font,
				 uint32_t		      ucs4);

    /* Read data from a sfnt font table.
     * @scaled_font: font
     * @tag: 4 byte table name specifying the table to read.
     * @offset: offset into the table
     * @buffer: buffer to write data into. Caller must ensure there is sufficient space.
     *          If NULL, return the size of the table in @length.
     * @length: If @buffer is NULL, the size of the table will be returned in @length.
     *          If @buffer is not null, @length specifies the number of bytes to read.
     *
     * If less than @length bytes are available to read this function
     * returns COMAC_INT_STATUS_UNSUPPORTED. Note that requesting more
     * bytes than are available in the table may continue reading data
     * from the following table and return success. If this is
     * undesirable the caller should first query the table size. If an
     * error occurs the output value of @length is undefined.
     *
     * Returns COMAC_INT_STATUS_UNSUPPORTED if not a sfnt style font or table not found.
     */
    comac_warn comac_int_status_t
    (*load_truetype_table)(void		        *scaled_font,
                           unsigned long         tag,
                           long                  offset,
                           unsigned char        *buffer,
                           unsigned long        *length);

    /* ucs4 is set to -1 if the unicode character could not be found
     * for the glyph */
    comac_warn comac_int_status_t
    (*index_to_ucs4)(void                       *scaled_font,
		     unsigned long               index,
                     uint32_t                   *ucs4);

    /* Determine if this scaled font differs from the outlines in the font tables.
     * eg synthesized bold/italic or a non default variant of a variable font.
     * @scaled_font: font
     * @is_sythetic: returns TRUE if scaled font is synthetic
     * Returns comac status
     */
    comac_warn comac_int_status_t
    (*is_synthetic)(void                       *scaled_font,
		    comac_bool_t               *is_synthetic);

    /* For type 1 fonts, return the glyph name for a given glyph index.
     * A glyph index and list of glyph names in the Type 1 fonts is provided.
     * The function returns the index of the glyph in the list of glyph names.
     * @scaled_font: font
     * @glyph_names: the names of each glyph in the Type 1 font in the
     *   order they appear in the CharStrings array
     * @num_glyph_names: the number of names in the glyph_names array
     * @glyph_index: the given glyph index
     * @glyph_array_index: (index into glyph_names) the glyph name corresponding
     *  to the glyph_index
     */
    comac_warn comac_int_status_t
    (*index_to_glyph_name)(void                 *scaled_font,
			   char                **glyph_names,
			   int                   num_glyph_names,
			   unsigned long         glyph_index,
			   unsigned long        *glyph_array_index);

    /* Read data from a PostScript font.
     * @scaled_font: font
     * @offset: offset into the table
     * @buffer: buffer to write data into. Caller must ensure there is sufficient space.
     *          If NULL, return the size of the table in @length.
     * @length: If @buffer is NULL, the size of the table will be returned in @length.
     *          If @buffer is not null, @length specifies the number of bytes to read.
     *
     * If less than @length bytes are available to read this function
     * returns COMAC_INT_STATUS_UNSUPPORTED. If an error occurs the
     * output value of @length is undefined.
     *
     * Returns COMAC_INT_STATUS_UNSUPPORTED if not a Type 1 font.
     */
    comac_warn comac_int_status_t
    (*load_type1_data)    (void		        *scaled_font,
                           long                  offset,
                           unsigned char        *buffer,
                           unsigned long        *length);

    /* Check if font has any color glyphs.
     * @scaled_font: font
     * Returns TRUE if font contains any color glyphs
     */
    comac_bool_t
    (*has_color_glyphs)   (void                 *scaled_font);
};

struct _comac_font_face_backend {
    comac_font_type_t	type;

    comac_warn comac_status_t
    (*create_for_toy)  (comac_toy_font_face_t	*toy_face,
			comac_font_face_t      **font_face);

    /* The destroy() function is allowed to resurrect the font face
     * by re-referencing. This is needed for the FreeType backend.
     */
    comac_bool_t
    (*destroy)     (void			*font_face);

    comac_warn comac_status_t
    (*scaled_font_create) (void				*font_face,
			   const comac_matrix_t		*font_matrix,
			   const comac_matrix_t		*ctm,
			   const comac_font_options_t	*options,
			   comac_scaled_font_t	       **scaled_font);

    comac_font_face_t *
    (*get_implementation) (void				*font_face,
			   const comac_matrix_t		*font_matrix,
			   const comac_matrix_t		*ctm,
			   const comac_font_options_t	*options);
};

extern const comac_private struct _comac_font_face_backend _comac_user_font_face_backend;

/* concrete font backends */
#if COMAC_HAS_FT_FONT

extern const comac_private struct _comac_font_face_backend _comac_ft_font_face_backend;

#endif

#if COMAC_HAS_WIN32_FONT

extern const comac_private struct _comac_font_face_backend _comac_win32_font_face_backend;

#endif

#if COMAC_HAS_DWRITE_FONT

extern const comac_private struct _comac_font_face_backend _comac_dwrite_font_face_backend;

#endif

#if COMAC_HAS_QUARTZ_FONT

extern const comac_private struct _comac_font_face_backend _comac_quartz_font_face_backend;

#endif

#define COMAC_EXTEND_SURFACE_DEFAULT COMAC_EXTEND_NONE
#define COMAC_EXTEND_GRADIENT_DEFAULT COMAC_EXTEND_PAD
#define COMAC_FILTER_DEFAULT COMAC_FILTER_GOOD

extern const comac_private comac_solid_pattern_t _comac_pattern_clear;
extern const comac_private comac_solid_pattern_t _comac_pattern_black;
extern const comac_private comac_solid_pattern_t _comac_pattern_white;

struct _comac_surface_attributes {
    comac_matrix_t matrix;
    comac_extend_t extend;
    comac_filter_t filter;
    comac_bool_t has_component_alpha;
    int		   x_offset;
    int		   y_offset;
    void	   *extra;
};

#define COMAC_FONT_SLANT_DEFAULT   COMAC_FONT_SLANT_NORMAL
#define COMAC_FONT_WEIGHT_DEFAULT  COMAC_FONT_WEIGHT_NORMAL

#define COMAC_WIN32_FONT_FAMILY_DEFAULT "Arial"
#define COMAC_DWRITE_FONT_FAMILY_DEFAULT "Arial"
#define COMAC_QUARTZ_FONT_FAMILY_DEFAULT  "Helvetica"
#define COMAC_FT_FONT_FAMILY_DEFAULT     ""
#define COMAC_USER_FONT_FAMILY_DEFAULT     "@comac:"

#if   COMAC_HAS_DWRITE_FONT

#define COMAC_FONT_FAMILY_DEFAULT COMAC_DWRITE_FONT_FAMILY_DEFAULT
#define COMAC_FONT_FACE_BACKEND_DEFAULT &_comac_dwrite_font_face_backend

#elif COMAC_HAS_WIN32_FONT

#define COMAC_FONT_FAMILY_DEFAULT COMAC_WIN32_FONT_FAMILY_DEFAULT
#define COMAC_FONT_FACE_BACKEND_DEFAULT &_comac_win32_font_face_backend

#elif COMAC_HAS_QUARTZ_FONT

#define COMAC_FONT_FAMILY_DEFAULT COMAC_QUARTZ_FONT_FAMILY_DEFAULT
#define COMAC_FONT_FACE_BACKEND_DEFAULT &_comac_quartz_font_face_backend

#elif COMAC_HAS_FT_FONT

#define COMAC_FONT_FAMILY_DEFAULT COMAC_FT_FONT_FAMILY_DEFAULT
#define COMAC_FONT_FACE_BACKEND_DEFAULT &_comac_ft_font_face_backend

#else

#define COMAC_FONT_FAMILY_DEFAULT COMAC_FT_FONT_FAMILY_DEFAULT
#define COMAC_FONT_FACE_BACKEND_DEFAULT &_comac_user_font_face_backend

#endif

#define COMAC_GSTATE_OPERATOR_DEFAULT	COMAC_OPERATOR_OVER
#define COMAC_GSTATE_TOLERANCE_DEFAULT	0.1
#define COMAC_GSTATE_FILL_RULE_DEFAULT	COMAC_FILL_RULE_WINDING
#define COMAC_GSTATE_LINE_WIDTH_DEFAULT	2.0
#define COMAC_GSTATE_LINE_CAP_DEFAULT	COMAC_LINE_CAP_BUTT
#define COMAC_GSTATE_LINE_JOIN_DEFAULT	COMAC_LINE_JOIN_MITER
#define COMAC_GSTATE_MITER_LIMIT_DEFAULT	10.0
#define COMAC_GSTATE_DEFAULT_FONT_SIZE  10.0

#define COMAC_SURFACE_RESOLUTION_DEFAULT 72.0
#define COMAC_SURFACE_FALLBACK_RESOLUTION_DEFAULT 300.0

typedef struct _comac_stroke_face {
    comac_point_t ccw;
    comac_point_t point;
    comac_point_t cw;
    comac_slope_t dev_vector;
    comac_point_double_t dev_slope;
    comac_point_double_t usr_vector;
    double length;
} comac_stroke_face_t;

/* comac.c */

static inline double comac_const
_comac_restrict_value (double value, double min, double max)
{
    if (value < min)
	return min;
    else if (value > max)
	return max;
    else
	return value;
}

/* C99 round() rounds to the nearest integral value with halfway cases rounded
 * away from 0. _comac_round rounds halfway cases toward positive infinity.
 * This matches the rounding behaviour of _comac_lround. */
static inline double comac_const
_comac_round (double r)
{
    return floor (r + .5);
}

#if DISABLE_SOME_FLOATING_POINT
comac_private int
_comac_lround (double d) comac_const;
#else
static inline int comac_const
_comac_lround (double r)
{
    return _comac_round (r);
}
#endif

comac_private uint16_t
_comac_half_from_float (float f) comac_const;

comac_private comac_bool_t
_comac_operator_bounded_by_mask (comac_operator_t op) comac_const;

comac_private comac_bool_t
_comac_operator_bounded_by_source (comac_operator_t op) comac_const;

enum {
    COMAC_OPERATOR_BOUND_BY_MASK = 1 << 1,
    COMAC_OPERATOR_BOUND_BY_SOURCE = 1 << 2,
};

comac_private uint32_t
_comac_operator_bounded_by_either (comac_operator_t op) comac_const;
/* comac-color.c */
comac_private const comac_color_t *
_comac_stock_color (comac_stock_t stock) comac_pure;

#define COMAC_COLOR_WHITE       _comac_stock_color (COMAC_STOCK_WHITE)
#define COMAC_COLOR_BLACK       _comac_stock_color (COMAC_STOCK_BLACK)
#define COMAC_COLOR_TRANSPARENT _comac_stock_color (COMAC_STOCK_TRANSPARENT)

comac_private uint16_t
_comac_color_double_to_short (double d) comac_const;

comac_private void
_comac_color_init_rgba (comac_color_t *color,
			double red, double green, double blue,
			double alpha);

comac_private void
_comac_color_multiply_alpha (comac_color_t *color,
			     double	    alpha);

comac_private void
_comac_color_get_rgba (comac_color_t *color,
		       double	     *red,
		       double	     *green,
		       double	     *blue,
		       double	     *alpha);

comac_private void
_comac_color_get_rgba_premultiplied (comac_color_t *color,
				     double	   *red,
				     double	   *green,
				     double	   *blue,
				     double	   *alpha);

comac_private comac_bool_t
_comac_color_equal (const comac_color_t *color_a,
                    const comac_color_t *color_b) comac_pure;

comac_private comac_bool_t
_comac_color_stop_equal (const comac_color_stop_t *color_a,
			 const comac_color_stop_t *color_b) comac_pure;

comac_private comac_content_t
_comac_color_get_content (const comac_color_t *color) comac_pure;

/* comac-font-face.c */

extern const comac_private comac_font_face_t _comac_font_face_nil;
extern const comac_private comac_font_face_t _comac_font_face_nil_file_not_found;

comac_private void
_comac_font_face_init (comac_font_face_t               *font_face,
		       const comac_font_face_backend_t *backend);

comac_private comac_bool_t
_comac_font_face_destroy (void *abstract_face);

comac_private comac_status_t
_comac_font_face_set_error (comac_font_face_t *font_face,
	                    comac_status_t     status);

comac_private void
_comac_unscaled_font_init (comac_unscaled_font_t               *font,
			   const comac_unscaled_font_backend_t *backend);

comac_private_no_warn comac_unscaled_font_t *
_comac_unscaled_font_reference (comac_unscaled_font_t *font);

comac_private void
_comac_unscaled_font_destroy (comac_unscaled_font_t *font);

/* comac-font-face-twin.c */

comac_private comac_font_face_t *
_comac_font_face_twin_create_fallback (void);

comac_private comac_status_t
_comac_font_face_twin_create_for_toy (comac_toy_font_face_t   *toy_face,
				      comac_font_face_t      **font_face);

/* comac-font-face-twin-data.c */

extern const comac_private int8_t _comac_twin_outlines[];
extern const comac_private uint16_t _comac_twin_charmap[128];

/* comac-font-options.c */

comac_private void
_comac_font_options_init_default (comac_font_options_t *options);

comac_private void
_comac_font_options_init_copy (comac_font_options_t		*options,
			       const comac_font_options_t	*other);

comac_private void
_comac_font_options_fini (comac_font_options_t *options);

comac_private void
_comac_font_options_set_lcd_filter (comac_font_options_t   *options,
				   comac_lcd_filter_t  lcd_filter);

comac_private comac_lcd_filter_t
_comac_font_options_get_lcd_filter (const comac_font_options_t *options);

comac_private void
_comac_font_options_set_round_glyph_positions (comac_font_options_t   *options,
					       comac_round_glyph_positions_t  round);

comac_private comac_round_glyph_positions_t
_comac_font_options_get_round_glyph_positions (const comac_font_options_t *options);

/* comac-hull.c */
comac_private comac_status_t
_comac_hull_compute (comac_pen_vertex_t *vertices, int *num_vertices);

/* comac-lzw.c */
comac_private unsigned char *
_comac_lzw_compress (unsigned char *data, unsigned long *size_in_out);

/* comac-misc.c */
comac_private comac_status_t
_comac_validate_text_clusters (const char		   *utf8,
			       int			    utf8_len,
			       const comac_glyph_t	   *glyphs,
			       int			    num_glyphs,
			       const comac_text_cluster_t  *clusters,
			       int			    num_clusters,
			       comac_text_cluster_flags_t   cluster_flags);

comac_private unsigned long
_comac_string_hash (const char *str, int len);

comac_private comac_status_t
_comac_intern_string (const char **str_inout, int len);

comac_private void
_comac_intern_string_reset_static_data (void);

comac_private const char *
_comac_get_locale_decimal_point (void);

comac_private double
_comac_strtod (const char *nptr, char **endptr);

/* comac-path-fixed.c */
comac_private comac_path_fixed_t *
_comac_path_fixed_create (void);

comac_private void
_comac_path_fixed_init (comac_path_fixed_t *path);

comac_private comac_status_t
_comac_path_fixed_init_copy (comac_path_fixed_t *path,
			     const comac_path_fixed_t *other);

comac_private void
_comac_path_fixed_fini (comac_path_fixed_t *path);

comac_private void
_comac_path_fixed_destroy (comac_path_fixed_t *path);

comac_private comac_status_t
_comac_path_fixed_move_to (comac_path_fixed_t  *path,
			   comac_fixed_t	x,
			   comac_fixed_t	y);

comac_private void
_comac_path_fixed_new_sub_path (comac_path_fixed_t *path);

comac_private comac_status_t
_comac_path_fixed_rel_move_to (comac_path_fixed_t *path,
			       comac_fixed_t	   dx,
			       comac_fixed_t	   dy);

comac_private comac_status_t
_comac_path_fixed_line_to (comac_path_fixed_t *path,
			   comac_fixed_t	x,
			   comac_fixed_t	y);

comac_private comac_status_t
_comac_path_fixed_rel_line_to (comac_path_fixed_t *path,
			       comac_fixed_t	   dx,
			       comac_fixed_t	   dy);

comac_private comac_status_t
_comac_path_fixed_curve_to (comac_path_fixed_t	*path,
			    comac_fixed_t x0, comac_fixed_t y0,
			    comac_fixed_t x1, comac_fixed_t y1,
			    comac_fixed_t x2, comac_fixed_t y2);

comac_private comac_status_t
_comac_path_fixed_rel_curve_to (comac_path_fixed_t *path,
				comac_fixed_t dx0, comac_fixed_t dy0,
				comac_fixed_t dx1, comac_fixed_t dy1,
				comac_fixed_t dx2, comac_fixed_t dy2);

comac_private comac_status_t
_comac_path_fixed_close_path (comac_path_fixed_t *path);

comac_private comac_bool_t
_comac_path_fixed_get_current_point (comac_path_fixed_t *path,
				     comac_fixed_t	*x,
				     comac_fixed_t	*y);

typedef comac_status_t
(comac_path_fixed_move_to_func_t) (void		 *closure,
				   const comac_point_t *point);

typedef comac_status_t
(comac_path_fixed_line_to_func_t) (void		 *closure,
				   const comac_point_t *point);

typedef comac_status_t
(comac_path_fixed_curve_to_func_t) (void	  *closure,
				    const comac_point_t *p0,
				    const comac_point_t *p1,
				    const comac_point_t *p2);

typedef comac_status_t
(comac_path_fixed_close_path_func_t) (void *closure);

comac_private comac_status_t
_comac_path_fixed_interpret (const comac_path_fixed_t	  *path,
		       comac_path_fixed_move_to_func_t	  *move_to,
		       comac_path_fixed_line_to_func_t	  *line_to,
		       comac_path_fixed_curve_to_func_t	  *curve_to,
		       comac_path_fixed_close_path_func_t *close_path,
		       void				  *closure);

comac_private comac_status_t
_comac_path_fixed_interpret_flat (const comac_path_fixed_t *path,
		       comac_path_fixed_move_to_func_t	  *move_to,
		       comac_path_fixed_line_to_func_t	  *line_to,
		       comac_path_fixed_close_path_func_t *close_path,
		       void				  *closure,
		       double				  tolerance);


comac_private comac_bool_t
_comac_path_bounder_extents (const comac_path_fixed_t *path,
			     comac_box_t *box);

comac_private comac_bool_t
_comac_path_fixed_extents (const comac_path_fixed_t *path,
			   comac_box_t *box);

comac_private void
_comac_path_fixed_approximate_clip_extents (const comac_path_fixed_t	*path,
					    comac_rectangle_int_t *extents);

comac_private void
_comac_path_fixed_approximate_fill_extents (const comac_path_fixed_t *path,
					    comac_rectangle_int_t *extents);

comac_private void
_comac_path_fixed_fill_extents (const comac_path_fixed_t	*path,
				comac_fill_rule_t	 fill_rule,
				double			 tolerance,
				comac_rectangle_int_t	*extents);

comac_private void
_comac_path_fixed_approximate_stroke_extents (const comac_path_fixed_t *path,
					      const comac_stroke_style_t *style,
					      const comac_matrix_t *ctm,
					      comac_bool_t vector,
					      comac_rectangle_int_t *extents);

comac_private comac_status_t
_comac_path_fixed_stroke_extents (const comac_path_fixed_t *path,
				  const comac_stroke_style_t *style,
				  const comac_matrix_t *ctm,
				  const comac_matrix_t *ctm_inverse,
				  double tolerance,
				  comac_rectangle_int_t *extents);

comac_private void
_comac_path_fixed_transform (comac_path_fixed_t	*path,
			     const comac_matrix_t	*matrix);

comac_private comac_bool_t
_comac_path_fixed_is_box (const comac_path_fixed_t *path,
                          comac_box_t *box);

comac_private comac_bool_t
_comac_path_fixed_is_rectangle (const comac_path_fixed_t *path,
				comac_box_t        *box);

/* comac-path-in-fill.c */
comac_private comac_bool_t
_comac_path_fixed_in_fill (const comac_path_fixed_t	*path,
			   comac_fill_rule_t	 fill_rule,
			   double		 tolerance,
			   double		 x,
			   double		 y);

/* comac-path-fill.c */
comac_private comac_status_t
_comac_path_fixed_fill_to_polygon (const comac_path_fixed_t *path,
				   double              tolerance,
				   comac_polygon_t      *polygon);

comac_private comac_status_t
_comac_path_fixed_fill_rectilinear_to_polygon (const comac_path_fixed_t *path,
					       comac_antialias_t antialias,
					       comac_polygon_t *polygon);

comac_private comac_status_t
_comac_path_fixed_fill_rectilinear_to_boxes (const comac_path_fixed_t *path,
					     comac_fill_rule_t fill_rule,
					     comac_antialias_t antialias,
					     comac_boxes_t *boxes);

comac_private comac_region_t *
_comac_path_fixed_fill_rectilinear_to_region (const comac_path_fixed_t	*path,
					      comac_fill_rule_t	 fill_rule,
					      const comac_rectangle_int_t *extents);

comac_private comac_status_t
_comac_path_fixed_fill_to_traps (const comac_path_fixed_t   *path,
				 comac_fill_rule_t	     fill_rule,
				 double			     tolerance,
				 comac_traps_t		    *traps);

/* comac-path-stroke.c */
comac_private comac_status_t
_comac_path_fixed_stroke_to_polygon (const comac_path_fixed_t	*path,
				     const comac_stroke_style_t	*stroke_style,
				     const comac_matrix_t	*ctm,
				     const comac_matrix_t	*ctm_inverse,
				     double		 tolerance,
				     comac_polygon_t	*polygon);

comac_private comac_int_status_t
_comac_path_fixed_stroke_to_tristrip (const comac_path_fixed_t	*path,
				      const comac_stroke_style_t*style,
				      const comac_matrix_t	*ctm,
				      const comac_matrix_t	*ctm_inverse,
				      double			 tolerance,
				      comac_tristrip_t		 *strip);

comac_private comac_status_t
_comac_path_fixed_stroke_dashed_to_polygon (const comac_path_fixed_t	*path,
					    const comac_stroke_style_t	*stroke_style,
					    const comac_matrix_t	*ctm,
					    const comac_matrix_t	*ctm_inverse,
					    double		 tolerance,
					    comac_polygon_t	*polygon);

comac_private comac_int_status_t
_comac_path_fixed_stroke_rectilinear_to_boxes (const comac_path_fixed_t	*path,
					       const comac_stroke_style_t	*stroke_style,
					       const comac_matrix_t	*ctm,
					       comac_antialias_t	 antialias,
					       comac_boxes_t		*boxes);

comac_private comac_int_status_t
_comac_path_fixed_stroke_to_traps (const comac_path_fixed_t	*path,
				   const comac_stroke_style_t	*stroke_style,
				   const comac_matrix_t	*ctm,
				   const comac_matrix_t	*ctm_inverse,
				   double		 tolerance,
				   comac_traps_t	*traps);

comac_private comac_int_status_t
_comac_path_fixed_stroke_polygon_to_traps (const comac_path_fixed_t	*path,
					   const comac_stroke_style_t	*stroke_style,
					   const comac_matrix_t	*ctm,
					   const comac_matrix_t	*ctm_inverse,
					   double		 tolerance,
					   comac_traps_t	*traps);

comac_private comac_status_t
_comac_path_fixed_stroke_to_shaper (comac_path_fixed_t	*path,
				   const comac_stroke_style_t	*stroke_style,
				   const comac_matrix_t	*ctm,
				   const comac_matrix_t	*ctm_inverse,
				   double		 tolerance,
				   comac_status_t (*add_triangle) (void *closure,
								   const comac_point_t triangle[3]),
				   comac_status_t (*add_triangle_fan) (void *closure,
								       const comac_point_t *midpt,
								       const comac_point_t *points,
								       int npoints),
				   comac_status_t (*add_quad) (void *closure,
							       const comac_point_t quad[4]),
				   void *closure);

/* comac-scaled-font.c */

comac_private void
_comac_scaled_font_freeze_cache (comac_scaled_font_t *scaled_font);

comac_private void
_comac_scaled_font_thaw_cache (comac_scaled_font_t *scaled_font);

comac_private void
_comac_scaled_font_reset_cache (comac_scaled_font_t *scaled_font);

comac_private comac_status_t
_comac_scaled_font_set_error (comac_scaled_font_t *scaled_font,
			      comac_status_t status);

comac_private comac_scaled_font_t *
_comac_scaled_font_create_in_error (comac_status_t status);

comac_private void
_comac_scaled_font_reset_static_data (void);

comac_private comac_status_t
_comac_scaled_font_register_placeholder_and_unlock_font_map (comac_scaled_font_t *scaled_font);

comac_private void
_comac_scaled_font_unregister_placeholder_and_lock_font_map (comac_scaled_font_t *scaled_font);

comac_private comac_status_t
_comac_scaled_font_init (comac_scaled_font_t               *scaled_font,
			 comac_font_face_t		   *font_face,
			 const comac_matrix_t              *font_matrix,
			 const comac_matrix_t              *ctm,
			 const comac_font_options_t	   *options,
			 const comac_scaled_font_backend_t *backend);

comac_private comac_status_t
_comac_scaled_font_set_metrics (comac_scaled_font_t	    *scaled_font,
				comac_font_extents_t	    *fs_metrics);

/* This should only be called on an error path by a scaled_font constructor */
comac_private void
_comac_scaled_font_fini (comac_scaled_font_t *scaled_font);

comac_private comac_status_t
_comac_scaled_font_font_extents (comac_scaled_font_t  *scaled_font,
				 comac_font_extents_t *extents);

comac_private comac_status_t
_comac_scaled_font_glyph_device_extents (comac_scaled_font_t	 *scaled_font,
					 const comac_glyph_t	 *glyphs,
					 int                      num_glyphs,
					 comac_rectangle_int_t   *extents,
					 comac_bool_t		 *overlap);

comac_private comac_bool_t
_comac_scaled_font_glyph_approximate_extents (comac_scaled_font_t	 *scaled_font,
					      const comac_glyph_t	 *glyphs,
					      int                      num_glyphs,
					      comac_rectangle_int_t   *extents);

comac_private comac_status_t
_comac_scaled_font_show_glyphs (comac_scaled_font_t *scaled_font,
				comac_operator_t     op,
				const comac_pattern_t *source,
				comac_surface_t	    *surface,
				int		     source_x,
				int		     source_y,
				int		     dest_x,
				int		     dest_y,
				unsigned int	     width,
				unsigned int	     height,
				comac_glyph_t	    *glyphs,
				int		     num_glyphs,
				comac_region_t	    *clip_region);

comac_private comac_status_t
_comac_scaled_font_glyph_path (comac_scaled_font_t *scaled_font,
			       const comac_glyph_t *glyphs,
			       int                  num_glyphs,
			       comac_path_fixed_t  *path);

comac_private void
_comac_scaled_glyph_set_metrics (comac_scaled_glyph_t *scaled_glyph,
				 comac_scaled_font_t *scaled_font,
				 comac_text_extents_t *fs_metrics);

comac_private void
_comac_scaled_glyph_set_surface (comac_scaled_glyph_t *scaled_glyph,
				 comac_scaled_font_t *scaled_font,
				 comac_image_surface_t *surface);

comac_private void
_comac_scaled_glyph_set_path (comac_scaled_glyph_t *scaled_glyph,
			      comac_scaled_font_t *scaled_font,
			      comac_path_fixed_t *path);

comac_private void
_comac_scaled_glyph_set_recording_surface (comac_scaled_glyph_t *scaled_glyph,
                                           comac_scaled_font_t *scaled_font,
                                           comac_surface_t *recording_surface);

comac_private void
_comac_scaled_glyph_set_color_surface (comac_scaled_glyph_t *scaled_glyph,
		                       comac_scaled_font_t *scaled_font,
		                       comac_image_surface_t *surface,
                                       comac_bool_t uses_foreground_color);

comac_private comac_int_status_t
_comac_scaled_glyph_lookup (comac_scaled_font_t *scaled_font,
			    unsigned long index,
			    comac_scaled_glyph_info_t info,
                            const comac_color_t   *foreground_color,
			    comac_scaled_glyph_t **scaled_glyph_ret);

comac_private double
_comac_scaled_font_get_max_scale (comac_scaled_font_t *scaled_font);

comac_private void
_comac_scaled_font_map_destroy (void);

/* comac-stroke-style.c */

comac_private void
_comac_stroke_style_init (comac_stroke_style_t *style);

comac_private comac_status_t
_comac_stroke_style_init_copy (comac_stroke_style_t *style,
			       const comac_stroke_style_t *other);

comac_private void
_comac_stroke_style_fini (comac_stroke_style_t *style);

comac_private void
_comac_stroke_style_max_distance_from_path (const comac_stroke_style_t *style,
					    const comac_path_fixed_t *path,
                                            const comac_matrix_t *ctm,
                                            double *dx, double *dy);
comac_private void
_comac_stroke_style_max_line_distance_from_path (const comac_stroke_style_t *style,
						 const comac_path_fixed_t *path,
						 const comac_matrix_t *ctm,
						 double *dx, double *dy);

comac_private void
_comac_stroke_style_max_join_distance_from_path (const comac_stroke_style_t *style,
						 const comac_path_fixed_t *path,
						 const comac_matrix_t *ctm,
						 double *dx, double *dy);

comac_private double
_comac_stroke_style_dash_period (const comac_stroke_style_t *style);

comac_private double
_comac_stroke_style_dash_stroked (const comac_stroke_style_t *style);

comac_private comac_bool_t
_comac_stroke_style_dash_can_approximate (const comac_stroke_style_t *style,
					  const comac_matrix_t *ctm,
					  double tolerance);

comac_private void
_comac_stroke_style_dash_approximate (const comac_stroke_style_t *style,
				      const comac_matrix_t *ctm,
				      double tolerance,
				      double *dash_offset,
				      double *dashes,
				      unsigned int *num_dashes);


/* comac-surface.c */

comac_private comac_bool_t
_comac_surface_has_mime_image (comac_surface_t *surface);

comac_private comac_status_t
_comac_surface_copy_mime_data (comac_surface_t *dst,
			       comac_surface_t *src);

comac_private_no_warn comac_int_status_t
_comac_surface_set_error (comac_surface_t	*surface,
			  comac_int_status_t	 status);

comac_private void
_comac_surface_set_resolution (comac_surface_t *surface,
                               double x_res,
                               double y_res);

comac_private comac_surface_t *
_comac_surface_create_for_rectangle_int (comac_surface_t *target,
					 const comac_rectangle_int_t *extents);

comac_private comac_surface_t *
_comac_surface_create_scratch (comac_surface_t	    *other,
			       comac_content_t	     content,
			       int		     width,
			       int		     height,
			       const comac_color_t  *color);

comac_private void
_comac_surface_init (comac_surface_t			*surface,
		     const comac_surface_backend_t	*backend,
		     comac_device_t			*device,
		     comac_content_t			 content,
		     comac_bool_t                        is_vector);

comac_private void
_comac_surface_set_font_options (comac_surface_t       *surface,
				 comac_font_options_t  *options);

comac_private comac_status_t
_comac_surface_paint (comac_surface_t	*surface,
		      comac_operator_t	 op,
		      const comac_pattern_t *source,
		      const comac_clip_t	    *clip);

comac_private comac_image_surface_t *
_comac_surface_map_to_image (comac_surface_t  *surface,
			     const comac_rectangle_int_t *extents);

comac_private_no_warn comac_int_status_t
_comac_surface_unmap_image (comac_surface_t       *surface,
			    comac_image_surface_t *image);

comac_private comac_status_t
_comac_surface_mask (comac_surface_t	*surface,
		     comac_operator_t	 op,
		     const comac_pattern_t	*source,
		     const comac_pattern_t	*mask,
		     const comac_clip_t		*clip);

comac_private comac_status_t
_comac_surface_fill_stroke (comac_surface_t	    *surface,
			    comac_operator_t	     fill_op,
			    const comac_pattern_t   *fill_source,
			    comac_fill_rule_t	     fill_rule,
			    double		     fill_tolerance,
			    comac_antialias_t	     fill_antialias,
			    comac_path_fixed_t	    *path,
			    comac_operator_t	     stroke_op,
			    const comac_pattern_t   *stroke_source,
			    const comac_stroke_style_t    *stroke_style,
			    const comac_matrix_t	    *stroke_ctm,
			    const comac_matrix_t	    *stroke_ctm_inverse,
			    double		     stroke_tolerance,
			    comac_antialias_t	     stroke_antialias,
			    const comac_clip_t	    *clip);

comac_private comac_status_t
_comac_surface_stroke (comac_surface_t		*surface,
		       comac_operator_t		 op,
		       const comac_pattern_t	*source,
		       const comac_path_fixed_t	*path,
		       const comac_stroke_style_t	*style,
		       const comac_matrix_t		*ctm,
		       const comac_matrix_t		*ctm_inverse,
		       double			 tolerance,
		       comac_antialias_t	 antialias,
		       const comac_clip_t		*clip);

comac_private comac_status_t
_comac_surface_fill (comac_surface_t	*surface,
		     comac_operator_t	 op,
		     const comac_pattern_t *source,
		     const comac_path_fixed_t	*path,
		     comac_fill_rule_t	 fill_rule,
		     double		 tolerance,
		     comac_antialias_t	 antialias,
		     const comac_clip_t	*clip);

comac_private comac_status_t
_comac_surface_show_text_glyphs (comac_surface_t	    *surface,
				 comac_operator_t	     op,
				 const comac_pattern_t	    *source,
				 const char		    *utf8,
				 int			     utf8_len,
				 comac_glyph_t		    *glyphs,
				 int			     num_glyphs,
				 const comac_text_cluster_t *clusters,
				 int			     num_clusters,
				 comac_text_cluster_flags_t  cluster_flags,
				 comac_scaled_font_t	    *scaled_font,
				 const comac_clip_t		    *clip);

comac_private comac_status_t
_comac_surface_tag (comac_surface_t	        *surface,
		    comac_bool_t                 begin,
		    const char                  *tag_name,
		    const char                  *attributes);

comac_private comac_status_t
_comac_surface_acquire_source_image (comac_surface_t         *surface,
				     comac_image_surface_t  **image_out,
				     void                   **image_extra);

comac_private void
_comac_surface_release_source_image (comac_surface_t        *surface,
				     comac_image_surface_t  *image,
				     void                   *image_extra);

comac_private comac_surface_t *
_comac_surface_snapshot (comac_surface_t *surface);

comac_private void
_comac_surface_attach_snapshot (comac_surface_t *surface,
				comac_surface_t *snapshot,
				comac_surface_func_t detach_func);

comac_private comac_surface_t *
_comac_surface_has_snapshot (comac_surface_t *surface,
			     const comac_surface_backend_t *backend);

comac_private void
_comac_surface_detach_snapshot (comac_surface_t *snapshot);

comac_private comac_status_t
_comac_surface_begin_modification (comac_surface_t *surface);

comac_private_no_warn comac_bool_t
_comac_surface_get_extents (comac_surface_t         *surface,
			    comac_rectangle_int_t   *extents);

comac_private comac_bool_t
_comac_surface_has_device_transform (comac_surface_t *surface) comac_pure;

comac_private void
_comac_surface_release_device_reference (comac_surface_t *surface);

/* comac-image-surface.c */

/* XXX: In comac 1.2.0 we added a new %COMAC_FORMAT_RGB16_565 but
 * neglected to adjust this macro. The net effect is that it's
 * impossible to externally create an image surface with this
 * format. This is perhaps a good thing since we also neglected to fix
 * up things like comac_surface_write_to_png() for the new format
 * (-Wswitch-enum will tell you where). Is it obvious that format was
 * added in haste?
 *
 * The reason for the new format was to allow the xlib backend to be
 * used on X servers with a 565 visual. So the new format did its job
 * for that, even without being considered "valid" for the sake of
 * things like comac_image_surface_create().
 *
 * Since 1.2.0 we ran into the same situation with X servers with BGR
 * visuals. This time we invented #comac_internal_format_t instead,
 * (see it for more discussion).
 *
 * The punchline is that %COMAC_FORMAT_VALID must not consider any
 * internal format to be valid. Also we need to decide if the
 * RGB16_565 should be moved to instead be an internal format. If so,
 * this macro need not change for it. (We probably will need to leave
 * an RGB16_565 value in the header files for the sake of code that
 * might have that value in it.)
 *
 * If we do decide to start fully supporting RGB16_565 as an external
 * format, then %COMAC_FORMAT_VALID needs to be adjusted to include
 * it. But that should not happen before all necessary code is fixed
 * to support it (at least comac_surface_write_to_png() and a few spots
 * in comac-xlib-surface.c--again see -Wswitch-enum).
 */
#define COMAC_FORMAT_VALID(format) ((format) >= COMAC_FORMAT_ARGB32 &&		\
                                    (format) <= COMAC_FORMAT_RGBA128F)

/* pixman-required stride alignment in bytes. */
#define COMAC_STRIDE_ALIGNMENT (sizeof (uint32_t))
#define COMAC_STRIDE_FOR_WIDTH_BPP(w,bpp) \
   ((((bpp)*(w)+7)/8 + COMAC_STRIDE_ALIGNMENT-1) & -COMAC_STRIDE_ALIGNMENT)

#define COMAC_CONTENT_VALID(content) ((content) && 			         \
				      (((content) & ~(COMAC_CONTENT_COLOR |      \
						      COMAC_CONTENT_ALPHA |      \
						      COMAC_CONTENT_COLOR_ALPHA))\
				       == 0))

comac_private int
_comac_format_bits_per_pixel (comac_format_t format) comac_const;

comac_private comac_format_t
_comac_format_from_content (comac_content_t content) comac_const;

comac_private comac_format_t
_comac_format_from_pixman_format (pixman_format_code_t pixman_format);

comac_private comac_content_t
_comac_content_from_format (comac_format_t format) comac_const;

comac_private comac_content_t
_comac_content_from_pixman_format (pixman_format_code_t pixman_format);

comac_private comac_surface_t *
_comac_image_surface_create_for_pixman_image (pixman_image_t		*pixman_image,
					      pixman_format_code_t	 pixman_format);

comac_private pixman_format_code_t
_comac_format_to_pixman_format_code (comac_format_t format);

comac_private comac_bool_t
_pixman_format_from_masks (comac_format_masks_t *masks,
			   pixman_format_code_t *format_ret);

comac_private comac_bool_t
_pixman_format_to_masks (pixman_format_code_t	 pixman_format,
			 comac_format_masks_t	*masks);

comac_private void
_comac_image_scaled_glyph_fini (comac_scaled_font_t *scaled_font,
				comac_scaled_glyph_t *scaled_glyph);

comac_private void
_comac_image_reset_static_data (void);

comac_private void
_comac_image_compositor_reset_static_data (void);

comac_private comac_surface_t *
_comac_image_surface_create_with_pixman_format (unsigned char		*data,
						pixman_format_code_t	 pixman_format,
						int			 width,
						int			 height,
						int			 stride);

comac_private comac_surface_t *
_comac_image_surface_create_with_content (comac_content_t	content,
					  int			width,
					  int			height);

comac_private void
_comac_image_surface_assume_ownership_of_data (comac_image_surface_t *surface);

comac_private comac_image_surface_t *
_comac_image_surface_coerce (comac_image_surface_t	*surface);

comac_private comac_image_surface_t *
_comac_image_surface_coerce_to_format (comac_image_surface_t	*surface,
			               comac_format_t		 format);

comac_private comac_image_transparency_t
_comac_image_analyze_transparency (comac_image_surface_t      *image);

comac_private comac_image_color_t
_comac_image_analyze_color (comac_image_surface_t      *image);

/* comac-pen.c */
comac_private int
_comac_pen_vertices_needed (double	    tolerance,
			    double	    radius,
			    const comac_matrix_t  *matrix);

comac_private comac_status_t
_comac_pen_init (comac_pen_t	*pen,
		 double		 radius,
		 double		 tolerance,
		 const comac_matrix_t	*ctm);

comac_private void
_comac_pen_init_empty (comac_pen_t *pen);

comac_private comac_status_t
_comac_pen_init_copy (comac_pen_t *pen, const comac_pen_t *other);

comac_private void
_comac_pen_fini (comac_pen_t *pen);

comac_private comac_status_t
_comac_pen_add_points (comac_pen_t *pen, comac_point_t *point, int num_points);

comac_private int
_comac_pen_find_active_cw_vertex_index (const comac_pen_t *pen,
					const comac_slope_t *slope);

comac_private int
_comac_pen_find_active_ccw_vertex_index (const comac_pen_t *pen,
					 const comac_slope_t *slope);

comac_private void
_comac_pen_find_active_cw_vertices (const comac_pen_t *pen,
				     const comac_slope_t *in,
				     const comac_slope_t *out,
				     int *start, int *stop);

comac_private void
_comac_pen_find_active_ccw_vertices (const comac_pen_t *pen,
				     const comac_slope_t *in,
				     const comac_slope_t *out,
				     int *start, int *stop);

/* comac-polygon.c */
comac_private void
_comac_polygon_init (comac_polygon_t   *polygon,
		     const comac_box_t *boxes,
		     int		num_boxes);

comac_private void
_comac_polygon_init_with_clip (comac_polygon_t *polygon,
			       const comac_clip_t *clip);

comac_private comac_status_t
_comac_polygon_init_boxes (comac_polygon_t *polygon,
			   const comac_boxes_t *boxes);

comac_private comac_status_t
_comac_polygon_init_box_array (comac_polygon_t *polygon,
			       comac_box_t *boxes,
			       int num_boxes);

comac_private void
_comac_polygon_limit (comac_polygon_t *polygon,
		     const comac_box_t *limits,
		     int num_limits);

comac_private void
_comac_polygon_limit_to_clip (comac_polygon_t *polygon,
			      const comac_clip_t *clip);

comac_private void
_comac_polygon_fini (comac_polygon_t *polygon);

comac_private_no_warn comac_status_t
_comac_polygon_add_line (comac_polygon_t *polygon,
			 const comac_line_t *line,
			 int top, int bottom,
			 int dir);

comac_private_no_warn comac_status_t
_comac_polygon_add_external_edge (void *polygon,
				  const comac_point_t *p1,
				  const comac_point_t *p2);

comac_private_no_warn comac_status_t
_comac_polygon_add_contour (comac_polygon_t *polygon,
			    const comac_contour_t *contour);

comac_private void
_comac_polygon_translate (comac_polygon_t *polygon, int dx, int dy);

comac_private comac_status_t
_comac_polygon_reduce (comac_polygon_t *polygon,
		       comac_fill_rule_t fill_rule);

comac_private comac_status_t
_comac_polygon_intersect (comac_polygon_t *a, int winding_a,
			  comac_polygon_t *b, int winding_b);

comac_private comac_status_t
_comac_polygon_intersect_with_boxes (comac_polygon_t *polygon,
				     comac_fill_rule_t *winding,
				     comac_box_t *boxes,
				     int num_boxes);

static inline comac_bool_t
_comac_polygon_is_empty (const comac_polygon_t *polygon)
{
    return
	polygon->num_edges == 0 ||
	polygon->extents.p2.x <= polygon->extents.p1.x;
}

#define _comac_polygon_status(P) ((comac_polygon_t *) (P))->status

/* comac-spline.c */
comac_private comac_bool_t
_comac_spline_init (comac_spline_t *spline,
		    comac_spline_add_point_func_t add_point_func,
		    void *closure,
		    const comac_point_t *a, const comac_point_t *b,
		    const comac_point_t *c, const comac_point_t *d);

comac_private comac_status_t
_comac_spline_decompose (comac_spline_t *spline, double tolerance);

comac_private comac_status_t
_comac_spline_bound (comac_spline_add_point_func_t add_point_func,
		     void *closure,
		     const comac_point_t *p0, const comac_point_t *p1,
		     const comac_point_t *p2, const comac_point_t *p3);

/* comac-matrix.c */
comac_private void
_comac_matrix_get_affine (const comac_matrix_t *matrix,
			  double *xx, double *yx,
			  double *xy, double *yy,
			  double *x0, double *y0);

comac_private void
_comac_matrix_transform_bounding_box (const comac_matrix_t *matrix,
				      double *x1, double *y1,
				      double *x2, double *y2,
				      comac_bool_t *is_tight);

comac_private void
_comac_matrix_transform_bounding_box_fixed (const comac_matrix_t *matrix,
					    comac_box_t          *bbox,
					    comac_bool_t         *is_tight);

comac_private comac_bool_t
_comac_matrix_is_invertible (const comac_matrix_t *matrix) comac_pure;

comac_private comac_bool_t
_comac_matrix_is_scale_0 (const comac_matrix_t *matrix) comac_pure;

comac_private double
_comac_matrix_compute_determinant (const comac_matrix_t *matrix) comac_pure;

comac_private comac_status_t
_comac_matrix_compute_basis_scale_factors (const comac_matrix_t *matrix,
					   double *sx, double *sy, int x_major);

static inline comac_bool_t
_comac_matrix_is_identity (const comac_matrix_t *matrix)
{
    return (matrix->xx == 1.0 && matrix->yx == 0.0 &&
	    matrix->xy == 0.0 && matrix->yy == 1.0 &&
	    matrix->x0 == 0.0 && matrix->y0 == 0.0);
}

static inline comac_bool_t
_comac_matrix_is_translation (const comac_matrix_t *matrix)
{
    return (matrix->xx == 1.0 && matrix->yx == 0.0 &&
	    matrix->xy == 0.0 && matrix->yy == 1.0);
}

static inline comac_bool_t
_comac_matrix_is_scale (const comac_matrix_t *matrix)
{
    return matrix->yx == 0.0 && matrix->xy == 0.0;
}

comac_private comac_bool_t
_comac_matrix_is_integer_translation(const comac_matrix_t *matrix,
				     int *itx, int *ity);

comac_private comac_bool_t
_comac_matrix_has_unity_scale (const comac_matrix_t *matrix);

comac_private comac_bool_t
_comac_matrix_is_pixel_exact (const comac_matrix_t *matrix) comac_pure;

comac_private double
_comac_matrix_transformed_circle_major_axis (const comac_matrix_t *matrix,
					     double radius) comac_pure;

comac_private comac_bool_t
_comac_matrix_is_pixman_translation (const comac_matrix_t     *matrix,
				     comac_filter_t            filter,
				     int                      *out_x_offset,
				     int                      *out_y_offset);

comac_private comac_status_t
_comac_matrix_to_pixman_matrix_offset (const comac_matrix_t	*matrix,
				       comac_filter_t            filter,
				       double                    xc,
				       double                    yc,
				       pixman_transform_t	*out_transform,
				       int                      *out_x_offset,
				       int                      *out_y_offset);

comac_private void
_comac_debug_print_matrix (FILE *file, const comac_matrix_t *matrix);

comac_private void
_comac_debug_print_rect (FILE *file, const comac_rectangle_int_t *rect);

comac_private comac_status_t
_comac_bentley_ottmann_tessellate_rectilinear_polygon (comac_traps_t	 *traps,
						       const comac_polygon_t *polygon,
						       comac_fill_rule_t	  fill_rule);

comac_private comac_status_t
_comac_bentley_ottmann_tessellate_polygon (comac_traps_t         *traps,
					   const comac_polygon_t *polygon,
					   comac_fill_rule_t      fill_rule);

comac_private comac_status_t
_comac_bentley_ottmann_tessellate_traps (comac_traps_t *traps,
					 comac_fill_rule_t fill_rule);

comac_private comac_status_t
_comac_bentley_ottmann_tessellate_rectangular_traps (comac_traps_t *traps,
						     comac_fill_rule_t fill_rule);

comac_private comac_status_t
_comac_bentley_ottmann_tessellate_boxes (const comac_boxes_t *in,
					 comac_fill_rule_t fill_rule,
					 comac_boxes_t *out);

comac_private comac_status_t
_comac_bentley_ottmann_tessellate_rectilinear_traps (comac_traps_t *traps,
						     comac_fill_rule_t fill_rule);

comac_private comac_status_t
_comac_bentley_ottmann_tessellate_rectilinear_polygon_to_boxes (const comac_polygon_t *polygon,
								comac_fill_rule_t fill_rule,
								comac_boxes_t *boxes);

comac_private void
_comac_trapezoid_array_translate_and_scale (comac_trapezoid_t *offset_traps,
					    comac_trapezoid_t *src_traps,
					    int num_traps,
					    double tx, double ty,
					    double sx, double sy);

comac_private void
_comac_clip_reset_static_data (void);

comac_private void
_comac_pattern_reset_static_data (void);

/* comac-unicode.c */

comac_private int
_comac_utf8_get_char_validated (const char *p,
				uint32_t   *unicode);

comac_private comac_status_t
_comac_utf8_to_ucs4 (const char *str,
		     int	 len,
		     uint32_t  **result,
		     int	*items_written);

comac_private int
_comac_ucs4_to_utf8 (uint32_t    unicode,
		     char       *utf8);

comac_private int
_comac_ucs4_to_utf16 (uint32_t    unicode,
		      uint16_t   *utf16);

#if _WIN32 || COMAC_HAS_WIN32_FONT || COMAC_HAS_QUARTZ_FONT || COMAC_HAS_PDF_OPERATORS
# define COMAC_HAS_UTF8_TO_UTF16 1
#endif
#if COMAC_HAS_UTF8_TO_UTF16
comac_private comac_status_t
_comac_utf8_to_utf16 (const char *str,
		      int	  len,
		      uint16_t  **result,
		      int	 *items_written);
#endif

comac_private void
_comac_matrix_multiply (comac_matrix_t *r,
			const comac_matrix_t *a,
			const comac_matrix_t *b);

/* comac-observer.c */

comac_private void
_comac_observers_notify (comac_list_t *observers, void *arg);

/* Open a file with a UTF-8 filename */
comac_private comac_status_t
_comac_fopen (const char *filename, const char *mode, FILE **file_out);

#if COMAC_HAS_PNG_FUNCTIONS


#endif


#include "comac-mutex-private.h"
#include "comac-fixed-private.h"
#include "comac-wideint-private.h"
#include "comac-malloc-private.h"
#include "comac-hash-private.h"

#if HAVE_VALGRIND
#include <memcheck.h>

#define VG(x) x

comac_private void
_comac_debug_check_image_surface_is_defined (const comac_surface_t *surface);

#else

#define VG(x)
#define _comac_debug_check_image_surface_is_defined(X)

#endif

comac_private void
_comac_debug_print_path (FILE *stream, const comac_path_fixed_t *path);

comac_private void
_comac_debug_print_polygon (FILE *stream, comac_polygon_t *polygon);

comac_private void
_comac_debug_print_traps (FILE *file, const comac_traps_t *traps);

comac_private void
_comac_debug_print_clip (FILE *stream, const comac_clip_t *clip);

#if 0
#define TRACE(x) fprintf (stderr, "%s: ", __FILE__), fprintf x
#define TRACE_(x) x
#else
#define TRACE(x)
#define TRACE_(x)
#endif

COMAC_END_DECLS

#endif
