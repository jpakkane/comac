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

#include "comacint.h"

#include "comac-surface-observer-private.h"
#include "comac-surface-observer-inline.h"

#include "comac-array-private.h"
#include "comac-combsort-inline.h"
#include "comac-composite-rectangles-private.h"
#include "comac-error-private.h"
#include "comac-image-surface-private.h"
#include "comac-list-inline.h"
#include "comac-pattern-private.h"
#include "comac-output-stream-private.h"
#include "comac-recording-surface-private.h"
#include "comac-surface-subsurface-inline.h"
#include "comac-reference-count-private.h"

#if COMAC_HAS_SCRIPT_SURFACE
#include "comac-script-private.h"
#endif

static const comac_surface_backend_t _comac_surface_observer_backend;

/* observation/stats */

static void
init_stats (struct stat *s)
{
    s->min = HUGE_VAL;
    s->max = -HUGE_VAL;
}

static void
init_extents (struct extents *e)
{
    init_stats (&e->area);
}

static void
init_pattern (struct pattern *p)
{
}

static void
init_path (struct path *p)
{
}

static void
init_clip (struct clip *c)
{
}

static void
init_paint (struct paint *p)
{
    init_extents (&p->extents);
    init_pattern (&p->source);
    init_clip (&p->clip);
}

static void
init_mask (struct mask *m)
{
    init_extents (&m->extents);
    init_pattern (&m->source);
    init_pattern (&m->mask);
    init_clip (&m->clip);
}

static void
init_fill (struct fill *f)
{
    init_extents (&f->extents);
    init_pattern (&f->source);
    init_path (&f->path);
    init_clip (&f->clip);
}

static void
init_stroke (struct stroke *s)
{
    init_extents (&s->extents);
    init_pattern (&s->source);
    init_path (&s->path);
    init_clip (&s->clip);
}

static void
init_glyphs (struct glyphs *g)
{
    init_extents (&g->extents);
    init_pattern (&g->source);
    init_clip (&g->clip);
}

static comac_status_t
log_init (comac_observation_t *log, comac_bool_t record)
{
    memset (log, 0, sizeof (*log));

    init_paint (&log->paint);
    init_mask (&log->mask);
    init_fill (&log->fill);
    init_stroke (&log->stroke);
    init_glyphs (&log->glyphs);

    _comac_array_init (&log->timings, sizeof (comac_observation_record_t));

    if (record) {
	log->record = (comac_recording_surface_t *)
	    comac_recording_surface_create (COMAC_CONTENT_COLOR_ALPHA, NULL);
	if (unlikely (log->record->base.status))
	    return log->record->base.status;

	log->record->optimize_clears = FALSE;
    }

    return COMAC_STATUS_SUCCESS;
}

static void
log_fini (comac_observation_t *log)
{
    _comac_array_fini (&log->timings);
    comac_surface_destroy (&log->record->base);
}

static comac_surface_t *
get_pattern_surface (const comac_pattern_t *pattern)
{
    return ((comac_surface_pattern_t *) pattern)->surface;
}

static int
classify_pattern (const comac_pattern_t *pattern, const comac_surface_t *target)
{
    int classify;

    switch (pattern->type) {
    case COMAC_PATTERN_TYPE_SURFACE:
	if (get_pattern_surface (pattern)->type == target->type)
	    classify = 0;
	else if (get_pattern_surface (pattern)->type ==
		 COMAC_SURFACE_TYPE_RECORDING)
	    classify = 1;
	else
	    classify = 2;
	break;
    default:
    case COMAC_PATTERN_TYPE_SOLID:
	classify = 3;
	break;
    case COMAC_PATTERN_TYPE_LINEAR:
	classify = 4;
	break;
    case COMAC_PATTERN_TYPE_RADIAL:
	classify = 5;
	break;
    case COMAC_PATTERN_TYPE_MESH:
	classify = 6;
	break;
    case COMAC_PATTERN_TYPE_RASTER_SOURCE:
	classify = 7;
	break;
    }
    return classify;
}

static void
add_pattern (struct pattern *stats,
	     const comac_pattern_t *pattern,
	     const comac_surface_t *target)
{
    stats->type[classify_pattern (pattern, target)]++;
}

static int
classify_path (const comac_path_fixed_t *path, comac_bool_t is_fill)
{
    int classify;

    /* XXX improve for stroke */
    classify = -1;
    if (is_fill) {
	if (path->fill_is_empty)
	    classify = 0;
	else if (_comac_path_fixed_fill_is_rectilinear (path))
	    classify = path->fill_maybe_region ? 1 : 2;
    } else {
	if (_comac_path_fixed_stroke_is_rectilinear (path))
	    classify = 2;
    }
    if (classify == -1)
	classify = 3 + (path->has_curve_to != 0);

    return classify;
}

static void
add_path (struct path *stats,
	  const comac_path_fixed_t *path,
	  comac_bool_t is_fill)
{
    stats->type[classify_path (path, is_fill)]++;
}

static int
classify_clip (const comac_clip_t *clip)
{
    int classify;

    if (clip == NULL)
	classify = 0;
    else if (_comac_clip_is_region (clip))
	classify = 1;
    else if (clip->path == NULL)
	classify = 2;
    else if (clip->path->prev == NULL)
	classify = 3;
    else if (_comac_clip_is_polygon (clip))
	classify = 4;
    else
	classify = 5;

    return classify;
}

static void
add_clip (struct clip *stats, const comac_clip_t *clip)
{
    stats->type[classify_clip (clip)]++;
}

static void
stats_add (struct stat *s, double v)
{
    if (v < s->min)
	s->min = v;
    if (v > s->max)
	s->max = v;
    s->sum += v;
    s->sum_sq += v * v;
    s->count++;
}

static void
add_extents (struct extents *stats, const comac_composite_rectangles_t *extents)
{
    const comac_rectangle_int_t *r =
	extents->is_bounded ? &extents->bounded : &extents->unbounded;
    stats_add (&stats->area, r->width * r->height);
    stats->bounded += extents->is_bounded != 0;
    stats->unbounded += extents->is_bounded == 0;
}

/* device interface */

static void
_comac_device_observer_lock (void *_device)
{
    comac_device_observer_t *device = (comac_device_observer_t *) _device;
    comac_status_t ignored;

    /* comac_device_acquire() can fail for nil and finished
     * devices. We don't care about observing them. */
    ignored = comac_device_acquire (device->target);
}

static void
_comac_device_observer_unlock (void *_device)
{
    comac_device_observer_t *device = (comac_device_observer_t *) _device;
    comac_device_release (device->target);
}

static comac_status_t
_comac_device_observer_flush (void *_device)
{
    comac_device_observer_t *device = (comac_device_observer_t *) _device;

    if (device->target == NULL)
	return COMAC_STATUS_SUCCESS;

    comac_device_flush (device->target);
    return device->target->status;
}

static void
_comac_device_observer_finish (void *_device)
{
    comac_device_observer_t *device = (comac_device_observer_t *) _device;
    log_fini (&device->log);
    comac_device_finish (device->target);
}

static void
_comac_device_observer_destroy (void *_device)
{
    comac_device_observer_t *device = (comac_device_observer_t *) _device;
    comac_device_destroy (device->target);
    free (device);
}

static const comac_device_backend_t _comac_device_observer_backend = {
    COMAC_INTERNAL_DEVICE_TYPE_OBSERVER,

    _comac_device_observer_lock,
    _comac_device_observer_unlock,

    _comac_device_observer_flush,
    _comac_device_observer_finish,
    _comac_device_observer_destroy,
};

