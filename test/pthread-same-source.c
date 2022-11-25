/*
 * Copyright 2009 Benjamin Otte
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Benjamin Otte <otte@gnome.org>
 */

#define GENERATE_REFERENCE 0

#include "comac-test.h"
#if ! GENERATE_REFERENCE
#include <pthread.h>
#endif

#define N_THREADS 8

#define WIDTH 64
#define HEIGHT 8

typedef struct {
    comac_surface_t *target;
    comac_surface_t *source;
    int id;
} thread_data_t;

static void *
draw_thread (void *arg)
{
    thread_data_t *thread_data = arg;
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    comac_matrix_t pattern_matrix = {2, 0, 0, 2, 0, 0};
    comac_t *cr;
    int x, y;

    cr = comac_create (thread_data->target);
    comac_surface_destroy (thread_data->target);

    pattern = comac_pattern_create_for_surface (thread_data->source);
    comac_surface_destroy (thread_data->source);
    comac_pattern_set_extend (pattern, thread_data->id % 4);
    comac_pattern_set_filter (pattern,
			      thread_data->id >= 4 ? COMAC_FILTER_BILINEAR
						   : COMAC_FILTER_NEAREST);
    comac_pattern_set_matrix (pattern, &pattern_matrix);

    for (y = 0; y < HEIGHT; y++) {
	for (x = 0; x < WIDTH; x++) {
	    comac_save (cr);
	    comac_translate (cr, 4 * x + 1, 4 * y + 1);
	    comac_rectangle (cr, 0, 0, 2, 2);
	    comac_set_source (cr, pattern);
	    comac_fill (cr);
	    comac_restore (cr);
	}
    }
    comac_pattern_destroy (pattern);

    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return surface;
}

static comac_surface_t *
create_source (comac_surface_t *similar)
{
    comac_surface_t *source;
    comac_t *cr;
    double colors[4][3] = {{0.75, 0, 0},
			   {0, 0.75, 0},
			   {0, 0, 0.75},
			   {0.75, 0.75, 0}};
    int i;

    source =
	comac_surface_create_similar (similar, COMAC_CONTENT_COLOR_ALPHA, 2, 2);

    cr = comac_create (source);
    comac_surface_destroy (source);

    for (i = 0; i < 4; i++) {
	comac_set_source_rgb (cr, colors[i][0], colors[i][1], colors[i][2]);
	comac_rectangle (cr, i % 2, i / 2, 1, 1);
	comac_fill (cr);
    }

    source = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return source;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
#if ! GENERATE_REFERENCE
    pthread_t threads[N_THREADS];
#endif
    thread_data_t thread_data[N_THREADS];
    comac_test_status_t test_status = COMAC_TEST_SUCCESS;
    comac_surface_t *source;
    comac_status_t status;
    int i;

    source = create_source (comac_get_target (cr));
    status = comac_surface_status (source);
    if (status) {
	comac_surface_destroy (source);
	return comac_test_status_from_status (comac_test_get_context (cr),
					      status);
    }

    comac_set_source_rgb (cr, 0.5, 0.5, 0.5);
    comac_paint (cr);

    for (i = 0; i < N_THREADS; i++) {
	thread_data[i].target =
	    comac_surface_create_similar (comac_get_target (cr),
					  COMAC_CONTENT_COLOR_ALPHA,
					  4 * WIDTH,
					  4 * HEIGHT);
	thread_data[i].source = comac_surface_reference (source);
	thread_data[i].id = i;
#if ! GENERATE_REFERENCE
	if (pthread_create (&threads[i], NULL, draw_thread, &thread_data[i]) !=
	    0) {
	    threads[i] = pthread_self (); /* to indicate error */
	    comac_surface_destroy (thread_data[i].target);
	    comac_surface_destroy (thread_data[i].source);
	    test_status = COMAC_TEST_FAILURE;
	    break;
	}
#else
	{
	    comac_surface_t *surface = draw_thread (&thread_data[i]);
	    comac_set_source_surface (cr, surface, 0, 0);
	    comac_surface_destroy (surface);
	    comac_paint (cr);

	    comac_translate (cr, 0, 4 * HEIGHT);
	}
#endif
    }

    comac_surface_destroy (source);

#if ! GENERATE_REFERENCE
    for (i = 0; i < N_THREADS; i++) {
	void *surface;

	if (pthread_equal (threads[i], pthread_self ()))
	    break;

	if (pthread_join (threads[i], &surface) == 0) {
	    comac_set_source_surface (cr, surface, 0, 0);
	    comac_surface_destroy (surface);
	    comac_paint (cr);

	    comac_translate (cr, 0, 4 * HEIGHT);
	} else {
	    test_status = COMAC_TEST_FAILURE;
	}
    }
#endif

    return test_status;
}

COMAC_TEST (pthread_same_source,
	    "Use the same source for drawing in different threads",
	    "threads", /* keywords */
	    NULL,      /* requirements */
	    4 * WIDTH,
	    4 * HEIGHT * N_THREADS,
	    NULL,
	    draw)
