#include <comac.h>
#include <comac-pdf.h>
#include <stdlib.h>

static void
create_test_pdf (const char *ofname, comac_colorspace_t colorspace)
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
    comac_set_source_rgb (cr, 0.9, 0.6, 0.1);
    comac_rectangle (cr, 100, 100, 100, 100);
    comac_fill (cr);
    comac_destroy (cr);
    comac_surface_destroy (surf);
}

int
main (int argc, char **argv)
{
    setenv ("COMAC_DEBUG_PDF", "1", 1);
    create_test_pdf ("rgbtest.pdf", COMAC_COLORSPACE_RGB);
    create_test_pdf ("graytest.pdf", COMAC_COLORSPACE_GRAY);
    create_test_pdf ("cmyktest.pdf", COMAC_COLORSPACE_CMYK);
    return 0;
}
