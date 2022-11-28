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

#ifndef COMAC_H
#define COMAC_H

#include "comac-version.h"
#include "comac-features.h"
#include "comac-deprecated.h"
#include "comac-colormanagement.h"

#ifdef __cplusplus
#define COMAC_BEGIN_DECLS extern "C" {
#define COMAC_END_DECLS }
#else
#define COMAC_BEGIN_DECLS
#define COMAC_END_DECLS
#endif

#ifndef comac_public
#if defined(_MSC_VER) && ! defined(COMAC_WIN32_STATIC_BUILD)
#define comac_public __declspec (dllimport)
#else
#define comac_public
#endif
#endif

COMAC_BEGIN_DECLS

#define COMAC_VERSION_ENCODE(major, minor, micro)                              \
    (((major) *10000) + ((minor) *100) + ((micro) *1))

#define COMAC_VERSION                                                          \
    COMAC_VERSION_ENCODE (COMAC_VERSION_MAJOR,                                 \
			  COMAC_VERSION_MINOR,                                 \
			  COMAC_VERSION_MICRO)

#define COMAC_VERSION_STRINGIZE_(major, minor, micro)                          \
#major "." #minor "." #micro
#define COMAC_VERSION_STRINGIZE(major, minor, micro)                           \
    COMAC_VERSION_STRINGIZE_ (major, minor, micro)

#define COMAC_VERSION_STRING                                                   \
    COMAC_VERSION_STRINGIZE (COMAC_VERSION_MAJOR,                              \
			     COMAC_VERSION_MINOR,                              \
			     COMAC_VERSION_MICRO)

comac_public int
comac_version (void);

comac_public const char *
comac_version_string (void);

/**
 * comac_bool_t:
 *
 * #comac_bool_t is used for boolean values. Returns of type
 * #comac_bool_t will always be either 0 or 1, but testing against
 * these values explicitly is not encouraged; just use the
 * value as a boolean condition.
 *
 * <informalexample><programlisting>
 *  if (comac_in_stroke (cr, x, y)) {
 *      /<!-- -->* do something *<!-- -->/
 *  }
 * </programlisting></informalexample>
 *
 * Since: 1.0
 **/
typedef int comac_bool_t;

/**
 * comac_t:
 *
 * A #comac_t contains the current state of the rendering device,
 * including coordinates of yet to be drawn shapes.
 *
 * Comac contexts, as #comac_t objects are named, are central to
 * comac and all drawing with comac is always done to a #comac_t
 * object.
 *
 * Memory management of #comac_t is done with
 * comac_reference() and comac_destroy().
 *
 * Since: 1.0
 **/
typedef struct _comac comac_t;

/**
 * comac_surface_t:
 *
 * A #comac_surface_t represents an image, either as the destination
 * of a drawing operation or as source when drawing onto another
 * surface.  To draw to a #comac_surface_t, create a comac context
 * with the surface as the target, using comac_create().
 *
 * There are different subtypes of #comac_surface_t for
 * different drawing backends; for example, comac_image_surface_create()
 * creates a bitmap image in memory.
 * The type of a surface can be queried with comac_surface_get_type().
 *
 * The initial contents of a surface after creation depend upon the manner
 * of its creation. If comac creates the surface and backing storage for
 * the user, it will be initially cleared; for example,
 * comac_image_surface_create() and comac_surface_create_similar().
 * Alternatively, if the user passes in a reference to some backing storage
 * and asks comac to wrap that in a #comac_surface_t, then the contents are
 * not modified; for example, comac_image_surface_create_for_data() and
 * comac_xlib_surface_create().
 *
 * Memory management of #comac_surface_t is done with
 * comac_surface_reference() and comac_surface_destroy().
 *
 * Since: 1.0
 **/
typedef struct _comac_surface comac_surface_t;

/**
 * comac_device_t:
 *
 * A #comac_device_t represents the driver interface for drawing
 * operations to a #comac_surface_t.  There are different subtypes of
 * #comac_device_t for different drawing backends; for example,
 * comac_egl_device_create() creates a device that wraps an EGL display and
 * context.
 *
 * The type of a device can be queried with comac_device_get_type().
 *
 * Memory management of #comac_device_t is done with
 * comac_device_reference() and comac_device_destroy().
 *
 * Since: 1.10
 **/
typedef struct _comac_device comac_device_t;

/**
 * comac_matrix_t:
 * @xx: xx component of the affine transformation
 * @yx: yx component of the affine transformation
 * @xy: xy component of the affine transformation
 * @yy: yy component of the affine transformation
 * @x0: X translation component of the affine transformation
 * @y0: Y translation component of the affine transformation
 *
 * A #comac_matrix_t holds an affine transformation, such as a scale,
 * rotation, shear, or a combination of those. The transformation of
 * a point (x, y) is given by:
 * <programlisting>
 *     x_new = xx * x + xy * y + x0;
 *     y_new = yx * x + yy * y + y0;
 * </programlisting>
 *
 * Since: 1.0
 **/
typedef struct _comac_matrix {
    double xx;
    double yx;
    double xy;
    double yy;
    double x0;
    double y0;
} comac_matrix_t;

/**
 * comac_pattern_t:
 *
 * A #comac_pattern_t represents a source when drawing onto a
 * surface. There are different subtypes of #comac_pattern_t,
 * for different types of sources; for example,
 * comac_pattern_create_rgb() creates a pattern for a solid
 * opaque color.
 *
 * Other than various
 * <function>comac_pattern_create_<emphasis>type</emphasis>()</function>
 * functions, some of the pattern types can be implicitly created using various
 * <function>comac_set_source_<emphasis>type</emphasis>()</function> functions;
 * for example comac_set_source_rgb().
 *
 * The type of a pattern can be queried with comac_pattern_get_type().
 *
 * Memory management of #comac_pattern_t is done with
 * comac_pattern_reference() and comac_pattern_destroy().
 *
 * Since: 1.0
 **/
typedef struct _comac_pattern comac_pattern_t;

/**
 * comac_destroy_func_t:
 * @data: The data element being destroyed.
 *
 * #comac_destroy_func_t the type of function which is called when a
 * data element is destroyed. It is passed the pointer to the data
 * element and should free any memory and resources allocated for it.
 *
 * Since: 1.0
 **/
typedef void (*comac_destroy_func_t) (void *data);

/**
 * comac_user_data_key_t:
 * @unused: not used; ignore.
 *
 * #comac_user_data_key_t is used for attaching user data to comac
 * data structures.  The actual contents of the struct is never used,
 * and there is no need to initialize the object; only the unique
 * address of a #comac_data_key_t object is used.  Typically, you
 * would just use the address of a static #comac_data_key_t object.
 *
 * Since: 1.0
 **/
typedef struct _comac_user_data_key {
    int unused;
} comac_user_data_key_t;

/**
 * comac_status_t:
 * @COMAC_STATUS_SUCCESS: no error has occurred (Since 1.0)
 * @COMAC_STATUS_NO_MEMORY: out of memory (Since 1.0)
 * @COMAC_STATUS_INVALID_RESTORE: comac_restore() called without matching comac_save() (Since 1.0)
 * @COMAC_STATUS_INVALID_POP_GROUP: no saved group to pop, i.e. comac_pop_group() without matching comac_push_group() (Since 1.0)
 * @COMAC_STATUS_NO_CURRENT_POINT: no current point defined (Since 1.0)
 * @COMAC_STATUS_INVALID_MATRIX: invalid matrix (not invertible) (Since 1.0)
 * @COMAC_STATUS_INVALID_STATUS: invalid value for an input #comac_status_t (Since 1.0)
 * @COMAC_STATUS_NULL_POINTER: %NULL pointer (Since 1.0)
 * @COMAC_STATUS_INVALID_STRING: input string not valid UTF-8 (Since 1.0)
 * @COMAC_STATUS_INVALID_PATH_DATA: input path data not valid (Since 1.0)
 * @COMAC_STATUS_READ_ERROR: error while reading from input stream (Since 1.0)
 * @COMAC_STATUS_WRITE_ERROR: error while writing to output stream (Since 1.0)
 * @COMAC_STATUS_SURFACE_FINISHED: target surface has been finished (Since 1.0)
 * @COMAC_STATUS_SURFACE_TYPE_MISMATCH: the surface type is not appropriate for the operation (Since 1.0)
 * @COMAC_STATUS_PATTERN_TYPE_MISMATCH: the pattern type is not appropriate for the operation (Since 1.0)
 * @COMAC_STATUS_INVALID_CONTENT: invalid value for an input #comac_content_t (Since 1.0)
 * @COMAC_STATUS_INVALID_FORMAT: invalid value for an input #comac_format_t (Since 1.0)
 * @COMAC_STATUS_INVALID_VISUAL: invalid value for an input Visual* (Since 1.0)
 * @COMAC_STATUS_FILE_NOT_FOUND: file not found (Since 1.0)
 * @COMAC_STATUS_INVALID_DASH: invalid value for a dash setting (Since 1.0)
 * @COMAC_STATUS_INVALID_DSC_COMMENT: invalid value for a DSC comment (Since 1.2)
 * @COMAC_STATUS_INVALID_INDEX: invalid index passed to getter (Since 1.4)
 * @COMAC_STATUS_CLIP_NOT_REPRESENTABLE: clip region not representable in desired format (Since 1.4)
 * @COMAC_STATUS_TEMP_FILE_ERROR: error creating or writing to a temporary file (Since 1.6)
 * @COMAC_STATUS_INVALID_STRIDE: invalid value for stride (Since 1.6)
 * @COMAC_STATUS_FONT_TYPE_MISMATCH: the font type is not appropriate for the operation (Since 1.8)
 * @COMAC_STATUS_USER_FONT_IMMUTABLE: the user-font is immutable (Since 1.8)
 * @COMAC_STATUS_USER_FONT_ERROR: error occurred in a user-font callback function (Since 1.8)
 * @COMAC_STATUS_NEGATIVE_COUNT: negative number used where it is not allowed (Since 1.8)
 * @COMAC_STATUS_INVALID_CLUSTERS: input clusters do not represent the accompanying text and glyph array (Since 1.8)
 * @COMAC_STATUS_INVALID_SLANT: invalid value for an input #comac_font_slant_t (Since 1.8)
 * @COMAC_STATUS_INVALID_WEIGHT: invalid value for an input #comac_font_weight_t (Since 1.8)
 * @COMAC_STATUS_INVALID_SIZE: invalid value (typically too big) for the size of the input (surface, pattern, etc.) (Since 1.10)
 * @COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED: user-font method not implemented (Since 1.10)
 * @COMAC_STATUS_DEVICE_TYPE_MISMATCH: the device type is not appropriate for the operation (Since 1.10)
 * @COMAC_STATUS_DEVICE_ERROR: an operation to the device caused an unspecified error (Since 1.10)
 * @COMAC_STATUS_INVALID_MESH_CONSTRUCTION: a mesh pattern
 *   construction operation was used outside of a
 *   comac_mesh_pattern_begin_patch()/comac_mesh_pattern_end_patch()
 *   pair (Since 1.12)
 * @COMAC_STATUS_DEVICE_FINISHED: target device has been finished (Since 1.12)
 * @COMAC_STATUS_JBIG2_GLOBAL_MISSING: %COMAC_MIME_TYPE_JBIG2_GLOBAL_ID has been used on at least one image
 *   but no image provided %COMAC_MIME_TYPE_JBIG2_GLOBAL (Since 1.14)
 * @COMAC_STATUS_PNG_ERROR: error occurred in libpng while reading from or writing to a PNG file (Since 1.16)
 * @COMAC_STATUS_FREETYPE_ERROR: error occurred in libfreetype (Since 1.16)
 * @COMAC_STATUS_WIN32_GDI_ERROR: error occurred in the Windows Graphics Device Interface (Since 1.16)
 * @COMAC_STATUS_TAG_ERROR: invalid tag name, attributes, or nesting (Since 1.16)
 * @COMAC_STATUS_DWRITE_ERROR: error occurred in the Windows Direct Write API (Since 1.18)
 * @COMAC_STATUS_LAST_STATUS: this is a special value indicating the number of
 *   status values defined in this enumeration.  When using this value, note
 *   that the version of comac at run-time may have additional status values
 *   defined than the value of this symbol at compile-time. (Since 1.10)
 *
 * #comac_status_t is used to indicate errors that can occur when
 * using Comac. In some cases it is returned directly by functions.
 * but when using #comac_t, the last error, if any, is stored in
 * the context and can be retrieved with comac_status().
 *
 * New entries may be added in future versions.  Use comac_status_to_string()
 * to get a human-readable representation of an error message.
 *
 * Since: 1.0
 **/
typedef enum _comac_status {
    COMAC_STATUS_SUCCESS = 0,

    COMAC_STATUS_NO_MEMORY,
    COMAC_STATUS_INVALID_RESTORE,
    COMAC_STATUS_INVALID_POP_GROUP,
    COMAC_STATUS_NO_CURRENT_POINT,
    COMAC_STATUS_INVALID_MATRIX,
    COMAC_STATUS_INVALID_STATUS,
    COMAC_STATUS_NULL_POINTER,
    COMAC_STATUS_INVALID_STRING,
    COMAC_STATUS_INVALID_PATH_DATA,
    COMAC_STATUS_READ_ERROR,
    COMAC_STATUS_WRITE_ERROR,
    COMAC_STATUS_SURFACE_FINISHED,
    COMAC_STATUS_SURFACE_TYPE_MISMATCH,
    COMAC_STATUS_PATTERN_TYPE_MISMATCH,
    COMAC_STATUS_INVALID_CONTENT,
    COMAC_STATUS_INVALID_FORMAT,
    COMAC_STATUS_INVALID_VISUAL,
    COMAC_STATUS_FILE_NOT_FOUND,
    COMAC_STATUS_INVALID_DASH,
    COMAC_STATUS_INVALID_DSC_COMMENT,
    COMAC_STATUS_INVALID_INDEX,
    COMAC_STATUS_CLIP_NOT_REPRESENTABLE,
    COMAC_STATUS_TEMP_FILE_ERROR,
    COMAC_STATUS_INVALID_STRIDE,
    COMAC_STATUS_FONT_TYPE_MISMATCH,
    COMAC_STATUS_USER_FONT_IMMUTABLE,
    COMAC_STATUS_USER_FONT_ERROR,
    COMAC_STATUS_NEGATIVE_COUNT,
    COMAC_STATUS_INVALID_CLUSTERS,
    COMAC_STATUS_INVALID_SLANT,
    COMAC_STATUS_INVALID_WEIGHT,
    COMAC_STATUS_INVALID_SIZE,
    COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED,
    COMAC_STATUS_DEVICE_TYPE_MISMATCH,
    COMAC_STATUS_DEVICE_ERROR,
    COMAC_STATUS_INVALID_MESH_CONSTRUCTION,
    COMAC_STATUS_DEVICE_FINISHED,
    COMAC_STATUS_JBIG2_GLOBAL_MISSING,
    COMAC_STATUS_PNG_ERROR,
    COMAC_STATUS_FREETYPE_ERROR,
    COMAC_STATUS_WIN32_GDI_ERROR,
    COMAC_STATUS_TAG_ERROR,
    COMAC_STATUS_DWRITE_ERROR,

    COMAC_STATUS_LAST_STATUS
} comac_status_t;