static comac_device_t *
_comac_device_create_observer_internal (comac_device_t *target,
					comac_bool_t record)
{
    comac_device_observer_t *device;
    comac_status_t status;

    device = _comac_malloc (sizeof (comac_device_observer_t));
    if (unlikely (device == NULL))
	return _comac_device_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_device_init (&device->base, &_comac_device_observer_backend);
    status = log_init (&device->log, record);
    if (unlikely (status)) {
	free (device);
	return _comac_device_create_in_error (status);
    }

    device->target = comac_device_reference (target);

    return &device->base;
}

/* surface interface */

static comac_device_observer_t *
to_device (comac_surface_observer_t *suface)
{
    return (comac_device_observer_t *) suface->base.device;
}

static comac_surface_t *
_comac_surface_create_observer_internal (comac_device_t *device,
					 comac_surface_t *target)
{
    comac_surface_observer_t *surface;
    comac_status_t status;

    surface = _comac_malloc (sizeof (comac_surface_observer_t));
    if (unlikely (surface == NULL))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_NO_MEMORY));

    _comac_surface_init (&surface->base,
			 &_comac_surface_observer_backend,
			 device,
			 target->content,
			 target->is_vector,
			 target->colorspace,
			 target->intent,
			 target->color_convert,
			 target->color_convert_ctx);

    status =
	log_init (&surface->log,
		  ((comac_device_observer_t *) device)->log.record != NULL);
    if (unlikely (status)) {
	free (surface);
	return _comac_surface_create_in_error (status);
    }

    surface->target = comac_surface_reference (target);
    surface->base.type = surface->target->type;
    surface->base.is_clear = surface->target->is_clear;

    comac_list_init (&surface->paint_callbacks);
    comac_list_init (&surface->mask_callbacks);
    comac_list_init (&surface->fill_callbacks);
    comac_list_init (&surface->stroke_callbacks);
    comac_list_init (&surface->glyphs_callbacks);

    comac_list_init (&surface->flush_callbacks);
    comac_list_init (&surface->finish_callbacks);

    surface->log.num_surfaces++;
    to_device (surface)->log.num_surfaces++;

    return &surface->base;
}

static inline void
do_callbacks (comac_surface_observer_t *surface, comac_list_t *head)
{
    struct callback_list *cb;

    comac_list_foreach_entry (cb, struct callback_list, head, link)
	cb->func (&surface->base, surface->target, cb->data);
}

static comac_status_t
_comac_surface_observer_finish (void *abstract_surface)
{
    comac_surface_observer_t *surface = abstract_surface;

    do_callbacks (surface, &surface->finish_callbacks);

    comac_surface_destroy (surface->target);
    log_fini (&surface->log);

    return COMAC_STATUS_SUCCESS;
}

static comac_surface_t *
_comac_surface_observer_create_similar (void *abstract_other,
					comac_content_t content,
					int width,
					int height)
{
    comac_surface_observer_t *other = abstract_other;
    comac_surface_t *target, *surface;

    target = NULL;
    if (other->target->backend->create_similar)
	target = other->target->backend->create_similar (other->target,
							 content,
							 width,
							 height);
    if (target == NULL)
	target =
	    _comac_image_surface_create_with_content (content, width, height);

    surface =
	_comac_surface_create_observer_internal (other->base.device, target);
    comac_surface_destroy (target);

    return surface;
}

static comac_surface_t *
_comac_surface_observer_create_similar_image (void *other,
					      comac_format_t format,
					      int width,
					      int height)
{
    comac_surface_observer_t *surface = other;

    if (surface->target->backend->create_similar_image)
	return surface->target->backend->create_similar_image (surface->target,
							       format,
							       width,
							       height);

    return NULL;
}

static comac_image_surface_t *
_comac_surface_observer_map_to_image (void *abstract_surface,
				      const comac_rectangle_int_t *extents)
{
    comac_surface_observer_t *surface = abstract_surface;
    return _comac_surface_map_to_image (surface->target, extents);
}

static comac_int_status_t
_comac_surface_observer_unmap_image (void *abstract_surface,
				     comac_image_surface_t *image)
{
    comac_surface_observer_t *surface = abstract_surface;
    return _comac_surface_unmap_image (surface->target, image);
}

static void
record_target (comac_observation_record_t *r, comac_surface_t *target)
{
    comac_rectangle_int_t extents;

    r->target_content = target->content;
    if (_comac_surface_get_extents (target, &extents)) {
	r->target_width = extents.width;
	r->target_height = extents.height;
    } else {
	r->target_width = -1;
	r->target_height = -1;
    }
}

static comac_observation_record_t *
record_paint (comac_observation_record_t *r,
	      comac_surface_t *target,
	      comac_operator_t op,
	      const comac_pattern_t *source,
	      const comac_clip_t *clip,
	      comac_time_t elapsed)
{
    record_target (r, target);

    r->op = op;
    r->source = classify_pattern (source, target);
    r->mask = -1;
    r->num_glyphs = -1;
    r->path = -1;
    r->fill_rule = -1;
    r->tolerance = -1;
    r->antialias = -1;
    r->clip = classify_clip (clip);
    r->elapsed = elapsed;

    return r;
}

static comac_observation_record_t *
record_mask (comac_observation_record_t *r,
	     comac_surface_t *target,
	     comac_operator_t op,
	     const comac_pattern_t *source,
	     const comac_pattern_t *mask,
	     const comac_clip_t *clip,
	     comac_time_t elapsed)
{
    record_target (r, target);

    r->op = op;
    r->source = classify_pattern (source, target);
    r->mask = classify_pattern (mask, target);
    r->num_glyphs = -1;
    r->path = -1;
    r->fill_rule = -1;
    r->tolerance = -1;
    r->antialias = -1;
    r->clip = classify_clip (clip);
    r->elapsed = elapsed;

    return r;
}

static comac_observation_record_t *
record_fill (comac_observation_record_t *r,
	     comac_surface_t *target,
	     comac_operator_t op,
	     const comac_pattern_t *source,
	     const comac_path_fixed_t *path,
	     comac_fill_rule_t fill_rule,
	     double tolerance,
	     comac_antialias_t antialias,
	     const comac_clip_t *clip,
	     comac_time_t elapsed)
{
    record_target (r, target);

    r->op = op;
    r->source = classify_pattern (source, target);
    r->mask = -1;
    r->num_glyphs = -1;
    r->path = classify_path (path, TRUE);
    r->fill_rule = fill_rule;
    r->tolerance = tolerance;
    r->antialias = antialias;
    r->clip = classify_clip (clip);
    r->elapsed = elapsed;

    return r;
}

static comac_observation_record_t *
record_stroke (comac_observation_record_t *r,
	       comac_surface_t *target,
	       comac_operator_t op,
	       const comac_pattern_t *source,
	       const comac_path_fixed_t *path,
	       const comac_stroke_style_t *style,
	       const comac_matrix_t *ctm,
	       const comac_matrix_t *ctm_inverse,
	       double tolerance,
	       comac_antialias_t antialias,
	       const comac_clip_t *clip,
	       comac_time_t elapsed)
{
    record_target (r, target);

    r->op = op;
    r->source = classify_pattern (source, target);
    r->mask = -1;
    r->num_glyphs = -1;
    r->path = classify_path (path, FALSE);
    r->fill_rule = -1;
    r->tolerance = tolerance;
    r->antialias = antialias;
    r->clip = classify_clip (clip);
    r->elapsed = elapsed;

    return r;
}

