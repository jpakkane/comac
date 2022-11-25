/*
 * Copyright © 2004,2006 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#ifndef _COMAC_BOILERPLATE_H_
#define _COMAC_BOILERPLATE_H_

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <comac.h>
#include <string.h>

#include "comac-compiler-private.h"

#if HAVE_STDINT_H
#include <stdint.h>
#elif HAVE_INTTYPES_H
#include <inttypes.h>
#elif HAVE_SYS_INT_TYPES_H
#include <sys/int_types.h>
#elif defined(_MSC_VER)
#include <stdint.h>
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#error Cannot find definitions for fixed-width integral types (uint8_t, uint32_t, etc.)
#endif

#ifndef HAVE_UINT64_T
#define HAVE_UINT64_T 1
#endif
#ifndef INT16_MIN
#define INT16_MIN (-32767 - 1)
#endif
#ifndef INT16_MAX
#define INT16_MAX (32767)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX (65535)
#endif

#ifndef COMAC_BOILERPLATE_DEBUG
#define COMAC_BOILERPLATE_DEBUG(x)
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#ifdef __MINGW32__
#define COMAC_BOILERPLATE_PRINTF_FORMAT(fmt_index, va_index)                   \
    __attribute__ ((__format__ (__MINGW_PRINTF_FORMAT, fmt_index, va_index)))
#else
#define COMAC_BOILERPLATE_PRINTF_FORMAT(fmt_index, va_index)                   \
    __attribute__ ((__format__ (__printf__, fmt_index, va_index)))
#endif
#else
#define COMAC_BOILERPLATE_PRINTF_FORMAT(fmt_index, va_index)
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

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(__array) ((int) (sizeof (__array) / sizeof (__array[0])))
#endif

COMAC_BEGIN_DECLS

/* A fake format we use for the flattened ARGB output of the PS and
 * PDF surfaces. */
#define COMAC_TEST_CONTENT_COLOR_ALPHA_FLATTENED ((unsigned int) -1)

extern const comac_user_data_key_t comac_boilerplate_output_basename_key;

comac_content_t
comac_boilerplate_content (comac_content_t content);

const char *
comac_boilerplate_content_name (comac_content_t content);

comac_format_t
comac_boilerplate_format_from_content (comac_content_t content);

typedef enum {
    COMAC_BOILERPLATE_MODE_TEST,
    COMAC_BOILERPLATE_MODE_PERF,

    /* This will allow running performance test with threads. The
     * GL backend is very slow on some drivers when run with thread
     * awareness turned on. */
    COMAC_BOILERPLATE_MODE_PERF_THREADS,
} comac_boilerplate_mode_t;

typedef comac_surface_t *(*comac_boilerplate_create_surface_t) (
    const char *name,
    comac_content_t content,
    double width,
    double height,
    double max_width,
    double max_height,
    comac_boilerplate_mode_t mode,
    void **closure);

typedef comac_surface_t *(*comac_boilerplate_create_similar_t) (
    comac_surface_t *other, comac_content_t content, int width, int height);

typedef void (*comac_boilerplate_force_fallbacks_t) (comac_surface_t *surface,
						     double x_pixels_per_inch,
						     double y_pixels_per_inch);

typedef comac_status_t (*comac_boilerplate_finish_surface_t) (
    comac_surface_t *surface);

typedef comac_surface_t *(*comac_boilerplate_get_image_surface_t) (
    comac_surface_t *surface, int page, int width, int height);

typedef comac_status_t (*comac_boilerplate_write_to_png_t) (
    comac_surface_t *surface, const char *filename);

typedef void (*comac_boilerplate_cleanup_t) (void *closure);

typedef void (*comac_boilerplate_wait_t) (void *closure);

typedef char *(*comac_boilerplate_describe_t) (void *closure);

typedef struct _comac_boilerplate_target {
    const char *name;
    const char *basename;
    const char *file_extension;
    const char *reference_target;
    comac_surface_type_t expected_type;
    comac_content_t content;
    unsigned int error_tolerance;
    const char *probe; /* runtime dl check */
    comac_boilerplate_create_surface_t create_surface;
    comac_boilerplate_create_similar_t create_similar;
    comac_boilerplate_force_fallbacks_t force_fallbacks;
    comac_boilerplate_finish_surface_t finish_surface;
    comac_boilerplate_get_image_surface_t get_image_surface;
    comac_boilerplate_write_to_png_t write_to_png;
    comac_boilerplate_cleanup_t cleanup;
    comac_boilerplate_wait_t synchronize;
    comac_boilerplate_describe_t describe;
    comac_bool_t is_measurable;
    comac_bool_t is_vector;
    comac_bool_t is_recording;
} comac_boilerplate_target_t;

const comac_boilerplate_target_t *
comac_boilerplate_get_image_target (comac_content_t content);

const comac_boilerplate_target_t *
comac_boilerplate_get_target_by_name (const char *name,
				      comac_content_t content);

const comac_boilerplate_target_t **
comac_boilerplate_get_targets (int *num_targets, comac_bool_t *limited_targets);

void
comac_boilerplate_free_targets (const comac_boilerplate_target_t **targets);

comac_surface_t *
_comac_boilerplate_get_image_surface (comac_surface_t *src,
				      int page,
				      int width,
				      int height);
comac_surface_t *
comac_boilerplate_get_image_surface_from_png (const char *filename,
					      int width,
					      int height,
					      comac_bool_t flatten);

comac_surface_t *
comac_boilerplate_surface_create_in_error (comac_status_t status);

enum {
    COMAC_BOILERPLATE_OPEN_NO_DAEMON = 0x1,
};

FILE *
comac_boilerplate_open_any2ppm (const char *filename,
				int page,
				unsigned int flags,
				int (**close_cb) (FILE *));

comac_surface_t *
comac_boilerplate_image_surface_create_from_ppm_stream (FILE *file);

comac_surface_t *
comac_boilerplate_convert_to_image (const char *filename, int page);

int
comac_boilerplate_version (void);

const char *
comac_boilerplate_version_string (void);

void
comac_boilerplate_fini (void);

#include "comac-boilerplate-system.h"

COMAC_END_DECLS

#endif