/**
 * comac_content_t:
 * @COMAC_CONTENT_COLOR: The surface will hold color content only. (Since 1.0)
 * @COMAC_CONTENT_ALPHA: The surface will hold alpha content only. (Since 1.0)
 * @COMAC_CONTENT_COLOR_ALPHA: The surface will hold color and alpha content. (Since 1.0)
 *
 * #comac_content_t is used to describe the content that a surface will
 * contain, whether color information, alpha information (translucence
 * vs. opacity), or both.
 *
 * Note: The large values here are designed to keep #comac_content_t
 * values distinct from #comac_format_t values so that the
 * implementation can detect the error if users confuse the two types.
 *
 * Since: 1.0
 **/
typedef enum _comac_content {
    COMAC_CONTENT_COLOR = 0x1000,
    COMAC_CONTENT_ALPHA = 0x2000,
    COMAC_CONTENT_COLOR_ALPHA = 0x3000
} comac_content_t;

/**
 * comac_format_t:
 * @COMAC_FORMAT_INVALID: no such format exists or is supported.
 * @COMAC_FORMAT_ARGB32: each pixel is a 32-bit quantity, with
 *   alpha in the upper 8 bits, then red, then green, then blue.
 *   The 32-bit quantities are stored native-endian. Pre-multiplied
 *   alpha is used. (That is, 50% transparent red is 0x80800000,
 *   not 0x80ff0000.) (Since 1.0)
 * @COMAC_FORMAT_RGB24: each pixel is a 32-bit quantity, with
 *   the upper 8 bits unused. Red, Green, and Blue are stored
 *   in the remaining 24 bits in that order. (Since 1.0)
 * @COMAC_FORMAT_A8: each pixel is a 8-bit quantity holding
 *   an alpha value. (Since 1.0)
 * @COMAC_FORMAT_A1: each pixel is a 1-bit quantity holding
 *   an alpha value. Pixels are packed together into 32-bit
 *   quantities. The ordering of the bits matches the
 *   endianness of the platform. On a big-endian machine, the
 *   first pixel is in the uppermost bit, on a little-endian
 *   machine the first pixel is in the least-significant bit. (Since 1.0)
 * @COMAC_FORMAT_RGB16_565: each pixel is a 16-bit quantity
 *   with red in the upper 5 bits, then green in the middle
 *   6 bits, and blue in the lower 5 bits. (Since 1.2)
 * @COMAC_FORMAT_RGB30: like RGB24 but with 10bpc. (Since 1.12)
 * @COMAC_FORMAT_RGB96F: 3 floats, R, G, B. (Since 1.17.2)
 * @COMAC_FORMAT_RGBA128F: 4 floats, R, G, B, A. (Since 1.17.2)
 *
 * #comac_format_t is used to identify the memory format of
 * image data.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.0
 **/
typedef enum _comac_format {
    COMAC_FORMAT_INVALID = -1,
    COMAC_FORMAT_ARGB32 = 0,
    COMAC_FORMAT_RGB24 = 1,
    COMAC_FORMAT_A8 = 2,
    COMAC_FORMAT_A1 = 3,
    COMAC_FORMAT_RGB16_565 = 4,
    COMAC_FORMAT_RGB30 = 5,
    COMAC_FORMAT_RGB96F = 6,
    COMAC_FORMAT_RGBA128F = 7
} comac_format_t;

/**
 * comac_write_func_t:
 * @closure: the output closure
 * @data: the buffer containing the data to write
 * @length: the amount of data to write
 *
 * #comac_write_func_t is the type of function which is called when a
 * backend needs to write data to an output stream.  It is passed the
 * closure which was specified by the user at the time the write
 * function was registered, the data to write and the length of the
 * data in bytes.  The write function should return
 * %COMAC_STATUS_SUCCESS if all the data was successfully written,
 * %COMAC_STATUS_WRITE_ERROR otherwise.
 *
 * Returns: the status code of the write operation
 *
 * Since: 1.0
 **/
typedef comac_status_t (*comac_write_func_t) (void *closure,
					      const unsigned char *data,
					      unsigned int length);

/**
 * comac_read_func_t:
 * @closure: the input closure
 * @data: the buffer into which to read the data
 * @length: the amount of data to read
 *
 * #comac_read_func_t is the type of function which is called when a
 * backend needs to read data from an input stream.  It is passed the
 * closure which was specified by the user at the time the read
 * function was registered, the buffer to read the data into and the
 * length of the data in bytes.  The read function should return
 * %COMAC_STATUS_SUCCESS if all the data was successfully read,
 * %COMAC_STATUS_READ_ERROR otherwise.
 *
 * Returns: the status code of the read operation
 *
 * Since: 1.0
 **/
typedef comac_status_t (*comac_read_func_t) (void *closure,
					     unsigned char *data,
					     unsigned int length);

/**
 * comac_rectangle_int_t:
 * @x: X coordinate of the left side of the rectangle
 * @y: Y coordinate of the the top side of the rectangle
 * @width: width of the rectangle
 * @height: height of the rectangle
 *
 * A data structure for holding a rectangle with integer coordinates.
 *
 * Since: 1.10
 **/

typedef struct _comac_rectangle_int {
    int x, y;
    int width, height;
} comac_rectangle_int_t;

/* Functions for manipulating state objects */
comac_public comac_t *
comac_create (comac_surface_t *target);

comac_public comac_t *
comac_reference (comac_t *cr);

comac_public void
comac_destroy (comac_t *cr);

comac_public unsigned int
comac_get_reference_count (comac_t *cr);

comac_public void *
comac_get_user_data (comac_t *cr, const comac_user_data_key_t *key);

comac_public comac_status_t
comac_set_user_data (comac_t *cr,
		     const comac_user_data_key_t *key,
		     void *user_data,
		     comac_destroy_func_t destroy);

comac_public void
comac_save (comac_t *cr);

comac_public void
comac_restore (comac_t *cr);

comac_public void
comac_push_group (comac_t *cr);

comac_public void
comac_push_group_with_content (comac_t *cr, comac_content_t content);

comac_public comac_pattern_t *
comac_pop_group (comac_t *cr);

comac_public void
comac_pop_group_to_source (comac_t *cr);

/* Modify state */

/**
 * comac_operator_t:
 * @COMAC_OPERATOR_CLEAR: clear destination layer (bounded) (Since 1.0)
 * @COMAC_OPERATOR_SOURCE: replace destination layer (bounded) (Since 1.0)
 * @COMAC_OPERATOR_OVER: draw source layer on top of destination layer
 * (bounded) (Since 1.0)
 * @COMAC_OPERATOR_IN: draw source where there was destination content
 * (unbounded) (Since 1.0)
 * @COMAC_OPERATOR_OUT: draw source where there was no destination
 * content (unbounded) (Since 1.0)
 * @COMAC_OPERATOR_ATOP: draw source on top of destination content and
 * only there (Since 1.0)
 * @COMAC_OPERATOR_DEST: ignore the source (Since 1.0)
 * @COMAC_OPERATOR_DEST_OVER: draw destination on top of source (Since 1.0)
 * @COMAC_OPERATOR_DEST_IN: leave destination only where there was
 * source content (unbounded) (Since 1.0)
 * @COMAC_OPERATOR_DEST_OUT: leave destination only where there was no
 * source content (Since 1.0)
 * @COMAC_OPERATOR_DEST_ATOP: leave destination on top of source content
 * and only there (unbounded) (Since 1.0)
 * @COMAC_OPERATOR_XOR: source and destination are shown where there is only
 * one of them (Since 1.0)
 * @COMAC_OPERATOR_ADD: source and destination layers are accumulated (Since 1.0)
 * @COMAC_OPERATOR_SATURATE: like over, but assuming source and dest are
 * disjoint geometries (Since 1.0)
 * @COMAC_OPERATOR_MULTIPLY: source and destination layers are multiplied.
 * This causes the result to be at least as dark as the darker inputs. (Since 1.10)
 * @COMAC_OPERATOR_SCREEN: source and destination are complemented and
 * multiplied. This causes the result to be at least as light as the lighter
 * inputs. (Since 1.10)
 * @COMAC_OPERATOR_OVERLAY: multiplies or screens, depending on the
 * lightness of the destination color. (Since 1.10)
 * @COMAC_OPERATOR_DARKEN: replaces the destination with the source if it
 * is darker, otherwise keeps the source. (Since 1.10)
 * @COMAC_OPERATOR_LIGHTEN: replaces the destination with the source if it
 * is lighter, otherwise keeps the source. (Since 1.10)
 * @COMAC_OPERATOR_COLOR_DODGE: brightens the destination color to reflect
 * the source color. (Since 1.10)
 * @COMAC_OPERATOR_COLOR_BURN: darkens the destination color to reflect
 * the source color. (Since 1.10)
 * @COMAC_OPERATOR_HARD_LIGHT: Multiplies or screens, dependent on source
 * color. (Since 1.10)
 * @COMAC_OPERATOR_SOFT_LIGHT: Darkens or lightens, dependent on source
 * color. (Since 1.10)
 * @COMAC_OPERATOR_DIFFERENCE: Takes the difference of the source and
 * destination color. (Since 1.10)
 * @COMAC_OPERATOR_EXCLUSION: Produces an effect similar to difference, but
 * with lower contrast. (Since 1.10)
 * @COMAC_OPERATOR_HSL_HUE: Creates a color with the hue of the source
 * and the saturation and luminosity of the target. (Since 1.10)
 * @COMAC_OPERATOR_HSL_SATURATION: Creates a color with the saturation
 * of the source and the hue and luminosity of the target. Painting with
 * this mode onto a gray area produces no change. (Since 1.10)
 * @COMAC_OPERATOR_HSL_COLOR: Creates a color with the hue and saturation
 * of the source and the luminosity of the target. This preserves the gray
 * levels of the target and is useful for coloring monochrome images or
 * tinting color images. (Since 1.10)
 * @COMAC_OPERATOR_HSL_LUMINOSITY: Creates a color with the luminosity of
 * the source and the hue and saturation of the target. This produces an
 * inverse effect to @COMAC_OPERATOR_HSL_COLOR. (Since 1.10)
 *
 * #comac_operator_t is used to set the compositing operator for all comac
 * drawing operations.
 *
 * The default operator is %COMAC_OPERATOR_OVER.
 *
 * The operators marked as <firstterm>unbounded</firstterm> modify their
 * destination even outside of the mask layer (that is, their effect is not
 * bound by the mask layer).  However, their effect can still be limited by
 * way of clipping.
 *
 * To keep things simple, the operator descriptions here
 * document the behavior for when both source and destination are either fully
 * transparent or fully opaque.  The actual implementation works for
 * translucent layers too.
 * For a more detailed explanation of the effects of each operator, including
 * the mathematical definitions, see
 * <ulink url="https://comacgraphics.org/operators/">https://comacgraphics.org/operators/</ulink>.
 *
 * Since: 1.0
 **/
typedef enum _comac_operator {
    COMAC_OPERATOR_CLEAR,

    COMAC_OPERATOR_SOURCE,
    COMAC_OPERATOR_OVER,
    COMAC_OPERATOR_IN,
    COMAC_OPERATOR_OUT,
    COMAC_OPERATOR_ATOP,

    COMAC_OPERATOR_DEST,
    COMAC_OPERATOR_DEST_OVER,
    COMAC_OPERATOR_DEST_IN,
    COMAC_OPERATOR_DEST_OUT,
    COMAC_OPERATOR_DEST_ATOP,

    COMAC_OPERATOR_XOR,
    COMAC_OPERATOR_ADD,
    COMAC_OPERATOR_SATURATE,

    COMAC_OPERATOR_MULTIPLY,
    COMAC_OPERATOR_SCREEN,
    COMAC_OPERATOR_OVERLAY,
    COMAC_OPERATOR_DARKEN,
    COMAC_OPERATOR_LIGHTEN,
    COMAC_OPERATOR_COLOR_DODGE,
    COMAC_OPERATOR_COLOR_BURN,
    COMAC_OPERATOR_HARD_LIGHT,
    COMAC_OPERATOR_SOFT_LIGHT,
    COMAC_OPERATOR_DIFFERENCE,
    COMAC_OPERATOR_EXCLUSION,
    COMAC_OPERATOR_HSL_HUE,
    COMAC_OPERATOR_HSL_SATURATION,
    COMAC_OPERATOR_HSL_COLOR,
    COMAC_OPERATOR_HSL_LUMINOSITY
} comac_operator_t;

comac_public void
comac_set_operator (comac_t *cr, comac_operator_t op);

comac_public void
comac_set_source (comac_t *cr, comac_pattern_t *source);

comac_public void
comac_set_source_rgb (comac_t *cr, double red, double green, double blue);

comac_public void
comac_set_source_gray (comac_t *cr, double graylevel);

comac_public void
comac_set_source_rgba (
    comac_t *cr, double red, double green, double blue, double alpha);

comac_public void
comac_set_source_surface (comac_t *cr,
			  comac_surface_t *surface,
			  double x,
			  double y);

comac_public void
comac_set_tolerance (comac_t *cr, double tolerance);

/**
 * comac_antialias_t:
 * @COMAC_ANTIALIAS_DEFAULT: Use the default antialiasing for
 *   the subsystem and target device, since 1.0
 * @COMAC_ANTIALIAS_NONE: Use a bilevel alpha mask, since 1.0
 * @COMAC_ANTIALIAS_GRAY: Perform single-color antialiasing (using
 *  shades of gray for black text on a white background, for example), since 1.0
 * @COMAC_ANTIALIAS_SUBPIXEL: Perform antialiasing by taking
 *  advantage of the order of subpixel elements on devices
 *  such as LCD panels, since 1.0
 * @COMAC_ANTIALIAS_FAST: Hint that the backend should perform some
 * antialiasing but prefer speed over quality, since 1.12
 * @COMAC_ANTIALIAS_GOOD: The backend should balance quality against
 * performance, since 1.12
 * @COMAC_ANTIALIAS_BEST: Hint that the backend should render at the highest
 * quality, sacrificing speed if necessary, since 1.12
 *
 * Specifies the type of antialiasing to do when rendering text or shapes.
 *
 * As it is not necessarily clear from the above what advantages a particular
 * antialias method provides, since 1.12, there is also a set of hints:
 * @COMAC_ANTIALIAS_FAST: Allow the backend to degrade raster quality for speed
 * @COMAC_ANTIALIAS_GOOD: A balance between speed and quality
 * @COMAC_ANTIALIAS_BEST: A high-fidelity, but potentially slow, raster mode
 *
 * These make no guarantee on how the backend will perform its rasterisation
 * (if it even rasterises!), nor that they have any differing effect other
 * than to enable some form of antialiasing. In the case of glyph rendering,
 * @COMAC_ANTIALIAS_FAST and @COMAC_ANTIALIAS_GOOD will be mapped to
 * @COMAC_ANTIALIAS_GRAY, with @COMAC_ANTALIAS_BEST being equivalent to
 * @COMAC_ANTIALIAS_SUBPIXEL.
 *
 * The interpretation of @COMAC_ANTIALIAS_DEFAULT is left entirely up to
 * the backend, typically this will be similar to @COMAC_ANTIALIAS_GOOD.
 *
 * Since: 1.0
 **/