static comac_observation_record_t *
record_glyphs (comac_observation_record_t *r,
	       comac_surface_t *target,
	       comac_operator_t op,
	       const comac_pattern_t *source,
	       comac_glyph_t *glyphs,
	       int num_glyphs,
	       comac_scaled_font_t *scaled_font,
	       const comac_clip_t *clip,
	       comac_time_t elapsed)
{
    record_target (r, target);

    r->op = op;
    r->source = classify_pattern (source, target);
    r->mask = -1;
    r->path = -1;
    r->num_glyphs = num_glyphs;
    r->fill_rule = -1;
    r->tolerance = -1;
    r->antialias = -1;
    r->clip = classify_clip (clip);
    r->elapsed = elapsed;

    return r;
}

static void
add_record (comac_observation_t *log, comac_observation_record_t *r)
{
    comac_int_status_t status;

    r->index = log->record ? log->record->commands.num_elements : 0;

    status = _comac_array_append (&log->timings, r);
    assert (status == COMAC_INT_STATUS_SUCCESS);
}

static void
_comac_surface_sync (comac_surface_t *target, int x, int y)
{
    comac_rectangle_int_t extents;

    extents.x = x;
    extents.y = y;
    extents.width = 1;
    extents.height = 1;

    _comac_surface_unmap_image (target,
				_comac_surface_map_to_image (target, &extents));
}

static void
midpt (const comac_composite_rectangles_t *extents, int *x, int *y)
{
    *x = extents->bounded.x + extents->bounded.width / 2;
    *y = extents->bounded.y + extents->bounded.height / 2;
}

static void
add_record_paint (comac_observation_t *log,
		  comac_surface_t *target,
		  comac_operator_t op,
		  const comac_pattern_t *source,
		  const comac_clip_t *clip,
		  comac_time_t elapsed)
{
    comac_observation_record_t record;
    comac_int_status_t status;

    add_record (log, record_paint (&record, target, op, source, clip, elapsed));

    /* We have to bypass the high-level surface layer in case it tries to be
     * too smart and discard operations; we need to record exactly what just
     * happened on the target.
     */
    if (log->record) {
	status = log->record->base.backend->paint (&log->record->base,
						   op,
						   source,
						   clip);
	assert (status == COMAC_INT_STATUS_SUCCESS);
    }

    if (_comac_time_gt (elapsed, log->paint.slowest.elapsed))
	log->paint.slowest = record;
    log->paint.elapsed = _comac_time_add (log->paint.elapsed, elapsed);
}

static comac_int_status_t
_comac_surface_observer_paint (void *abstract_surface,
			       comac_operator_t op,
			       const comac_pattern_t *source,
			       const comac_clip_t *clip)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_device_observer_t *device = to_device (surface);
    comac_composite_rectangles_t composite;
    comac_int_status_t status;
    comac_time_t t;
    int x, y;

    /* XXX device locking */

    surface->log.paint.count++;
    surface->log.paint.operators[op]++;
    add_pattern (&surface->log.paint.source, source, surface->target);
    add_clip (&surface->log.paint.clip, clip);

    device->log.paint.count++;
    device->log.paint.operators[op]++;
    add_pattern (&device->log.paint.source, source, surface->target);
    add_clip (&device->log.paint.clip, clip);

    status = _comac_composite_rectangles_init_for_paint (&composite,
							 surface->target,
							 op,
							 source,
							 clip);
    if (unlikely (status)) {
	surface->log.paint.noop++;
	device->log.paint.noop++;
	return status;
    }

    midpt (&composite, &x, &y);

    add_extents (&surface->log.paint.extents, &composite);
    add_extents (&device->log.paint.extents, &composite);
    _comac_composite_rectangles_fini (&composite);

    t = _comac_time_get ();
    status = _comac_surface_paint (surface->target, op, source, clip);
    if (unlikely (status))
	return status;

    _comac_surface_sync (surface->target, x, y);
    t = _comac_time_get_delta (t);

    add_record_paint (&surface->log, surface->target, op, source, clip, t);
    add_record_paint (&device->log, surface->target, op, source, clip, t);

    do_callbacks (surface, &surface->paint_callbacks);

    return COMAC_STATUS_SUCCESS;
}

static void
add_record_mask (comac_observation_t *log,
		 comac_surface_t *target,
		 comac_operator_t op,
		 const comac_pattern_t *source,
		 const comac_pattern_t *mask,
		 const comac_clip_t *clip,
		 comac_time_t elapsed)
{
    comac_observation_record_t record;
    comac_int_status_t status;

    add_record (log,
		record_mask (&record, target, op, source, mask, clip, elapsed));

    if (log->record) {
	status = log->record->base.backend->mask (&log->record->base,
						  op,
						  source,
						  mask,
						  clip);
	assert (status == COMAC_INT_STATUS_SUCCESS);
    }

    if (_comac_time_gt (elapsed, log->mask.slowest.elapsed))
	log->mask.slowest = record;
    log->mask.elapsed = _comac_time_add (log->mask.elapsed, elapsed);
}

static comac_int_status_t
_comac_surface_observer_mask (void *abstract_surface,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_pattern_t *mask,
			      const comac_clip_t *clip)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_device_observer_t *device = to_device (surface);
    comac_composite_rectangles_t composite;
    comac_int_status_t status;
    comac_time_t t;
    int x, y;

    surface->log.mask.count++;
    surface->log.mask.operators[op]++;
    add_pattern (&surface->log.mask.source, source, surface->target);
    add_pattern (&surface->log.mask.mask, mask, surface->target);
    add_clip (&surface->log.mask.clip, clip);

    device->log.mask.count++;
    device->log.mask.operators[op]++;
    add_pattern (&device->log.mask.source, source, surface->target);
    add_pattern (&device->log.mask.mask, mask, surface->target);
    add_clip (&device->log.mask.clip, clip);

    status = _comac_composite_rectangles_init_for_mask (&composite,
							surface->target,
							op,
							source,
							mask,
							clip);
    if (unlikely (status)) {
	surface->log.mask.noop++;
	device->log.mask.noop++;
	return status;
    }

    midpt (&composite, &x, &y);

    add_extents (&surface->log.mask.extents, &composite);
    add_extents (&device->log.mask.extents, &composite);
    _comac_composite_rectangles_fini (&composite);

    t = _comac_time_get ();
    status = _comac_surface_mask (surface->target, op, source, mask, clip);
    if (unlikely (status))
	return status;

    _comac_surface_sync (surface->target, x, y);
    t = _comac_time_get_delta (t);

    add_record_mask (&surface->log, surface->target, op, source, mask, clip, t);
    add_record_mask (&device->log, surface->target, op, source, mask, clip, t);

    do_callbacks (surface, &surface->mask_callbacks);

    return COMAC_STATUS_SUCCESS;
}

static void
add_record_fill (comac_observation_t *log,
		 comac_surface_t *target,
		 comac_operator_t op,
		 const comac_pattern_t *source,
		 const comac_path_fixed_t *path,
		 comac_fill_rule_t fill_rule,
		 double tolerance,
		 comac_antialias_t antialias,
		 const comac_clip_t *clip,
		 comac_time_t elapsed)
{
    comac_observation_record_t record;
    comac_int_status_t status;

    add_record (log,
		record_fill (&record,
			     target,
			     op,
			     source,
			     path,
			     fill_rule,
			     tolerance,
			     antialias,
			     clip,
			     elapsed));

    if (log->record) {
	status = log->record->base.backend->fill (&log->record->base,
						  op,
						  source,
						  path,
						  fill_rule,
						  tolerance,
						  antialias,
						  clip);
	assert (status == COMAC_INT_STATUS_SUCCESS);
    }

    if (_comac_time_gt (elapsed, log->fill.slowest.elapsed))
	log->fill.slowest = record;
    log->fill.elapsed = _comac_time_add (log->fill.elapsed, elapsed);
}

