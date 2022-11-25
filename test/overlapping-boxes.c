/*
 * Copyright © 2011 Intel Corporation
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
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

/* Not strictly overlapping, but it does highlight the error in
 * an optimisation of fill-box handling that I frequently am
 * tempted to write.
 */

#include "comac-test.h"

#define WIDTH (20)
#define HEIGHT (20)

static void
border (comac_t *cr)
{
    comac_rectangle (cr, 1, 1, 8, 8);
    comac_rectangle (cr, 1.25, 1.25, 7.5, 7.5);
    comac_rectangle (cr, 1.75, 1.75, 6.5, 6.5);
    comac_rectangle (cr, 2, 2, 6, 6);
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    border (cr);
    comac_set_source_rgb (cr, 1, 0, 0);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_fill (cr);

    comac_translate (cr, 10, 0);

    border (cr);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_fill (cr);

    comac_translate (cr, 0, 10);

    comac_rectangle (cr, 0, 0, 10, 10);
    comac_clip (cr);

    border (cr);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_set_operator (cr, COMAC_OPERATOR_IN);
    comac_fill (cr);

    comac_reset_clip (cr);

    comac_translate (cr, -10, 0);

    comac_rectangle (cr, 0, 0, 10, 10);
    comac_clip (cr);

    border (cr);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_set_operator (cr, COMAC_OPERATOR_CLEAR);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    overlapping_boxes,
    "A sub-pixel double border to highlight the danger in an easy optimisation",
    "fill", /* keywords */
    NULL,   /* requirements */
    WIDTH,
    HEIGHT,
    NULL,
    draw)