typedef enum _comac_antialias {
    COMAC_ANTIALIAS_DEFAULT,

    /* method */
    COMAC_ANTIALIAS_NONE,
    COMAC_ANTIALIAS_GRAY,
    COMAC_ANTIALIAS_SUBPIXEL,

    /* hints */
    COMAC_ANTIALIAS_FAST,
    COMAC_ANTIALIAS_GOOD,
    COMAC_ANTIALIAS_BEST
} comac_antialias_t;

comac_public void
comac_set_antialias (comac_t *cr, comac_antialias_t antialias);

/**
 * comac_fill_rule_t:
 * @COMAC_FILL_RULE_WINDING: If the path crosses the ray from
 * left-to-right, counts +1. If the path crosses the ray
 * from right to left, counts -1. (Left and right are determined
 * from the perspective of looking along the ray from the starting
 * point.) If the total count is non-zero, the point will be filled. (Since 1.0)
 * @COMAC_FILL_RULE_EVEN_ODD: Counts the total number of
 * intersections, without regard to the orientation of the contour. If
 * the total number of intersections is odd, the point will be
 * filled. (Since 1.0)
 *
 * #comac_fill_rule_t is used to select how paths are filled. For both
 * fill rules, whether or not a point is included in the fill is
 * determined by taking a ray from that point to infinity and looking
 * at intersections with the path. The ray can be in any direction,
 * as long as it doesn't pass through the end point of a segment
 * or have a tricky intersection such as intersecting tangent to the path.
 * (Note that filling is not actually implemented in this way. This
 * is just a description of the rule that is applied.)
 *
 * The default fill rule is %COMAC_FILL_RULE_WINDING.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.0
 **/
typedef enum _comac_fill_rule {
    COMAC_FILL_RULE_WINDING,
    COMAC_FILL_RULE_EVEN_ODD
} comac_fill_rule_t;

comac_public void
comac_set_fill_rule (comac_t *cr, comac_fill_rule_t fill_rule);

comac_public void
comac_set_line_width (comac_t *cr, double width);

comac_public void
comac_set_hairline (comac_t *cr, comac_bool_t set_hairline);

/**
 * comac_line_cap_t:
 * @COMAC_LINE_CAP_BUTT: start(stop) the line exactly at the start(end) point (Since 1.0)
 * @COMAC_LINE_CAP_ROUND: use a round ending, the center of the circle is the end point (Since 1.0)
 * @COMAC_LINE_CAP_SQUARE: use squared ending, the center of the square is the end point (Since 1.0)
 *
 * Specifies how to render the endpoints of the path when stroking.
 *
 * The default line cap style is %COMAC_LINE_CAP_BUTT.
 *
 * Since: 1.0
 **/
typedef enum _comac_line_cap {
    COMAC_LINE_CAP_BUTT,
    COMAC_LINE_CAP_ROUND,
    COMAC_LINE_CAP_SQUARE
} comac_line_cap_t;

comac_public void
comac_set_line_cap (comac_t *cr, comac_line_cap_t line_cap);

/**
 * comac_line_join_t:
 * @COMAC_LINE_JOIN_MITER: use a sharp (angled) corner, see
 * comac_set_miter_limit() (Since 1.0)
 * @COMAC_LINE_JOIN_ROUND: use a rounded join, the center of the circle is the
 * joint point (Since 1.0)
 * @COMAC_LINE_JOIN_BEVEL: use a cut-off join, the join is cut off at half
 * the line width from the joint point (Since 1.0)
 *
 * Specifies how to render the junction of two lines when stroking.
 *
 * The default line join style is %COMAC_LINE_JOIN_MITER.
 *
 * Since: 1.0
 **/
typedef enum _comac_line_join {
    COMAC_LINE_JOIN_MITER,
    COMAC_LINE_JOIN_ROUND,
    COMAC_LINE_JOIN_BEVEL
} comac_line_join_t;

comac_public void
comac_set_line_join (comac_t *cr, comac_line_join_t line_join);

comac_public void
comac_set_dash (comac_t *cr,
		const double *dashes,
		int num_dashes,
		double offset);

comac_public void
comac_set_miter_limit (comac_t *cr, double limit);

comac_public void
comac_translate (comac_t *cr, double tx, double ty);

comac_public void
comac_scale (comac_t *cr, double sx, double sy);

comac_public void
comac_rotate (comac_t *cr, double angle);

comac_public void
comac_transform (comac_t *cr, const comac_matrix_t *matrix);

comac_public void
comac_set_matrix (comac_t *cr, const comac_matrix_t *matrix);

comac_public void
comac_identity_matrix (comac_t *cr);

comac_public void
comac_user_to_device (comac_t *cr, double *x, double *y);

comac_public void
comac_user_to_device_distance (comac_t *cr, double *dx, double *dy);

comac_public void
comac_device_to_user (comac_t *cr, double *x, double *y);

comac_public void
comac_device_to_user_distance (comac_t *cr, double *dx, double *dy);

/* Path creation functions */
comac_public void
comac_new_path (comac_t *cr);

comac_public void
comac_move_to (comac_t *cr, double x, double y);

comac_public void
comac_new_sub_path (comac_t *cr);

comac_public void
comac_line_to (comac_t *cr, double x, double y);

comac_public void
comac_curve_to (comac_t *cr,
		double x1,
		double y1,
		double x2,
		double y2,
		double x3,
		double y3);

comac_public void
comac_arc (comac_t *cr,
	   double xc,
	   double yc,
	   double radius,
	   double angle1,
	   double angle2);

comac_public void
comac_arc_negative (comac_t *cr,
		    double xc,
		    double yc,
		    double radius,
		    double angle1,
		    double angle2);

/* XXX: NYI
comac_public void
comac_arc_to (comac_t *cr,
	      double x1, double y1,
	      double x2, double y2,
	      double radius);
*/

comac_public void
comac_rel_move_to (comac_t *cr, double dx, double dy);

comac_public void
comac_rel_line_to (comac_t *cr, double dx, double dy);

comac_public void
comac_rel_curve_to (comac_t *cr,
		    double dx1,
		    double dy1,
		    double dx2,
		    double dy2,
		    double dx3,
		    double dy3);

comac_public void
comac_rectangle (comac_t *cr, double x, double y, double width, double height);

/* XXX: NYI
comac_public void
comac_stroke_to_path (comac_t *cr);
*/

comac_public void
comac_close_path (comac_t *cr);

comac_public void
comac_path_extents (
    comac_t *cr, double *x1, double *y1, double *x2, double *y2);

/* Painting functions */
comac_public void
comac_paint (comac_t *cr);

comac_public void
comac_paint_with_alpha (comac_t *cr, double alpha);

comac_public void
comac_mask (comac_t *cr, comac_pattern_t *pattern);

comac_public void
comac_mask_surface (comac_t *cr,
		    comac_surface_t *surface,
		    double surface_x,
		    double surface_y);

comac_public void
comac_stroke (comac_t *cr);

comac_public void
comac_stroke_preserve (comac_t *cr);

comac_public void
comac_fill (comac_t *cr);

comac_public void
comac_fill_preserve (comac_t *cr);

comac_public void
comac_copy_page (comac_t *cr);

comac_public void
comac_show_page (comac_t *cr);

/* Insideness testing */
comac_public comac_bool_t
comac_in_stroke (comac_t *cr, double x, double y);

comac_public comac_bool_t
comac_in_fill (comac_t *cr, double x, double y);

comac_public comac_bool_t
comac_in_clip (comac_t *cr, double x, double y);

/* Rectangular extents */
comac_public void
comac_stroke_extents (
    comac_t *cr, double *x1, double *y1, double *x2, double *y2);

comac_public void
comac_fill_extents (
    comac_t *cr, double *x1, double *y1, double *x2, double *y2);

/* Clipping */
comac_public void
comac_reset_clip (comac_t *cr);

comac_public void
comac_clip (comac_t *cr);

comac_public void
comac_clip_preserve (comac_t *cr);

comac_public void
comac_clip_extents (
    comac_t *cr, double *x1, double *y1, double *x2, double *y2);

/**
 * comac_rectangle_t:
 * @x: X coordinate of the left side of the rectangle
 * @y: Y coordinate of the the top side of the rectangle
 * @width: width of the rectangle
 * @height: height of the rectangle
 *
 * A data structure for holding a rectangle.
 *
 * Since: 1.4
 **/
typedef struct _comac_rectangle {
    double x, y, width, height;
} comac_rectangle_t;

/**
 * comac_rectangle_list_t:
 * @status: Error status of the rectangle list
 * @rectangles: Array containing the rectangles
 * @num_rectangles: Number of rectangles in this list
 * 
 * A data structure for holding a dynamically allocated
 * array of rectangles.
 *
 * Since: 1.4
 **/
typedef struct _comac_rectangle_list {
    comac_status_t status;
    comac_rectangle_t *rectangles;
    int num_rectangles;
} comac_rectangle_list_t;

comac_public comac_rectangle_list_t *
comac_copy_clip_rectangle_list (comac_t *cr);

comac_public void
comac_rectangle_list_destroy (comac_rectangle_list_t *rectangle_list);

/* Logical structure tagging functions */

#define COMAC_TAG_DEST "comac.dest"
#define COMAC_TAG_LINK "Link"

comac_public void
comac_tag_begin (comac_t *cr, const char *tag_name, const char *attributes);

comac_public void
comac_tag_end (comac_t *cr, const char *tag_name);

/* Font/Text functions */

/**
 * comac_scaled_font_t:
 *
 * A #comac_scaled_font_t is a font scaled to a particular size and device
 * resolution. A #comac_scaled_font_t is most useful for low-level font
 * usage where a library or application wants to cache a reference
 * to a scaled font to speed up the computation of metrics.
 *
 * There are various types of scaled fonts, depending on the
 * <firstterm>font backend</firstterm> they use. The type of a
 * scaled font can be queried using comac_scaled_font_get_type().
 *
 * Memory management of #comac_scaled_font_t is done with
 * comac_scaled_font_reference() and comac_scaled_font_destroy().
 *
 * Since: 1.0
 **/
typedef struct _comac_scaled_font comac_scaled_font_t;

/**
 * comac_font_face_t:
 *
 * A #comac_font_face_t specifies all aspects of a font other
 * than the size or font matrix (a font matrix is used to distort
 * a font by shearing it or scaling it unequally in the two
 * directions) . A font face can be set on a #comac_t by using
 * comac_set_font_face(); the size and font matrix are set with
 * comac_set_font_size() and comac_set_font_matrix().
 *
 * There are various types of font faces, depending on the
 * <firstterm>font backend</firstterm> they use. The type of a
 * font face can be queried using comac_font_face_get_type().
 *
 * Memory management of #comac_font_face_t is done with
 * comac_font_face_reference() and comac_font_face_destroy().
 *
 * Since: 1.0
 **/
typedef struct _comac_font_face comac_font_face_t;

/**
 * comac_glyph_t:
 * @index: glyph index in the font. The exact interpretation of the
 *      glyph index depends on the font technology being used.
 * @x: the offset in the X direction between the origin used for
 *     drawing or measuring the string and the origin of this glyph.
 * @y: the offset in the Y direction between the origin used for
 *     drawing or measuring the string and the origin of this glyph.
 *
 * The #comac_glyph_t structure holds information about a single glyph
 * when drawing or measuring text. A font is (in simple terms) a
 * collection of shapes used to draw text. A glyph is one of these
 * shapes. There can be multiple glyphs for a single character
 * (alternates to be used in different contexts, for example), or a
 * glyph can be a <firstterm>ligature</firstterm> of multiple
 * characters. Comac doesn't expose any way of converting input text
 * into glyphs, so in order to use the Comac interfaces that take
 * arrays of glyphs, you must directly access the appropriate
 * underlying font system.
 *
 * Note that the offsets given by @x and @y are not cumulative. When
 * drawing or measuring text, each glyph is individually positioned
 * with respect to the overall origin
 *
 * Since: 1.0
 **/
typedef struct {
    unsigned long index;
    double x;
    double y;
} comac_glyph_t;

comac_public comac_glyph_t *
comac_glyph_allocate (int num_glyphs);

comac_public void
comac_glyph_free (comac_glyph_t *glyphs);

/**
 * comac_text_cluster_t:
 * @num_bytes: the number of bytes of UTF-8 text covered by cluster
 * @num_glyphs: the number of glyphs covered by cluster
 *
 * The #comac_text_cluster_t structure holds information about a single
 * <firstterm>text cluster</firstterm>.  A text cluster is a minimal
 * mapping of some glyphs corresponding to some UTF-8 text.
 *
 * For a cluster to be valid, both @num_bytes and @num_glyphs should
 * be non-negative, and at least one should be non-zero.
 * Note that clusters with zero glyphs are not as well supported as
 * normal clusters.  For example, PDF rendering applications typically
 * ignore those clusters when PDF text is being selected.
 *
 * See comac_show_text_glyphs() for how clusters are used in advanced
 * text operations.
 *
 * Since: 1.8
 **/
typedef struct {
    int num_bytes;
    int num_glyphs;
} comac_text_cluster_t;

comac_public comac_text_cluster_t *
comac_text_cluster_allocate (int num_clusters);

comac_public void
comac_text_cluster_free (comac_text_cluster_t *clusters);

/**
 * comac_text_cluster_flags_t:
 * @COMAC_TEXT_CLUSTER_FLAG_BACKWARD: The clusters in the cluster array
 * map to glyphs in the glyph array from end to start. (Since 1.8)
 *
 * Specifies properties of a text cluster mapping.
 *
 * Since: 1.8
 **/
typedef enum _comac_text_cluster_flags {
    COMAC_TEXT_CLUSTER_FLAG_BACKWARD = 0x00000001
} comac_text_cluster_flags_t;

/**
 * comac_text_extents_t:
 * @x_bearing: the horizontal distance from the origin to the
 *   leftmost part of the glyphs as drawn. Positive if the
 *   glyphs lie entirely to the right of the origin.
 * @y_bearing: the vertical distance from the origin to the
 *   topmost part of the glyphs as drawn. Positive only if the
 *   glyphs lie completely below the origin; will usually be
 *   negative.
 * @width: width of the glyphs as drawn
 * @height: height of the glyphs as drawn
 * @x_advance:distance to advance in the X direction
 *    after drawing these glyphs
 * @y_advance: distance to advance in the Y direction
 *   after drawing these glyphs. Will typically be zero except
 *   for vertical text layout as found in East-Asian languages.
 *
 * The #comac_text_extents_t structure stores the extents of a single
 * glyph or a string of glyphs in user-space coordinates. Because text
 * extents are in user-space coordinates, they are mostly, but not
 * entirely, independent of the current transformation matrix. If you call
 * <literal>comac_scale(cr, 2.0, 2.0)</literal>, text will
 * be drawn twice as big, but the reported text extents will not be
 * doubled. They will change slightly due to hinting (so you can't
 * assume that metrics are independent of the transformation matrix),
 * but otherwise will remain unchanged.
 *
 * Since: 1.0
 **/
