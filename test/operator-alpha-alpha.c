/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright 2002 University of Southern California
 * Copyright 2005 Red Hat, Inc.
 * Copyright 2007 Emmanuel Pacaud
 * Copyright 2008 Benjamin Otte
 * Copyright 2008 Chris Wilson
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *      Owen Taylor <otaylor@redhat.com>
 *      Kristian Høgsberg <krh@redhat.com>
 *      Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
 *      Chris Wilson <chris@chris-wilson.co.uk>
 *      Andrea Canciani <ranma42@gmail.com>
 */

#include "comac-test.h"

#define STEPS 16
#define START_OPERATOR	COMAC_OPERATOR_CLEAR
#define STOP_OPERATOR	COMAC_OPERATOR_HSL_LUMINOSITY

#define SIZE 3
#define COUNT 6
#define FULL_WIDTH  ((STEPS + 1) * COUNT - 1)
#define FULL_HEIGHT ((COUNT + STOP_OPERATOR - START_OPERATOR) / COUNT) * (STEPS + 1)

static void
create_patterns (comac_t *bg, comac_t *fg)
{
    int x;

    for (x = 0; x < STEPS; x++) {
	double i = (double) x / (STEPS - 1);
	comac_set_source_rgba (bg, 0, 0, 0, i);
	comac_rectangle (bg, x, 0, 1, STEPS);
	comac_fill (bg);

	comac_set_source_rgba (fg, 0, 0, 0, i);
	comac_rectangle (fg, 0, x, STEPS, 1);
	comac_fill (fg);
    }
}

/* expects a STEP*STEP pixel rectangle */
static void
do_composite (comac_t *cr, comac_operator_t op, comac_surface_t *bg, comac_surface_t *fg)
{
    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
    comac_set_source_surface (cr, bg, 0, 0);
    comac_paint (cr);

    comac_set_operator (cr, op);
    comac_set_source_surface (cr, fg, 0, 0);
    comac_paint (cr);
}

static void
subdraw (comac_t *cr, int width, int height)
{
    size_t i = 0;
    comac_operator_t op;
    comac_t *bgcr, *fgcr;
    comac_surface_t *bg, *fg;

    bg = comac_surface_create_similar (comac_get_target (cr),
	    COMAC_CONTENT_ALPHA, SIZE * STEPS, SIZE * STEPS);
    fg = comac_surface_create_similar (comac_get_target (cr),
	    COMAC_CONTENT_ALPHA, SIZE * STEPS, SIZE * STEPS);
    bgcr = comac_create (bg);
    fgcr = comac_create (fg);
    comac_scale (bgcr, SIZE, SIZE);
    comac_scale (fgcr, SIZE, SIZE);
    create_patterns (bgcr, fgcr);
    comac_destroy (bgcr);
    comac_destroy (fgcr);

    for (op = START_OPERATOR; op <= STOP_OPERATOR; op++, i++) {
	comac_save (cr);
	comac_translate (cr,
		SIZE * (STEPS + 1) * (i % COUNT),
		SIZE * (STEPS + 1) * (i / COUNT));
	comac_rectangle (cr, 0, 0, SIZE * (STEPS + 1), SIZE * (STEPS+1));
	comac_clip (cr);
	do_composite (cr, op, bg, fg);
	comac_restore (cr);
    }

    comac_surface_destroy (fg);
    comac_surface_destroy (bg);
}


static comac_surface_t *
create_source (comac_surface_t *target, int width, int height)
{
    comac_surface_t *similar;
    comac_t *cr;

    similar = comac_surface_create_similar (target,
					    COMAC_CONTENT_ALPHA,
					    width, height);
    cr = comac_create (similar);
    comac_surface_destroy (similar);

    subdraw (cr, width, height);

    similar = comac_surface_reference (comac_get_target (cr));
    comac_destroy (cr);

    return similar;
}

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_surface_t *source;

    comac_set_source_rgb (cr, 1, 1, 1);
    comac_paint (cr);

    source = create_source (comac_get_target (cr), width, height);
    comac_set_source_surface (cr, source, 0, 0);
    comac_surface_destroy (source);

    comac_paint (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (operator_alpha_alpha,
	    "Tests result of compositing pure-alpha surfaces"
	    "\nCompositing of pure-alpha sources is inconsistent across backends.",
	    "alpha, similar, operator", /* keywords */
	    NULL, /* requirements */
	    FULL_WIDTH * SIZE, FULL_HEIGHT * SIZE,
	    NULL, draw)
