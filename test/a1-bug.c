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

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    static const struct point {
	double x;
	double y;
    } xy[] = {
	{627.016212, 221.749777},
	{756.120787, 221.749777},
	{756.120787, 557.602766},
	{626.952721, 557.602766},
	{626.548456, 493.315729},
    };
    unsigned int i;

    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    for (i = 0; i < ARRAY_LENGTH (xy); i++)
	comac_line_to (cr, xy[i].x, xy[i].y);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_fill_preserve (cr);

    comac_set_antialias (cr, COMAC_ANTIALIAS_NONE);
    comac_set_source_rgb (cr, 0, 1, 0);
    comac_fill (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (a1_bug,
	    "Check the fidelity of the rasterisation.",
	    "a1, raster",    /* keywords */
	    "target=raster", /* requirements */
	    1000,
	    800,
	    NULL,
	    draw)
