#include <comac.h>
#include <stdio.h>

int
main (void)
{
  printf ("Check linking to the just built comac library\n");
  if (comac_version () == COMAC_VERSION) {
    return 0;
  } else {
    fprintf (stderr,
	     "Error: linked to comac version %s instead of %s\n",
	     comac_version_string (),
	     COMAC_VERSION_STRING);
    return 1;
  }
}