static comac_int_status_t
_comac_surface_observer_fill (void *abstract_surface,
			      comac_operator_t op,
			      const comac_pattern_t *source,
			      const comac_path_fixed_t *path,
			      comac_fill_rule_t fill_rule,
			      double tolerance,
			      comac_antialias_t antialias,
			      const comac_clip_t *clip)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_device_observer_t *device = to_device (surface);
    comac_composite_rectangles_t composite;
    comac_int_status_t status;
    comac_time_t t;
    int x, y;

    surface->log.fill.count++;
    surface->log.fill.operators[op]++;
    surface->log.fill.fill_rule[fill_rule]++;
    surface->log.fill.antialias[antialias]++;
    add_pattern (&surface->log.fill.source, source, surface->target);
    add_path (&surface->log.fill.path, path, TRUE);
    add_clip (&surface->log.fill.clip, clip);

    device->log.fill.count++;
    device->log.fill.operators[op]++;
    device->log.fill.fill_rule[fill_rule]++;
    device->log.fill.antialias[antialias]++;
    add_pattern (&device->log.fill.source, source, surface->target);
    add_path (&device->log.fill.path, path, TRUE);
    add_clip (&device->log.fill.clip, clip);

    status = _comac_composite_rectangles_init_for_fill (&composite,
							surface->target,
							op,
							source,
							path,
							clip);
    if (unlikely (status)) {
	surface->log.fill.noop++;
	device->log.fill.noop++;
	return status;
    }

    midpt (&composite, &x, &y);

    add_extents (&surface->log.fill.extents, &composite);
    add_extents (&device->log.fill.extents, &composite);
    _comac_composite_rectangles_fini (&composite);

    t = _comac_time_get ();
    status = _comac_surface_fill (surface->target,
				  op,
				  source,
				  path,
				  fill_rule,
				  tolerance,
				  antialias,
				  clip);
    if (unlikely (status))
	return status;

    _comac_surface_sync (surface->target, x, y);
    t = _comac_time_get_delta (t);

    add_record_fill (&surface->log,
		     surface->target,
		     op,
		     source,
		     path,
		     fill_rule,
		     tolerance,
		     antialias,
		     clip,
		     t);

    add_record_fill (&device->log,
		     surface->target,
		     op,
		     source,
		     path,
		     fill_rule,
		     tolerance,
		     antialias,
		     clip,
		     t);

    do_callbacks (surface, &surface->fill_callbacks);

    return COMAC_STATUS_SUCCESS;
}

static void
add_record_stroke (comac_observation_t *log,
		   comac_surface_t *target,
		   comac_operator_t op,
		   const comac_pattern_t *source,
		   const comac_path_fixed_t *path,
		   const comac_stroke_style_t *style,
		   const comac_matrix_t *ctm,
		   const comac_matrix_t *ctm_inverse,
		   double tolerance,
		   comac_antialias_t antialias,
		   const comac_clip_t *clip,
		   comac_time_t elapsed)
{
    comac_observation_record_t record;
    comac_int_status_t status;

    add_record (log,
		record_stroke (&record,
			       target,
			       op,
			       source,
			       path,
			       style,
			       ctm,
			       ctm_inverse,
			       tolerance,
			       antialias,
			       clip,
			       elapsed));

    if (log->record) {
	status = log->record->base.backend->stroke (&log->record->base,
						    op,
						    source,
						    path,
						    style,
						    ctm,
						    ctm_inverse,
						    tolerance,
						    antialias,
						    clip);
	assert (status == COMAC_INT_STATUS_SUCCESS);
    }

    if (_comac_time_gt (elapsed, log->stroke.slowest.elapsed))
	log->stroke.slowest = record;
    log->stroke.elapsed = _comac_time_add (log->stroke.elapsed, elapsed);
}

static comac_int_status_t
_comac_surface_observer_stroke (void *abstract_surface,
				comac_operator_t op,
				const comac_pattern_t *source,
				const comac_path_fixed_t *path,
				const comac_stroke_style_t *style,
				const comac_matrix_t *ctm,
				const comac_matrix_t *ctm_inverse,
				double tolerance,
				comac_antialias_t antialias,
				const comac_clip_t *clip)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_device_observer_t *device = to_device (surface);
    comac_composite_rectangles_t composite;
    comac_int_status_t status;
    comac_time_t t;
    int x, y;

    surface->log.stroke.count++;
    surface->log.stroke.operators[op]++;
    surface->log.stroke.antialias[antialias]++;
    surface->log.stroke.caps[style->line_cap]++;
    surface->log.stroke.joins[style->line_join]++;
    add_pattern (&surface->log.stroke.source, source, surface->target);
    add_path (&surface->log.stroke.path, path, FALSE);
    add_clip (&surface->log.stroke.clip, clip);

    device->log.stroke.count++;
    device->log.stroke.operators[op]++;
    device->log.stroke.antialias[antialias]++;
    device->log.stroke.caps[style->line_cap]++;
    device->log.stroke.joins[style->line_join]++;
    add_pattern (&device->log.stroke.source, source, surface->target);
    add_path (&device->log.stroke.path, path, FALSE);
    add_clip (&device->log.stroke.clip, clip);

    status = _comac_composite_rectangles_init_for_stroke (&composite,
							  surface->target,
							  op,
							  source,
							  path,
							  style,
							  ctm,
							  clip);
    if (unlikely (status)) {
	surface->log.stroke.noop++;
	device->log.stroke.noop++;
	return status;
    }

    midpt (&composite, &x, &y);

    add_extents (&surface->log.stroke.extents, &composite);
    add_extents (&device->log.stroke.extents, &composite);
    _comac_composite_rectangles_fini (&composite);

    t = _comac_time_get ();
    status = _comac_surface_stroke (surface->target,
				    op,
				    source,
				    path,
				    style,
				    ctm,
				    ctm_inverse,
				    tolerance,
				    antialias,
				    clip);
    if (unlikely (status))
	return status;

    _comac_surface_sync (surface->target, x, y);
    t = _comac_time_get_delta (t);

    add_record_stroke (&surface->log,
		       surface->target,
		       op,
		       source,
		       path,
		       style,
		       ctm,
		       ctm_inverse,
		       tolerance,
		       antialias,
		       clip,
		       t);

    add_record_stroke (&device->log,
		       surface->target,
		       op,
		       source,
		       path,
		       style,
		       ctm,
		       ctm_inverse,
		       tolerance,
		       antialias,
		       clip,
		       t);

    do_callbacks (surface, &surface->stroke_callbacks);

    return COMAC_STATUS_SUCCESS;
}

static void
add_record_glyphs (comac_observation_t *log,
		   comac_surface_t *target,
		   comac_operator_t op,
		   const comac_pattern_t *source,
		   comac_glyph_t *glyphs,
		   int num_glyphs,
		   comac_scaled_font_t *scaled_font,
		   const comac_clip_t *clip,
		   comac_time_t elapsed)
{
    comac_observation_record_t record;
    comac_int_status_t status;

    add_record (log,
		record_glyphs (&record,
			       target,
			       op,
			       source,
			       glyphs,
			       num_glyphs,
			       scaled_font,
			       clip,
			       elapsed));

    if (log->record) {
	status =
	    log->record->base.backend->show_text_glyphs (&log->record->base,
							 op,
							 source,
							 NULL,
							 0,
							 glyphs,
							 num_glyphs,
							 NULL,
							 0,
							 0,
							 scaled_font,
							 clip);
	assert (status == COMAC_INT_STATUS_SUCCESS);
    }

    if (_comac_time_gt (elapsed, log->glyphs.slowest.elapsed))
	log->glyphs.slowest = record;
    log->glyphs.elapsed = _comac_time_add (log->glyphs.elapsed, elapsed);
}

