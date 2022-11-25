#include "comac-test.h"

static const char *png_filename = "romedalen.png";

static comac_surface_t *
clone_similar_surface (comac_surface_t *target, comac_surface_t *surface)
{
    comac_t *cr;
    comac_surface_t *similar;

    similar =
	comac_surface_create_similar (target,
				      comac_surface_get_content (surface),
				      comac_image_surface_get_width (surface),
				      comac_image_surface_get_height (surface));
    cr = comac_create (similar);
    comac_surface_destroy (similar);
    comac_set_source_surface (cr, surface, 0, 0);
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_paint (cr);
    similar = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return similar;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    const comac_test_context_t *ctx = comac_test_get_context (cr);
    comac_surface_t *surface;
    comac_surface_t *similar;

    surface = comac_test_create_surface_from_png (ctx, png_filename);
    similar = clone_similar_surface (comac_get_group_target (cr), surface);
    comac_set_source_surface (cr, similar, 32, 32);
    comac_pattern_set_extend (comac_get_source (cr), COMAC_EXTEND_REPEAT);

    comac_paint (cr);

    comac_surface_destroy (similar);
    comac_surface_destroy (surface);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (extend_repeat_similar,
	    "Test COMAC_EXTEND_REPEAT for surface patterns",
	    "extend", /* keywords */
	    NULL,     /* requirements */
	    256 + 32 * 2,
	    192 + 32 * 2,
	    NULL,
	    draw)
