/*
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2011 Intel Corporation
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
 * Authors:
 *	Carl D. Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comac-test.h"

#define TEXT_SIZE 12
#define SIZE 60 /* needs to be big to check large area effects (dithering) */
#define PAD 2

#define TT_SIZE 100
#define TT_PAD 5
#define TT_FONT_SIZE 32.0

#define GENERATE_REF 0

static uint32_t data[16] = {
    0xffffffff, 0xffffffff,		0xffff0000, 0xffff0000,
    0xffffffff, 0xffffffff,		0xffff0000, 0xffff0000,

    0xff00ff00, 0xff00ff00,		0xff0000ff, 0xff0000ff,
    0xff00ff00, 0xff00ff00,		0xff0000ff, 0xff0000ff
};

static const char *unique_id = "data";

static const char *png_filename = "romedalen.png";

static comac_t *
paint (comac_t *cr)
{
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_paint (cr);

    comac_translate (cr, 2, 2);
    comac_scale (cr, 0.5, 0.5);

    comac_set_source_rgb (cr, 1, 0, 0);
    comac_paint (cr);

    return cr;
}

static comac_t *
paint_alpha (comac_t *cr)
{
    comac_surface_t *surface;

    surface = comac_image_surface_create_for_data ((unsigned char *) data,
						   COMAC_FORMAT_RGB24, 4, 4, 16);
    comac_surface_set_mime_data (surface, COMAC_MIME_TYPE_UNIQUE_ID,
				 (unsigned char *)unique_id, strlen(unique_id),
				 NULL, NULL);

    comac_test_paint_checkered (cr);

    comac_scale (cr, 4, 4);

    comac_set_source_surface (cr, surface, 2 , 2);
    comac_pattern_set_filter (comac_get_source (cr), COMAC_FILTER_NEAREST);
    comac_paint_with_alpha (cr, 0.5);

    comac_surface_finish (surface); /* data will go out of scope */
    comac_surface_destroy (surface);

    return cr;
}

static comac_t *
paint_alpha_solid_clip (comac_t *cr)
{
    comac_test_paint_checkered (cr);

    comac_rectangle (cr, 2.5, 2.5, 27, 27);
    comac_clip (cr);

    comac_set_source_rgb (cr, 1., 0.,0.);
    comac_paint_with_alpha (cr, 0.5);

    return cr;
}

static comac_t *
paint_alpha_clip (comac_t *cr)
{
    comac_surface_t *surface;

    surface = comac_image_surface_create_for_data ((unsigned char *) data,
						   COMAC_FORMAT_RGB24, 4, 4, 16);
    comac_surface_set_mime_data (surface, COMAC_MIME_TYPE_UNIQUE_ID,
				 (unsigned char *)unique_id, strlen(unique_id),
				 NULL, NULL);

    comac_test_paint_checkered (cr);

    comac_rectangle (cr, 10.5, 10.5, 11, 11);
    comac_clip (cr);

    comac_scale (cr, 4, 4);

    comac_set_source_surface (cr, surface, 2 , 2);
    comac_pattern_set_filter (comac_get_source (cr), COMAC_FILTER_NEAREST);
    comac_paint_with_alpha (cr, 0.5);

    comac_surface_finish (surface); /* data will go out of scope */
    comac_surface_destroy (surface);

    return cr;
}

static comac_t *
paint_alpha_clip_mask (comac_t *cr)
{
    comac_surface_t *surface;

    surface = comac_image_surface_create_for_data ((unsigned char *) data,
						   COMAC_FORMAT_RGB24, 4, 4, 16);
    comac_surface_set_mime_data (surface, COMAC_MIME_TYPE_UNIQUE_ID,
				 (unsigned char *)unique_id, strlen(unique_id),
				 NULL, NULL);

    comac_test_paint_checkered (cr);

    comac_move_to (cr, 16, 5);
    comac_line_to (cr, 5, 16);
    comac_line_to (cr, 16, 27);
    comac_line_to (cr, 27, 16);
    comac_clip (cr);

    comac_scale (cr, 4, 4);

    comac_set_source_surface (cr, surface, 2 , 2);
    comac_pattern_set_filter (comac_get_source (cr), COMAC_FILTER_NEAREST);
    comac_paint_with_alpha (cr, 0.5);

    comac_surface_finish (surface); /* data will go out of scope */
    comac_surface_destroy (surface);

    return cr;
}

static comac_t *
select_font_face (comac_t *cr)
{
    /* We draw in the default black, so paint white first. */
    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);

    comac_set_source_rgb (cr, 0, 0, 0); /* black */

    comac_set_font_size (cr, TEXT_SIZE);
    comac_move_to (cr, 0, TEXT_SIZE);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Serif",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_show_text (cr, "i-am-serif");

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_show_text (cr, " i-am-sans");

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans Mono",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_show_text (cr, " i-am-mono");

    return cr;
}