static comac_int_status_t
_comac_surface_observer_glyphs (void *abstract_surface,
				comac_operator_t op,
				const comac_pattern_t *source,
				comac_glyph_t *glyphs,
				int num_glyphs,
				comac_scaled_font_t *scaled_font,
				const comac_clip_t *clip)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_device_observer_t *device = to_device (surface);
    comac_composite_rectangles_t composite;
    comac_int_status_t status;
    comac_glyph_t *dev_glyphs;
    comac_time_t t;
    int x, y;

    surface->log.glyphs.count++;
    surface->log.glyphs.operators[op]++;
    add_pattern (&surface->log.glyphs.source, source, surface->target);
    add_clip (&surface->log.glyphs.clip, clip);

    device->log.glyphs.count++;
    device->log.glyphs.operators[op]++;
    add_pattern (&device->log.glyphs.source, source, surface->target);
    add_clip (&device->log.glyphs.clip, clip);

    status = _comac_composite_rectangles_init_for_glyphs (&composite,
							  surface->target,
							  op,
							  source,
							  scaled_font,
							  glyphs,
							  num_glyphs,
							  clip,
							  NULL);
    if (unlikely (status)) {
	surface->log.glyphs.noop++;
	device->log.glyphs.noop++;
	return status;
    }

    midpt (&composite, &x, &y);

    add_extents (&surface->log.glyphs.extents, &composite);
    add_extents (&device->log.glyphs.extents, &composite);
    _comac_composite_rectangles_fini (&composite);

    /* XXX We have to copy the glyphs, because the backend is allowed to
     * modify! */
    dev_glyphs = _comac_malloc_ab (num_glyphs, sizeof (comac_glyph_t));
    if (unlikely (dev_glyphs == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);
    memcpy (dev_glyphs, glyphs, num_glyphs * sizeof (comac_glyph_t));

    t = _comac_time_get ();
    status = _comac_surface_show_text_glyphs (surface->target,
					      op,
					      source,
					      NULL,
					      0,
					      dev_glyphs,
					      num_glyphs,
					      NULL,
					      0,
					      0,
					      scaled_font,
					      clip);
    free (dev_glyphs);
    if (unlikely (status))
	return status;

    _comac_surface_sync (surface->target, x, y);
    t = _comac_time_get_delta (t);

    add_record_glyphs (&surface->log,
		       surface->target,
		       op,
		       source,
		       glyphs,
		       num_glyphs,
		       scaled_font,
		       clip,
		       t);

    add_record_glyphs (&device->log,
		       surface->target,
		       op,
		       source,
		       glyphs,
		       num_glyphs,
		       scaled_font,
		       clip,
		       t);

    do_callbacks (surface, &surface->glyphs_callbacks);

    return COMAC_STATUS_SUCCESS;
}

static comac_status_t
_comac_surface_observer_flush (void *abstract_surface, unsigned flags)
{
    comac_surface_observer_t *surface = abstract_surface;

    do_callbacks (surface, &surface->flush_callbacks);
    return _comac_surface_flush (surface->target, flags);
}

static comac_status_t
_comac_surface_observer_mark_dirty (
    void *abstract_surface, int x, int y, int width, int height)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_status_t status;

    status = COMAC_STATUS_SUCCESS;
    if (surface->target->backend->mark_dirty_rectangle)
	status =
	    surface->target->backend->mark_dirty_rectangle (surface->target,
							    x,
							    y,
							    width,
							    height);

    return status;
}

static comac_int_status_t
_comac_surface_observer_copy_page (void *abstract_surface)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_status_t status;

    status = COMAC_STATUS_SUCCESS;
    if (surface->target->backend->copy_page)
	status = surface->target->backend->copy_page (surface->target);

    return status;
}

static comac_int_status_t
_comac_surface_observer_show_page (void *abstract_surface)
{
    comac_surface_observer_t *surface = abstract_surface;
    comac_status_t status;

    status = COMAC_STATUS_SUCCESS;
    if (surface->target->backend->show_page)
	status = surface->target->backend->show_page (surface->target);

    return status;
}

static comac_bool_t
_comac_surface_observer_get_extents (void *abstract_surface,
				     comac_rectangle_int_t *extents)
{
    comac_surface_observer_t *surface = abstract_surface;
    return _comac_surface_get_extents (surface->target, extents);
}

static void
_comac_surface_observer_get_font_options (void *abstract_surface,
					  comac_font_options_t *options)
{
    comac_surface_observer_t *surface = abstract_surface;

    if (surface->target->backend->get_font_options != NULL)
	surface->target->backend->get_font_options (surface->target, options);
}

static comac_surface_t *
_comac_surface_observer_source (void *abstract_surface,
				comac_rectangle_int_t *extents)
{
    comac_surface_observer_t *surface = abstract_surface;
    return _comac_surface_get_source (surface->target, extents);
}

static comac_status_t
_comac_surface_observer_acquire_source_image (void *abstract_surface,
					      comac_image_surface_t **image_out,
					      void **image_extra)
{
    comac_surface_observer_t *surface = abstract_surface;

    surface->log.num_sources_acquired++;
    to_device (surface)->log.num_sources_acquired++;

    return _comac_surface_acquire_source_image (surface->target,
						image_out,
						image_extra);
}

static void
_comac_surface_observer_release_source_image (void *abstract_surface,
					      comac_image_surface_t *image,
					      void *image_extra)
{
    comac_surface_observer_t *surface = abstract_surface;

    _comac_surface_release_source_image (surface->target, image, image_extra);
}

static comac_surface_t *
_comac_surface_observer_snapshot (void *abstract_surface)
{
    comac_surface_observer_t *surface = abstract_surface;

    /* XXX hook onto the snapshot so that we measure number of reads */

    if (surface->target->backend->snapshot)
	return surface->target->backend->snapshot (surface->target);

    return NULL;
}

static comac_t *
_comac_surface_observer_create_context (void *target)
{
    comac_surface_observer_t *surface = target;

    if (_comac_surface_is_subsurface (&surface->base))
	surface =
	    (comac_surface_observer_t *) _comac_surface_subsurface_get_target (
		&surface->base);

    surface->log.num_contexts++;
    to_device (surface)->log.num_contexts++;

    return surface->target->backend->create_context (target);
}

static const comac_surface_backend_t _comac_surface_observer_backend = {
    COMAC_INTERNAL_SURFACE_TYPE_OBSERVER,
    _comac_surface_observer_finish,

    _comac_surface_observer_create_context,

    _comac_surface_observer_create_similar,
    _comac_surface_observer_create_similar_image,
    _comac_surface_observer_map_to_image,
    _comac_surface_observer_unmap_image,

    _comac_surface_observer_source,
    _comac_surface_observer_acquire_source_image,
    _comac_surface_observer_release_source_image,
    _comac_surface_observer_snapshot,

    _comac_surface_observer_copy_page,
    _comac_surface_observer_show_page,

    _comac_surface_observer_get_extents,
    _comac_surface_observer_get_font_options,

    _comac_surface_observer_flush,
    _comac_surface_observer_mark_dirty,

    _comac_surface_observer_paint,
    _comac_surface_observer_mask,
    _comac_surface_observer_stroke,
    _comac_surface_observer_fill,
    NULL, /* fill-stroke */
    _comac_surface_observer_glyphs,
};

