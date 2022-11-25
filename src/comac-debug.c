/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the comac graphics library.
 *
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

#include "comacint.h"
#include "comac-image-surface-private.h"

/**
 * comac_debug_reset_static_data:
 *
 * Resets all static data within comac to its original state,
 * (ie. identical to the state at the time of program invocation). For
 * example, all caches within comac will be flushed empty.
 *
 * This function is intended to be useful when using memory-checking
 * tools such as valgrind. When valgrind's memcheck analyzes a
 * comac-using program without a call to comac_debug_reset_static_data(),
 * it will report all data reachable via comac's static objects as
 * "still reachable". Calling comac_debug_reset_static_data() just prior
 * to program termination will make it easier to get squeaky clean
 * reports from valgrind.
 *
 * WARNING: It is only safe to call this function when there are no
 * active comac objects remaining, (ie. the appropriate destroy
 * functions have been called as necessary). If there are active comac
 * objects, this call is likely to cause a crash, (eg. an assertion
 * failure due to a hash table being destroyed when non-empty).
 *
 * Since: 1.0
 **/
void
comac_debug_reset_static_data (void)
{
    COMAC_MUTEX_INITIALIZE ();

    _comac_scaled_font_map_destroy ();

    _comac_toy_font_face_reset_static_data ();

#if COMAC_HAS_FT_FONT
    _comac_ft_font_reset_static_data ();
#endif

#if COMAC_HAS_WIN32_FONT
    _comac_win32_font_reset_static_data ();
#endif

    _comac_intern_string_reset_static_data ();

    _comac_scaled_font_reset_static_data ();

    _comac_pattern_reset_static_data ();

    _comac_clip_reset_static_data ();

    _comac_image_reset_static_data ();

    _comac_image_compositor_reset_static_data ();

    _comac_default_context_reset_static_data ();

    COMAC_MUTEX_FINALIZE ();
}

#if HAVE_VALGRIND
void
_comac_debug_check_image_surface_is_defined (const comac_surface_t *surface)
{
    const comac_image_surface_t *image = (comac_image_surface_t *) surface;
    const uint8_t *bits;
    int row, width;

    if (surface == NULL)
	return;

    if (! RUNNING_ON_VALGRIND)
	return;

    bits = image->data;
    switch (image->format) {
    case COMAC_FORMAT_A1:
	width = (image->width + 7)/8;
	break;
    case COMAC_FORMAT_A8:
	width = image->width;
	break;
    case COMAC_FORMAT_RGB16_565:
	width = image->width*2;
	break;
    case COMAC_FORMAT_RGB24:
    case COMAC_FORMAT_RGB30:
    case COMAC_FORMAT_ARGB32:
	width = image->width*4;
	break;
    case COMAC_FORMAT_RGB96F:
	width = image->width*12;
	break;
    case COMAC_FORMAT_RGBA128F:
	width = image->width*16;
	break;
    case COMAC_FORMAT_INVALID:
    default:
	/* XXX compute width from pixman bpp */
	return;
    }

    for (row = 0; row < image->height; row++) {
	VALGRIND_CHECK_MEM_IS_DEFINED (bits, width);
	/* and then silence any future valgrind warnings */
	VALGRIND_MAKE_MEM_DEFINED (bits, width);
	bits += image->stride;
    }
}
#endif


#if 0
void
_comac_image_surface_write_to_ppm (comac_image_surface_t *isurf, const char *fn)
{
    char *fmt;
    if (isurf->format == COMAC_FORMAT_ARGB32 || isurf->format == COMAC_FORMAT_RGB24)
        fmt = "P6";
    else if (isurf->format == COMAC_FORMAT_A8)
        fmt = "P5";
    else
        return;

    FILE *fp = fopen(fn, "wb");
    if (!fp)
        return;

    fprintf (fp, "%s %d %d 255\n", fmt,isurf->width, isurf->height);
    for (int j = 0; j < isurf->height; j++) {
        unsigned char *row = isurf->data + isurf->stride * j;
        for (int i = 0; i < isurf->width; i++) {
            if (isurf->format == COMAC_FORMAT_ARGB32 || isurf->format == COMAC_FORMAT_RGB24) {
                unsigned char r = *row++;
                unsigned char g = *row++;
                unsigned char b = *row++;
                *row++;
                putc(r, fp);
                putc(g, fp);
                putc(b, fp);
            } else {
                unsigned char a = *row++;
                putc(a, fp);
            }
        }
    }

    fclose (fp);

    fprintf (stderr, "Wrote %s\n", fn);
}
#endif

