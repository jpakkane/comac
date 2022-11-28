/*
 * Copyright © 2010 M Joonas Pihlaja
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
 * Author: M Joonas Pihlaja <jpihlaja@cc.helsinki.fi>
 */
#include "comac-test.h"

/* Test that we can simultaneously downscale and extend a surface
 * pattern.  Reported by Franz Schmid to the comac mailing list as a
 * regression in 1.9.6:
 *
 * https://lists.cairographics.org/archives/comac/2010-February/019492.html
 */

static comac_test_status_t
draw_with_extend (comac_t *cr, int w, int h, comac_extend_t extend)
{
    comac_pattern_t *pattern;
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_save (cr);

    /* When the destination surface is created by comac-test-suite to
     * test device-offset, it is bigger than w x h. This test expects
     * the group to have a size which is exactly w x h, so it must
     * clip to the this rectangle to guarantee that the group will
     * have the correct size.
     */
    comac_rectangle (cr, 0, 0, w, h);
    comac_clip (cr);

    comac_push_group_with_content (cr, COMAC_CONTENT_COLOR);
    {
	/* A two by two checkerboard with black, red and yellow
         * cells. */
	comac_set_source_rgb (cr, 1, 0, 0);
	comac_rectangle (cr, w / 2, 0, w - w / 2, h / 2);
	comac_fill (cr);
	comac_set_source_rgb (cr, 1, 1, 0);
	comac_rectangle (cr, 0, h / 2, w / 2, h - h / 2);
	comac_fill (cr);
    }
    pattern = comac_pop_group (cr);
    comac_pattern_set_extend (pattern, extend);

    comac_restore (cr);

    comac_scale (cr, 0.5, 0.5);
    comac_set_source (cr, pattern);
    comac_paint (cr);

    comac_pattern_destroy (pattern);
    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
draw_repeat (comac_t *cr, int w, int h)
{
    return draw_with_extend (cr, w, h, COMAC_EXTEND_REPEAT);
}
static comac_test_status_t
draw_none (comac_t *cr, int w, int h)
{
    return draw_with_extend (cr, w, h, COMAC_EXTEND_NONE);
}
static comac_test_status_t
draw_reflect (comac_t *cr, int w, int h)
{
    return draw_with_extend (cr, w, h, COMAC_EXTEND_REFLECT);
}
static comac_test_status_t
draw_pad (comac_t *cr, int w, int h)
{
    return draw_with_extend (cr, w, h, COMAC_EXTEND_PAD);
}

COMAC_TEST (
    surface_pattern_scale_down_extend_repeat,
    "Test interaction of downscaling a surface pattern and extend-repeat",
    "pattern, transform, extend", /* keywords */
    NULL,			  /* requirements */
    100,
    100,
    NULL,
    draw_repeat)
COMAC_TEST (surface_pattern_scale_down_extend_none,
	    "Test interaction of downscaling a surface pattern and extend-none",
	    "pattern, transform, extend", /* keywords */
	    NULL,			  /* requirements */
	    100,
	    100,
	    NULL,
	    draw_none)
COMAC_TEST (
    surface_pattern_scale_down_extend_reflect,
    "Test interaction of downscaling a surface pattern and extend-reflect",
    "pattern, transform, extend", /* keywords */
    NULL,			  /* requirements */
    100,
    100,
    NULL,
    draw_reflect)
COMAC_TEST (surface_pattern_scale_down_extend_pad,
	    "Test interaction of downscaling a surface pattern and extend-pad",
	    "pattern, transform, extend", /* keywords */
	    NULL,			  /* requirements */
	    100,
	    100,
	    NULL,
	    draw_pad)
