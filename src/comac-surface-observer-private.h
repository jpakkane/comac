/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2011 Intel Corporation
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
 * The Initial Developer of the Original Code is Intel Corporation.
 *
 * Contributor(s):
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef COMAC_SURFACE_OBSERVER_PRIVATE_H
#define COMAC_SURFACE_OBSERVER_PRIVATE_H

#include "comacint.h"

#include "comac-device-private.h"
#include "comac-list-private.h"
#include "comac-recording-surface-private.h"
#include "comac-surface-private.h"
#include "comac-surface-backend-private.h"
#include "comac-time-private.h"

struct stat {
    double min, max, sum, sum_sq;
    unsigned count;
};

#define NUM_OPERATORS (COMAC_OPERATOR_HSL_LUMINOSITY+1)
#define NUM_CAPS (COMAC_LINE_CAP_SQUARE+1)
#define NUM_JOINS (COMAC_LINE_JOIN_BEVEL+1)
#define NUM_ANTIALIAS (COMAC_ANTIALIAS_BEST+1)
#define NUM_FILL_RULE (COMAC_FILL_RULE_EVEN_ODD+1)

struct extents {
    struct stat area;
    unsigned int bounded, unbounded;
};

struct pattern {
    unsigned int type[8]; /* native/record/other surface/gradients */
};

struct path {
    unsigned int type[5]; /* empty/pixel/rectilinear/straight/curved */
};

struct clip {
    unsigned int type[6]; /* none, region, boxes, single path, polygon, general */
};

typedef struct _comac_observation comac_observation_t;
typedef struct _comac_observation_record comac_observation_record_t;
typedef struct _comac_device_observer comac_device_observer_t;

struct _comac_observation_record {
    comac_content_t target_content;
    int target_width;
    int target_height;

    int index;
    comac_operator_t op;
    int source;
    int mask;
    int num_glyphs;
    int path;
    int fill_rule;
    double tolerance;
    int antialias;
    int clip;
    comac_time_t elapsed;
};

struct _comac_observation {
    int num_surfaces;
    int num_contexts;
    int num_sources_acquired;

    /* XXX put interesting stats here! */

    struct paint {
	comac_time_t elapsed;
	unsigned int count;
	struct extents extents;
	unsigned int operators[NUM_OPERATORS];
	struct pattern source;
	struct clip clip;
	unsigned int noop;

	comac_observation_record_t slowest;
    } paint;

    struct mask {
	comac_time_t elapsed;
	unsigned int count;
	struct extents extents;
	unsigned int operators[NUM_OPERATORS];
	struct pattern source;
	struct pattern mask;
	struct clip clip;
	unsigned int noop;

	comac_observation_record_t slowest;
    } mask;

    struct fill {
	comac_time_t elapsed;
	unsigned int count;
	struct extents extents;
	unsigned int operators[NUM_OPERATORS];
	struct pattern source;
	struct path path;
	unsigned int antialias[NUM_ANTIALIAS];
	unsigned int fill_rule[NUM_FILL_RULE];
	struct clip clip;
	unsigned int noop;

	comac_observation_record_t slowest;
    } fill;

    struct stroke {
	comac_time_t elapsed;
	unsigned int count;
	struct extents extents;
	unsigned int operators[NUM_OPERATORS];
	unsigned int caps[NUM_CAPS];
	unsigned int joins[NUM_CAPS];
	unsigned int antialias[NUM_ANTIALIAS];
	struct pattern source;
	struct path path;
	struct stat line_width;
	struct clip clip;
	unsigned int noop;

	comac_observation_record_t slowest;
    } stroke;

    struct glyphs {
	comac_time_t elapsed;
	unsigned int count;
	struct extents extents;
	unsigned int operators[NUM_OPERATORS];
	struct pattern source;
	struct clip clip;
	unsigned int noop;

	comac_observation_record_t slowest;
    } glyphs;

    comac_array_t timings;
    comac_recording_surface_t *record;
};

struct _comac_device_observer {
    comac_device_t base;
    comac_device_t *target;

    comac_observation_t log;
};

struct callback_list {
    comac_list_t link;

    comac_surface_observer_callback_t func;
    void *data;
};

struct _comac_surface_observer {
    comac_surface_t base;
    comac_surface_t *target;

    comac_observation_t log;

    comac_list_t paint_callbacks;
    comac_list_t mask_callbacks;
    comac_list_t fill_callbacks;
    comac_list_t stroke_callbacks;
    comac_list_t glyphs_callbacks;

    comac_list_t flush_callbacks;
    comac_list_t finish_callbacks;
};

#endif /* COMAC_SURFACE_OBSERVER_PRIVATE_H */
