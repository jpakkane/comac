/*
 * Copyright © 2006 Mozilla Corporation
 * Copyright © 2006 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * the authors not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The authors make no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Vladimir Vukicevic <vladimir@pobox.com>
 *	    Carl Worth <cworth@cworth.org>
 */

#ifndef _COMAC_PERF_H_
#define _COMAC_PERF_H_

#include "comac-boilerplate.h"
#include "../src/comac-time-private.h"
#include <stdio.h>

typedef struct _comac_stats {
    comac_time_t min_ticks;
    comac_time_t median_ticks;
    double ticks_per_ms;
    double std_dev;
    int iterations;
    comac_time_t *values;
} comac_stats_t;

typedef struct _comac_histogram {
    int width, height, max_count;
    int num_columns, num_rows;
    comac_time_t min_value, max_value;
    int *columns;
} comac_histogram_t;

/* timers */

void
comac_perf_timer_start (void);

void
comac_perf_timer_stop (void);

typedef void (*comac_perf_timer_synchronize_t) (void *closure);

void
comac_perf_timer_set_synchronize (comac_perf_timer_synchronize_t synchronize,
				  void *closure);

comac_time_t
comac_perf_timer_elapsed (void);

/* yield */

void
comac_perf_yield (void);

/* running a test case */
typedef struct _comac_perf {
    FILE *summary;
    comac_bool_t summary_continuous;

    /* Options from command-line */
    unsigned int iterations;
    comac_bool_t exact_iterations;
    comac_bool_t raw;
    comac_bool_t list_only;
    comac_bool_t observe;
    char **names;
    unsigned int num_names;
    char **exclude_names;
    unsigned int num_exclude_names;
    comac_bool_t exact_names;

    double ms_per_iteration;
    comac_bool_t fast_and_sloppy;

    unsigned int tile_size;

    /* Stuff used internally */
    comac_time_t *times;
    const comac_boilerplate_target_t **targets;
    int num_targets;
    const comac_boilerplate_target_t *target;
    comac_bool_t has_described_backend;
    unsigned int test_number;
    unsigned int size;
    comac_t *cr;
} comac_perf_t;

typedef comac_time_t (*comac_perf_func_t) (comac_t *cr,
					   int width,
					   int height,
					   int loops);

typedef double (*comac_count_func_t) (comac_t *cr, int width, int height);

comac_bool_t
comac_perf_can_run (comac_perf_t *perf,
		    const char *name,
		    comac_bool_t *is_explicit);

void
comac_perf_run (comac_perf_t *perf,
		const char *name,
		comac_perf_func_t perf_func,
		comac_count_func_t count_func);

void
comac_perf_cover_sources_and_operators (comac_perf_t *perf,
					const char *name,
					comac_perf_func_t perf_func,
					comac_count_func_t count_func);

/* reporter convenience routines */

typedef struct _test_report {
    int id;
    int fileno;
    const char *configuration;
    char *backend;
    char *content;
    char *name;
    int size;

    /* The samples only exists for "raw" reports */
    comac_time_t *samples;
    unsigned int samples_size;
    unsigned int samples_count;

    /* The stats are either read directly or computed from samples.
     * If the stats have not yet been computed from samples, then
     * iterations will be 0. */
    comac_stats_t stats;
} test_report_t;

typedef struct _test_diff {
    test_report_t **tests;
    int num_tests;
    double min;
    double max;
    double change;
} test_diff_t;

typedef struct _comac_perf_report {
    char *configuration;
    const char *name;
    int fileno;
    test_report_t *tests;
    int tests_size;
    int tests_count;
} comac_perf_report_t;

typedef enum {
    TEST_REPORT_STATUS_SUCCESS,
    TEST_REPORT_STATUS_COMMENT,
    TEST_REPORT_STATUS_ERROR
} test_report_status_t;

void
comac_perf_report_load (comac_perf_report_t *report,
			const char *filename,
			int id,
			int (*cmp) (const void *, const void *));

void
comac_perf_report_sort_and_compute_stats (comac_perf_report_t *report,
					  int (*cmp) (const void *,
						      const void *));

int
test_report_cmp_backend_then_name (const void *a, const void *b);

int
test_report_cmp_name (const void *a, const void *b);

#define COMAC_PERF_ENABLED_DECL(func)                                          \
    comac_bool_t (func##_enabled) (comac_perf_t * perf)
#define COMAC_PERF_RUN_DECL(func)                                              \
    void (func) (comac_perf_t * perf, comac_t * cr, int width, int height)

#define COMAC_PERF_DECL(func)                                                  \
    COMAC_PERF_RUN_DECL (func);                                                \
    COMAC_PERF_ENABLED_DECL (func)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

COMAC_PERF_DECL (fill);
COMAC_PERF_DECL (paint);
COMAC_PERF_DECL (paint_with_alpha);
COMAC_PERF_DECL (mask);
COMAC_PERF_DECL (stroke);
COMAC_PERF_DECL (subimage_copy);
COMAC_PERF_DECL (disjoint);
COMAC_PERF_DECL (hatching);
COMAC_PERF_DECL (tessellate);
COMAC_PERF_DECL (text);
COMAC_PERF_DECL (glyphs);
COMAC_PERF_DECL (hash_table);
COMAC_PERF_DECL (pattern_create_radial);
COMAC_PERF_DECL (zrusin);
COMAC_PERF_DECL (world_map);
COMAC_PERF_DECL (box_outline);
COMAC_PERF_DECL (mosaic);
COMAC_PERF_DECL (long_lines);
COMAC_PERF_DECL (unaligned_clip);
COMAC_PERF_DECL (rectangles);
COMAC_PERF_DECL (rounded_rectangles);
COMAC_PERF_DECL (long_dashed_lines);
COMAC_PERF_DECL (composite_checker);
COMAC_PERF_DECL (twin);
COMAC_PERF_DECL (dragon);
COMAC_PERF_DECL (pythagoras_tree);
COMAC_PERF_DECL (intersections);
COMAC_PERF_DECL (spiral);
COMAC_PERF_DECL (wave);
COMAC_PERF_DECL (many_strokes);
COMAC_PERF_DECL (wide_strokes);
COMAC_PERF_DECL (many_fills);
COMAC_PERF_DECL (wide_fills);
COMAC_PERF_DECL (many_curves);
COMAC_PERF_DECL (curve);
COMAC_PERF_DECL (a1_curve);
COMAC_PERF_DECL (line);
COMAC_PERF_DECL (a1_line);
COMAC_PERF_DECL (pixel);
COMAC_PERF_DECL (a1_pixel);
COMAC_PERF_DECL (sierpinski);
COMAC_PERF_DECL (fill_clip);
COMAC_PERF_DECL (tiger);

#endif