typedef struct {
    double x_bearing;
    double y_bearing;
    double width;
    double height;
    double x_advance;
    double y_advance;
} comac_text_extents_t;

/**
 * comac_font_extents_t:
 * @ascent: the distance that the font extends above the baseline.
 *          Note that this is not always exactly equal to the maximum
 *          of the extents of all the glyphs in the font, but rather
 *          is picked to express the font designer's intent as to
 *          how the font should align with elements above it.
 * @descent: the distance that the font extends below the baseline.
 *           This value is positive for typical fonts that include
 *           portions below the baseline. Note that this is not always
 *           exactly equal to the maximum of the extents of all the
 *           glyphs in the font, but rather is picked to express the
 *           font designer's intent as to how the font should
 *           align with elements below it.
 * @height: the recommended vertical distance between baselines when
 *          setting consecutive lines of text with the font. This
 *          is greater than @ascent+@descent by a
 *          quantity known as the <firstterm>line spacing</firstterm>
 *          or <firstterm>external leading</firstterm>. When space
 *          is at a premium, most fonts can be set with only
 *          a distance of @ascent+@descent between lines.
 * @max_x_advance: the maximum distance in the X direction that
 *         the origin is advanced for any glyph in the font.
 * @max_y_advance: the maximum distance in the Y direction that
 *         the origin is advanced for any glyph in the font.
 *         This will be zero for normal fonts used for horizontal
 *         writing. (The scripts of East Asia are sometimes written
 *         vertically.)
 *
 * The #comac_font_extents_t structure stores metric information for
 * a font. Values are given in the current user-space coordinate
 * system.
 *
 * Because font metrics are in user-space coordinates, they are
 * mostly, but not entirely, independent of the current transformation
 * matrix. If you call <literal>comac_scale(cr, 2.0, 2.0)</literal>,
 * text will be drawn twice as big, but the reported text extents will
 * not be doubled. They will change slightly due to hinting (so you
 * can't assume that metrics are independent of the transformation
 * matrix), but otherwise will remain unchanged.
 *
 * Since: 1.0
 **/
typedef struct {
    double ascent;
    double descent;
    double height;
    double max_x_advance;
    double max_y_advance;
} comac_font_extents_t;

/**
 * comac_font_slant_t:
 * @COMAC_FONT_SLANT_NORMAL: Upright font style, since 1.0
 * @COMAC_FONT_SLANT_ITALIC: Italic font style, since 1.0
 * @COMAC_FONT_SLANT_OBLIQUE: Oblique font style, since 1.0
 *
 * Specifies variants of a font face based on their slant.
 *
 * Since: 1.0
 **/
typedef enum _comac_font_slant {
    COMAC_FONT_SLANT_NORMAL,
    COMAC_FONT_SLANT_ITALIC,
    COMAC_FONT_SLANT_OBLIQUE
} comac_font_slant_t;

/**
 * comac_font_weight_t:
 * @COMAC_FONT_WEIGHT_NORMAL: Normal font weight, since 1.0
 * @COMAC_FONT_WEIGHT_BOLD: Bold font weight, since 1.0
 *
 * Specifies variants of a font face based on their weight.
 *
 * Since: 1.0
 **/
typedef enum _comac_font_weight {
    COMAC_FONT_WEIGHT_NORMAL,
    COMAC_FONT_WEIGHT_BOLD
} comac_font_weight_t;

/**
 * comac_subpixel_order_t:
 * @COMAC_SUBPIXEL_ORDER_DEFAULT: Use the default subpixel order for
 *   for the target device, since 1.0
 * @COMAC_SUBPIXEL_ORDER_RGB: Subpixel elements are arranged horizontally
 *   with red at the left, since 1.0
 * @COMAC_SUBPIXEL_ORDER_BGR:  Subpixel elements are arranged horizontally
 *   with blue at the left, since 1.0
 * @COMAC_SUBPIXEL_ORDER_VRGB: Subpixel elements are arranged vertically
 *   with red at the top, since 1.0
 * @COMAC_SUBPIXEL_ORDER_VBGR: Subpixel elements are arranged vertically
 *   with blue at the top, since 1.0
 *
 * The subpixel order specifies the order of color elements within
 * each pixel on the display device when rendering with an
 * antialiasing mode of %COMAC_ANTIALIAS_SUBPIXEL.
 *
 * Since: 1.0
 **/
typedef enum _comac_subpixel_order {
    COMAC_SUBPIXEL_ORDER_DEFAULT,
    COMAC_SUBPIXEL_ORDER_RGB,
    COMAC_SUBPIXEL_ORDER_BGR,
    COMAC_SUBPIXEL_ORDER_VRGB,
    COMAC_SUBPIXEL_ORDER_VBGR
} comac_subpixel_order_t;

/**
 * comac_hint_style_t:
 * @COMAC_HINT_STYLE_DEFAULT: Use the default hint style for
 *   font backend and target device, since 1.0
 * @COMAC_HINT_STYLE_NONE: Do not hint outlines, since 1.0
 * @COMAC_HINT_STYLE_SLIGHT: Hint outlines slightly to improve
 *   contrast while retaining good fidelity to the original
 *   shapes, since 1.0
 * @COMAC_HINT_STYLE_MEDIUM: Hint outlines with medium strength
 *   giving a compromise between fidelity to the original shapes
 *   and contrast, since 1.0
 * @COMAC_HINT_STYLE_FULL: Hint outlines to maximize contrast, since 1.0
 *
 * Specifies the type of hinting to do on font outlines. Hinting
 * is the process of fitting outlines to the pixel grid in order
 * to improve the appearance of the result. Since hinting outlines
 * involves distorting them, it also reduces the faithfulness
 * to the original outline shapes. Not all of the outline hinting
 * styles are supported by all font backends.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.0
 **/
typedef enum _comac_hint_style {
    COMAC_HINT_STYLE_DEFAULT,
    COMAC_HINT_STYLE_NONE,
    COMAC_HINT_STYLE_SLIGHT,
    COMAC_HINT_STYLE_MEDIUM,
    COMAC_HINT_STYLE_FULL
} comac_hint_style_t;

/**
 * comac_hint_metrics_t:
 * @COMAC_HINT_METRICS_DEFAULT: Hint metrics in the default
 *  manner for the font backend and target device, since 1.0
 * @COMAC_HINT_METRICS_OFF: Do not hint font metrics, since 1.0
 * @COMAC_HINT_METRICS_ON: Hint font metrics, since 1.0
 *
 * Specifies whether to hint font metrics; hinting font metrics
 * means quantizing them so that they are integer values in
 * device space. Doing this improves the consistency of
 * letter and line spacing, however it also means that text
 * will be laid out differently at different zoom factors.
 *
 * Since: 1.0
 **/
typedef enum _comac_hint_metrics {
    COMAC_HINT_METRICS_DEFAULT,
    COMAC_HINT_METRICS_OFF,
    COMAC_HINT_METRICS_ON
} comac_hint_metrics_t;

/**
 * comac_color_mode_t:
 * @COMAC_COLOR_MODE_DEFAULT: Use the default color mode for
 * font backend and target device, since 1.18.
 * @COMAC_COLOR_MODE_NO_COLOR: Disable rendering color glyphs. Glyphs are
 * always rendered as outline glyphs, since 1.18.
 * @COMAC_COLOR_MODE_COLOR: Enable rendering color glyphs. If the font
 * contains a color presentation for a glyph, and when supported by
 * the font backend, the glyph will be rendered in color, since 1.18.
 *
 * Specifies if color fonts are to be rendered using the the color
 * glyphs or outline glyphs. Glyphs that do not have a color
 * presentation, and non-color fonts are not affected by this font
 * option.
 *
 * Since: 1.18
 **/
typedef enum _comac_color_mode {
    COMAC_COLOR_MODE_DEFAULT,
    COMAC_COLOR_MODE_NO_COLOR,
    COMAC_COLOR_MODE_COLOR
} comac_color_mode_t;

/**
 * comac_font_options_t:
 *
 * An opaque structure holding all options that are used when
 * rendering fonts.
 *
 * Individual features of a #comac_font_options_t can be set or
 * accessed using functions named
 * <function>comac_font_options_set_<emphasis>feature_name</emphasis>()</function> and
 * <function>comac_font_options_get_<emphasis>feature_name</emphasis>()</function>, like
 * comac_font_options_set_antialias() and
 * comac_font_options_get_antialias().
 *
 * New features may be added to a #comac_font_options_t in the
 * future.  For this reason, comac_font_options_copy(),
 * comac_font_options_equal(), comac_font_options_merge(), and
 * comac_font_options_hash() should be used to copy, check
 * for equality, merge, or compute a hash value of
 * #comac_font_options_t objects.
 *
 * Since: 1.0
 **/
typedef struct _comac_font_options comac_font_options_t;

comac_public comac_font_options_t *
comac_font_options_create (void);

comac_public comac_font_options_t *
comac_font_options_copy (const comac_font_options_t *original);

comac_public void
comac_font_options_destroy (comac_font_options_t *options);

comac_public comac_status_t
comac_font_options_status (comac_font_options_t *options);

comac_public void
comac_font_options_merge (comac_font_options_t *options,
			  const comac_font_options_t *other);
comac_public comac_bool_t
comac_font_options_equal (const comac_font_options_t *options,
			  const comac_font_options_t *other);

comac_public unsigned long
comac_font_options_hash (const comac_font_options_t *options);

comac_public void
comac_font_options_set_antialias (comac_font_options_t *options,
				  comac_antialias_t antialias);
comac_public comac_antialias_t
comac_font_options_get_antialias (const comac_font_options_t *options);

comac_public void
comac_font_options_set_subpixel_order (comac_font_options_t *options,
				       comac_subpixel_order_t subpixel_order);
comac_public comac_subpixel_order_t
comac_font_options_get_subpixel_order (const comac_font_options_t *options);

comac_public void
comac_font_options_set_hint_style (comac_font_options_t *options,
				   comac_hint_style_t hint_style);
comac_public comac_hint_style_t
comac_font_options_get_hint_style (const comac_font_options_t *options);

comac_public void
comac_font_options_set_hint_metrics (comac_font_options_t *options,
				     comac_hint_metrics_t hint_metrics);
comac_public comac_hint_metrics_t
comac_font_options_get_hint_metrics (const comac_font_options_t *options);

comac_public const char *
comac_font_options_get_variations (comac_font_options_t *options);

comac_public void
comac_font_options_set_variations (comac_font_options_t *options,
				   const char *variations);

#define COMAC_COLOR_PALETTE_DEFAULT 0

comac_public void
comac_font_options_set_color_mode (comac_font_options_t *options,
				   comac_color_mode_t color_mode);

comac_public comac_color_mode_t
comac_font_options_get_color_mode (const comac_font_options_t *options);

comac_public unsigned int
comac_font_options_get_color_palette (const comac_font_options_t *options);

comac_public void
comac_font_options_set_color_palette (comac_font_options_t *options,
				      unsigned int palette_index);

/* This interface is for dealing with text as text, not caring about the
   font object inside the the comac_t. */

comac_public void
comac_select_font_face (comac_t *cr,
			const char *family,
			comac_font_slant_t slant,
			comac_font_weight_t weight);

comac_public void
comac_set_font_size (comac_t *cr, double size);

comac_public void
comac_set_font_matrix (comac_t *cr, const comac_matrix_t *matrix);

comac_public void
comac_get_font_matrix (comac_t *cr, comac_matrix_t *matrix);

comac_public void
comac_set_font_options (comac_t *cr, const comac_font_options_t *options);

comac_public void
comac_get_font_options (comac_t *cr, comac_font_options_t *options);

comac_public void
comac_set_font_face (comac_t *cr, comac_font_face_t *font_face);

comac_public comac_font_face_t *
comac_get_font_face (comac_t *cr);

comac_public void
comac_set_scaled_font (comac_t *cr, const comac_scaled_font_t *scaled_font);

comac_public comac_scaled_font_t *
comac_get_scaled_font (comac_t *cr);

comac_public void
comac_show_text (comac_t *cr, const char *utf8);

comac_public void
comac_show_glyphs (comac_t *cr, const comac_glyph_t *glyphs, int num_glyphs);

comac_public void
comac_show_text_glyphs (comac_t *cr,
			const char *utf8,
			int utf8_len,
			const comac_glyph_t *glyphs,
			int num_glyphs,
			const comac_text_cluster_t *clusters,
			int num_clusters,
			comac_text_cluster_flags_t cluster_flags);

comac_public void
comac_text_path (comac_t *cr, const char *utf8);

comac_public void
comac_glyph_path (comac_t *cr, const comac_glyph_t *glyphs, int num_glyphs);

comac_public void
comac_text_extents (comac_t *cr,
		    const char *utf8,
		    comac_text_extents_t *extents);

comac_public void
comac_glyph_extents (comac_t *cr,
		     const comac_glyph_t *glyphs,
		     int num_glyphs,
		     comac_text_extents_t *extents);

comac_public void
comac_font_extents (comac_t *cr, comac_font_extents_t *extents);

/* Generic identifier for a font style */

comac_public comac_font_face_t *
comac_font_face_reference (comac_font_face_t *font_face);

comac_public void
comac_font_face_destroy (comac_font_face_t *font_face);

comac_public unsigned int
comac_font_face_get_reference_count (comac_font_face_t *font_face);

comac_public comac_status_t
comac_font_face_status (comac_font_face_t *font_face);

/**
 * comac_font_type_t:
 * @COMAC_FONT_TYPE_TOY: The font was created using comac's toy font api (Since: 1.2)
 * @COMAC_FONT_TYPE_FT: The font is of type FreeType (Since: 1.2)
 * @COMAC_FONT_TYPE_WIN32: The font is of type Win32 (Since: 1.2)
 * @COMAC_FONT_TYPE_QUARTZ: The font is of type Quartz (Since: 1.6, in 1.2 and
 * 1.4 it was named COMAC_FONT_TYPE_ATSUI)
 * @COMAC_FONT_TYPE_USER: The font was create using comac's user font api (Since: 1.8)
 * @COMAC_FONT_TYPE_DWRITE: The font is of type Win32 DWrite (Since: 1.18)
 *
 * #comac_font_type_t is used to describe the type of a given font
 * face or scaled font. The font types are also known as "font
 * backends" within comac.
 *
 * The type of a font face is determined by the function used to
 * create it, which will generally be of the form
 * <function>comac_<emphasis>type</emphasis>_font_face_create(<!-- -->)</function>.
 * The font face type can be queried with comac_font_face_get_type()
 *
 * The various #comac_font_face_t functions can be used with a font face
 * of any type.
 *
 * The type of a scaled font is determined by the type of the font
 * face passed to comac_scaled_font_create(). The scaled font type can
 * be queried with comac_scaled_font_get_type()
 *
 * The various #comac_scaled_font_t functions can be used with scaled
 * fonts of any type, but some font backends also provide
 * type-specific functions that must only be called with a scaled font
 * of the appropriate type. These functions have names that begin with
 * <function>comac_<emphasis>type</emphasis>_scaled_font(<!-- -->)</function>
 * such as comac_ft_scaled_font_lock_face().
 *
 * The behavior of calling a type-specific function with a scaled font
 * of the wrong type is undefined.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.2
 **/
