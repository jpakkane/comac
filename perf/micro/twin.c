#define WIDTH 1350
#define HEIGHT 900

#include "comac-perf.h"

static comac_time_t
do_twin (comac_t *cr, int width, int height, int loops)
{
    int i, j, h;
    unsigned char s[2] = {0, 0};

    comac_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    comac_paint (cr);
    comac_set_source_rgb (cr, 0, 0, 0);

    comac_select_font_face (cr,
			    "@comac:",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);

    comac_perf_timer_start ();

    while (loops--) {
	h = 2;
	for (i = 8; i < 48; i >= 24 ? i += 3 : i++) {
	    comac_set_font_size (cr, i);
	    for (j = 33; j < 128; j++) {
		if (j == 33 || (j == 80 && i > 24)) {
		    h += i + 2;
		    comac_move_to (cr, 10, h);
		}
		s[0] = j;
		comac_text_path (cr, (const char *) s);
	    }
	}
	comac_fill (cr);
    }

    comac_perf_timer_stop ();
    return comac_perf_timer_elapsed ();
}

comac_bool_t
twin_enabled (comac_perf_t *perf)
{
    return comac_perf_can_run (perf, "twin", NULL);
}

void
twin (comac_perf_t *perf, comac_t *cr, int width, int height)
{
    comac_perf_run (perf, "twin", do_twin, NULL);
}
