/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@cworth.org>
 *      Adrian Johnson <ajohnson@redneon.com>
 */

#include "comacint.h"
#include "comac-error-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#ifdef HAVE_XLOCALE_H
#include <xlocale.h>
#endif

COMPILE_TIME_ASSERT ((int) COMAC_STATUS_LAST_STATUS <
		     (int) COMAC_INT_STATUS_UNSUPPORTED);
COMPILE_TIME_ASSERT (COMAC_INT_STATUS_LAST_STATUS <= 127);

/**
 * SECTION:comac-status
 * @Title: Error handling
 * @Short_Description: Decoding comac's status
 * @See_Also: comac_status(), comac_surface_status(), comac_pattern_status(),
 *            comac_font_face_status(), comac_scaled_font_status(),
 *            comac_region_status()
 *
 * Comac uses a single status type to represent all kinds of errors.  A status
 * value of %COMAC_STATUS_SUCCESS represents no error and has an integer value
 * of zero.  All other status values represent an error.
 *
 * Comac's error handling is designed to be easy to use and safe.  All major
 * comac objects <firstterm>retain</firstterm> an error status internally which
 * can be queried anytime by the users using comac*_status() calls.  In
 * the mean time, it is safe to call all comac functions normally even if the
 * underlying object is in an error status.  This means that no error handling
 * code is required before or after each individual comac function call.
 **/

/* Public stuff */

/**
 * comac_status_to_string:
 * @status: a comac status
 *
 * Provides a human-readable description of a #comac_status_t.
 *
 * Returns: a string representation of the status
 *
 * Since: 1.0
 **/
const char *
comac_status_to_string (comac_status_t status)
{
    switch (status) {
    case COMAC_STATUS_SUCCESS:
	return "no error has occurred";
    case COMAC_STATUS_NO_MEMORY:
	return "out of memory";
    case COMAC_STATUS_INVALID_RESTORE:
	return "comac_restore() without matching comac_save()";
    case COMAC_STATUS_INVALID_POP_GROUP:
	return "no saved group to pop, i.e. comac_pop_group() without matching "
	       "comac_push_group()";
    case COMAC_STATUS_NO_CURRENT_POINT:
	return "no current point defined";
    case COMAC_STATUS_INVALID_MATRIX:
	return "invalid matrix (not invertible)";
    case COMAC_STATUS_INVALID_STATUS:
	return "invalid value for an input comac_status_t";
    case COMAC_STATUS_NULL_POINTER:
	return "NULL pointer";
    case COMAC_STATUS_INVALID_STRING:
	return "input string not valid UTF-8";
    case COMAC_STATUS_INVALID_PATH_DATA:
	return "input path data not valid";
    case COMAC_STATUS_READ_ERROR:
	return "error while reading from input stream";
    case COMAC_STATUS_WRITE_ERROR:
	return "error while writing to output stream";
    case COMAC_STATUS_SURFACE_FINISHED:
	return "the target surface has been finished";
    case COMAC_STATUS_SURFACE_TYPE_MISMATCH:
	return "the surface type is not appropriate for the operation";
    case COMAC_STATUS_PATTERN_TYPE_MISMATCH:
	return "the pattern type is not appropriate for the operation";
    case COMAC_STATUS_INVALID_CONTENT:
	return "invalid value for an input comac_content_t";
    case COMAC_STATUS_INVALID_FORMAT:
	return "invalid value for an input comac_format_t";
    case COMAC_STATUS_INVALID_VISUAL:
	return "invalid value for an input Visual*";
    case COMAC_STATUS_FILE_NOT_FOUND:
	return "file not found";
    case COMAC_STATUS_INVALID_DASH:
	return "invalid value for a dash setting";
    case COMAC_STATUS_INVALID_DSC_COMMENT:
	return "invalid value for a DSC comment";
    case COMAC_STATUS_INVALID_INDEX:
	return "invalid index passed to getter";
    case COMAC_STATUS_CLIP_NOT_REPRESENTABLE:
	return "clip region not representable in desired format";
    case COMAC_STATUS_TEMP_FILE_ERROR:
	return "error creating or writing to a temporary file";
    case COMAC_STATUS_INVALID_STRIDE:
	return "invalid value for stride";
    case COMAC_STATUS_FONT_TYPE_MISMATCH:
	return "the font type is not appropriate for the operation";
    case COMAC_STATUS_USER_FONT_IMMUTABLE:
	return "the user-font is immutable";
    case COMAC_STATUS_USER_FONT_ERROR:
	return "error occurred in a user-font callback function";
    case COMAC_STATUS_NEGATIVE_COUNT:
	return "negative number used where it is not allowed";
    case COMAC_STATUS_INVALID_CLUSTERS:
	return "input clusters do not represent the accompanying text and "
	       "glyph arrays";
    case COMAC_STATUS_INVALID_SLANT:
	return "invalid value for an input comac_font_slant_t";
    case COMAC_STATUS_INVALID_WEIGHT:
	return "invalid value for an input comac_font_weight_t";
    case COMAC_STATUS_INVALID_SIZE:
	return "invalid value (typically too big) for the size of the input "
	       "(surface, pattern, etc.)";
    case COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED:
	return "user-font method not implemented";
    case COMAC_STATUS_DEVICE_TYPE_MISMATCH:
	return "the device type is not appropriate for the operation";
    case COMAC_STATUS_DEVICE_ERROR:
	return "an operation to the device caused an unspecified error";
    case COMAC_STATUS_INVALID_MESH_CONSTRUCTION:
	return "invalid operation during mesh pattern construction";
    case COMAC_STATUS_DEVICE_FINISHED:
	return "the target device has been finished";
    case COMAC_STATUS_JBIG2_GLOBAL_MISSING:
	return "COMAC_MIME_TYPE_JBIG2_GLOBAL_ID used but no "
	       "COMAC_MIME_TYPE_JBIG2_GLOBAL data provided";
    case COMAC_STATUS_PNG_ERROR:
	return "error occurred in libpng while reading from or writing to a "
	       "PNG file";
    case COMAC_STATUS_FREETYPE_ERROR:
	return "error occurred in libfreetype";
    case COMAC_STATUS_WIN32_GDI_ERROR:
	return "error occurred in the Windows Graphics Device Interface";
    case COMAC_STATUS_TAG_ERROR:
	return "invalid tag name, attributes, or nesting";
    case COMAC_STATUS_DWRITE_ERROR:
	return "Window Direct Write error";
    default:
    case COMAC_STATUS_LAST_STATUS:
	return "<unknown error status>";
    }
}