typedef enum _comac_font_type {
    COMAC_FONT_TYPE_TOY,
    COMAC_FONT_TYPE_FT,
    COMAC_FONT_TYPE_WIN32,
    COMAC_FONT_TYPE_QUARTZ,
    COMAC_FONT_TYPE_USER,
    COMAC_FONT_TYPE_DWRITE
} comac_font_type_t;

comac_public comac_font_type_t
comac_font_face_get_type (comac_font_face_t *font_face);

comac_public void *
comac_font_face_get_user_data (comac_font_face_t *font_face,
			       const comac_user_data_key_t *key);

comac_public comac_status_t
comac_font_face_set_user_data (comac_font_face_t *font_face,
			       const comac_user_data_key_t *key,
			       void *user_data,
			       comac_destroy_func_t destroy);

/* Portable interface to general font features. */

comac_public comac_scaled_font_t *
comac_scaled_font_create (comac_font_face_t *font_face,
			  const comac_matrix_t *font_matrix,
			  const comac_matrix_t *ctm,
			  const comac_font_options_t *options);

comac_public comac_scaled_font_t *
comac_scaled_font_reference (comac_scaled_font_t *scaled_font);

comac_public void
comac_scaled_font_destroy (comac_scaled_font_t *scaled_font);

comac_public unsigned int
comac_scaled_font_get_reference_count (comac_scaled_font_t *scaled_font);

comac_public comac_status_t
comac_scaled_font_status (comac_scaled_font_t *scaled_font);

comac_public comac_font_type_t
comac_scaled_font_get_type (comac_scaled_font_t *scaled_font);

comac_public void *
comac_scaled_font_get_user_data (comac_scaled_font_t *scaled_font,
				 const comac_user_data_key_t *key);

comac_public comac_status_t
comac_scaled_font_set_user_data (comac_scaled_font_t *scaled_font,
				 const comac_user_data_key_t *key,
				 void *user_data,
				 comac_destroy_func_t destroy);

comac_public void
comac_scaled_font_extents (comac_scaled_font_t *scaled_font,
			   comac_font_extents_t *extents);

comac_public void
comac_scaled_font_text_extents (comac_scaled_font_t *scaled_font,
				const char *utf8,
				comac_text_extents_t *extents);

comac_public void
comac_scaled_font_glyph_extents (comac_scaled_font_t *scaled_font,
				 const comac_glyph_t *glyphs,
				 int num_glyphs,
				 comac_text_extents_t *extents);

comac_public comac_status_t
comac_scaled_font_text_to_glyphs (comac_scaled_font_t *scaled_font,
				  double x,
				  double y,
				  const char *utf8,
				  int utf8_len,
				  comac_glyph_t **glyphs,
				  int *num_glyphs,
				  comac_text_cluster_t **clusters,
				  int *num_clusters,
				  comac_text_cluster_flags_t *cluster_flags);

comac_public comac_font_face_t *
comac_scaled_font_get_font_face (comac_scaled_font_t *scaled_font);

comac_public void
comac_scaled_font_get_font_matrix (comac_scaled_font_t *scaled_font,
				   comac_matrix_t *font_matrix);

comac_public void
comac_scaled_font_get_ctm (comac_scaled_font_t *scaled_font,
			   comac_matrix_t *ctm);

comac_public void
comac_scaled_font_get_scale_matrix (comac_scaled_font_t *scaled_font,
				    comac_matrix_t *scale_matrix);

comac_public void
comac_scaled_font_get_font_options (comac_scaled_font_t *scaled_font,
				    comac_font_options_t *options);

/* Toy fonts */

comac_public comac_font_face_t *
comac_toy_font_face_create (const char *family,
			    comac_font_slant_t slant,
			    comac_font_weight_t weight);

comac_public const char *
comac_toy_font_face_get_family (comac_font_face_t *font_face);

comac_public comac_font_slant_t
comac_toy_font_face_get_slant (comac_font_face_t *font_face);

comac_public comac_font_weight_t
comac_toy_font_face_get_weight (comac_font_face_t *font_face);

/* User fonts */

comac_public comac_font_face_t *
comac_user_font_face_create (void);

/* User-font method signatures */

/**
 * comac_user_scaled_font_init_func_t:
 * @scaled_font: the scaled-font being created
 * @cr: a comac context, in font space
 * @extents: font extents to fill in, in font space
 *
 * #comac_user_scaled_font_init_func_t is the type of function which is
 * called when a scaled-font needs to be created for a user font-face.
 *
 * The comac context @cr is not used by the caller, but is prepared in font
 * space, similar to what the comac contexts passed to the render_glyph
 * method will look like.  The callback can use this context for extents
 * computation for example.  After the callback is called, @cr is checked
 * for any error status.
 *
 * The @extents argument is where the user font sets the font extents for
 * @scaled_font.  It is in font space, which means that for most cases its
 * ascent and descent members should add to 1.0.  @extents is preset to
 * hold a value of 1.0 for ascent, height, and max_x_advance, and 0.0 for
 * descent and max_y_advance members.
 *
 * The callback is optional.  If not set, default font extents as described
 * in the previous paragraph will be used.
 *
 * Note that @scaled_font is not fully initialized at this
 * point and trying to use it for text operations in the callback will result
 * in deadlock.
 *
 * Returns: %COMAC_STATUS_SUCCESS upon success, or an error status on error.
 *
 * Since: 1.8
 **/
typedef comac_status_t (*comac_user_scaled_font_init_func_t) (
    comac_scaled_font_t *scaled_font,
    comac_t *cr,
    comac_font_extents_t *extents);

/**
 * comac_user_scaled_font_render_glyph_func_t:
 * @scaled_font: user scaled-font
 * @glyph: glyph code to render
 * @cr: comac context to draw to, in font space
 * @extents: glyph extents to fill in, in font space
 *
 * #comac_user_scaled_font_render_glyph_func_t is the type of function which
 * is called when a user scaled-font needs to render a glyph.
 *
 * The callback is mandatory, and expected to draw the glyph with code @glyph to
 * the comac context @cr.  @cr is prepared such that the glyph drawing is done in
 * font space.  That is, the matrix set on @cr is the scale matrix of @scaled_font.
 * The @extents argument is where the user font sets the font extents for
 * @scaled_font.  However, if user prefers to draw in user space, they can
 * achieve that by changing the matrix on @cr.
 *
 * All comac rendering operations to @cr are permitted. However, when
 * this callback is set with
 * comac_user_font_face_set_render_glyph_func(), the result is
 * undefined if any source other than the default source on @cr is
 * used.  That means, glyph bitmaps should be rendered using
 * comac_mask() instead of comac_paint(). When this callback is set with
 * comac_user_font_face_set_render_color_glyph_func(), setting the
 * source is a valid operation.
 *
 * When this callback is set with
 * comac_user_font_face_set_render_color_glyph_func(), the default
 * source is the current source color of the context that is rendering
 * the user font. That is, the same color a non-color user font will
 * be rendered in. In most cases the callback will want to set a
 * specific color. If the callback wishes to use the current context
 * color after using another source, it should retain a reference to
 * the source or use comac_save()/comac_restore() prior to changing
 * the source. Note that the default source contains an internal
 * marker to indicate that it is to be substituted with the current
 * context source color when rendered to a surface. Querying the
 * default source pattern will reveal a solid black color, however
 * this is not representative of the color that will actually be
 * used. Similarly, setting a solid black color will render black, not
 * the current context source when the glyph is painted to a surface.
 *
 * Other non-default settings on @cr include a font size of 1.0 (given that
 * it is set up to be in font space), and font options corresponding to
 * @scaled_font.
 *
 * The @extents argument is preset to have <literal>x_bearing</literal>,
 * <literal>width</literal>, and <literal>y_advance</literal> of zero,
 * <literal>y_bearing</literal> set to <literal>-font_extents.ascent</literal>,
 * <literal>height</literal> to <literal>font_extents.ascent+font_extents.descent</literal>,
 * and <literal>x_advance</literal> to <literal>font_extents.max_x_advance</literal>.
 * The only field user needs to set in majority of cases is
 * <literal>x_advance</literal>.
 * If the <literal>width</literal> field is zero upon the callback returning
 * (which is its preset value), the glyph extents are automatically computed
 * based on the drawings done to @cr.  This is in most cases exactly what the
 * desired behavior is.  However, if for any reason the callback sets the
 * extents, it must be ink extents, and include the extents of all drawing
 * done to @cr in the callback.
 *
 * Where both color and non-color callbacks has been set using
 * comac_user_font_face_set_render_color_glyph_func(), and
 * comac_user_font_face_set_render_glyph_func(), the color glyph
 * callback may return %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED if the
 * glyph is not a color glyph. This is the only case in which the
 * %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED may be returned from a
 * render callback.
 *
 * Returns: %COMAC_STATUS_SUCCESS upon success,
 * %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED if fallback options should be tried,
 * or %COMAC_STATUS_USER_FONT_ERROR or any other error status on error.
 *
 * Since: 1.8
 **/
typedef comac_status_t (*comac_user_scaled_font_render_glyph_func_t) (
    comac_scaled_font_t *scaled_font,
    unsigned long glyph,
    comac_t *cr,
    comac_text_extents_t *extents);

/**
 * comac_user_scaled_font_text_to_glyphs_func_t:
 * @scaled_font: the scaled-font being created
 * @utf8: a string of text encoded in UTF-8
 * @utf8_len: length of @utf8 in bytes
 * @glyphs: pointer to array of glyphs to fill, in font space
 * @num_glyphs: pointer to number of glyphs
 * @clusters: pointer to array of cluster mapping information to fill, or %NULL
 * @num_clusters: pointer to number of clusters
 * @cluster_flags: pointer to location to store cluster flags corresponding to the
 *                 output @clusters
 *
 * #comac_user_scaled_font_text_to_glyphs_func_t is the type of function which
 * is called to convert input text to an array of glyphs.  This is used by the
 * comac_show_text() operation.
 *
 * Using this callback the user-font has full control on glyphs and their
 * positions.  That means, it allows for features like ligatures and kerning,
 * as well as complex <firstterm>shaping</firstterm> required for scripts like
 * Arabic and Indic.
 *
 * The @num_glyphs argument is preset to the number of glyph entries available
 * in the @glyphs buffer. If the @glyphs buffer is %NULL, the value of
 * @num_glyphs will be zero.  If the provided glyph array is too short for
 * the conversion (or for convenience), a new glyph array may be allocated
 * using comac_glyph_allocate() and placed in @glyphs.  Upon return,
 * @num_glyphs should contain the number of generated glyphs.  If the value
 * @glyphs points at has changed after the call, the caller will free the
 * allocated glyph array using comac_glyph_free().  The caller will also free
 * the original value of @glyphs, so the callback shouldn't do so.
 * The callback should populate the glyph indices and positions (in font space)
 * assuming that the text is to be shown at the origin.
 *
 * If @clusters is not %NULL, @num_clusters and @cluster_flags are also
 * non-%NULL, and cluster mapping should be computed. The semantics of how
 * cluster array allocation works is similar to the glyph array.  That is,
 * if @clusters initially points to a non-%NULL value, that array may be used
 * as a cluster buffer, and @num_clusters points to the number of cluster
 * entries available there.  If the provided cluster array is too short for
 * the conversion (or for convenience), a new cluster array may be allocated
 * using comac_text_cluster_allocate() and placed in @clusters.  In this case,
 * the original value of @clusters will still be freed by the caller.  Upon
 * return, @num_clusters should contain the number of generated clusters.
 * If the value @clusters points at has changed after the call, the caller
 * will free the allocated cluster array using comac_text_cluster_free().
 *
 * The callback is optional.  If @num_glyphs is negative upon
 * the callback returning or if the return value
 * is %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED, the unicode_to_glyph callback
 * is tried.  See #comac_user_scaled_font_unicode_to_glyph_func_t.
 *
 * Note: While comac does not impose any limitation on glyph indices,
 * some applications may assume that a glyph index fits in a 16-bit
 * unsigned integer.  As such, it is advised that user-fonts keep their
 * glyphs in the 0 to 65535 range.  Furthermore, some applications may
 * assume that glyph 0 is a special glyph-not-found glyph.  User-fonts
 * are advised to use glyph 0 for such purposes and do not use that
 * glyph value for other purposes.
 *
 * Returns: %COMAC_STATUS_SUCCESS upon success,
 * %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED if fallback options should be tried,
 * or %COMAC_STATUS_USER_FONT_ERROR or any other error status on error.
 *
 * Since: 1.8
 **/
typedef comac_status_t (*comac_user_scaled_font_text_to_glyphs_func_t) (
    comac_scaled_font_t *scaled_font,
    const char *utf8,
    int utf8_len,
    comac_glyph_t **glyphs,
    int *num_glyphs,
    comac_text_cluster_t **clusters,
    int *num_clusters,
    comac_text_cluster_flags_t *cluster_flags);

/**
 * comac_user_scaled_font_unicode_to_glyph_func_t:
 * @scaled_font: the scaled-font being created
 * @unicode: input unicode character code-point
 * @glyph_index: output glyph index
 *
 * #comac_user_scaled_font_unicode_to_glyph_func_t is the type of function which
 * is called to convert an input Unicode character to a single glyph.
 * This is used by the comac_show_text() operation.
 *
 * This callback is used to provide the same functionality as the
 * text_to_glyphs callback does (see #comac_user_scaled_font_text_to_glyphs_func_t)
 * but has much less control on the output,
 * in exchange for increased ease of use.  The inherent assumption to using
 * this callback is that each character maps to one glyph, and that the
 * mapping is context independent.  It also assumes that glyphs are positioned
 * according to their advance width.  These mean no ligatures, kerning, or
 * complex scripts can be implemented using this callback.
 *
 * The callback is optional, and only used if text_to_glyphs callback is not
 * set or fails to return glyphs.  If this callback is not set or if it returns
 * %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED, an identity mapping from Unicode
 * code-points to glyph indices is assumed.
 *
 * Note: While comac does not impose any limitation on glyph indices,
 * some applications may assume that a glyph index fits in a 16-bit
 * unsigned integer.  As such, it is advised that user-fonts keep their
 * glyphs in the 0 to 65535 range.  Furthermore, some applications may
 * assume that glyph 0 is a special glyph-not-found glyph.  User-fonts
 * are advised to use glyph 0 for such purposes and do not use that
 * glyph value for other purposes.
 *
 * Returns: %COMAC_STATUS_SUCCESS upon success,
 * %COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED if fallback options should be tried,
 * or %COMAC_STATUS_USER_FONT_ERROR or any other error status on error.
 *
 * Since: 1.8
 **/