static comac_status_t
_print_move_to (void *closure,
		const comac_point_t *point)
{
    fprintf (closure,
	     " %f %f m",
	     _comac_fixed_to_double (point->x),
	     _comac_fixed_to_double (point->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_print_line_to (void *closure,
		const comac_point_t *point)
{
    fprintf (closure,
	     " %f %f l",
	     _comac_fixed_to_double (point->x),
	     _comac_fixed_to_double (point->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_print_curve_to (void *closure,
		 const comac_point_t *p1,
		 const comac_point_t *p2,
		 const comac_point_t *p3)
{
    fprintf (closure,
	     " %f %f %f %f %f %f c",
	     _comac_fixed_to_double (p1->x),
	     _comac_fixed_to_double (p1->y),
	     _comac_fixed_to_double (p2->x),
	     _comac_fixed_to_double (p2->y),
	     _comac_fixed_to_double (p3->x),
	     _comac_fixed_to_double (p3->y));

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_print_close (void *closure)
{
    fprintf (closure, " h");

    return COMAC_STATUS_SUCCESS;
}

void
_comac_debug_print_path (FILE *stream, const comac_path_fixed_t *path)
{
    comac_status_t status;
    comac_box_t box;

    fprintf (stream,
	     "path: extents=(%f, %f), (%f, %f)\n",
	    _comac_fixed_to_double (path->extents.p1.x),
	    _comac_fixed_to_double (path->extents.p1.y),
	    _comac_fixed_to_double (path->extents.p2.x),
	    _comac_fixed_to_double (path->extents.p2.y));

    status = _comac_path_fixed_interpret (path,
					  _print_move_to,
					  _print_line_to,
					  _print_curve_to,
					  _print_close,
					  stream);
    assert (status == COMAC_STATUS_SUCCESS);

    if (_comac_path_fixed_is_box (path, &box)) {
	fprintf (stream, "[box (%d, %d), (%d, %d)]",
		 box.p1.x, box.p1.y, box.p2.x, box.p2.y);
    }

    fprintf (stream, "\n");
}

void
_comac_debug_print_polygon (FILE *stream, comac_polygon_t *polygon)
{
    int n;

    fprintf (stream,
	     "polygon: extents=(%f, %f), (%f, %f)\n",
	    _comac_fixed_to_double (polygon->extents.p1.x),
	    _comac_fixed_to_double (polygon->extents.p1.y),
	    _comac_fixed_to_double (polygon->extents.p2.x),
	    _comac_fixed_to_double (polygon->extents.p2.y));
    if (polygon->num_limits) {
	fprintf (stream,
		 "       : limit=(%f, %f), (%f, %f) x %d\n",
		 _comac_fixed_to_double (polygon->limit.p1.x),
		 _comac_fixed_to_double (polygon->limit.p1.y),
		 _comac_fixed_to_double (polygon->limit.p2.x),
		 _comac_fixed_to_double (polygon->limit.p2.y),
		 polygon->num_limits);
    }

    for (n = 0; n < polygon->num_edges; n++) {
	comac_edge_t *edge = &polygon->edges[n];

	fprintf (stream,
		 "  [%d] = [(%f, %f), (%f, %f)], top=%f, bottom=%f, dir=%d\n",
		 n,
		 _comac_fixed_to_double (edge->line.p1.x),
		 _comac_fixed_to_double (edge->line.p1.y),
		 _comac_fixed_to_double (edge->line.p2.x),
		 _comac_fixed_to_double (edge->line.p2.y),
		 _comac_fixed_to_double (edge->top),
		 _comac_fixed_to_double (edge->bottom),
		 edge->dir);

    }
}

void
_comac_debug_print_matrix (FILE *file, const comac_matrix_t *matrix)
{
    fprintf (file, "[%g %g %g %g %g %g]\n",
	     matrix->xx, matrix->yx,
	     matrix->xy, matrix->yy,
	     matrix->x0, matrix->y0);
}

void
_comac_debug_print_rect (FILE *file, const comac_rectangle_int_t *rect)
{
    fprintf (file, "x: %d y: %d width: %d height: %d\n",
	     rect->x, rect->y,
	     rect->width, rect->height);
}