/**
 * comac_surface_create_observer:
 * @target: an existing surface for which the observer will watch
 * @mode: sets the mode of operation (normal vs. record)
 *
 * Create a new surface that exists solely to watch another is doing. In
 * the process it will log operations and times, which are fast, which are
 * slow, which are frequent, etc.
 *
 * The @mode parameter can be set to either COMAC_SURFACE_OBSERVER_NORMAL
 * or COMAC_SURFACE_OBSERVER_RECORD_OPERATIONS, to control whether or not
 * the internal observer should record operations.
 *
 * Return value: a pointer to the newly allocated surface. The caller
 * owns the surface and should call comac_surface_destroy() when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs.
 *
 * Since: 1.12
 **/
comac_surface_t *
comac_surface_create_observer (comac_surface_t *target,
			       comac_surface_observer_mode_t mode)
{
    comac_device_t *device;
    comac_surface_t *surface;
    comac_bool_t record;

    if (unlikely (target->status))
	return _comac_surface_create_in_error (target->status);
    if (unlikely (target->finished))
	return _comac_surface_create_in_error (
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    record = mode & COMAC_SURFACE_OBSERVER_RECORD_OPERATIONS;
    device = _comac_device_create_observer_internal (target->device, record);
    if (unlikely (device->status))
	return _comac_surface_create_in_error (device->status);

    surface = _comac_surface_create_observer_internal (device, target);
    comac_device_destroy (device);

    return surface;
}

static comac_status_t
_comac_surface_observer_add_callback (comac_list_t *head,
				      comac_surface_observer_callback_t func,
				      void *data)
{
    struct callback_list *cb;

    cb = _comac_malloc (sizeof (*cb));
    if (unlikely (cb == NULL))
	return _comac_error (COMAC_STATUS_NO_MEMORY);

    comac_list_add (&cb->link, head);
    cb->func = func;
    cb->data = data;

    return COMAC_STATUS_SUCCESS;
}

comac_status_t
comac_surface_observer_add_paint_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return abstract_surface->status;

    if (! _comac_surface_is_observer (abstract_surface))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_surface_observer_add_callback (&surface->paint_callbacks,
						 func,
						 data);
}

comac_status_t
comac_surface_observer_add_mask_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return abstract_surface->status;

    if (! _comac_surface_is_observer (abstract_surface))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_surface_observer_add_callback (&surface->mask_callbacks,
						 func,
						 data);
}

comac_status_t
comac_surface_observer_add_fill_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return abstract_surface->status;

    if (! _comac_surface_is_observer (abstract_surface))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_surface_observer_add_callback (&surface->fill_callbacks,
						 func,
						 data);
}

comac_status_t
comac_surface_observer_add_stroke_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return abstract_surface->status;

    if (! _comac_surface_is_observer (abstract_surface))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_surface_observer_add_callback (&surface->stroke_callbacks,
						 func,
						 data);
}

comac_status_t
comac_surface_observer_add_glyphs_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return abstract_surface->status;

    if (! _comac_surface_is_observer (abstract_surface))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_surface_observer_add_callback (&surface->glyphs_callbacks,
						 func,
						 data);
}

comac_status_t
comac_surface_observer_add_flush_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return abstract_surface->status;

    if (! _comac_surface_is_observer (abstract_surface))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_surface_observer_add_callback (&surface->flush_callbacks,
						 func,
						 data);
}

comac_status_t
comac_surface_observer_add_finish_callback (
    comac_surface_t *abstract_surface,
    comac_surface_observer_callback_t func,
    void *data)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return abstract_surface->status;

    if (! _comac_surface_is_observer (abstract_surface))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_surface_observer_add_callback (&surface->finish_callbacks,
						 func,
						 data);
}

static void
print_extents (comac_output_stream_t *stream, const struct extents *e)
{
    _comac_output_stream_printf (stream,
				 "  extents: total %g, avg %g [unbounded %d]\n",
				 e->area.sum,
				 e->area.sum / e->area.count,
				 e->unbounded);
}

static inline int
ordercmp (int a, int b, const unsigned int *array)
{
    /* high to low */
    return array[b] - array[a];
}
COMAC_COMBSORT_DECLARE_WITH_DATA (sort_order, int, ordercmp)

static void
print_array (comac_output_stream_t *stream,
	     const unsigned int *array,
	     const char **names,
	     int count)
{
    int order[64];
    int i, j;

    assert (count < ARRAY_LENGTH (order));
    for (i = j = 0; i < count; i++) {
	if (array[i] != 0)
	    order[j++] = i;
    }

    sort_order (order, j, (void *) array);
    for (i = 0; i < j; i++)
	_comac_output_stream_printf (stream,
				     " %d %s%s",
				     array[order[i]],
				     names[order[i]],
				     i < j - 1 ? "," : "");
}

static const char *operator_names[] = {
    "CLEAR", /* COMAC_OPERATOR_CLEAR */

    "SOURCE", /* COMAC_OPERATOR_SOURCE */
    "OVER",   /* COMAC_OPERATOR_OVER */
    "IN",     /* COMAC_OPERATOR_IN */
    "OUT",    /* COMAC_OPERATOR_OUT */
    "ATOP",   /* COMAC_OPERATOR_ATOP */

    "DEST",	 /* COMAC_OPERATOR_DEST */
    "DEST_OVER", /* COMAC_OPERATOR_DEST_OVER */
    "DEST_IN",	 /* COMAC_OPERATOR_DEST_IN */
    "DEST_OUT",	 /* COMAC_OPERATOR_DEST_OUT */
    "DEST_ATOP", /* COMAC_OPERATOR_DEST_ATOP */

    "XOR",	/* COMAC_OPERATOR_XOR */
    "ADD",	/* COMAC_OPERATOR_ADD */
    "SATURATE", /* COMAC_OPERATOR_SATURATE */

    "MULTIPLY",	      /* COMAC_OPERATOR_MULTIPLY */
    "SCREEN",	      /* COMAC_OPERATOR_SCREEN */
    "OVERLAY",	      /* COMAC_OPERATOR_OVERLAY */
    "DARKEN",	      /* COMAC_OPERATOR_DARKEN */
    "LIGHTEN",	      /* COMAC_OPERATOR_LIGHTEN */
    "DODGE",	      /* COMAC_OPERATOR_COLOR_DODGE */
    "BURN",	      /* COMAC_OPERATOR_COLOR_BURN */
    "HARD_LIGHT",     /* COMAC_OPERATOR_HARD_LIGHT */
    "SOFT_LIGHT",     /* COMAC_OPERATOR_SOFT_LIGHT */
    "DIFFERENCE",     /* COMAC_OPERATOR_DIFFERENCE */
    "EXCLUSION",      /* COMAC_OPERATOR_EXCLUSION */
    "HSL_HUE",	      /* COMAC_OPERATOR_HSL_HUE */
    "HSL_SATURATION", /* COMAC_OPERATOR_HSL_SATURATION */
    "HSL_COLOR",      /* COMAC_OPERATOR_HSL_COLOR */
    "HSL_LUMINOSITY"  /* COMAC_OPERATOR_HSL_LUMINOSITY */
};
static void
print_operators (comac_output_stream_t *stream, unsigned int *array)
{
    _comac_output_stream_printf (stream, "  op:");
    print_array (stream, array, operator_names, NUM_OPERATORS);
    _comac_output_stream_printf (stream, "\n");
}

static const char *fill_rule_names[] = {
    "non-zero",
    "even-odd",
};
static void
print_fill_rule (comac_output_stream_t *stream, unsigned int *array)
{
    _comac_output_stream_printf (stream, "  fill rule:");
    print_array (stream,
		 array,
		 fill_rule_names,
		 ARRAY_LENGTH (fill_rule_names));
    _comac_output_stream_printf (stream, "\n");
}