typedef comac_status_t (*comac_user_scaled_font_unicode_to_glyph_func_t) (
    comac_scaled_font_t *scaled_font,
    unsigned long unicode,
    unsigned long *glyph_index);

/* User-font method setters */

comac_public void
comac_user_font_face_set_init_func (
    comac_font_face_t *font_face, comac_user_scaled_font_init_func_t init_func);

comac_public void
comac_user_font_face_set_render_glyph_func (
    comac_font_face_t *font_face,
    comac_user_scaled_font_render_glyph_func_t render_glyph_func);

comac_public void
comac_user_font_face_set_render_color_glyph_func (
    comac_font_face_t *font_face,
    comac_user_scaled_font_render_glyph_func_t render_glyph_func);

comac_public void
comac_user_font_face_set_text_to_glyphs_func (
    comac_font_face_t *font_face,
    comac_user_scaled_font_text_to_glyphs_func_t text_to_glyphs_func);

comac_public void
comac_user_font_face_set_unicode_to_glyph_func (
    comac_font_face_t *font_face,
    comac_user_scaled_font_unicode_to_glyph_func_t unicode_to_glyph_func);

/* User-font method getters */

comac_public comac_user_scaled_font_init_func_t
comac_user_font_face_get_init_func (comac_font_face_t *font_face);

comac_public comac_user_scaled_font_render_glyph_func_t
comac_user_font_face_get_render_glyph_func (comac_font_face_t *font_face);

comac_public comac_user_scaled_font_render_glyph_func_t
comac_user_font_face_get_render_color_glyph_func (comac_font_face_t *font_face);

comac_public comac_user_scaled_font_text_to_glyphs_func_t
comac_user_font_face_get_text_to_glyphs_func (comac_font_face_t *font_face);

comac_public comac_user_scaled_font_unicode_to_glyph_func_t
comac_user_font_face_get_unicode_to_glyph_func (comac_font_face_t *font_face);

/* Query functions */

comac_public comac_operator_t
comac_get_operator (comac_t *cr);

comac_public comac_pattern_t *
comac_get_source (comac_t *cr);

comac_public double
comac_get_tolerance (comac_t *cr);

comac_public comac_antialias_t
comac_get_antialias (comac_t *cr);

comac_public comac_bool_t
comac_has_current_point (comac_t *cr);

comac_public void
comac_get_current_point (comac_t *cr, double *x, double *y);

comac_public comac_fill_rule_t
comac_get_fill_rule (comac_t *cr);

comac_public double
comac_get_line_width (comac_t *cr);

comac_public comac_bool_t
comac_get_hairline (comac_t *cr);

comac_public comac_line_cap_t
comac_get_line_cap (comac_t *cr);

comac_public comac_line_join_t
comac_get_line_join (comac_t *cr);

comac_public double
comac_get_miter_limit (comac_t *cr);

comac_public int
comac_get_dash_count (comac_t *cr);

comac_public void
comac_get_dash (comac_t *cr, double *dashes, double *offset);

comac_public void
comac_get_matrix (comac_t *cr, comac_matrix_t *matrix);

comac_public comac_surface_t *
comac_get_target (comac_t *cr);

comac_public comac_surface_t *
comac_get_group_target (comac_t *cr);

/**
 * comac_path_data_type_t:
 * @COMAC_PATH_MOVE_TO: A move-to operation, since 1.0
 * @COMAC_PATH_LINE_TO: A line-to operation, since 1.0
 * @COMAC_PATH_CURVE_TO: A curve-to operation, since 1.0
 * @COMAC_PATH_CLOSE_PATH: A close-path operation, since 1.0
 *
 * #comac_path_data_t is used to describe the type of one portion
 * of a path when represented as a #comac_path_t.
 * See #comac_path_data_t for details.
 *
 * Since: 1.0
 **/
typedef enum _comac_path_data_type {
    COMAC_PATH_MOVE_TO,
    COMAC_PATH_LINE_TO,
    COMAC_PATH_CURVE_TO,
    COMAC_PATH_CLOSE_PATH
} comac_path_data_type_t;

/**
 * comac_path_data_t:
 *
 * #comac_path_data_t is used to represent the path data inside a
 * #comac_path_t.
 *
 * The data structure is designed to try to balance the demands of
 * efficiency and ease-of-use. A path is represented as an array of
 * #comac_path_data_t, which is a union of headers and points.
 *
 * Each portion of the path is represented by one or more elements in
 * the array, (one header followed by 0 or more points). The length
 * value of the header is the number of array elements for the current
 * portion including the header, (ie. length == 1 + # of points), and
 * where the number of points for each element type is as follows:
 *
 * <programlisting>
 *     %COMAC_PATH_MOVE_TO:     1 point
 *     %COMAC_PATH_LINE_TO:     1 point
 *     %COMAC_PATH_CURVE_TO:    3 points
 *     %COMAC_PATH_CLOSE_PATH:  0 points
 * </programlisting>
 *
 * The semantics and ordering of the coordinate values are consistent
 * with comac_move_to(), comac_line_to(), comac_curve_to(), and
 * comac_close_path().
 *
 * Here is sample code for iterating through a #comac_path_t:
 *
 * <informalexample><programlisting>
 *      int i;
 *      comac_path_t *path;
 *      comac_path_data_t *data;
 * &nbsp;
 *      path = comac_copy_path (cr);
 * &nbsp;
 *      for (i=0; i < path->num_data; i += path->data[i].header.length) {
 *          data = &amp;path->data[i];
 *          switch (data->header.type) {
 *          case COMAC_PATH_MOVE_TO:
 *              do_move_to_things (data[1].point.x, data[1].point.y);
 *              break;
 *          case COMAC_PATH_LINE_TO:
 *              do_line_to_things (data[1].point.x, data[1].point.y);
 *              break;
 *          case COMAC_PATH_CURVE_TO:
 *              do_curve_to_things (data[1].point.x, data[1].point.y,
 *                                  data[2].point.x, data[2].point.y,
 *                                  data[3].point.x, data[3].point.y);
 *              break;
 *          case COMAC_PATH_CLOSE_PATH:
 *              do_close_path_things ();
 *              break;
 *          }
 *      }
 *      comac_path_destroy (path);
 * </programlisting></informalexample>
 *
 * As of comac 1.4, comac does not mind if there are more elements in
 * a portion of the path than needed.  Such elements can be used by
 * users of the comac API to hold extra values in the path data
 * structure.  For this reason, it is recommended that applications
 * always use <literal>data->header.length</literal> to
 * iterate over the path data, instead of hardcoding the number of
 * elements for each element type.
 *
 * Since: 1.0
 **/
typedef union _comac_path_data_t comac_path_data_t;
union _comac_path_data_t {
    struct {
	comac_path_data_type_t type;
	int length;
    } header;
    struct {
	double x, y;
    } point;
};

/**
 * comac_path_t:
 * @status: the current error status
 * @data: the elements in the path
 * @num_data: the number of elements in the data array
 *
 * A data structure for holding a path. This data structure serves as
 * the return value for comac_copy_path() and
 * comac_copy_path_flat() as well the input value for
 * comac_append_path().
 *
 * See #comac_path_data_t for hints on how to iterate over the
 * actual data within the path.
 *
 * The num_data member gives the number of elements in the data
 * array. This number is larger than the number of independent path
 * portions (defined in #comac_path_data_type_t), since the data
 * includes both headers and coordinates for each portion.
 *
 * Since: 1.0
 **/
typedef struct comac_path {
    comac_status_t status;
    comac_path_data_t *data;
    int num_data;
} comac_path_t;

comac_public comac_path_t *
comac_copy_path (comac_t *cr);

comac_public comac_path_t *
comac_copy_path_flat (comac_t *cr);

comac_public void
comac_append_path (comac_t *cr, const comac_path_t *path);

comac_public void
comac_path_destroy (comac_path_t *path);

/* Error status queries */

comac_public comac_status_t
comac_status (comac_t *cr);

comac_public const char *
comac_status_to_string (comac_status_t status);

/* Backend device manipulation */

comac_public comac_device_t *
comac_device_reference (comac_device_t *device);

/**
 * comac_device_type_t:
 * @COMAC_DEVICE_TYPE_DRM: The device is of type Direct Render Manager, since 1.10
 * @COMAC_DEVICE_TYPE_GL: The device is of type OpenGL, since 1.10
 * @COMAC_DEVICE_TYPE_SCRIPT: The device is of type script, since 1.10
 * @COMAC_DEVICE_TYPE_XCB: The device is of type xcb, since 1.10
 * @COMAC_DEVICE_TYPE_XLIB: The device is of type xlib, since 1.10
 * @COMAC_DEVICE_TYPE_XML: The device is of type XML, since 1.10
 * @COMAC_DEVICE_TYPE_COGL: The device is of type cogl, since 1.12
 * @COMAC_DEVICE_TYPE_WIN32: The device is of type win32, since 1.12
 * @COMAC_DEVICE_TYPE_INVALID: The device is invalid, since 1.10
 *
 * #comac_device_type_t is used to describe the type of a given
 * device. The devices types are also known as "backends" within comac.
 *
 * The device type can be queried with comac_device_get_type()
 *
 * The various #comac_device_t functions can be used with devices of
 * any type, but some backends also provide type-specific functions
 * that must only be called with a device of the appropriate
 * type. These functions have names that begin with
 * <literal>comac_<emphasis>type</emphasis>_device</literal> such as
 * comac_xcb_device_debug_cap_xrender_version().
 *
 * The behavior of calling a type-specific function with a device of
 * the wrong type is undefined.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.10
 **/
typedef enum _comac_device_type {
    COMAC_DEVICE_TYPE_DRM,
    COMAC_DEVICE_TYPE_GL,
    COMAC_DEVICE_TYPE_SCRIPT,
    COMAC_DEVICE_TYPE_XCB,
    COMAC_DEVICE_TYPE_XLIB,
    COMAC_DEVICE_TYPE_XML,
    COMAC_DEVICE_TYPE_COGL,
    COMAC_DEVICE_TYPE_WIN32,

    COMAC_DEVICE_TYPE_INVALID = -1
} comac_device_type_t;

comac_public comac_device_type_t
comac_device_get_type (comac_device_t *device);

comac_public comac_status_t
comac_device_status (comac_device_t *device);

comac_public comac_status_t
comac_device_acquire (comac_device_t *device);

comac_public void
comac_device_release (comac_device_t *device);

comac_public void
comac_device_flush (comac_device_t *device);

comac_public void
comac_device_finish (comac_device_t *device);

comac_public void
comac_device_destroy (comac_device_t *device);

comac_public unsigned int
comac_device_get_reference_count (comac_device_t *device);

comac_public void *
comac_device_get_user_data (comac_device_t *device,
			    const comac_user_data_key_t *key);

comac_public comac_status_t
comac_device_set_user_data (comac_device_t *device,
			    const comac_user_data_key_t *key,
			    void *user_data,
			    comac_destroy_func_t destroy);

/* Surface manipulation */

comac_public comac_surface_t *
comac_surface_create_similar (comac_surface_t *other,
			      comac_content_t content,
			      int width,
			      int height);

comac_public comac_surface_t *
comac_surface_create_similar_image (comac_surface_t *other,
				    comac_format_t format,
				    int width,
				    int height);

comac_public comac_surface_t *
comac_surface_map_to_image (comac_surface_t *surface,
			    const comac_rectangle_int_t *extents);

comac_public void
comac_surface_unmap_image (comac_surface_t *surface, comac_surface_t *image);

comac_public comac_surface_t *
comac_surface_create_for_rectangle (
    comac_surface_t *target, double x, double y, double width, double height);

/**
 * comac_surface_observer_mode_t:
 * @COMAC_SURFACE_OBSERVER_NORMAL: no recording is done
 * @COMAC_SURFACE_OBSERVER_RECORD_OPERATIONS: operations are recorded
 *
 * Whether operations should be recorded.
 *
 * Since: 1.12
 **/
typedef enum {
    COMAC_SURFACE_OBSERVER_NORMAL = 0,
    COMAC_SURFACE_OBSERVER_RECORD_OPERATIONS = 0x1
} comac_surface_observer_mode_t;

comac_public comac_surface_t *
comac_surface_create_observer (comac_surface_t *target,
			       comac_surface_observer_mode_t mode);

typedef void (*comac_surface_observer_callback_t) (comac_surface_t *observer,
						   comac_surface_t *target,
						   void *data);

comac_public comac_status_t
comac_surface_observer_add_paint_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data);

comac_public comac_status_t
comac_surface_observer_add_mask_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data);

comac_public comac_status_t
comac_surface_observer_add_fill_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data);

comac_public comac_status_t
comac_surface_observer_add_stroke_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data);

comac_public comac_status_t
comac_surface_observer_add_glyphs_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data);

comac_public comac_status_t
comac_surface_observer_add_flush_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data);

comac_public comac_status_t
comac_surface_observer_add_finish_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data);

comac_public comac_status_t
comac_surface_observer_print (comac_surface_t *surface,
			      comac_write_func_t write_func,
			      void *closure);
comac_public double
comac_surface_observer_elapsed (comac_surface_t *surface);

comac_public comac_status_t
comac_device_observer_print (comac_device_t *device,
			     comac_write_func_t write_func,
			     void *closure);

comac_public double
comac_device_observer_elapsed (comac_device_t *device);

comac_public double
comac_device_observer_paint_elapsed (comac_device_t *device);

comac_public double
comac_device_observer_mask_elapsed (comac_device_t *device);

comac_public double
comac_device_observer_fill_elapsed (comac_device_t *device);

comac_public double
comac_device_observer_stroke_elapsed (comac_device_t *device);

comac_public double
comac_device_observer_glyphs_elapsed (comac_device_t *device);

comac_public comac_surface_t *
comac_surface_reference (comac_surface_t *surface);

comac_public void
comac_surface_finish (comac_surface_t *surface);

comac_public void
comac_surface_destroy (comac_surface_t *surface);

comac_public comac_device_t *
comac_surface_get_device (comac_surface_t *surface);

comac_public unsigned int
comac_surface_get_reference_count (comac_surface_t *surface);

comac_public comac_status_t
comac_surface_status (comac_surface_t *surface);

comac_public comac_colorspace_t
comac_surface_get_colorspace (comac_surface_t *surface);

comac_public void
comac_surface_set_colorspace (comac_surface_t *surface, comac_colorspace_t cs);

comac_public void
comac_surface_set_color_conversion_callback (comac_surface_t *surface,
					     comac_color_convert_cb callback,
					     void *ctx);

