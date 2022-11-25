/*
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
 */

#include "comac-test.h"

static comac_pattern_t *
create_pattern (comac_surface_t *target)
{
    comac_surface_t *surface;
    comac_pattern_t *pattern;
    comac_t *cr;
    comac_matrix_t m;

    surface = comac_surface_create_similar (target,
					    comac_surface_get_content (target),
					    1000,
					    600);
    cr = comac_create (surface);
    comac_surface_destroy (surface);

    comac_set_source_rgb (cr, 0, 1, 0);
    comac_paint (cr);

    pattern = comac_pattern_create_for_surface (comac_get_target (cr));
    comac_destroy (cr);

    comac_matrix_init_translate (&m, 0, 0.1); // y offset must be non-integer
    comac_pattern_set_matrix (pattern, &m);
    return pattern;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *pattern;

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    comac_new_path (cr);
    comac_move_to (cr, 10, 400.1);
    comac_line_to (cr, 990, 400.1);
    comac_line_to (cr, 990, 600);
    comac_line_to (cr, 10, 600);
    comac_close_path (cr);

    pattern = create_pattern (comac_get_target (cr));
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);

    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    bug_source_cu,
    "Exercises a bug discovered in the tracking of unbounded source extents",
    "fill", /* keywords */
    NULL,   /* requirements */
    1000,
    600,
    NULL,
    draw)
