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

#include "tiger.inc"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    unsigned int i;

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_rgba (cr, 0.1, 0.2, 0.3, 1.0);
    comac_paint (cr);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);

    comac_translate (cr, width / 2, height / 2);
    comac_scale (cr, .85, .85);

    for (i = 0; i < ARRAY_LENGTH (tiger_commands); i++) {
	const struct command *cmd = &tiger_commands[i];
	switch (cmd->type) {
	case 'm':
	    comac_move_to (cr, cmd->x0, cmd->y0);
	    break;
	case 'l':
	    comac_line_to (cr, cmd->x0, cmd->y0);
	    break;
	case 'c':
	    comac_curve_to (cr,
			    cmd->x0,
			    cmd->y0,
			    cmd->x1,
			    cmd->y1,
			    cmd->x2,
			    cmd->y2);
	    break;
	case 'f':
	    comac_set_source_rgba (cr, cmd->x0, cmd->y0, cmd->x1, cmd->y1);
	    comac_fill (cr);
	    break;
	}
    }

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
a1_draw (comac_t *cr, int width, int height)
{
    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    return draw (cr, width, height);
}

COMAC_TEST (tiger,
	    "Check the fidelity of the rasterisation.",
	    "raster", /* keywords */
	    NULL,     /* requirements */
	    500,
	    500,
	    NULL,
	    draw)

COMAC_TEST (a1_tiger,
	    "Check the fidelity of the rasterisation.",
	    "fill",	     /* keywords */
	    "target=raster", /* requirements */
	    500,
	    500,
	    NULL,
	    a1_draw)
