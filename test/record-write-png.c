/*
 * Copyright © 2021 Matthias Clasen
 * Copyright © 2021 Uli Schlachter
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
 *	Matthias Clasen
 *	Uli Schlachter
 */

// Test case for https://gitlab.freedesktop.org/comac/comac/-/merge_requests/118
// A recording surface with a non-zero origin gets cut off when passed to
// comac_surface_write_to_png().

#include "comac-test.h"

#include <assert.h>

struct buffer {
    char *data;
    size_t length;
};

static comac_surface_t *
prepare_recording (void)
{
    comac_surface_t *surface;
    comac_rectangle_t rect;
    comac_t *cr;

    rect.x = -1;
    rect.y = -2;
    rect.width = 3;
    rect.height = 4;
    surface = comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, &rect);

    cr = comac_create (surface);
    comac_set_line_width (cr, 1);
    comac_set_source_rgb (cr, 1, 1, 1);
    comac_rectangle (cr, 0.5, -0.5, 1., 1.);
    comac_stroke (cr);
    comac_destroy (cr);

    return surface;
}

static comac_status_t
write_callback (void *closure, const unsigned char *data, unsigned int length)
{
    struct buffer *buffer = closure;

    buffer->data = realloc (buffer->data, buffer->length + length);
    memcpy (&buffer->data[buffer->length], data, length);
    buffer->length += length;

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
read_callback (void *closure, unsigned char *data, unsigned int length)
{
    struct buffer *buffer = closure;

    assert (buffer->length >= length);
    memcpy (data, buffer->data, length);
    buffer->data += length;
    buffer->length -= length;

    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
png_round_trip (comac_surface_t *input_surface)
{
    comac_surface_t *output_surface;
    struct buffer buffer;
    void *to_free;

    // Turn the surface into a PNG
    buffer.data = NULL;
    buffer.length = 0;
    comac_surface_write_to_png_stream (input_surface, write_callback, &buffer);
    to_free = buffer.data;

    // Load the PNG again
    output_surface = comac_image_surface_create_from_png_stream (read_callback, &buffer);

    free (to_free);
    return output_surface;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *recording, *surface;

    // Draw a black background so that the output does not vary with alpha
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_paint (cr);

    recording = prepare_recording ();
    surface = png_round_trip (recording);

    comac_set_source_surface (cr, surface, 0, 0);
    comac_paint (cr);

    comac_surface_destroy (recording);
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (record_write_png,
	    "Test writing to png with non-zero origin",
	    "record, transform", /* keywords */
	    NULL, /* requirements */
	    4, 4,
	    NULL, draw)
