#include "config.h"

#include <comac-xml.h>
#include <comac-script-interpreter.h>

#include <stdio.h>
#include <string.h>

static comac_surface_t *
_surface_create (void *_closure,
		 comac_content_t content,
		 double width, double height,
		 long uid)
{
    comac_surface_t **closure = _closure;
    comac_surface_t *surface;
    comac_rectangle_t extents;

    extents.x = extents.y = 0;
    extents.width  = width;
    extents.height = height;
    surface = comac_recording_surface_create (content, &extents);
    if (*closure == NULL)
	*closure = comac_surface_reference (surface);

    return surface;
}

static comac_status_t
stdio_write (void *closure, const unsigned char *data, unsigned len)
{
    if (fwrite (data, len, 1, closure) == 1)
	return COMAC_STATUS_SUCCESS;
    else
	return COMAC_STATUS_WRITE_ERROR;
}

int
main (int argc, char **argv)
{
    comac_surface_t *surface = NULL;
    const comac_script_interpreter_hooks_t hooks = {
	.closure = &surface,
	.surface_create = _surface_create,
    };
    comac_script_interpreter_t *csi;
    FILE *in = stdin, *out = stdout;

    if (argc >= 2 && strcmp (argv[1], "-"))
	in = fopen (argv[1], "r");
    if (argc >= 3 && strcmp (argv[2], "-"))
	out = fopen (argv[2], "w");

    csi = comac_script_interpreter_create ();
    comac_script_interpreter_install_hooks (csi, &hooks);
    comac_script_interpreter_feed_stream (csi, in);
    comac_script_interpreter_finish (csi);
    comac_script_interpreter_destroy (csi);

    if (surface != NULL) {
	comac_device_t *xml;

	xml = comac_xml_create_for_stream (stdio_write, out);
	comac_xml_for_recording_surface (xml, surface);
	comac_device_destroy (xml);

	comac_surface_destroy (surface);
    }

    if (in != stdin)
	fclose (in);
    if (out != stdout)
	fclose (out);

    return 0;
}
