/*
 * Copyright © 2004 Red Hat, Inc.
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

#ifndef _COMAC_TEST_H_
#define _COMAC_TEST_H_

#include "comac-boilerplate.h"

#include <stdarg.h>

COMAC_BEGIN_DECLS

#if   HAVE_STDINT_H
# include <stdint.h>
#elif HAVE_INTTYPES_H
# include <inttypes.h>
#elif HAVE_SYS_INT_TYPES_H
# include <sys/int_types.h>
#elif defined(_MSC_VER)
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
# ifndef HAVE_UINT64_T
#  define HAVE_UINT64_T 1
# endif
#else
#error Cannot find definitions for fixed-width integral types (uint8_t, uint32_t, \etc.)
#endif

#ifdef _MSC_VER
#define _USE_MATH_DEFINES

#include <float.h>
#if _MSC_VER <= 1600
#define isnan(x) _isnan(x)
#endif

#endif

#if HAVE_FENV_H
# include <fenv.h>
#endif
/* The following are optional in C99, so define them if they aren't yet */
#ifndef FE_DIVBYZERO
#define FE_DIVBYZERO 0
#endif
#ifndef FE_INEXACT
#define FE_INEXACT 0
#endif
#ifndef FE_INVALID
#define FE_INVALID 0
#endif
#ifndef FE_OVERFLOW
#define FE_OVERFLOW 0
#endif
#ifndef FE_UNDERFLOW
#define FE_UNDERFLOW 0
#endif

#include <math.h>

static inline double
comac_test_NaN (void)
{
#ifdef _MSC_VER
    /* MSVC strtod("NaN", NULL) returns 0.0 */
    union {
	uint32_t i[2];
	double d;
    } nan = {{0xffffffff, 0x7fffffff}};
    return nan.d;
#else
    return strtod("NaN", NULL);
#endif
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define COMAC_TEST_OUTPUT_DIR "output"

#define COMAC_TEST_LOG_SUFFIX ".log"

#define COMAC_TEST_FONT_FAMILY "DejaVu"

/* What is a fail and what isn't?
 * When running the test suite we want to detect unexpected output. This
 * can be caused by a change we have made to comac itself, or a change
 * in our environment. To capture this we classify the expected output into 3
 * classes:
 *
 *   REF  -- Perfect output.
 *           Might be different for each backend, due to slight implementation
 *           differences.
 *
 *   NEW  -- A new failure. We have uncovered a bug within comac and have
 *           recorded the current failure (along with the expected output
 *           if possible!) so we can detect any changes in our attempt to
 *           fix the bug.
 *
 *  XFAIL -- An external failure. We believe the comac output is perfect,
 *           but an external renderer is causing gross failure.
 *           (We also use this to capture current WONTFIX issues within comac,
 *           such as overflow in internal coordinates, so as not to distract
 *           us when regression testing.)
 *
 *  If no REF is given for a test, then it is assumed to be XFAIL.
 */
#define COMAC_TEST_REF_SUFFIX ".ref"
#define COMAC_TEST_XFAIL_SUFFIX ".xfail"
#define COMAC_TEST_NEW_SUFFIX ".new"

#define COMAC_TEST_OUT_SUFFIX ".out"
#define COMAC_TEST_DIFF_SUFFIX ".diff"

#define COMAC_TEST_PNG_EXTENSION ".png"
#define COMAC_TEST_OUT_PNG COMAC_TEST_OUT_SUFFIX COMAC_TEST_PNG_EXTENSION
#define COMAC_TEST_REF_PNG COMAC_TEST_REF_SUFFIX COMAC_TEST_PNG_EXTENSION
#define COMAC_TEST_DIFF_PNG COMAC_TEST_DIFF_SUFFIX COMAC_TEST_PNG_EXTENSION

typedef enum comac_test_status {
    COMAC_TEST_SUCCESS = 0,
    COMAC_TEST_NO_MEMORY,
    COMAC_TEST_FAILURE,
    COMAC_TEST_NEW,
    COMAC_TEST_XFAILURE,
    COMAC_TEST_ERROR,
    COMAC_TEST_CRASHED,
    COMAC_TEST_UNTESTED = 77 /* match automake's skipped exit status */
} comac_test_status_t;

typedef struct _comac_test_context comac_test_context_t;
typedef struct _comac_test comac_test_t;

typedef comac_test_status_t
(comac_test_preamble_function_t) (comac_test_context_t *ctx);

typedef comac_test_status_t
(comac_test_draw_function_t) (comac_t *cr, int width, int height);

struct _comac_test {
    const char *name;
    const char *description;
    const char *keywords;
    const char *requirements;
    double width;
    double height;
    comac_test_preamble_function_t *preamble;
    comac_test_draw_function_t *draw;
};

/* The standard test interface which works by examining result image.
 *
 * COMAC_TEST() constructs a test which will be called once before (the
 * preamble callback), and then once for each testable backend (the draw
 * callback). The following checks will be performed for each backend:
 *
 * 1) If preamble() returns COMAC_TEST_UNTESTED, the test is skipped.
 *
 * 2) If preamble() does not return COMAC_TEST_SUCCESS, the test fails.
 *
 * 3) If draw() does not return COMAC_TEST_SUCCESS then this backend
 *    fails.
 *
 * 4) Otherwise, if comac_status(cr) indicates an error then this
 *    backend fails.
 *
 * 5) Otherwise, if the image size is 0, then this backend passes.
 *
 * 6) Otherwise, if every channel of every pixel exactly matches the
 *    reference image then this backend passes. If not, this backend
 *    fails.
 *
 * The overall test result is PASS if and only if there is at least
 * one backend that is tested and if all tested backend pass according
 * to the four criteria above.
 */
#define COMAC_TEST(name, description, keywords, requirements, width, height, preamble, draw) \
void _register_##name (void); \
void _register_##name (void) { \
    static comac_test_t test = { \
	#name, description, \
	keywords, requirements, \
	width, height, \
	preamble, draw \
    }; \
    comac_test_register (&test); \
}

