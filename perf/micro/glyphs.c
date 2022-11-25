/*
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2009 Chris Wilson
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
 * Author: Carl D. Worth <cworth@cworth.org>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-perf.h"

static comac_time_t
do_glyphs (double font_size,
	   comac_antialias_t antialias,
	   comac_t *cr,
	   int width,
	   int height,
	   int loops)
{
    const char text[] = "the jay, pig, fox, zebra and my wolves quack";
    comac_scaled_font_t *scaled_font;
    comac_glyph_t *glyphs = NULL, *glyphs_copy;
    comac_text_extents_t extents;
    comac_font_options_t *options;
    comac_status_t status;
    double x, y;
    int num_glyphs, n;

    options = comac_font_options_create ();
    comac_font_options_set_antialias (options, antialias);
    comac_set_font_options (cr, options);
    comac_font_options_destroy (options);

    comac_select_font_face (cr,
			    "@comac:",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, font_size);
    scaled_font = comac_get_scaled_font (cr);
    status = comac_scaled_font_text_to_glyphs (scaled_font,
					       0.,
					       0.,
					       text,
					       -1,
					       &glyphs,
					       &num_glyphs,
					       NULL,
					       NULL,
					       NULL);
    if (status)
	return 0;

    glyphs_copy = comac_glyph_allocate (num_glyphs);
    if (glyphs_copy == NULL) {
	comac_glyph_free (glyphs);
	return 0;
    }

    comac_scaled_font_glyph_extents (scaled_font, glyphs, num_glyphs, &extents);

    comac_perf_timer_start ();

    while (loops--) {
	y = 0;
	do {
	    x = 0;
	    do {
		for (n = 0; n < num_glyphs; n++) {
		    glyphs_copy[n] = glyphs[n];
		    glyphs_copy[n].x += x;
		    glyphs_copy[n].y += y;
		}
		comac_show_glyphs (cr, glyphs_copy, num_glyphs);

		x += extents.width;
	    } while (x < width);
	    y += extents.height;
	} while (y < height);
    }

    comac_perf_timer_stop ();

    comac_glyph_free (glyphs);
    comac_glyph_free (glyphs_copy);

    return comac_perf_timer_elapsed ();
}

static double
count_glyphs (double font_size,
	      comac_antialias_t antialias,
	      comac_t *cr,
	      int width,
	      int height)
{
    const char text[] = "the jay, pig, fox, zebra and my wolves quack";
    comac_scaled_font_t *scaled_font;
    comac_glyph_t *glyphs = NULL;
    comac_text_extents_t extents;
    comac_font_options_t *options;
    comac_status_t status;
    int num_glyphs;
    int glyphs_per_line, lines_per_loop;

    options = comac_font_options_create ();
    comac_font_options_set_antialias (options, antialias);
    comac_set_font_options (cr, options);
    comac_font_options_destroy (options);

    comac_select_font_face (cr,
			    "@comac:",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, font_size);
    scaled_font = comac_get_scaled_font (cr);
    status = comac_scaled_font_text_to_glyphs (scaled_font,
					       0.,
					       0.,
					       text,
					       -1,
					       &glyphs,
					       &num_glyphs,
					       NULL,
					       NULL,
					       NULL);
    if (status)
	return 0;

    comac_scaled_font_glyph_extents (scaled_font, glyphs, num_glyphs, &extents);
    comac_glyph_free (glyphs);

    glyphs_per_line = num_glyphs * width / extents.width + 1;
    lines_per_loop = height / extents.height + 1;
    return glyphs_per_line * lines_per_loop / 1000.; /* kiloglyphs */
}

#define DECL(name, size, aa)                                                   \
    static comac_time_t do_glyphs##name (comac_t *cr,                          \
					 int width,                            \
					 int height,                           \
					 int loops)                            \
    {                                                                          \
	return do_glyphs (size, aa, cr, width, height, loops);                 \
    }                                                                          \
                                                                               \
    static double count_glyphs##name (comac_t *cr, int width, int height)      \
    {                                                                          \
	return count_glyphs (size, aa, cr, width, height);                     \
    }

DECL (8, 8, COMAC_ANTIALIAS_GRAY)
DECL (10, 10, COMAC_ANTIALIAS_GRAY)
DECL (12, 12, COMAC_ANTIALIAS_GRAY)
DECL (16, 16, COMAC_ANTIALIAS_GRAY)
DECL (20, 20, COMAC_ANTIALIAS_GRAY)
DECL (24, 24, COMAC_ANTIALIAS_GRAY)
DECL (32, 32, COMAC_ANTIALIAS_GRAY)
DECL (40, 40, COMAC_ANTIALIAS_GRAY)
DECL (48, 48, COMAC_ANTIALIAS_GRAY)

DECL (8ca, 8, COMAC_ANTIALIAS_SUBPIXEL)
DECL (48ca, 48, COMAC_ANTIALIAS_SUBPIXEL)

DECL (8mono, 8, COMAC_ANTIALIAS_NONE)
DECL (48mono, 48, COMAC_ANTIALIAS_NONE)

comac_bool_t
glyphs_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "glyphs", NULL);
}

void
glyphs (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_cover_sources_and_operators (perf,
					    "glyphs8mono",
					    do_glyphs8mono,
					    count_glyphs8mono);
    comac_perf_cover_sources_and_operators (perf,
					    "glyphs8",
					    do_glyphs8,
					    count_glyphs8);
    comac_perf_cover_sources_and_operators (perf,
					    "glyphs8ca",
					    do_glyphs8ca,
					    count_glyphs8ca);

    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_perf_run (perf, "glyphs8", do_glyphs8, count_glyphs8);
    comac_perf_run (perf, "glyphs10", do_glyphs10, count_glyphs10);
    comac_perf_run (perf, "glyphs12", do_glyphs12, count_glyphs12);
    comac_perf_run (perf, "glyphs16", do_glyphs16, count_glyphs16);
    comac_perf_run (perf, "glyphs20", do_glyphs20, count_glyphs20);
    comac_perf_run (perf, "glyphs24", do_glyphs24, count_glyphs24);
    comac_perf_run (perf, "glyphs32", do_glyphs32, count_glyphs32);
    comac_perf_run (perf, "glyphs40", do_glyphs40, count_glyphs40);
    comac_perf_run (perf, "glyphs48", do_glyphs48, count_glyphs48);

    comac_perf_cover_sources_and_operators (perf,
					    "glyphs48mono",
					    do_glyphs48mono,
					    count_glyphs48mono);
    comac_perf_cover_sources_and_operators (perf,
					    "glyphs48",
					    do_glyphs48,
					    count_glyphs48);
    comac_perf_cover_sources_and_operators (perf,
					    "glyphs48ca",
					    do_glyphs48ca,
					    count_glyphs48ca);
}
