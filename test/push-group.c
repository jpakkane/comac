/*
 * Copyright © 2005 Mozilla Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Mozilla Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Mozilla Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * MOZILLA CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MOZILLA CORPORATION BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Vladimir Vukicevic <vladimir@pobox.com>
 */

#include "comac-test.h"

#define UNIT_SIZE 100
#define PAD 5
#define INNER_PAD 10

#define WIDTH (UNIT_SIZE + PAD) + PAD
#define HEIGHT (UNIT_SIZE + PAD) + PAD

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_pattern_t *gradient;
    int i, j;

    gradient = comac_pattern_create_linear (UNIT_SIZE - (INNER_PAD*2), 0,
                                            UNIT_SIZE - (INNER_PAD*2), UNIT_SIZE - (INNER_PAD*2));
    comac_pattern_add_color_stop_rgba (gradient, 0.0, 0.3, 0.3, 0.3, 1.0);
    comac_pattern_add_color_stop_rgba (gradient, 1.0, 1.0, 1.0, 1.0, 1.0);

    for (j = 0; j < 1; j++) {
        for (i = 0; i < 1; i++) {
            double x = (i * UNIT_SIZE) + (i + 1) * PAD;
            double y = (j * UNIT_SIZE) + (j + 1) * PAD;

            comac_save (cr);

            comac_translate (cr, x, y);

            /* draw a gradient background */
            comac_save (cr);
            comac_translate (cr, INNER_PAD, INNER_PAD);
            comac_new_path (cr);
            comac_rectangle (cr, 0, 0,
                             UNIT_SIZE - (INNER_PAD*2), UNIT_SIZE - (INNER_PAD*2));
            comac_set_source (cr, gradient);
            comac_fill (cr);
            comac_restore (cr);

            /* clip to the unit size */
            comac_rectangle (cr, 0, 0,
                             UNIT_SIZE, UNIT_SIZE);
            comac_clip (cr);

            comac_rectangle (cr, 0, 0,
                             UNIT_SIZE, UNIT_SIZE);
            comac_set_source_rgba (cr, 0, 0, 0, 1);
            comac_set_line_width (cr, 2);
            comac_stroke (cr);

            /* start a group */
            comac_push_group (cr);

            /* draw diamond */
            comac_move_to (cr, UNIT_SIZE / 2, 0);
            comac_line_to (cr, UNIT_SIZE    , UNIT_SIZE / 2);
            comac_line_to (cr, UNIT_SIZE / 2, UNIT_SIZE);
            comac_line_to (cr, 0            , UNIT_SIZE / 2);
            comac_close_path (cr);
            comac_set_source_rgba (cr, 0, 0, 1, 1);
            comac_fill (cr);

            /* draw circle */
            comac_arc (cr,
                       UNIT_SIZE / 2, UNIT_SIZE / 2,
                       UNIT_SIZE / 3.5,
                       0, M_PI * 2);
            comac_set_source_rgba (cr, 1, 0, 0, 1);
            comac_fill (cr);

            comac_pop_group_to_source (cr);
            comac_paint_with_alpha (cr, 0.5);

            comac_restore (cr);
        }
    }

    comac_pattern_destroy (gradient);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (push_group,
	    "Verify that comac_push_group works.",
	    "group", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)
