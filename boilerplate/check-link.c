#include <comac-boilerplate.h>
#include <stdio.h>

int
main (void)
{
    printf ("Check linking to the just built comac boilerplate library\n");
    if (comac_boilerplate_version () == COMAC_VERSION) {
	return 0;
    } else {
	fprintf (
	    stderr,
	    "Error: linked to comac boilerplate version %s instead of %s\n",
	    comac_boilerplate_version_string (),
	    COMAC_VERSION_STRING);
	return 1;
    }
}
