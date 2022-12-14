/*
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

/* Extracted from a test case reported by Jeff Muizelaar found whilst running
 * firefox http://people.mozilla.com/~jmuizelaar/BerlinDistricts-check.svg
 */

#include "comac-test.h"

#define WIDTH 205
#define HEIGHT 260

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const double dash[2] = {.5, .5};

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_set_source_rgb (cr, 1., 0., 0);

    /* By adjusting the miter limit, we can see variations on the artifact.
     * comac_set_miter_limit (cr, 4.);
     */

    comac_translate (cr, -720, -484);
    comac_scale (cr, 2.5, 2.5);

    comac_set_dash (cr, dash, 2, 0);

    comac_move_to (cr, 293.622, 330);
    comac_line_to (cr, 293.703, 337.028);
    comac_line_to (cr, 297.45, 336.851);
    comac_line_to (cr, 308.88, 342.609);
    comac_line_to (cr, 309.736, 346.107);
    comac_line_to (cr, 312.972, 348.128);
    comac_line_to (cr, 312.977, 353.478);
    comac_line_to (cr, 322.486, 359.355);
    comac_line_to (cr, 320.831, 363.642);
    comac_line_to (cr, 315.175, 367.171);
    comac_line_to (cr, 308.987, 365.715);
    comac_line_to (cr, 301.3, 365.964);
    comac_line_to (cr, 304.712, 368.852);
    comac_line_to (cr, 305.349, 373.022);
    comac_line_to (cr, 303.211, 376.551);
    comac_line_to (cr, 304.915, 382.855);
    comac_line_to (cr, 323.715, 400.475);
    comac_line_to (cr, 355.323, 424.072);
    comac_line_to (cr, 443.078, 426.534);
    comac_line_to (cr, 455.26, 400.603);
    comac_line_to (cr, 471.924, 392.604);
    comac_line_to (cr, 478.556, 390.797);
    comac_line_to (cr, 477.715, 386);
    comac_line_to (cr, 456.807, 376.507);
    comac_line_to (cr, 449.134, 368.722);
    comac_line_to (cr, 449.147, 365.847);
    comac_line_to (cr, 439.981, 361.692);
    comac_line_to (cr, 439.994, 358.603);
    comac_line_to (cr, 454.645, 336.128);
    comac_line_to (cr, 434.995, 324.005);
    comac_line_to (cr, 423.884, 319.354);
    comac_line_to (cr, 421.098, 312.569);
    comac_line_to (cr, 424.291, 305.997);
    comac_line_to (cr, 431.308, 305.069);
    comac_line_to (cr, 437.257, 296.882);
    comac_line_to (cr, 448.544, 296.808);
    comac_line_to (cr, 452.113, 290.651);
    comac_line_to (cr, 448.469, 285.483);
    comac_line_to (cr, 442.903, 282.877);
    comac_line_to (cr, 447.798, 281.124);
    comac_line_to (cr, 454.622, 274.911);
    comac_line_to (cr, 449.491, 269.978);
    comac_line_to (cr, 443.666, 253.148);
    comac_line_to (cr, 445.741, 250.834);
    comac_line_to (cr, 441.87, 247.131);
    comac_line_to (cr, 436.932, 246.203);
    comac_line_to (cr, 430.5, 251.252);
    comac_line_to (cr, 427.483, 250.751);
    comac_line_to (cr, 427.26, 253.572);
    comac_line_to (cr, 423.621, 255.539);
    comac_line_to (cr, 423.824, 257.933);
    comac_line_to (cr, 425.239, 259.582);
    comac_line_to (cr, 422.385, 261.443);
    comac_line_to (cr, 421.665, 260.53);
    comac_line_to (cr, 419.238, 262.819);
    comac_line_to (cr, 418.731, 257.849);
    comac_line_to (cr, 419.72, 255.227);
    comac_line_to (cr, 418.786, 250.258);
    comac_line_to (cr, 405.685, 235.254);
    comac_line_to (cr, 427.167, 215.127);
    comac_line_to (cr, 413.852, 196.281);
    comac_line_to (cr, 420.177, 192.379);
    comac_line_to (cr, 419.885, 185.701);
    comac_line_to (cr, 413.401, 185.428);
    comac_line_to (cr, 407.985, 186.863);
    comac_line_to (cr, 397.11, 189.112);
    comac_line_to (cr, 390.505, 186.664);
    comac_line_to (cr, 388.527, 183.694);
    comac_line_to (cr, 336.503, 221.048);
    comac_line_to (cr, 367.028, 241.656);
    comac_line_to (cr, 365.103, 244.117);
    comac_line_to (cr, 364.886, 246.792);
    comac_line_to (cr, 361.467, 247.119);
    comac_line_to (cr, 360.396, 245.525);
    comac_line_to (cr, 356.336, 245.638);
    comac_line_to (cr, 353.344, 242.122);
    comac_line_to (cr, 347.149, 242.876);
    comac_line_to (cr, 341.809, 256.652);
    comac_line_to (cr, 342.232, 268.72);
    comac_line_to (cr, 329.579, 269.095);
    comac_line_to (cr, 327.001, 271.009);
    comac_line_to (cr, 325.579, 275.598);
    comac_line_to (cr, 318.941, 277.313);
    comac_line_to (cr, 306.048, 277.231);
    comac_line_to (cr, 304.071, 276.27);
    comac_line_to (cr, 301.153, 277.175);
    comac_line_to (cr, 293.52, 277.529);
    comac_line_to (cr, 290.682, 281.947);
    comac_line_to (cr, 293.911, 286.63);
    comac_line_to (cr, 302.417, 290.547);
    comac_line_to (cr, 303.521, 294.73);
    comac_line_to (cr, 307.787, 298.088);
    comac_line_to (cr, 311.718, 299.126);
    comac_line_to (cr, 313.255, 302.146);
    comac_line_to (cr, 314.6, 306.206);
    comac_line_to (cr, 322.603, 308.96);
    comac_line_to (cr, 321.718, 314.477);
    comac_line_to (cr, 319.596, 320.341);
    comac_line_to (cr, 300.689, 323.69);
    comac_line_to (cr, 301.232, 326.789);
    comac_line_to (cr, 293.622, 330);
    comac_close_path (cr);

    comac_stroke (cr);
    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (
    leaky_dashed_stroke,
    "Exercises bug in which a dashed stroke leaks in from outside the surface",
    "dash, stroke", /* keywords */
    NULL,	    /* requirements */
    WIDTH,
    HEIGHT,
    NULL,
    draw)
