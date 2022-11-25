/*
 * Copyright © 2005 Red Hat, Inc.
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
 * Author: Owen Taylor <otaylor@redhat.com>
 */

#include "comac-test.h"
#include "stdio.h"

/* The test matrix is
 *
 * A) Horizontal   B) 5°          C) 45°          D) Vertical
 * 1) Rotated 0°   2) Rotated 45° C) Rotated 90°
 * a) 2 stop       b) 3 stop
 *
 *  A1a   B1a  C1a  D1a
 *  A2a   B2a  C2a  D2a
 *  A3a   B3a  C3a  D3a
 *  A1b   B1b  C1b  D1b
 *  A2b   B2b  C2b  D2b
 *  A3b   B3b  C3b  D3b
 */

static const double gradient_angles[] = {0, 45, 90};
#define N_GRADIENT_ANGLES 3
static const double rotate_angles[] = {0, 45, 90};
#define N_ROTATE_ANGLES 3
static const int n_stops[] = {2, 3};
#define N_N_STOPS 2

#define UNIT_SIZE 6
#define UNIT_SIZE 6
#define PAD 1

#define WIDTH N_GRADIENT_ANGLES *UNIT_SIZE + (N_GRADIENT_ANGLES + 1) * PAD
#define HEIGHT                                                                 \
    N_N_STOPS *N_ROTATE_ANGLES *UNIT_SIZE +                                    \
	(N_N_STOPS * N_ROTATE_ANGLES + 1) * PAD

static void
draw_unit (comac_t *cr, double gradient_angle, double rotate_angle, int n_stops)
{
    comac_pattern_t *pattern;

    comac_rectangle (cr, 0, 0, 1, 1);
    comac_clip (cr);
    comac_new_path (cr);

    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_rectangle (cr, 0, 0, 1, 1);
    comac_fill (cr);

    comac_translate (cr, 0.5, 0.5);
    comac_scale (cr, 1 / 1.5, 1 / 1.5);
    comac_rotate (cr, rotate_angle);

    pattern = comac_pattern_create_linear (-0.5 * cos (gradient_angle),
					   -0.5 * sin (gradient_angle),
					   0.5 * cos (gradient_angle),
					   0.5 * sin (gradient_angle));

    if (n_stops == 2) {
	comac_pattern_add_color_stop_rgb (pattern, 0., 0.3, 0.3, 0.3);
	comac_pattern_add_color_stop_rgb (pattern, 1., 1.0, 1.0, 1.0);
    } else {
	comac_pattern_add_color_stop_rgb (pattern, 0., 1.0, 0.0, 0.0);
	comac_pattern_add_color_stop_rgb (pattern, 0.5, 1.0, 1.0, 1.0);
	comac_pattern_add_color_stop_rgb (pattern, 1., 0.0, 0.0, 1.0);
    }

    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);
    comac_rectangle (cr, -0.5, -0.5, 1, 1);
    comac_fill (cr);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    int i, j, k;

    comac_set_source_rgb (cr, 0.5, 0.5, 0.5);
    comac_paint (cr);

    for (i = 0; i < N_GRADIENT_ANGLES; i++)
	for (j = 0; j < N_ROTATE_ANGLES; j++)
	    for (k = 0; k < N_N_STOPS; k++) {
		comac_save (cr);
		comac_translate (cr,
				 PAD + (PAD + UNIT_SIZE) * i,
				 PAD + (PAD + UNIT_SIZE) *
					   (N_ROTATE_ANGLES * k + j));
		comac_scale (cr, UNIT_SIZE, UNIT_SIZE);

		draw_unit (cr,
			   gradient_angles[i] * M_PI / 180.,
			   rotate_angles[j] * M_PI / 180.,
			   n_stops[k]);
		comac_restore (cr);
	    }

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (linear_gradient,
	    "Tests the drawing of linear gradients",
	    "gradient", /* keywords */
	    NULL,	/* requirements */
	    WIDTH,
	    HEIGHT,
	    NULL,
	    draw)