/**
 * comac_glyph_allocate:
 * @num_glyphs: number of glyphs to allocate
 *
 * Allocates an array of #comac_glyph_t's.
 * This function is only useful in implementations of
 * #comac_user_scaled_font_text_to_glyphs_func_t where the user
 * needs to allocate an array of glyphs that comac will free.
 * For all other uses, user can use their own allocation method
 * for glyphs.
 *
 * This function returns %NULL if @num_glyphs is not positive,
 * or if out of memory.  That means, the %NULL return value
 * signals out-of-memory only if @num_glyphs was positive.
 *
 * Returns: the newly allocated array of glyphs that should be
 *          freed using comac_glyph_free()
 *
 * Since: 1.8
 **/
comac_glyph_t *
comac_glyph_allocate (int num_glyphs)
{
    if (num_glyphs <= 0)
	return NULL;

    return _comac_malloc_ab (num_glyphs, sizeof (comac_glyph_t));
}

/**
 * comac_glyph_free:
 * @glyphs: array of glyphs to free, or %NULL
 *
 * Frees an array of #comac_glyph_t's allocated using comac_glyph_allocate().
 * This function is only useful to free glyph array returned
 * by comac_scaled_font_text_to_glyphs() where comac returns
 * an array of glyphs that the user will free.
 * For all other uses, user can use their own allocation method
 * for glyphs.
 *
 * Since: 1.8
 **/
void
comac_glyph_free (comac_glyph_t *glyphs)
{
    free (glyphs);
}

/**
 * comac_text_cluster_allocate:
 * @num_clusters: number of text_clusters to allocate
 *
 * Allocates an array of #comac_text_cluster_t's.
 * This function is only useful in implementations of
 * #comac_user_scaled_font_text_to_glyphs_func_t where the user
 * needs to allocate an array of text clusters that comac will free.
 * For all other uses, user can use their own allocation method
 * for text clusters.
 *
 * This function returns %NULL if @num_clusters is not positive,
 * or if out of memory.  That means, the %NULL return value
 * signals out-of-memory only if @num_clusters was positive.
 *
 * Returns: the newly allocated array of text clusters that should be
 *          freed using comac_text_cluster_free()
 *
 * Since: 1.8
 **/
comac_text_cluster_t *
comac_text_cluster_allocate (int num_clusters)
{
    if (num_clusters <= 0)
	return NULL;

    return _comac_malloc_ab (num_clusters, sizeof (comac_text_cluster_t));
}

/**
 * comac_text_cluster_free:
 * @clusters: array of text clusters to free, or %NULL
 *
 * Frees an array of #comac_text_cluster's allocated using comac_text_cluster_allocate().
 * This function is only useful to free text cluster array returned
 * by comac_scaled_font_text_to_glyphs() where comac returns
 * an array of text clusters that the user will free.
 * For all other uses, user can use their own allocation method
 * for text clusters.
 *
 * Since: 1.8
 **/