static const char *cap_names[] = {
    "butt",  /* COMAC_LINE_CAP_BUTT */
    "round", /* COMAC_LINE_CAP_ROUND */
    "square" /* COMAC_LINE_CAP_SQUARE */
};
static void
print_line_caps (comac_output_stream_t *stream, unsigned int *array)
{
    _comac_output_stream_printf (stream, "  caps:");
    print_array (stream, array, cap_names, NUM_CAPS);
    _comac_output_stream_printf (stream, "\n");
}

static const char *join_names[] = {
    "miter", /* COMAC_LINE_JOIN_MITER */
    "round", /* COMAC_LINE_JOIN_ROUND */
    "bevel", /* COMAC_LINE_JOIN_BEVEL */
};
static void
print_line_joins (comac_output_stream_t *stream, unsigned int *array)
{
    _comac_output_stream_printf (stream, "  joins:");
    print_array (stream, array, join_names, NUM_JOINS);
    _comac_output_stream_printf (stream, "\n");
}

static const char *antialias_names[] = {
    "default", "none", "gray", "subpixel", "fast", "good", "best"};
static void
print_antialias (comac_output_stream_t *stream, unsigned int *array)
{
    _comac_output_stream_printf (stream, "  antialias:");
    print_array (stream, array, antialias_names, NUM_ANTIALIAS);
    _comac_output_stream_printf (stream, "\n");
}

static const char *pattern_names[] = {"native",
				      "record",
				      "other surface",
				      "solid",
				      "linear",
				      "radial",
				      "mesh",
				      "raster"};
static void
print_pattern (comac_output_stream_t *stream,
	       const char *name,
	       const struct pattern *p)
{
    _comac_output_stream_printf (stream, "  %s:", name);
    print_array (stream, p->type, pattern_names, ARRAY_LENGTH (pattern_names));
    _comac_output_stream_printf (stream, "\n");
}

static const char *path_names[] = {
    "empty",
    "pixel-aligned",
    "rectliinear",
    "straight",
    "curved",
};
static void
print_path (comac_output_stream_t *stream, const struct path *p)
{
    _comac_output_stream_printf (stream, "  path:");
    print_array (stream, p->type, path_names, ARRAY_LENGTH (path_names));
    _comac_output_stream_printf (stream, "\n");
}

static const char *clip_names[] = {
    "none",
    "region",
    "boxes",
    "single path",
    "polygon",
    "general",
};
static void
print_clip (comac_output_stream_t *stream, const struct clip *c)
{
    _comac_output_stream_printf (stream, "  clip:");
    print_array (stream, c->type, clip_names, ARRAY_LENGTH (clip_names));
    _comac_output_stream_printf (stream, "\n");
}

static void
print_record (comac_output_stream_t *stream, comac_observation_record_t *r)
{
    _comac_output_stream_printf (stream, "  op: %s\n", operator_names[r->op]);
    _comac_output_stream_printf (stream,
				 "  source: %s\n",
				 pattern_names[r->source]);
    if (r->mask != -1)
	_comac_output_stream_printf (stream,
				     "  mask: %s\n",
				     pattern_names[r->mask]);
    if (r->num_glyphs != -1)
	_comac_output_stream_printf (stream,
				     "  num_glyphs: %d\n",
				     r->num_glyphs);
    if (r->path != -1)
	_comac_output_stream_printf (stream,
				     "  path: %s\n",
				     path_names[r->path]);
    if (r->fill_rule != -1)
	_comac_output_stream_printf (stream,
				     "  fill rule: %s\n",
				     fill_rule_names[r->fill_rule]);
    if (r->antialias != -1)
	_comac_output_stream_printf (stream,
				     "  antialias: %s\n",
				     antialias_names[r->antialias]);
    _comac_output_stream_printf (stream, "  clip: %s\n", clip_names[r->clip]);
    _comac_output_stream_printf (stream,
				 "  elapsed: %f ns\n",
				 _comac_time_to_ns (r->elapsed));
}

static double
percent (comac_time_t a, comac_time_t b)
{
    /* Fake %.1f */
    return _comac_round (_comac_time_to_s (a) * 1000 / _comac_time_to_s (b)) /
	   10;
}

static comac_bool_t
replay_record (comac_observation_t *log,
	       comac_observation_record_t *r,
	       comac_device_t *script)
{
#if COMAC_HAS_SCRIPT_SURFACE
    comac_surface_t *surface;
    comac_int_status_t status;

    if (log->record == NULL || script == NULL)
	return FALSE;

    surface = comac_script_surface_create (script,
					   r->target_content,
					   r->target_width,
					   r->target_height);
    status =
	_comac_recording_surface_replay_one (log->record, r->index, surface);
    comac_surface_destroy (surface);

    assert (status == COMAC_INT_STATUS_SUCCESS);

    return TRUE;
#else
    return FALSE;
#endif
}

static comac_time_t
_comac_observation_total_elapsed (comac_observation_t *log)
{
    comac_time_t total;

    total = log->paint.elapsed;
    total = _comac_time_add (total, log->mask.elapsed);
    total = _comac_time_add (total, log->fill.elapsed);
    total = _comac_time_add (total, log->stroke.elapsed);
    total = _comac_time_add (total, log->glyphs.elapsed);

    return total;
}