static comac_t *
fill_alpha (comac_t *cr)
{
    const double alpha = 1./3;
    int n;

    /* flatten to white */
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    /* square */
    comac_rectangle (cr, PAD, PAD, SIZE, SIZE);
    comac_set_source_rgba (cr, 1, 0, 0, alpha);
    comac_fill (cr);

    /* circle */
    comac_translate (cr, SIZE + 2 * PAD, 0);
    comac_arc (cr, PAD + SIZE / 2., PAD + SIZE / 2., SIZE / 2., 0, 2 * M_PI);
    comac_set_source_rgba (cr, 0, 1, 0, alpha);
    comac_fill (cr);

    /* triangle */
    comac_translate (cr, 0, SIZE + 2 * PAD);
    comac_move_to (cr, PAD + SIZE / 2, PAD);
    comac_line_to (cr, PAD + SIZE, PAD + SIZE);
    comac_line_to (cr, PAD, PAD + SIZE);
    comac_set_source_rgba (cr, 0, 0, 1, alpha);
    comac_fill (cr);

    /* star */
    comac_translate (cr, -(SIZE + 2 * PAD) + SIZE/2., SIZE/2.);
    for (n = 0; n < 5; n++) {
	comac_line_to (cr,
		       SIZE/2 * cos (2*n * 2*M_PI / 10),
		       SIZE/2 * sin (2*n * 2*M_PI / 10));

	comac_line_to (cr,
		       SIZE/4 * cos ((2*n+1)*2*M_PI / 10),
		       SIZE/4 * sin ((2*n+1)*2*M_PI / 10));
    }
    comac_set_source_rgba (cr, 0, 0, 0, alpha);
    comac_fill (cr);

    return cr;
}

static comac_t *
self_intersecting (comac_t *cr)
{
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    comac_translate (cr, 1.0, 1.0);

    comac_set_source_rgb (cr, 1, 0, 0); /* red */

    /* First draw the desired shape with a fill */
    comac_rectangle (cr, 0.5, 0.5,  4.0, 4.0);
    comac_rectangle (cr, 3.5, 3.5,  4.0, 4.0);
    comac_rectangle (cr, 3.5, 1.5, -2.0, 2.0);
    comac_rectangle (cr, 6.5, 4.5, -2.0, 2.0);

    comac_fill (cr);

    /* Then try the same thing with a stroke */
    comac_translate (cr, 0, 10);
    comac_move_to (cr, 1.0, 1.0);
    comac_rel_line_to (cr,  3.0,  0.0);
    comac_rel_line_to (cr,  0.0,  6.0);
    comac_rel_line_to (cr,  3.0,  0.0);
    comac_rel_line_to (cr,  0.0, -3.0);
    comac_rel_line_to (cr, -6.0,  0.0);
    comac_close_path (cr);

    comac_set_line_width (cr, 1.0);
    comac_stroke (cr);

    return cr;
}

static void
draw_text_transform (comac_t *cr)
{
    comac_matrix_t tm;

    /* skew */
    comac_matrix_init (&tm, 1, 0,
                       -0.25, 1,
                       0, 0);
    comac_matrix_scale (&tm, TT_FONT_SIZE, TT_FONT_SIZE);
    comac_set_font_matrix (cr, &tm);

    comac_new_path (cr);
    comac_move_to (cr, 50, TT_SIZE-TT_PAD);
    comac_show_text (cr, "A");

    /* rotate and scale */
    comac_matrix_init_rotate (&tm, M_PI / 2);
    comac_matrix_scale (&tm, TT_FONT_SIZE, TT_FONT_SIZE * 2.0);
    comac_set_font_matrix (cr, &tm);

    comac_new_path (cr);
    comac_move_to (cr, TT_PAD, TT_PAD + 25);
    comac_show_text (cr, "A");

    comac_matrix_init_rotate (&tm, M_PI / 2);
    comac_matrix_scale (&tm, TT_FONT_SIZE * 2.0, TT_FONT_SIZE);
    comac_set_font_matrix (cr, &tm);

    comac_new_path (cr);
    comac_move_to (cr, TT_PAD, TT_PAD + 50);
    comac_show_text (cr, "A");
}

static comac_t *
text_transform (comac_t *cr)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_pattern_t *pattern;

    comac_set_source_rgb (cr, 1., 1., 1.);
    comac_paint (cr);

    comac_set_source_rgb (cr, 0., 0., 0.);

    comac_select_font_face (cr, COMAC_TEST_FONT_FAMILY " Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    draw_text_transform (cr);

    comac_translate (cr, TT_SIZE, TT_SIZE);
    comac_rotate (cr, M_PI);

    pattern = comac_test_create_pattern_from_png (ctx, png_filename);
    comac_pattern_set_extend (pattern, COMAC_EXTEND_REPEAT);
    comac_set_source (cr, pattern);
    comac_pattern_destroy (pattern);

    draw_text_transform (cr);

    return cr;
}

/* And here begins the recording and replaying... */