void
comac_text_cluster_free (comac_text_cluster_t *clusters)
{
    free (clusters);
}

/* Private stuff */

/**
 * _comac_validate_text_clusters:
 * @utf8: UTF-8 text
 * @utf8_len: length of @utf8 in bytes
 * @glyphs: array of glyphs
 * @num_glyphs: number of glyphs
 * @clusters: array of cluster mapping information
 * @num_clusters: number of clusters in the mapping
 * @cluster_flags: cluster flags
 *
 * Check that clusters cover the entire glyphs and utf8 arrays,
 * and that cluster boundaries are UTF-8 boundaries.
 *
 * Return value: %COMAC_STATUS_SUCCESS upon success, or
 *               %COMAC_STATUS_INVALID_CLUSTERS on error.
 *               The error is either invalid UTF-8 input,
 *               or bad cluster mapping.
 **/
comac_status_t
_comac_validate_text_clusters (const char *utf8,
			       int utf8_len,
			       const comac_glyph_t *glyphs,
			       int num_glyphs,
			       const comac_text_cluster_t *clusters,
			       int num_clusters,
			       comac_text_cluster_flags_t cluster_flags)
{
    comac_status_t status;
    unsigned int n_bytes = 0;
    unsigned int n_glyphs = 0;
    int i;

    for (i = 0; i < num_clusters; i++) {
	int cluster_bytes = clusters[i].num_bytes;
	int cluster_glyphs = clusters[i].num_glyphs;

	if (cluster_bytes < 0 || cluster_glyphs < 0)
	    goto BAD;

	/* A cluster should cover at least one character or glyph.
	 * I can't see any use for a 0,0 cluster.
	 * I can't see an immediate use for a zero-text cluster
	 * right now either, but they don't harm.
	 * Zero-glyph clusters on the other hand are useful for
	 * things like U+200C ZERO WIDTH NON-JOINER */
	if (cluster_bytes == 0 && cluster_glyphs == 0)
	    goto BAD;

	/* Since n_bytes and n_glyphs are unsigned, but the rest of
	 * values involved are signed, we can detect overflow easily */
	if (n_bytes + cluster_bytes > (unsigned int) utf8_len ||
	    n_glyphs + cluster_glyphs > (unsigned int) num_glyphs)
	    goto BAD;

	/* Make sure we've got valid UTF-8 for the cluster */
	status =
	    _comac_utf8_to_ucs4 (utf8 + n_bytes, cluster_bytes, NULL, NULL);
	if (unlikely (status))
	    return _comac_error (COMAC_STATUS_INVALID_CLUSTERS);

	n_bytes += cluster_bytes;
	n_glyphs += cluster_glyphs;
    }

    if (n_bytes != (unsigned int) utf8_len ||
	n_glyphs != (unsigned int) num_glyphs) {
    BAD:
	return _comac_error (COMAC_STATUS_INVALID_CLUSTERS);
    }

    return COMAC_STATUS_SUCCESS;
}

/**
 * _comac_operator_bounded_by_mask:
 * @op: a #comac_operator_t
 *
 * A bounded operator is one where mask pixel
 * of zero results in no effect on the destination image.
 *
 * Unbounded operators often require special handling; if you, for
 * example, draw trapezoids with an unbounded operator, the effect
 * extends past the bounding box of the trapezoids.
 *
 * Return value: %TRUE if the operator is bounded by the mask operand
 **/
comac_bool_t
_comac_operator_bounded_by_mask (comac_operator_t op)
{
    switch (op) {
    case COMAC_OPERATOR_CLEAR:
    case COMAC_OPERATOR_SOURCE:
    case COMAC_OPERATOR_OVER:
    case COMAC_OPERATOR_ATOP:
    case COMAC_OPERATOR_DEST:
    case COMAC_OPERATOR_DEST_OVER:
    case COMAC_OPERATOR_DEST_OUT:
    case COMAC_OPERATOR_XOR:
    case COMAC_OPERATOR_ADD:
    case COMAC_OPERATOR_SATURATE:
    case COMAC_OPERATOR_MULTIPLY:
    case COMAC_OPERATOR_SCREEN:
    case COMAC_OPERATOR_OVERLAY:
    case COMAC_OPERATOR_DARKEN:
    case COMAC_OPERATOR_LIGHTEN:
    case COMAC_OPERATOR_COLOR_DODGE:
    case COMAC_OPERATOR_COLOR_BURN:
    case COMAC_OPERATOR_HARD_LIGHT:
    case COMAC_OPERATOR_SOFT_LIGHT:
    case COMAC_OPERATOR_DIFFERENCE:
    case COMAC_OPERATOR_EXCLUSION:
    case COMAC_OPERATOR_HSL_HUE:
    case COMAC_OPERATOR_HSL_SATURATION:
    case COMAC_OPERATOR_HSL_COLOR:
    case COMAC_OPERATOR_HSL_LUMINOSITY:
	return TRUE;
    case COMAC_OPERATOR_OUT:
    case COMAC_OPERATOR_IN:
    case COMAC_OPERATOR_DEST_IN:
    case COMAC_OPERATOR_DEST_ATOP:
	return FALSE;
    default:
	ASSERT_NOT_REACHED;
	return FALSE; /* squelch warning */
    }
}

