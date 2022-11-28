#include <comac.h>
#include <comac-pdf.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

static void
image_test_pdf (const char *ofname,
		const char *png_file,
		comac_colorspace_t colorspace)
{
    comac_surface_t *surf =
	comac_pdf_surface_create2 (ofname,
				   colorspace,
				   COMAC_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
				   comac_default_color_convert_func,
				   NULL,
				   595.28,
				   841.89);
    comac_t *cr = comac_create (surf);
    comac_surface_t *image = comac_image_surface_create_from_png (png_file);
    assert (image);
    comac_rectangle (cr, 0, 0, 400, 300);
    comac_scale (cr, 400.0 / 4000, 300.0 / 3000);
    comac_set_source_surface (cr, image, 0, 0);
    comac_fill (cr);
    comac_surface_destroy (image);
    comac_destroy (cr);
    comac_surface_destroy (surf);
}

int
main (int argc, char **argv)
{
    setenv ("COMAC_DEBUG_PDF", "1", 1);
    if (argc != 2) {
	printf ("%s <png image file>\n", argv[0]);
	return 1;
    }
    image_test_pdf ("imagetest.pdf", argv[1], COMAC_COLORSPACE_RGB);
    return 0;
}
