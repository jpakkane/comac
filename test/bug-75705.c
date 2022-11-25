#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    double dsh[2] = {1,3};

    comac_set_source_rgba (cr, 0, 0, 0, 1);
    comac_paint (cr);

    comac_set_line_cap (cr, COMAC_LINE_CAP_ROUND);

    comac_move_to (cr, 3, 3);
    /* struct glitter_scan_converter spans_embedded array size is 64 */
    comac_line_to (cr, 65+3, 3);

    comac_set_antialias (cr, COMAC_ANTIALIAS_FAST);
    comac_set_tolerance (cr, 1);

    comac_set_dash (cr, dsh, 2, 0);
    comac_set_line_width (cr, 2);

    comac_stroke (cr);
    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (bug_75705,
	    "Bug 75705 (exercise tor22-scan-converter)",
	    "dash, stroke, antialias", /* keywords */
	    NULL, /* requirements */
	    72, 8,
	    NULL, draw)
