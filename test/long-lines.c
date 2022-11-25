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
 * Authors: Carl D. Worth <cworth@cworth.org>
 *	    Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
 */

#include "comac-test.h"

#define LINE_WIDTH	1.
#define SIZE		10
#define LINE_NBR	6
#define WIDTH (SIZE * (LINE_NBR + 1))
#define HEIGHT (SIZE * (LINE_NBR + 1))


struct {
    double length;
    double red, green, blue;
} lines[LINE_NBR] = {
    {       100.0, 1.0, 0.0, 0.0 },
    {     10000.0, 0.0, 1.0, 0.0 },
    {    100000.0, 0.0, 0.0, 1.0 },
    {   1000000.0, 1.0, 1.0, 0.0 },
    {  10000000.0, 0.0, 1.0, 1.0 },
    { 100000000.0, 1.0, 0.0, 1.0 }
};

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    double pos;
    int i;

    /* We draw in the default black, so paint white first. */
    comac_save (cr);
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_restore (cr);

    comac_set_line_width (cr, LINE_WIDTH);

    pos = SIZE + .5;
    for (i = 0; i < LINE_NBR; i++) {
	comac_move_to (cr, pos, -lines[i].length);
	comac_line_to (cr, pos, +lines[i].length);
	comac_set_source_rgb (cr, lines[i].red, lines[i].green, lines[i].blue);
	comac_stroke (cr);
	pos += SIZE;
    }

    /* This should display a perfect vertically centered black line */
    comac_move_to (cr, -1e100, HEIGHT/2);
    comac_line_to (cr, 1e100, HEIGHT/2);
    comac_set_source_rgb (cr, 0.0, 0.0, 0.0);
    comac_stroke (cr);

    return COMAC_TEST_SUCCESS;
}

/* XFAIL: range overflow of fixed-point */
COMAC_TEST (long_lines,
	    "Test long lines"
	    "\nLong lines are not drawn due to the limitations of the internal 16.16 fixed-point coordinates",
	    "stroke, stress", /* keywords */
	    NULL, /* requirements */
	    WIDTH, HEIGHT,
	    NULL, draw)