/**
 * _comac_operator_bounded_by_source:
 * @op: a #comac_operator_t
 *
 * A bounded operator is one where source pixels of zero
 * (in all four components, r, g, b and a) effect no change
 * in the resulting destination image.
 *
 * Unbounded operators often require special handling; if you, for
 * example, copy a surface with the SOURCE operator, the effect
 * extends past the bounding box of the source surface.
 *
 * Return value: %TRUE if the operator is bounded by the source operand
 **/
comac_bool_t
_comac_operator_bounded_by_source (comac_operator_t op)
{
    switch (op) {
    case COMAC_OPERATOR_OVER:
    case COMAC_OPERATOR_ATOP:
    case COMAC_OPERATOR_DEST:
    case COMAC_OPERATOR_DEST_OVER:
    case COMAC_OPERATOR_DEST_OUT:
    case COMAC_OPERATOR_XOR:
    case COMAC_OPERATOR_ADD:
    case COMAC_OPERATOR_SATURATE:
    case COMAC_OPERATOR_MULTIPLY:
    case COMAC_OPERATOR_SCREEN:
    case COMAC_OPERATOR_OVERLAY:
    case COMAC_OPERATOR_DARKEN:
    case COMAC_OPERATOR_LIGHTEN:
    case COMAC_OPERATOR_COLOR_DODGE:
    case COMAC_OPERATOR_COLOR_BURN:
    case COMAC_OPERATOR_HARD_LIGHT:
    case COMAC_OPERATOR_SOFT_LIGHT:
    case COMAC_OPERATOR_DIFFERENCE:
    case COMAC_OPERATOR_EXCLUSION:
    case COMAC_OPERATOR_HSL_HUE:
    case COMAC_OPERATOR_HSL_SATURATION:
    case COMAC_OPERATOR_HSL_COLOR:
    case COMAC_OPERATOR_HSL_LUMINOSITY:
	return TRUE;
    case COMAC_OPERATOR_CLEAR:
    case COMAC_OPERATOR_SOURCE:
    case COMAC_OPERATOR_OUT:
    case COMAC_OPERATOR_IN:
    case COMAC_OPERATOR_DEST_IN:
    case COMAC_OPERATOR_DEST_ATOP:
	return FALSE;
    default:
	ASSERT_NOT_REACHED;
	return FALSE; /* squelch warning */
    }
}

uint32_t
_comac_operator_bounded_by_either (comac_operator_t op)
{
    switch (op) {
    case COMAC_OPERATOR_OVER:
    case COMAC_OPERATOR_ATOP:
    case COMAC_OPERATOR_DEST:
    case COMAC_OPERATOR_DEST_OVER:
    case COMAC_OPERATOR_DEST_OUT:
    case COMAC_OPERATOR_XOR:
    case COMAC_OPERATOR_ADD:
    case COMAC_OPERATOR_SATURATE:
    case COMAC_OPERATOR_MULTIPLY:
    case COMAC_OPERATOR_SCREEN:
    case COMAC_OPERATOR_OVERLAY:
    case COMAC_OPERATOR_DARKEN:
    case COMAC_OPERATOR_LIGHTEN:
    case COMAC_OPERATOR_COLOR_DODGE:
    case COMAC_OPERATOR_COLOR_BURN:
    case COMAC_OPERATOR_HARD_LIGHT:
    case COMAC_OPERATOR_SOFT_LIGHT:
    case COMAC_OPERATOR_DIFFERENCE:
    case COMAC_OPERATOR_EXCLUSION:
    case COMAC_OPERATOR_HSL_HUE:
    case COMAC_OPERATOR_HSL_SATURATION:
    case COMAC_OPERATOR_HSL_COLOR:
    case COMAC_OPERATOR_HSL_LUMINOSITY:
	return COMAC_OPERATOR_BOUND_BY_MASK | COMAC_OPERATOR_BOUND_BY_SOURCE;
    case COMAC_OPERATOR_CLEAR:
    case COMAC_OPERATOR_SOURCE:
	return COMAC_OPERATOR_BOUND_BY_MASK;
    case COMAC_OPERATOR_OUT:
    case COMAC_OPERATOR_IN:
    case COMAC_OPERATOR_DEST_IN:
    case COMAC_OPERATOR_DEST_ATOP:
	return 0;
    default:
	ASSERT_NOT_REACHED;
	return FALSE; /* squelch warning */
    }
}