/**
 * comac_surface_type_t:
 * @COMAC_SURFACE_TYPE_IMAGE: The surface is of type image, since 1.2
 * @COMAC_SURFACE_TYPE_PDF: The surface is of type pdf, since 1.2
 * @COMAC_SURFACE_TYPE_PS: The surface is of type ps, since 1.2
 * @COMAC_SURFACE_TYPE_XLIB: The surface is of type xlib, since 1.2
 * @COMAC_SURFACE_TYPE_XCB: The surface is of type xcb, since 1.2
 * @COMAC_SURFACE_TYPE_GLITZ: The surface is of type glitz, since 1.2
 * @COMAC_SURFACE_TYPE_QUARTZ: The surface is of type quartz, since 1.2
 * @COMAC_SURFACE_TYPE_WIN32: The surface is of type win32, since 1.2
 * @COMAC_SURFACE_TYPE_BEOS: The surface is of type beos, since 1.2
 * @COMAC_SURFACE_TYPE_DIRECTFB: The surface is of type directfb, since 1.2
 * @COMAC_SURFACE_TYPE_SVG: The surface is of type svg, since 1.2
 * @COMAC_SURFACE_TYPE_OS2: The surface is of type os2, since 1.4
 * @COMAC_SURFACE_TYPE_WIN32_PRINTING: The surface is a win32 printing surface, since 1.6
 * @COMAC_SURFACE_TYPE_QUARTZ_IMAGE: The surface is of type quartz_image, since 1.6
 * @COMAC_SURFACE_TYPE_SCRIPT: The surface is of type script, since 1.10
 * @COMAC_SURFACE_TYPE_QT: The surface is of type Qt, since 1.10
 * @COMAC_SURFACE_TYPE_RECORDING: The surface is of type recording, since 1.10
 * @COMAC_SURFACE_TYPE_VG: The surface is a OpenVG surface, since 1.10
 * @COMAC_SURFACE_TYPE_GL: The surface is of type OpenGL, since 1.10
 * @COMAC_SURFACE_TYPE_DRM: The surface is of type Direct Render Manager, since 1.10
 * @COMAC_SURFACE_TYPE_TEE: The surface is of type 'tee' (a multiplexing surface), since 1.10
 * @COMAC_SURFACE_TYPE_XML: The surface is of type XML (for debugging), since 1.10
 * @COMAC_SURFACE_TYPE_SUBSURFACE: The surface is a subsurface created with
 *   comac_surface_create_for_rectangle(), since 1.10
 * @COMAC_SURFACE_TYPE_COGL: This surface is of type Cogl, since 1.12
 *
 * #comac_surface_type_t is used to describe the type of a given
 * surface. The surface types are also known as "backends" or "surface
 * backends" within comac.
 *
 * The type of a surface is determined by the function used to create
 * it, which will generally be of the form
 * <function>comac_<emphasis>type</emphasis>_surface_create(<!-- -->)</function>,
 * (though see comac_surface_create_similar() as well).
 *
 * The surface type can be queried with comac_surface_get_type()
 *
 * The various #comac_surface_t functions can be used with surfaces of
 * any type, but some backends also provide type-specific functions
 * that must only be called with a surface of the appropriate
 * type. These functions have names that begin with
 * <literal>comac_<emphasis>type</emphasis>_surface</literal> such as comac_image_surface_get_width().
 *
 * The behavior of calling a type-specific function with a surface of
 * the wrong type is undefined.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.2
 **/
typedef enum _comac_surface_type {
    COMAC_SURFACE_TYPE_IMAGE,
    COMAC_SURFACE_TYPE_PDF,
    COMAC_SURFACE_TYPE_PS,
    COMAC_SURFACE_TYPE_XLIB,
    COMAC_SURFACE_TYPE_XCB,
    COMAC_SURFACE_TYPE_GLITZ,
    COMAC_SURFACE_TYPE_QUARTZ,
    COMAC_SURFACE_TYPE_WIN32,
    COMAC_SURFACE_TYPE_BEOS,
    COMAC_SURFACE_TYPE_DIRECTFB,
    COMAC_SURFACE_TYPE_SVG,
    COMAC_SURFACE_TYPE_OS2,
    COMAC_SURFACE_TYPE_WIN32_PRINTING,
    COMAC_SURFACE_TYPE_QUARTZ_IMAGE,
    COMAC_SURFACE_TYPE_SCRIPT,
    COMAC_SURFACE_TYPE_QT,
    COMAC_SURFACE_TYPE_RECORDING,
    COMAC_SURFACE_TYPE_VG,
    COMAC_SURFACE_TYPE_GL,
    COMAC_SURFACE_TYPE_DRM,
    COMAC_SURFACE_TYPE_TEE,
    COMAC_SURFACE_TYPE_XML,
    COMAC_SURFACE_TYPE_SKIA,
    COMAC_SURFACE_TYPE_SUBSURFACE,
    COMAC_SURFACE_TYPE_COGL
} comac_surface_type_t;

comac_public comac_surface_type_t
comac_surface_get_type (comac_surface_t *surface);

comac_public comac_content_t
comac_surface_get_content (comac_surface_t *surface);

#if COMAC_HAS_PNG_FUNCTIONS

comac_public comac_status_t
comac_surface_write_to_png (comac_surface_t *surface, const char *filename);

comac_public comac_status_t
comac_surface_write_to_png_stream (comac_surface_t *surface,
				   comac_write_func_t write_func,
				   void *closure);

#endif

comac_public void *
comac_surface_get_user_data (comac_surface_t *surface,
			     const comac_user_data_key_t *key);

comac_public comac_status_t
comac_surface_set_user_data (comac_surface_t *surface,
			     const comac_user_data_key_t *key,
			     void *user_data,
			     comac_destroy_func_t destroy);

#define COMAC_MIME_TYPE_JPEG "image/jpeg"
#define COMAC_MIME_TYPE_PNG "image/png"
#define COMAC_MIME_TYPE_JP2 "image/jp2"
#define COMAC_MIME_TYPE_URI "text/x-uri"
#define COMAC_MIME_TYPE_UNIQUE_ID "application/x-comac.uuid"
#define COMAC_MIME_TYPE_JBIG2 "application/x-comac.jbig2"
#define COMAC_MIME_TYPE_JBIG2_GLOBAL "application/x-comac.jbig2-global"
#define COMAC_MIME_TYPE_JBIG2_GLOBAL_ID "application/x-comac.jbig2-global-id"
#define COMAC_MIME_TYPE_CCITT_FAX "image/g3fax"
#define COMAC_MIME_TYPE_CCITT_FAX_PARAMS "application/x-comac.ccitt.params"
#define COMAC_MIME_TYPE_EPS "application/postscript"
#define COMAC_MIME_TYPE_EPS_PARAMS "application/x-comac.eps.params"

comac_public void
comac_surface_get_mime_data (comac_surface_t *surface,
			     const char *mime_type,
			     const unsigned char **data,
			     unsigned long *length);

comac_public comac_status_t
comac_surface_set_mime_data (comac_surface_t *surface,
			     const char *mime_type,
			     const unsigned char *data,
			     unsigned long length,
			     comac_destroy_func_t destroy,
			     void *closure);

comac_public comac_bool_t
comac_surface_supports_mime_type (comac_surface_t *surface,
				  const char *mime_type);

comac_public void
comac_surface_get_font_options (comac_surface_t *surface,
				comac_font_options_t *options);

comac_public void
comac_surface_flush (comac_surface_t *surface);

comac_public void
comac_surface_mark_dirty (comac_surface_t *surface);

comac_public void
comac_surface_mark_dirty_rectangle (
    comac_surface_t *surface, int x, int y, int width, int height);

comac_public void
comac_surface_set_device_scale (comac_surface_t *surface,
				double x_scale,
				double y_scale);

comac_public void
comac_surface_get_device_scale (comac_surface_t *surface,
				double *x_scale,
				double *y_scale);

comac_public void
comac_surface_set_device_offset (comac_surface_t *surface,
				 double x_offset,
				 double y_offset);

comac_public void
comac_surface_get_device_offset (comac_surface_t *surface,
				 double *x_offset,
				 double *y_offset);

comac_public void
comac_surface_set_fallback_resolution (comac_surface_t *surface,
				       double x_pixels_per_inch,
				       double y_pixels_per_inch);

comac_public void
comac_surface_get_fallback_resolution (comac_surface_t *surface,
				       double *x_pixels_per_inch,
				       double *y_pixels_per_inch);

comac_public void
comac_surface_copy_page (comac_surface_t *surface);

comac_public void
comac_surface_show_page (comac_surface_t *surface);

comac_public comac_bool_t
comac_surface_has_show_text_glyphs (comac_surface_t *surface);

/* Image-surface functions */

comac_public comac_surface_t *
comac_image_surface_create (comac_format_t format, int width, int height);

comac_public int
comac_format_stride_for_width (comac_format_t format, int width);

comac_public comac_surface_t *
comac_image_surface_create_for_data (unsigned char *data,
				     comac_format_t format,
				     int width,
				     int height,
				     int stride);

comac_public unsigned char *
comac_image_surface_get_data (comac_surface_t *surface);

comac_public comac_format_t
comac_image_surface_get_format (comac_surface_t *surface);

comac_public int
comac_image_surface_get_width (comac_surface_t *surface);

comac_public int
comac_image_surface_get_height (comac_surface_t *surface);

comac_public int
comac_image_surface_get_stride (comac_surface_t *surface);

#if COMAC_HAS_PNG_FUNCTIONS

comac_public comac_surface_t *
comac_image_surface_create_from_png (const char *filename);

comac_public comac_surface_t *
comac_image_surface_create_from_png_stream (comac_read_func_t read_func,
					    void *closure);

#endif

/* Recording-surface functions */

comac_public comac_surface_t *
comac_recording_surface_create (comac_content_t content,
				const comac_rectangle_t *extents);

comac_public void
comac_recording_surface_ink_extents (comac_surface_t *surface,
				     double *x0,
				     double *y0,
				     double *width,
				     double *height);

comac_public comac_bool_t
comac_recording_surface_get_extents (comac_surface_t *surface,
				     comac_rectangle_t *extents);

/* raster-source pattern (callback) functions */

/**
 * comac_raster_source_acquire_func_t:
 * @pattern: the pattern being rendered from
 * @callback_data: the user data supplied during creation
 * @target: the rendering target surface
 * @extents: rectangular region of interest in pixels in sample space
 *
 * #comac_raster_source_acquire_func_t is the type of function which is
 * called when a pattern is being rendered from. It should create a surface
 * that provides the pixel data for the region of interest as defined by
 * extents, though the surface itself does not have to be limited to that
 * area. For convenience the surface should probably be of image type,
 * created with comac_surface_create_similar_image() for the target (which
 * enables the number of copies to be reduced during transfer to the
 * device). Another option, might be to return a similar surface to the
 * target for explicit handling by the application of a set of cached sources
 * on the device. The region of sample data provided should be defined using
 * comac_surface_set_device_offset() to specify the top-left corner of the
 * sample data (along with width and height of the surface).
 *
 * Returns: a #comac_surface_t
 *
 * Since: 1.12
 **/
typedef comac_surface_t *(*comac_raster_source_acquire_func_t) (
    comac_pattern_t *pattern,
    void *callback_data,
    comac_surface_t *target,
    const comac_rectangle_int_t *extents);

/**
 * comac_raster_source_release_func_t:
 * @pattern: the pattern being rendered from
 * @callback_data: the user data supplied during creation
 * @surface: the surface created during acquire
 *
 * #comac_raster_source_release_func_t is the type of function which is
 * called when the pixel data is no longer being access by the pattern
 * for the rendering operation. Typically this function will simply
 * destroy the surface created during acquire.
 *
 * Since: 1.12
 **/
typedef void (*comac_raster_source_release_func_t) (comac_pattern_t *pattern,
						    void *callback_data,
						    comac_surface_t *surface);

/**
 * comac_raster_source_snapshot_func_t:
 * @pattern: the pattern being rendered from
 * @callback_data: the user data supplied during creation
 *
 * #comac_raster_source_snapshot_func_t is the type of function which is
 * called when the pixel data needs to be preserved for later use
 * during printing. This pattern will be accessed again later, and it
 * is expected to provide the pixel data that was current at the time
 * of snapshotting.
 *
 * Return value: COMAC_STATUS_SUCCESS on success, or one of the
 * #comac_status_t error codes for failure.
 *
 * Since: 1.12
 **/
typedef comac_status_t (*comac_raster_source_snapshot_func_t) (
    comac_pattern_t *pattern, void *callback_data);

/**
 * comac_raster_source_copy_func_t:
 * @pattern: the #comac_pattern_t that was copied to
 * @callback_data: the user data supplied during creation
 * @other: the #comac_pattern_t being used as the source for the copy
 *
 * #comac_raster_source_copy_func_t is the type of function which is
 * called when the pattern gets copied as a normal part of rendering.
 *
 * Return value: COMAC_STATUS_SUCCESS on success, or one of the
 * #comac_status_t error codes for failure.
 *
 * Since: 1.12
 **/
typedef comac_status_t (*comac_raster_source_copy_func_t) (
    comac_pattern_t *pattern,
    void *callback_data,
    const comac_pattern_t *other);

/**
 * comac_raster_source_finish_func_t:
 * @pattern: the pattern being rendered from
 * @callback_data: the user data supplied during creation
 *
 * #comac_raster_source_finish_func_t is the type of function which is
 * called when the pattern (or a copy thereof) is no longer required.
 *
 * Since: 1.12
 **/
typedef void (*comac_raster_source_finish_func_t) (comac_pattern_t *pattern,
						   void *callback_data);

comac_public comac_pattern_t *
comac_pattern_create_raster_source (void *user_data,
				    comac_content_t content,
				    int width,
				    int height);

comac_public void
comac_raster_source_pattern_set_callback_data (comac_pattern_t *pattern,
					       void *data);

comac_public void *
comac_raster_source_pattern_get_callback_data (comac_pattern_t *pattern);

comac_public void
comac_raster_source_pattern_set_acquire (
    comac_pattern_t *pattern,
    comac_raster_source_acquire_func_t acquire,
    comac_raster_source_release_func_t release);

comac_public void
comac_raster_source_pattern_get_acquire (
    comac_pattern_t *pattern,
    comac_raster_source_acquire_func_t *acquire,
    comac_raster_source_release_func_t *release);
comac_public void
comac_raster_source_pattern_set_snapshot (
    comac_pattern_t *pattern, comac_raster_source_snapshot_func_t snapshot);

comac_public comac_raster_source_snapshot_func_t
comac_raster_source_pattern_get_snapshot (comac_pattern_t *pattern);

comac_public void
comac_raster_source_pattern_set_copy (comac_pattern_t *pattern,
				      comac_raster_source_copy_func_t copy);

comac_public comac_raster_source_copy_func_t
comac_raster_source_pattern_get_copy (comac_pattern_t *pattern);

comac_public void
comac_raster_source_pattern_set_finish (
    comac_pattern_t *pattern, comac_raster_source_finish_func_t finish);

