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

#include "comac-test.h"
#include <pthread.h>

#define N_THREADS 8

#define WIDTH 64
#define HEIGHT 8

static void *
draw_thread (void *arg)
{
    comac_surface_t *surface = arg;
    comac_t *cr;
    int x, y;

    cr = comac_create (surface);
    comac_surface_destroy (surface);

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            comac_rectangle (cr, x, y, 1, 1);
            comac_set_source_rgba (cr, 0, 0.75, 0.75, (double) x / WIDTH);
            comac_fill (cr);
        }
    }

    surface = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return surface;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    pthread_t threads[N_THREADS];
    comac_test_status_t test_status = COMAC_TEST_SUCCESS;
    int i;

    for (i = 0; i < N_THREADS; i++) {
	comac_surface_t *surface;

        surface = comac_surface_create_similar (comac_get_target (cr),
						COMAC_CONTENT_COLOR,
						WIDTH, HEIGHT);
        if (pthread_create (&threads[i], NULL, draw_thread, surface) != 0) {
	    threads[i] = pthread_self ();
            test_status = comac_test_status_from_status (comac_test_get_context (cr),
							 comac_surface_status (surface));
            comac_surface_destroy (surface);
	    break;
        }
    }

    for (i = 0; i < N_THREADS; i++) {
	void *surface;

        if (pthread_equal (threads[i], pthread_self ()))
            break;

        if (pthread_join (threads[i], &surface) == 0) {
	    comac_set_source_surface (cr, surface, 0, 0);
	    comac_surface_destroy (surface);
	    comac_paint (cr);

	    comac_translate (cr, 0, HEIGHT);
	} else {
            test_status = COMAC_TEST_FAILURE;
	}
    }

    return test_status;
}

COMAC_TEST (pthread_similar,
	    "Draw lots of 1x1 rectangles on similar surfaces in lots of threads",
	    "threads", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT * N_THREADS,
	    NULL, draw)