void
comac_test_register (comac_test_t *test);

/* The full context for the test.
 * For ordinary tests (using the COMAC_TEST()->draw interface) the context
 * is passed to the draw routine via user_data on the comac_t.
 * The reason why the context is not passed as an explicit parameter is that
 * it is rarely required by the test itself and by removing the parameter
 * we can keep the draw routines simple and serve as example code.
 *
 * In contrast, for the preamble phase the context is passed as the only
 * parameter.
 */
struct _comac_test_context {
    const comac_test_t *test;
    const char *test_name;

    FILE *log_file;
    const char *output;
    const char *srcdir; /* directory containing sources and input data */
    const char *refdir; /* directory containing reference images */

    char *ref_name; /* cache of the current reference image */
    comac_surface_t *ref_image;
    comac_surface_t *ref_image_flattened;

    size_t num_targets;
    comac_bool_t limited_targets;
    const comac_boilerplate_target_t **targets_to_test;
    comac_bool_t own_targets;

    int malloc_failure;
    int last_fault_count;

    int timeout;
};

/* Retrieve the test context from the comac_t, used for logging, paths etc */
const comac_test_context_t *
comac_test_get_context (comac_t *cr);


/* Print a message to the log file, ala printf. */
void
comac_test_log (const comac_test_context_t *ctx,
	        const char *fmt, ...) COMAC_BOILERPLATE_PRINTF_FORMAT(2, 3);
void
comac_test_logv (const comac_test_context_t *ctx,
	        const char *fmt, va_list ap) COMAC_BOILERPLATE_PRINTF_FORMAT(2, 0);

/* Helper functions that take care of finding source images even when
 * building in a non-srcdir manner, (i.e. the tests will be run in a
 * directory that is different from the one where the source image
 * exists). */
comac_surface_t *
comac_test_create_surface_from_png (const comac_test_context_t *ctx,
	                            const char *filename);

comac_pattern_t *
comac_test_create_pattern_from_png (const comac_test_context_t *ctx,
	                            const char *filename);

void
comac_test_paint_checkered (comac_t *cr);

#define COMAC_TEST_DOUBLE_EQUALS(a,b)  (fabs((a)-(b)) < 0.00001)

comac_bool_t
comac_test_is_target_enabled (const comac_test_context_t *ctx,
	                      const char *target);

char *
comac_test_get_name (const comac_test_t *test);

comac_bool_t
comac_test_malloc_failure (const comac_test_context_t *ctx,
	                   comac_status_t status);

comac_test_status_t
comac_test_status_from_status (const comac_test_context_t *ctx,
			       comac_status_t status);

char *
comac_test_reference_filename (const comac_test_context_t *ctx,
			       const char *base_name,
			       const char *test_name,
			       const char *target_name,
			       const char *base_target_name,
			       const char *format,
			       const char *suffix,
			       const char *extension);

comac_surface_t *
comac_test_get_reference_image (comac_test_context_t *ctx,
				const char *filename,
				comac_bool_t flatten);

comac_bool_t
comac_test_mkdir (const char *path);

comac_t *
comac_test_create (comac_surface_t *surface,
		   const comac_test_context_t *ctx);

COMAC_END_DECLS

#endif