#if DISABLE_SOME_FLOATING_POINT
/* This function is identical to the C99 function lround(), except that it
 * performs arithmetic rounding (floor(d + .5) instead of away-from-zero rounding) and
 * has a valid input range of (INT_MIN, INT_MAX] instead of
 * [INT_MIN, INT_MAX]. It is much faster on both x86 and FPU-less systems
 * than other commonly used methods for rounding (lround, round, rint, lrint
 * or float (d + 0.5)).
 *
 * The reason why this function is much faster on x86 than other
 * methods is due to the fact that it avoids the fldcw instruction.
 * This instruction incurs a large performance penalty on modern Intel
 * processors due to how it prevents efficient instruction pipelining.
 *
 * The reason why this function is much faster on FPU-less systems is for
 * an entirely different reason. All common rounding methods involve multiple
 * floating-point operations. Each one of these operations has to be
 * emulated in software, which adds up to be a large performance penalty.
 * This function doesn't perform any floating-point calculations, and thus
 * avoids this penalty.
  */
int
_comac_lround (double d)
{
    uint32_t top, shift_amount, output;
    union {
	double d;
	uint64_t ui64;
	uint32_t ui32[2];
    } u;

    u.d = d;

    /* If the integer word order doesn't match the float word order, we swap
     * the words of the input double. This is needed because we will be
     * treating the whole double as a 64-bit unsigned integer. Notice that we
     * use WORDS_BIGENDIAN to detect the integer word order, which isn't
     * exactly correct because WORDS_BIGENDIAN refers to byte order, not word
     * order. Thus, we are making the assumption that the byte order is the
     * same as the integer word order which, on the modern machines that we
     * care about, is OK.
     */
#if (defined(FLOAT_WORDS_BIGENDIAN) && ! defined(WORDS_BIGENDIAN)) ||          \
    (! defined(FLOAT_WORDS_BIGENDIAN) && defined(WORDS_BIGENDIAN))
    {
	uint32_t temp = u.ui32[0];
	u.ui32[0] = u.ui32[1];
	u.ui32[1] = temp;
    }
#endif

#ifdef WORDS_BIGENDIAN
#define MSW (0) /* Most Significant Word */
#define LSW (1) /* Least Significant Word */
#else
#define MSW (1)
#define LSW (0)
#endif

    /* By shifting the most significant word of the input double to the
     * right 20 places, we get the very "top" of the double where the exponent
     * and sign bit lie.
     */
    top = u.ui32[MSW] >> 20;

    /* Here, we calculate how much we have to shift the mantissa to normalize
     * it to an integer value. We extract the exponent "top" by masking out the
     * sign bit, then we calculate the shift amount by subtracting the exponent
     * from the bias. Notice that the correct bias for 64-bit doubles is
     * actually 1075, but we use 1053 instead for two reasons:
     *
     *  1) To perform rounding later on, we will first need the target
     *     value in a 31.1 fixed-point format. Thus, the bias needs to be one
     *     less: (1075 - 1: 1074).
     *
     *  2) To avoid shifting the mantissa as a full 64-bit integer (which is
     *     costly on certain architectures), we break the shift into two parts.
     *     First, the upper and lower parts of the mantissa are shifted
     *     individually by a constant amount that all valid inputs will require
     *     at the very least. This amount is chosen to be 21, because this will
     *     allow the two parts of the mantissa to later be combined into a
     *     single 32-bit representation, on which the remainder of the shift
     *     will be performed. Thus, we decrease the bias by an additional 21:
     *     (1074 - 21: 1053).
     */
    shift_amount = 1053 - (top & 0x7FF);

    /* We are done with the exponent portion in "top", so here we shift it off
     * the end.
     */
    top >>= 11;

    /* Before we perform any operations on the mantissa, we need to OR in
     * the implicit 1 at the top (see the IEEE-754 spec). We needn't mask
     * off the sign bit nor the exponent bits because these higher bits won't
     * make a bit of difference in the rest of our calculations.
     */
    u.ui32[MSW] |= 0x100000;

    /* If the input double is negative, we have to decrease the mantissa
     * by a hair. This is an important part of performing arithmetic rounding,
     * as negative numbers must round towards positive infinity in the
     * halfwase case of -x.5. Since "top" contains only the sign bit at this
     * point, we can just decrease the mantissa by the value of "top".
     */
    u.ui64 -= top;

    /* By decrementing "top", we create a bitmask with a value of either
     * 0x0 (if the input was negative) or 0xFFFFFFFF (if the input was positive
     * and thus the unsigned subtraction underflowed) that we'll use later.
     */
    top--;

    /* Here, we shift the mantissa by the constant value as described above.
     * We can emulate a 64-bit shift right by 21 through shifting the top 32
     * bits left 11 places and ORing in the bottom 32 bits shifted 21 places
     * to the right. Both parts of the mantissa are now packed into a single
     * 32-bit integer. Although we severely truncate the lower part in the
     * process, we still have enough significant bits to perform the conversion
     * without error (for all valid inputs).
     */
    output = (u.ui32[MSW] << 11) | (u.ui32[LSW] >> 21);

    /* Next, we perform the shift that converts the X.Y fixed-point number
     * currently found in "output" to the desired 31.1 fixed-point format
     * needed for the following rounding step. It is important to consider
     * all possible values for "shift_amount" at this point:
     *
     * - {shift_amount < 0} Since shift_amount is an unsigned integer, it
     *   really can't have a value less than zero. But, if the shift_amount
     *   calculation above caused underflow (which would happen with
     *   input > INT_MAX or input <= INT_MIN) then shift_amount will now be
     *   a very large number, and so this shift will result in complete
     *   garbage. But that's OK, as the input was out of our range, so our
     *   output is undefined.
     *
     * - {shift_amount > 31} If the magnitude of the input was very small
     *   (i.e. |input| << 1.0), shift_amount will have a value greater than
     *   31. Thus, this shift will also result in garbage. After performing
     *   the shift, we will zero-out "output" if this is the case.
     *
     * - {0 <= shift_amount < 32} In this case, the shift will properly convert
     *   the mantissa into a 31.1 fixed-point number.
     */
    output >>= shift_amount;

    /* This is where we perform rounding with the 31.1 fixed-point number.
     * Since what we're after is arithmetic rounding, we simply add the single
     * fractional bit into the integer part of "output", and just keep the
     * integer part.
     */
    output = (output >> 1) + (output & 1);

    /* Here, we zero-out the result if the magnitude if the input was very small
     * (as explained in the section above). Notice that all input out of the
     * valid range is also caught by this condition, which means we produce 0
     * for all invalid input, which is a nice side effect.
     *
     * The most straightforward way to do this would be:
     *
     *      if (shift_amount > 31)
     *          output = 0;
     *
     * But we can use a little trick to avoid the potential branch. The
     * expression (shift_amount > 31) will be either 1 or 0, which when
     * decremented will be either 0x0 or 0xFFFFFFFF (unsigned underflow),
     * which can be used to conditionally mask away all the bits in "output"
     * (in the 0x0 case), effectively zeroing it out. Certain, compilers would
     * have done this for us automatically.
     */
    output &= ((shift_amount > 31) - 1);

    /* If the input double was a negative number, then we have to negate our
     * output. The most straightforward way to do this would be:
     *
     *      if (!top)
     *          output = -output;
     *
     * as "top" at this point is either 0x0 (if the input was negative) or
     * 0xFFFFFFFF (if the input was positive). But, we can use a trick to
     * avoid the branch. Observe that the following snippet of code has the
     * same effect as the reference snippet above:
     *
     *      if (!top)
     *          output = 0 - output;
     *      else
     *          output = output - 0;
     *
     * Armed with the bitmask found in "top", we can condense the two statements
     * into the following:
     *
     *      output = (output & top) - (output & ~top);
     *
     * where, in the case that the input double was negative, "top" will be 0,
     * and the statement will be equivalent to:
     *
     *      output = (0) - (output);
     *
     * and if the input double was positive, "top" will be 0xFFFFFFFF, and the
     * statement will be equivalent to:
     *
     *      output = (output) - (0);
     *
     * Which, as pointed out earlier, is equivalent to the original reference
     * snippet.
     */
    output = (output & top) - (output & ~top);

    return output;
#undef MSW
#undef LSW
}
#endif