static void
_comac_observation_print (comac_output_stream_t *stream,
			  comac_observation_t *log)
{
    comac_device_t *script;
    comac_time_t total;

#if COMAC_HAS_SCRIPT_SURFACE
    script = _comac_script_context_create_internal (stream);
    _comac_script_context_attach_snapshots (script, FALSE);
#else
    script = NULL;
#endif

    total = _comac_observation_total_elapsed (log);

    _comac_output_stream_printf (stream,
				 "elapsed: %f\n",
				 _comac_time_to_ns (total));
    _comac_output_stream_printf (stream, "surfaces: %d\n", log->num_surfaces);
    _comac_output_stream_printf (stream, "contexts: %d\n", log->num_contexts);
    _comac_output_stream_printf (stream,
				 "sources acquired: %d\n",
				 log->num_sources_acquired);

    _comac_output_stream_printf (
	stream,
	"paint: count %d [no-op %d], elapsed %f [%f%%]\n",
	log->paint.count,
	log->paint.noop,
	_comac_time_to_ns (log->paint.elapsed),
	percent (log->paint.elapsed, total));
    if (log->paint.count) {
	print_extents (stream, &log->paint.extents);
	print_operators (stream, log->paint.operators);
	print_pattern (stream, "source", &log->paint.source);
	print_clip (stream, &log->paint.clip);

	_comac_output_stream_printf (
	    stream,
	    "slowest paint: %f%%\n",
	    percent (log->paint.slowest.elapsed, log->paint.elapsed));
	print_record (stream, &log->paint.slowest);

	_comac_output_stream_printf (stream, "\n");
	if (replay_record (log, &log->paint.slowest, script))
	    _comac_output_stream_printf (stream, "\n\n");
    }

    _comac_output_stream_printf (
	stream,
	"mask: count %d [no-op %d], elapsed %f [%f%%]\n",
	log->mask.count,
	log->mask.noop,
	_comac_time_to_ns (log->mask.elapsed),
	percent (log->mask.elapsed, total));
    if (log->mask.count) {
	print_extents (stream, &log->mask.extents);
	print_operators (stream, log->mask.operators);
	print_pattern (stream, "source", &log->mask.source);
	print_pattern (stream, "mask", &log->mask.mask);
	print_clip (stream, &log->mask.clip);

	_comac_output_stream_printf (
	    stream,
	    "slowest mask: %f%%\n",
	    percent (log->mask.slowest.elapsed, log->mask.elapsed));
	print_record (stream, &log->mask.slowest);

	_comac_output_stream_printf (stream, "\n");
	if (replay_record (log, &log->mask.slowest, script))
	    _comac_output_stream_printf (stream, "\n\n");
    }

    _comac_output_stream_printf (
	stream,
	"fill: count %d [no-op %d], elaspsed %f [%f%%]\n",
	log->fill.count,
	log->fill.noop,
	_comac_time_to_ns (log->fill.elapsed),
	percent (log->fill.elapsed, total));
    if (log->fill.count) {
	print_extents (stream, &log->fill.extents);
	print_operators (stream, log->fill.operators);
	print_pattern (stream, "source", &log->fill.source);
	print_path (stream, &log->fill.path);
	print_fill_rule (stream, log->fill.fill_rule);
	print_antialias (stream, log->fill.antialias);
	print_clip (stream, &log->fill.clip);

	_comac_output_stream_printf (
	    stream,
	    "slowest fill: %f%%\n",
	    percent (log->fill.slowest.elapsed, log->fill.elapsed));
	print_record (stream, &log->fill.slowest);

	_comac_output_stream_printf (stream, "\n");
	if (replay_record (log, &log->fill.slowest, script))
	    _comac_output_stream_printf (stream, "\n\n");
    }

    _comac_output_stream_printf (
	stream,
	"stroke: count %d [no-op %d], elapsed %f [%f%%]\n",
	log->stroke.count,
	log->stroke.noop,
	_comac_time_to_ns (log->stroke.elapsed),
	percent (log->stroke.elapsed, total));
    if (log->stroke.count) {
	print_extents (stream, &log->stroke.extents);
	print_operators (stream, log->stroke.operators);
	print_pattern (stream, "source", &log->stroke.source);
	print_path (stream, &log->stroke.path);
	print_antialias (stream, log->stroke.antialias);
	print_line_caps (stream, log->stroke.caps);
	print_line_joins (stream, log->stroke.joins);
	print_clip (stream, &log->stroke.clip);

	_comac_output_stream_printf (
	    stream,
	    "slowest stroke: %f%%\n",
	    percent (log->stroke.slowest.elapsed, log->stroke.elapsed));
	print_record (stream, &log->stroke.slowest);

	_comac_output_stream_printf (stream, "\n");
	if (replay_record (log, &log->stroke.slowest, script))
	    _comac_output_stream_printf (stream, "\n\n");
    }

    _comac_output_stream_printf (
	stream,
	"glyphs: count %d [no-op %d], elasped %f [%f%%]\n",
	log->glyphs.count,
	log->glyphs.noop,
	_comac_time_to_ns (log->glyphs.elapsed),
	percent (log->glyphs.elapsed, total));
    if (log->glyphs.count) {
	print_extents (stream, &log->glyphs.extents);
	print_operators (stream, log->glyphs.operators);
	print_pattern (stream, "source", &log->glyphs.source);
	print_clip (stream, &log->glyphs.clip);

	_comac_output_stream_printf (
	    stream,
	    "slowest glyphs: %f%%\n",
	    percent (log->glyphs.slowest.elapsed, log->glyphs.elapsed));
	print_record (stream, &log->glyphs.slowest);

	_comac_output_stream_printf (stream, "\n");
	if (replay_record (log, &log->glyphs.slowest, script))
	    _comac_output_stream_printf (stream, "\n\n");
    }

    comac_device_destroy (script);
}

comac_status_t
comac_surface_observer_print (comac_surface_t *abstract_surface,
			      comac_write_func_t write_func,
			      void *closure)
{
    comac_output_stream_t *stream;
    comac_surface_observer_t *surface;

    if (unlikely (abstract_surface->status))
	return abstract_surface->status;

    if (unlikely (! _comac_surface_is_observer (abstract_surface)))
	return _comac_error (COMAC_STATUS_SURFACE_TYPE_MISMATCH);

    surface = (comac_surface_observer_t *) abstract_surface;

    stream = _comac_output_stream_create (write_func, NULL, closure);
    _comac_observation_print (stream, &surface->log);
    return _comac_output_stream_destroy (stream);
}

double
comac_surface_observer_elapsed (comac_surface_t *abstract_surface)
{
    comac_surface_observer_t *surface;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_surface->ref_count)))
	return -1;

    if (! _comac_surface_is_observer (abstract_surface))
	return -1;

    surface = (comac_surface_observer_t *) abstract_surface;
    return _comac_time_to_ns (_comac_observation_total_elapsed (&surface->log));
}

comac_status_t
comac_device_observer_print (comac_device_t *abstract_device,
			     comac_write_func_t write_func,
			     void *closure)
{
    comac_output_stream_t *stream;
    comac_device_observer_t *device;

    if (unlikely (abstract_device->status))
	return abstract_device->status;

    if (unlikely (! _comac_device_is_observer (abstract_device)))
	return _comac_error (COMAC_STATUS_DEVICE_TYPE_MISMATCH);

    device = (comac_device_observer_t *) abstract_device;

    stream = _comac_output_stream_create (write_func, NULL, closure);
    _comac_observation_print (stream, &device->log);
    return _comac_output_stream_destroy (stream);
}

double
comac_device_observer_elapsed (comac_device_t *abstract_device)
{
    comac_device_observer_t *device;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_device->ref_count)))
	return -1;

    if (! _comac_device_is_observer (abstract_device))
	return -1;

    device = (comac_device_observer_t *) abstract_device;
    return _comac_time_to_ns (_comac_observation_total_elapsed (&device->log));
}

double
comac_device_observer_paint_elapsed (comac_device_t *abstract_device)
{
    comac_device_observer_t *device;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_device->ref_count)))
	return -1;

    if (! _comac_device_is_observer (abstract_device))
	return -1;

    device = (comac_device_observer_t *) abstract_device;
    return _comac_time_to_ns (device->log.paint.elapsed);
}

double
comac_device_observer_mask_elapsed (comac_device_t *abstract_device)
{
    comac_device_observer_t *device;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_device->ref_count)))
	return -1;

    if (! _comac_device_is_observer (abstract_device))
	return -1;

    device = (comac_device_observer_t *) abstract_device;
    return _comac_time_to_ns (device->log.mask.elapsed);
}

double
comac_device_observer_fill_elapsed (comac_device_t *abstract_device)
{
    comac_device_observer_t *device;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_device->ref_count)))
	return -1;

    if (! _comac_device_is_observer (abstract_device))
	return -1;

    device = (comac_device_observer_t *) abstract_device;
    return _comac_time_to_ns (device->log.fill.elapsed);
}

double
comac_device_observer_stroke_elapsed (comac_device_t *abstract_device)
{
    comac_device_observer_t *device;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_device->ref_count)))
	return -1;

    if (! _comac_device_is_observer (abstract_device))
	return -1;

    device = (comac_device_observer_t *) abstract_device;
    return _comac_time_to_ns (device->log.stroke.elapsed);
}

double
comac_device_observer_glyphs_elapsed (comac_device_t *abstract_device)
{
    comac_device_observer_t *device;

    if (unlikely (
	    COMAC_REFERENCE_COUNT_IS_INVALID (&abstract_device->ref_count)))
	return -1;

    if (! _comac_device_is_observer (abstract_device))
	return -1;

    device = (comac_device_observer_t *) abstract_device;
    return _comac_time_to_ns (device->log.glyphs.elapsed);
}
