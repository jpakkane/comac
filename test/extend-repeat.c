#include "comac-test.h"

static const char *png_filename = "romedalen.png";

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *surface;

    surface = comac_test_create_surface_from_png (ctx, png_filename);
    comac_set_source_surface (cr, surface, 32, 32);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_REPEAT);

    comac_paint (cr);

    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (extend_repeat,
	    "Test COMAC_EXTEND_REPEAT for surface patterns",
	    "extend", /* keywords */
	    NULL,     /* requirements */
	    256 + 32 * 2,
	    192 + 32 * 2,
	    NULL,
	    draw)