/* Convert a 32-bit IEEE single precision floating point number to a
 * 'half' representation (s10.5)
 */
uint16_t
_comac_half_from_float (float f)
{
    union {
	uint32_t ui;
	float f;
    } u;
    int s, e, m;

    u.f = f;
    s = (u.ui >> 16) & 0x00008000;
    e = ((u.ui >> 23) & 0x000000ff) - (127 - 15);
    m = u.ui & 0x007fffff;
    if (e <= 0) {
	if (e < -10) {
	    /* underflow */
	    return 0;
	}

	m = (m | 0x00800000) >> (1 - e);

	/* round to nearest, round 0.5 up. */
	if (m & 0x00001000)
	    m += 0x00002000;
	return s | (m >> 13);
    } else if (e == 0xff - (127 - 15)) {
	if (m == 0) {
	    /* infinity */
	    return s | 0x7c00;
	} else {
	    /* nan */
	    m >>= 13;
	    return s | 0x7c00 | m | (m == 0);
	}
    } else {
	/* round to nearest, round 0.5 up. */
	if (m & 0x00001000) {
	    m += 0x00002000;

	    if (m & 0x00800000) {
		m = 0;
		e += 1;
	    }
	}

	if (e > 30) {
	    /* overflow -> infinity */
	    return s | 0x7c00;
	}

	return s | (e << 10) | (m >> 13);
    }
}