static comac_t *
record_create (comac_t *target)
{
    comac_surface_t *surface;
    comac_t *cr;

    surface = comac_recording_surface_create (comac_surface_get_content (comac_get_target (target)), NULL);
    cr = comac_test_create (surface, comac_test_get_context (target));
    comac_surface_destroy (surface);

    return cr;
}

static comac_surface_t *
record_get (comac_t *target)
{
    comac_surface_t *surface;

    surface = comac_surface_reference (comac_get_target (target));
    comac_destroy (target);

    return surface;
}

static comac_test_status_t
record_replay (comac_t *cr, comac_t *(*func)(comac_t *), int width, int height)
{
    comac_surface_t *surface;
    int x, y;

#if GENERATE_REF
    func(cr);
#else
    surface = record_get (func (record_create (cr)));

    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_surface (cr, surface, 0, 0);
    comac_surface_destroy (surface);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_NONE);

    for (y = 0; y < height; y += 2) {
	for (x = 0; x < width; x += 2) {
	    comac_rectangle (cr, x, y, 2, 2);
	    comac_clip (cr);
	    comac_paint (cr);
	    comac_reset_clip (cr);
	}
    }
#endif

    return COMAC_TEST_SUCCESS;
}

static comac_test_status_t
record_paint (comac_t *cr, int width, int height)
{
    return record_replay (cr, paint, width, height);
}

static comac_test_status_t
record_paint_alpha (comac_t *cr, int width, int height)
{
    return record_replay (cr, paint_alpha, width, height);
}

static comac_test_status_t
record_paint_alpha_solid_clip (comac_t *cr, int width, int height)
{
    return record_replay (cr, paint_alpha_solid_clip, width, height);
}

static comac_test_status_t
record_paint_alpha_clip (comac_t *cr, int width, int height)
{
    return record_replay (cr, paint_alpha_clip, width, height);
}

static comac_test_status_t
record_paint_alpha_clip_mask (comac_t *cr, int width, int height)
{
    return record_replay (cr, paint_alpha_clip_mask, width, height);
}

static comac_test_status_t
record_fill_alpha (comac_t *cr, int width, int height)
{
    return record_replay (cr, fill_alpha, width, height);
}

static comac_test_status_t
record_self_intersecting (comac_t *cr, int width, int height)
{
    return record_replay (cr, self_intersecting, width, height);
}

static comac_test_status_t
record_select_font_face (comac_t *cr, int width, int height)
{
    return record_replay (cr, select_font_face, width, height);
}

static comac_test_status_t
record_text_transform (comac_t *cr, int width, int height)
{
    return record_replay (cr, text_transform, width, height);
}

COMAC_TEST (record_paint,
	    "Test replayed calls to comac_paint",
	    "paint,record", /* keywords */
	    NULL, /* requirements */
	    8, 8,
	    NULL, record_paint)
COMAC_TEST (record_paint_alpha,
	    "Simple test of comac_paint_with_alpha",
	    "record, paint, alpha", /* keywords */
	    NULL, /* requirements */
	    32, 32,
	    NULL, record_paint_alpha)
COMAC_TEST (record_paint_alpha_solid_clip,
	    "Simple test of comac_paint_with_alpha+unaligned clip",
	    "record, paint, alpha, clip", /* keywords */
	    NULL, /* requirements */
	    32, 32,
	    NULL, record_paint_alpha_solid_clip)
COMAC_TEST (record_paint_alpha_clip,
	    "Simple test of comac_paint_with_alpha+unaligned clip",
	    "record, paint, alpha, clip", /* keywords */
	    NULL, /* requirements */
	    32, 32,
	    NULL, record_paint_alpha_clip)
COMAC_TEST (record_paint_alpha_clip_mask,
	    "Simple test of comac_paint_with_alpha+triangular clip",
	    "record, paint, alpha, clip", /* keywords */
	    NULL, /* requirements */
	    32, 32,
	    NULL, record_paint_alpha_clip_mask)
COMAC_TEST (record_fill_alpha,
	    "Tests using set_rgba();fill()",
	    "record,fill, alpha", /* keywords */
	    NULL, /* requirements */
	    2*SIZE + 4*PAD, 2*SIZE + 4*PAD,
	    NULL, record_fill_alpha)
COMAC_TEST (record_select_font_face,
	    "Tests using comac_select_font_face to draw text in different faces",
	    "record, font", /* keywords */
	    NULL, /* requirements */
	    192, TEXT_SIZE + 4,
	    NULL, record_select_font_face)
COMAC_TEST (record_self_intersecting,
	    "Test strokes of self-intersecting paths",
	    "record, stroke, trap", /* keywords */
	    NULL, /* requirements */
	    10, 20,
	    NULL, record_self_intersecting)
COMAC_TEST (record_text_transform,
	    "Test various applications of the font matrix",
	    "record, text, transform", /* keywords */
	    NULL, /* requirements */
	    TT_SIZE, TT_SIZE,
	    NULL, record_text_transform)