comac_public comac_raster_source_finish_func_t
comac_raster_source_pattern_get_finish (comac_pattern_t *pattern);

/* Pattern creation functions */

comac_public comac_pattern_t *
comac_pattern_create_rgb (double red, double green, double blue);

comac_public comac_pattern_t *
comac_pattern_create_rgba (double red, double green, double blue, double alpha);

comac_public comac_pattern_t *
comac_pattern_create_for_surface (comac_surface_t *surface);

comac_public comac_pattern_t *
comac_pattern_create_linear (double x0, double y0, double x1, double y1);

comac_public comac_pattern_t *
comac_pattern_create_radial (double cx0,
			     double cy0,
			     double radius0,
			     double cx1,
			     double cy1,
			     double radius1);

comac_public comac_pattern_t *
comac_pattern_create_mesh (void);

comac_public comac_pattern_t *
comac_pattern_reference (comac_pattern_t *pattern);

comac_public void
comac_pattern_destroy (comac_pattern_t *pattern);

comac_public unsigned int
comac_pattern_get_reference_count (comac_pattern_t *pattern);

comac_public comac_status_t
comac_pattern_status (comac_pattern_t *pattern);

comac_public void *
comac_pattern_get_user_data (comac_pattern_t *pattern,
			     const comac_user_data_key_t *key);

comac_public comac_status_t
comac_pattern_set_user_data (comac_pattern_t *pattern,
			     const comac_user_data_key_t *key,
			     void *user_data,
			     comac_destroy_func_t destroy);

/**
 * comac_pattern_type_t:
 * @COMAC_PATTERN_TYPE_SOLID: The pattern is a solid (uniform)
 * color. It may be opaque or translucent, since 1.2.
 * @COMAC_PATTERN_TYPE_SURFACE: The pattern is a based on a surface (an image), since 1.2.
 * @COMAC_PATTERN_TYPE_LINEAR: The pattern is a linear gradient, since 1.2.
 * @COMAC_PATTERN_TYPE_RADIAL: The pattern is a radial gradient, since 1.2.
 * @COMAC_PATTERN_TYPE_MESH: The pattern is a mesh, since 1.12.
 * @COMAC_PATTERN_TYPE_RASTER_SOURCE: The pattern is a user pattern providing raster data, since 1.12.
 *
 * #comac_pattern_type_t is used to describe the type of a given pattern.
 *
 * The type of a pattern is determined by the function used to create
 * it. The comac_pattern_create_rgb() and comac_pattern_create_rgba()
 * functions create SOLID patterns. The remaining
 * comac_pattern_create<!-- --> functions map to pattern types in obvious
 * ways.
 *
 * The pattern type can be queried with comac_pattern_get_type()
 *
 * Most #comac_pattern_t functions can be called with a pattern of any
 * type, (though trying to change the extend or filter for a solid
 * pattern will have no effect). A notable exception is
 * comac_pattern_add_color_stop_rgb() and
 * comac_pattern_add_color_stop_rgba() which must only be called with
 * gradient patterns (either LINEAR or RADIAL). Otherwise the pattern
 * will be shutdown and put into an error state.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.2
 **/
typedef enum _comac_pattern_type {
    COMAC_PATTERN_TYPE_SOLID,
    COMAC_PATTERN_TYPE_SURFACE,
    COMAC_PATTERN_TYPE_LINEAR,
    COMAC_PATTERN_TYPE_RADIAL,
    COMAC_PATTERN_TYPE_MESH,
    COMAC_PATTERN_TYPE_RASTER_SOURCE
} comac_pattern_type_t;

comac_public comac_pattern_type_t
comac_pattern_get_type (comac_pattern_t *pattern);

comac_public void
comac_pattern_add_color_stop_rgb (comac_pattern_t *pattern,
				  double offset,
				  double red,
				  double green,
				  double blue);

comac_public void
comac_pattern_add_color_stop_rgba (comac_pattern_t *pattern,
				   double offset,
				   double red,
				   double green,
				   double blue,
				   double alpha);

comac_public void
comac_mesh_pattern_begin_patch (comac_pattern_t *pattern);

comac_public void
comac_mesh_pattern_end_patch (comac_pattern_t *pattern);

comac_public void
comac_mesh_pattern_curve_to (comac_pattern_t *pattern,
			     double x1,
			     double y1,
			     double x2,
			     double y2,
			     double x3,
			     double y3);

comac_public void
comac_mesh_pattern_line_to (comac_pattern_t *pattern, double x, double y);

comac_public void
comac_mesh_pattern_move_to (comac_pattern_t *pattern, double x, double y);

comac_public void
comac_mesh_pattern_set_control_point (comac_pattern_t *pattern,
				      unsigned int point_num,
				      double x,
				      double y);

comac_public void
comac_mesh_pattern_set_corner_color_rgb (comac_pattern_t *pattern,
					 unsigned int corner_num,
					 double red,
					 double green,
					 double blue);

comac_public void
comac_mesh_pattern_set_corner_color_rgba (comac_pattern_t *pattern,
					  unsigned int corner_num,
					  double red,
					  double green,
					  double blue,
					  double alpha);

comac_public void
comac_pattern_set_matrix (comac_pattern_t *pattern,
			  const comac_matrix_t *matrix);

comac_public void
comac_pattern_get_matrix (comac_pattern_t *pattern, comac_matrix_t *matrix);

/**
 * comac_extend_t:
 * @COMAC_EXTEND_NONE: pixels outside of the source pattern
 *   are fully transparent (Since 1.0)
 * @COMAC_EXTEND_REPEAT: the pattern is tiled by repeating (Since 1.0)
 * @COMAC_EXTEND_REFLECT: the pattern is tiled by reflecting
 *   at the edges (Since 1.0; but only implemented for surface patterns since 1.6)
 * @COMAC_EXTEND_PAD: pixels outside of the pattern copy
 *   the closest pixel from the source (Since 1.2; but only
 *   implemented for surface patterns since 1.6)
 *
 * #comac_extend_t is used to describe how pattern color/alpha will be
 * determined for areas "outside" the pattern's natural area, (for
 * example, outside the surface bounds or outside the gradient
 * geometry).
 *
 * Mesh patterns are not affected by the extend mode.
 *
 * The default extend mode is %COMAC_EXTEND_NONE for surface patterns
 * and %COMAC_EXTEND_PAD for gradient patterns.
 *
 * New entries may be added in future versions.
 *
 * Since: 1.0
 **/
typedef enum _comac_extend {
    COMAC_EXTEND_NONE,
    COMAC_EXTEND_REPEAT,
    COMAC_EXTEND_REFLECT,
    COMAC_EXTEND_PAD
} comac_extend_t;

comac_public void
comac_pattern_set_extend (comac_pattern_t *pattern, comac_extend_t extend);

comac_public comac_extend_t
comac_pattern_get_extend (comac_pattern_t *pattern);

/**
 * comac_filter_t:
 * @COMAC_FILTER_FAST: A high-performance filter, with quality similar
 *     to %COMAC_FILTER_NEAREST (Since 1.0)
 * @COMAC_FILTER_GOOD: A reasonable-performance filter, with quality
 *     similar to %COMAC_FILTER_BILINEAR (Since 1.0)
 * @COMAC_FILTER_BEST: The highest-quality available, performance may
 *     not be suitable for interactive use. (Since 1.0)
 * @COMAC_FILTER_NEAREST: Nearest-neighbor filtering (Since 1.0)
 * @COMAC_FILTER_BILINEAR: Linear interpolation in two dimensions (Since 1.0)
 * @COMAC_FILTER_GAUSSIAN: This filter value is currently
 *     unimplemented, and should not be used in current code. (Since 1.0)
 *
 * #comac_filter_t is used to indicate what filtering should be
 * applied when reading pixel values from patterns. See
 * comac_pattern_set_filter() for indicating the desired filter to be
 * used with a particular pattern.
 *
 * Since: 1.0
 **/
typedef enum _comac_filter {
    COMAC_FILTER_FAST,
    COMAC_FILTER_GOOD,
    COMAC_FILTER_BEST,
    COMAC_FILTER_NEAREST,
    COMAC_FILTER_BILINEAR,
    COMAC_FILTER_GAUSSIAN
} comac_filter_t;

comac_public void
comac_pattern_set_filter (comac_pattern_t *pattern, comac_filter_t filter);

comac_public comac_filter_t
comac_pattern_get_filter (comac_pattern_t *pattern);

comac_public comac_status_t
comac_pattern_get_rgba (comac_pattern_t *pattern,
			double *red,
			double *green,
			double *blue,
			double *alpha);

comac_public comac_status_t
comac_pattern_get_surface (comac_pattern_t *pattern, comac_surface_t **surface);

comac_public comac_status_t
comac_pattern_get_color_stop_rgba (comac_pattern_t *pattern,
				   int index,
				   double *offset,
				   double *red,
				   double *green,
				   double *blue,
				   double *alpha);

comac_public comac_status_t
comac_pattern_get_color_stop_count (comac_pattern_t *pattern, int *count);

comac_public comac_status_t
comac_pattern_get_linear_points (
    comac_pattern_t *pattern, double *x0, double *y0, double *x1, double *y1);

comac_public comac_status_t
comac_pattern_get_radial_circles (comac_pattern_t *pattern,
				  double *x0,
				  double *y0,
				  double *r0,
				  double *x1,
				  double *y1,
				  double *r1);

comac_public comac_status_t
comac_mesh_pattern_get_patch_count (comac_pattern_t *pattern,
				    unsigned int *count);

comac_public comac_path_t *
comac_mesh_pattern_get_path (comac_pattern_t *pattern, unsigned int patch_num);

comac_public comac_status_t
comac_mesh_pattern_get_corner_color_rgba (comac_pattern_t *pattern,
					  unsigned int patch_num,
					  unsigned int corner_num,
					  double *red,
					  double *green,
					  double *blue,
					  double *alpha);

comac_public comac_status_t
comac_mesh_pattern_get_control_point (comac_pattern_t *pattern,
				      unsigned int patch_num,
				      unsigned int point_num,
				      double *x,
				      double *y);

/* Matrix functions */

comac_public void
comac_matrix_init (comac_matrix_t *matrix,
		   double xx,
		   double yx,
		   double xy,
		   double yy,
		   double x0,
		   double y0);

comac_public void
comac_matrix_init_identity (comac_matrix_t *matrix);

comac_public void
comac_matrix_init_translate (comac_matrix_t *matrix, double tx, double ty);

comac_public void
comac_matrix_init_scale (comac_matrix_t *matrix, double sx, double sy);

comac_public void
comac_matrix_init_rotate (comac_matrix_t *matrix, double radians);

comac_public void
comac_matrix_translate (comac_matrix_t *matrix, double tx, double ty);

comac_public void
comac_matrix_scale (comac_matrix_t *matrix, double sx, double sy);

comac_public void
comac_matrix_rotate (comac_matrix_t *matrix, double radians);

comac_public comac_status_t
comac_matrix_invert (comac_matrix_t *matrix);

comac_public void
comac_matrix_multiply (comac_matrix_t *result,
		       const comac_matrix_t *a,
		       const comac_matrix_t *b);

comac_public void
comac_matrix_transform_distance (const comac_matrix_t *matrix,
				 double *dx,
				 double *dy);

comac_public void
comac_matrix_transform_point (const comac_matrix_t *matrix,
			      double *x,
			      double *y);

/* Region functions */

/**
 * comac_region_t:
 *
 * A #comac_region_t represents a set of integer-aligned rectangles.
 *
 * It allows set-theoretical operations like comac_region_union() and
 * comac_region_intersect() to be performed on them.
 *
 * Memory management of #comac_region_t is done with
 * comac_region_reference() and comac_region_destroy().
 *
 * Since: 1.10
 **/
typedef struct _comac_region comac_region_t;

/**
 * comac_region_overlap_t:
 * @COMAC_REGION_OVERLAP_IN: The contents are entirely inside the region. (Since 1.10)
 * @COMAC_REGION_OVERLAP_OUT: The contents are entirely outside the region. (Since 1.10)
 * @COMAC_REGION_OVERLAP_PART: The contents are partially inside and
 *     partially outside the region. (Since 1.10)
 *
 * Used as the return value for comac_region_contains_rectangle().
 *
 * Since: 1.10
 **/
typedef enum _comac_region_overlap {
    COMAC_REGION_OVERLAP_IN,  /* completely inside region */
    COMAC_REGION_OVERLAP_OUT, /* completely outside region */
    COMAC_REGION_OVERLAP_PART /* partly inside region */
} comac_region_overlap_t;

comac_public comac_region_t *
comac_region_create (void);

comac_public comac_region_t *
comac_region_create_rectangle (const comac_rectangle_int_t *rectangle);

comac_public comac_region_t *
comac_region_create_rectangles (const comac_rectangle_int_t *rects, int count);

comac_public comac_region_t *
comac_region_copy (const comac_region_t *original);

comac_public comac_region_t *
comac_region_reference (comac_region_t *region);

comac_public void
comac_region_destroy (comac_region_t *region);

comac_public comac_bool_t
comac_region_equal (const comac_region_t *a, const comac_region_t *b);

comac_public comac_status_t
comac_region_status (const comac_region_t *region);

comac_public void
comac_region_get_extents (const comac_region_t *region,
			  comac_rectangle_int_t *extents);

comac_public int
comac_region_num_rectangles (const comac_region_t *region);

comac_public void
comac_region_get_rectangle (const comac_region_t *region,
			    int nth,
			    comac_rectangle_int_t *rectangle);

comac_public comac_bool_t
comac_region_is_empty (const comac_region_t *region);

comac_public comac_region_overlap_t
comac_region_contains_rectangle (const comac_region_t *region,
				 const comac_rectangle_int_t *rectangle);

comac_public comac_bool_t
comac_region_contains_point (const comac_region_t *region, int x, int y);

comac_public void
comac_region_translate (comac_region_t *region, int dx, int dy);

comac_public comac_status_t
comac_region_subtract (comac_region_t *dst, const comac_region_t *other);

comac_public comac_status_t
comac_region_subtract_rectangle (comac_region_t *dst,
				 const comac_rectangle_int_t *rectangle);

comac_public comac_status_t
comac_region_intersect (comac_region_t *dst, const comac_region_t *other);

comac_public comac_status_t
comac_region_intersect_rectangle (comac_region_t *dst,
				  const comac_rectangle_int_t *rectangle);

comac_public comac_status_t
comac_region_union (comac_region_t *dst, const comac_region_t *other);

comac_public comac_status_t
comac_region_union_rectangle (comac_region_t *dst,
			      const comac_rectangle_int_t *rectangle);

comac_public comac_status_t
comac_region_xor (comac_region_t *dst, const comac_region_t *other);

comac_public comac_status_t
comac_region_xor_rectangle (comac_region_t *dst,
			    const comac_rectangle_int_t *rectangle);

/* Functions to be used while debugging (not intended for use in production code) */
comac_public void
comac_debug_reset_static_data (void);

COMAC_END_DECLS

#endif /* COMAC_H */