#ifndef __BIONIC__
#include <locale.h>

const char *
_comac_get_locale_decimal_point (void)
{
    struct lconv *locale_data = localeconv ();
    return locale_data->decimal_point;
}

#else
/* Android's Bionic libc doesn't provide decimal_point */
const char *
_comac_get_locale_decimal_point (void)
{
    return ".";
}
#endif

#if defined(HAVE_NEWLOCALE) && defined(HAVE_STRTOD_L)

static locale_t C_locale;

static locale_t
get_C_locale (void)
{
    locale_t C;

retry:
    C = (locale_t) _comac_atomic_ptr_get ((void **) &C_locale);

    if (unlikely (! C)) {
	C = newlocale (LC_ALL_MASK, "C", NULL);

	if (! _comac_atomic_ptr_cmpxchg ((void **) &C_locale, NULL, C)) {
	    freelocale (C_locale);
	    goto retry;
	}
    }

    return C;
}

double
_comac_strtod (const char *nptr, char **endptr)
{
    return strtod_l (nptr, endptr, get_C_locale ());
}

#else

/* strtod replacement that ignores locale and only accepts decimal points */
double
_comac_strtod (const char *nptr, char **endptr)
{
    const char *decimal_point;
    int decimal_point_len;
    const char *p;
    char buf[100];
    char *bufptr;
    char *bufend = buf + sizeof (buf) - 1;
    double value;
    char *end;
    int delta;
    comac_bool_t have_dp;

    decimal_point = _comac_get_locale_decimal_point ();
    decimal_point_len = strlen (decimal_point);
    assert (decimal_point_len != 0);

    p = nptr;
    bufptr = buf;
    delta = 0;
    have_dp = FALSE;
    while (*p && _comac_isspace (*p)) {
	p++;
	delta++;
    }

    while (*p && (bufptr + decimal_point_len < bufend)) {
	if (_comac_isdigit (*p)) {
	    *bufptr++ = *p;
	} else if (*p == '.') {
	    if (have_dp)
		break;
	    strncpy (bufptr, decimal_point, decimal_point_len);
	    bufptr += decimal_point_len;
	    delta -= decimal_point_len - 1;
	    have_dp = TRUE;
	} else if (bufptr == buf && (*p == '-' || *p == '+')) {
	    *bufptr++ = *p;
	} else {
	    break;
	}
	p++;
    }
    *bufptr = 0;

    value = strtod (buf, &end);
    if (endptr) {
	if (end == buf)
	    *endptr = (char *) (nptr);
	else
	    *endptr = (char *) (nptr + (end - buf) + delta);
    }

    return value;
}
#endif

/**
 * _comac_fopen:
 * @filename: filename to open
 * @mode: mode string with which to open the file
 * @file_out: reference to file handle
 *
 * Exactly like the C library function, but possibly doing encoding
 * conversion on the filename. On all platforms, the filename is
 * passed directly to the system, but on Windows, the filename is
 * interpreted as UTF-8, rather than in a codepage that would depend
 * on system settings.
 *
 * Return value: COMAC_STATUS_SUCCESS when the filename was converted
 * successfully to the native encoding, or the error reported by
 * _comac_utf8_to_utf16 otherwise. To check if the file handle could
 * be obtained, dereference file_out and compare its value against
 * NULL
 **/
comac_status_t
_comac_fopen (const char *filename, const char *mode, FILE **file_out)
{
    FILE *result;
#ifdef _WIN32 /* also defined on x86_64 */
    uint16_t *filename_w;
    uint16_t *mode_w;
    comac_status_t status;

    *file_out = NULL;

    if (filename == NULL || mode == NULL) {
	errno = EINVAL;
	return COMAC_STATUS_SUCCESS;
    }

    if ((status = _comac_utf8_to_utf16 (filename, -1, &filename_w, NULL)) !=
	COMAC_STATUS_SUCCESS) {
	errno = EINVAL;
	return status;
    }

    if ((status = _comac_utf8_to_utf16 (mode, -1, &mode_w, NULL)) !=
	COMAC_STATUS_SUCCESS) {
	free (filename_w);
	errno = EINVAL;
	return status;
    }

    result = _wfopen (filename_w, mode_w);

    free (filename_w);
    free (mode_w);

#else /* Use fopen directly */
    result = fopen (filename, mode);
#endif

    *file_out = result;

    return COMAC_STATUS_SUCCESS;
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
/* We require Windows 2000 features such as ETO_PDY */
#if ! defined(WINVER) || (WINVER < 0x0500)
#define WINVER 0x0500
#endif
#if ! defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <io.h>

#if ! _WIN32_WCE
/* tmpfile() replacement for Windows.
 *
 * On Windows tmpfile() creates the file in the root directory. This
 * may fail due to insufficient privileges. However, this isn't a
 * problem on Windows CE so we don't use it there.
 */
FILE *
_comac_win32_tmpfile (void)
{
    DWORD path_len;
    WCHAR path_name[MAX_PATH + 1];
    WCHAR file_name[MAX_PATH + 1];
    HANDLE handle;
    int fd;
    FILE *fp;

    path_len = GetTempPathW (MAX_PATH, path_name);
    if (path_len <= 0 || path_len >= MAX_PATH)
	return NULL;

    if (GetTempFileNameW (path_name, L"ps_", 0, file_name) == 0)
	return NULL;

    handle = CreateFileW (file_name,
			  GENERIC_READ | GENERIC_WRITE,
			  0,
			  NULL,
			  CREATE_ALWAYS,
			  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
			  NULL);
    if (handle == INVALID_HANDLE_VALUE) {
	DeleteFileW (file_name);
	return NULL;
    }

    fd = _open_osfhandle ((intptr_t) handle, 0);
    if (fd < 0) {
	CloseHandle (handle);
	return NULL;
    }

    fp = fdopen (fd, "w+b");
    if (fp == NULL) {
	_close (fd);
	return NULL;
    }

    return fp;
}
#endif /* !_WIN32_WCE */

#endif /* _WIN32 */

typedef struct _comac_intern_string {
    comac_hash_entry_t hash_entry;
    int len;
    char *string;
} comac_intern_string_t;

static comac_hash_table_t *_comac_intern_string_ht;

unsigned long
_comac_string_hash (const char *str, int len)
{
    const signed char *p = (const signed char *) str;
    unsigned int h = *p;

    for (p += 1; len > 0; len--, p++)
	h = (h << 5) - h + *p;

    return h;
}

static comac_bool_t
_intern_string_equal (const void *_a, const void *_b)
{
    const comac_intern_string_t *a = _a;
    const comac_intern_string_t *b = _b;

    if (a->len != b->len)
	return FALSE;

    return memcmp (a->string, b->string, a->len) == 0;
}

comac_status_t
_comac_intern_string (const char **str_inout, int len)
{
    char *str = (char *) *str_inout;
    comac_intern_string_t tmpl, *istring;
    comac_status_t status = COMAC_STATUS_SUCCESS;

    if (COMAC_INJECT_FAULT ())
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    if (len < 0)
	len = strlen (str);
    tmpl.hash_entry.hash = _comac_string_hash (str, len);
    tmpl.len = len;
    tmpl.string = (char *) str;

    COMAC_MUTEX_LOCK (_comac_intern_string_mutex);
    if (_comac_intern_string_ht == NULL) {
	_comac_intern_string_ht =
	    _comac_hash_table_create (_intern_string_equal);
	if (unlikely (_comac_intern_string_ht == NULL)) {
	    status = _comac_error (COMAC_STATUS_NO_MEMORY);
	    goto BAIL;
	}
    }

    istring =
	_comac_hash_table_lookup (_comac_intern_string_ht, &tmpl.hash_entry);
    if (istring == NULL) {
	istring = _comac_malloc (sizeof (comac_intern_string_t) + len + 1);
	if (likely (istring != NULL)) {
	    istring->hash_entry.hash = tmpl.hash_entry.hash;
	    istring->len = tmpl.len;
	    istring->string = (char *) (istring + 1);
	    memcpy (istring->string, str, len);
	    istring->string[len] = '\0';

	    status = _comac_hash_table_insert (_comac_intern_string_ht,
					       &istring->hash_entry);
	    if (unlikely (status)) {
		free (istring);
		goto BAIL;
	    }
	} else {
	    status = _comac_error (COMAC_STATUS_NO_MEMORY);
	    goto BAIL;
	}
    }

    *str_inout = istring->string;

BAIL:
    COMAC_MUTEX_UNLOCK (_comac_intern_string_mutex);
    return status;
}

static void
_intern_string_pluck (void *entry, void *closure)
{
    _comac_hash_table_remove (closure, entry);
    free (entry);
}

void
_comac_intern_string_reset_static_data (void)
{
    COMAC_MUTEX_LOCK (_comac_intern_string_mutex);
    if (_comac_intern_string_ht != NULL) {
	_comac_hash_table_foreach (_comac_intern_string_ht,
				   _intern_string_pluck,
				   _comac_intern_string_ht);
	_comac_hash_table_destroy (_comac_intern_string_ht);
	_comac_intern_string_ht = NULL;
    }
    COMAC_MUTEX_UNLOCK (_comac_intern_string_mutex);
}
