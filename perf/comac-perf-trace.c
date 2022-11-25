/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright © 2006 Mozilla Corporation
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2009 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * the authors not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The authors make no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Vladimir Vukicevic <vladimir@pobox.com>
 *	    Carl Worth <cworth@cworth.org>
 *	    Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "config.h"

#include "comac-missing.h"
#include "comac-perf.h"
#include "comac-stats.h"

#include "comac-boilerplate-getopt.h"
#include <comac-script-interpreter.h>
#include <comac-types-private.h> /* for INTERNAL_SURFACE_TYPE */

/* rudely reuse bits of the library... */
#include "../src/comac-hash-private.h"
#include "../src/comac-error-private.h"

/* For basename */
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <ctype.h> /* isspace() */

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include "dirent-win32.h"

static char *
basename_no_ext (char *path)
{
    static char name[_MAX_FNAME + 1];

    _splitpath (path, NULL, NULL, name, NULL);

    name[_MAX_FNAME] = '\0';

    return name;
}


#else
#include <dirent.h>

static char *
basename_no_ext (char *path)
{
    char *dot, *name;

    name = basename (path);

    dot = strrchr (name, '.');
    if (dot)
	*dot = '\0';

    return name;
}

#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>

#if HAVE_FCFINI
#include <fontconfig/fontconfig.h>
#endif

#define COMAC_PERF_ITERATIONS_DEFAULT	15
#define COMAC_PERF_LOW_STD_DEV		0.05
#define COMAC_PERF_MIN_STD_DEV_COUNT	3
#define COMAC_PERF_STABLE_STD_DEV_COUNT 3

struct trace {
    const comac_boilerplate_target_t *target;
    void            *closure;
    comac_surface_t *surface;
    comac_bool_t observe;
    int tile_size;
};

comac_bool_t
comac_perf_can_run (comac_perf_t *perf,
		    const char	 *name,
		    comac_bool_t *is_explicit)
{
    unsigned int i;
    char *copy, *dot;
    comac_bool_t ret;

    if (is_explicit)
	*is_explicit = FALSE;

    if (perf->exact_names) {
	if (is_explicit)
	    *is_explicit = TRUE;
	return TRUE;
    }

    if (perf->num_names == 0 && perf->num_exclude_names == 0)
	return TRUE;

    copy = xstrdup (name);
    dot = strrchr (copy, '.');
    if (dot != NULL)
	*dot = '\0';

    if (perf->num_names) {
	ret = TRUE;
	for (i = 0; i < perf->num_names; i++)
	    if (strstr (copy, perf->names[i])) {
		if (is_explicit)
		    *is_explicit = strcmp (copy, perf->names[i]) == 0;
		goto check_exclude;
	    }

	ret = FALSE;
	goto done;
    }

check_exclude:
    if (perf->num_exclude_names) {
	ret = FALSE;
	for (i = 0; i < perf->num_exclude_names; i++)
	    if (strstr (copy, perf->exclude_names[i])) {
		if (is_explicit)
		    *is_explicit = strcmp (copy, perf->exclude_names[i]) == 0;
		goto done;
	    }

	ret = TRUE;
	goto done;
    }

done:
    free (copy);

    return ret;
}

static void
fill_surface (comac_surface_t *surface)
{
    comac_t *cr = comac_create (surface);
    /* This needs to be an operation that the backends can't optimise away */
    comac_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
    comac_set_operator (cr, COMAC_OPERATOR_OVER);
    comac_paint (cr);
    comac_destroy (cr);
}

struct scache {
    comac_hash_entry_t entry;
    comac_content_t content;
    int width, height;
    comac_surface_t *surface;
};

static comac_hash_table_t *surface_cache;
static comac_surface_t *surface_holdovers[16];

static comac_bool_t
scache_equal (const void *A,
	      const void *B)
{
    const struct scache *a = A, *b = B;
    return a->entry.hash == b->entry.hash;
}

static void
scache_mark_active (comac_surface_t *surface)
{
    comac_surface_t *t0, *t1;
    unsigned n;

    if (surface_cache == NULL)
	return;

    t0 = comac_surface_reference (surface);
    for (n = 0; n < ARRAY_LENGTH (surface_holdovers); n++) {
	if (surface_holdovers[n] == surface) {
	    surface_holdovers[n] = t0;
	    t0 = surface;
	    break;
	}

	t1 = surface_holdovers[n];
	surface_holdovers[n] = t0;
	t0 = t1;
    }
    comac_surface_destroy (t0);
}

static void
scache_clear (void)
{
    unsigned n;

    if (surface_cache == NULL)
	return;

    for (n = 0; n < ARRAY_LENGTH (surface_holdovers); n++) {
	comac_surface_destroy (surface_holdovers[n]);
	surface_holdovers[n] = NULL;
    }
}

static void
scache_remove (void *closure)
{
    _comac_hash_table_remove (surface_cache, closure);
    free (closure);
}

static comac_surface_t *
_similar_surface_create (void		 *closure,
			 comac_content_t  content,
			 double		  width,
			 double		  height,
			 long		  uid)
{
    struct trace *args = closure;
    comac_surface_t *surface;
    struct scache skey, *s;

    if (args->observe)
	    return comac_surface_create_similar (args->surface,
						 content, width, height);

    if (uid == 0 || surface_cache == NULL)
	return args->target->create_similar (args->surface, content, width, height);

    skey.entry.hash = uid;
    s = _comac_hash_table_lookup (surface_cache, &skey.entry);
    if (s != NULL) {
	if (s->content == content &&
	    s->width   == width   &&
	    s->height  == height)
	{
	    return comac_surface_reference (s->surface);
	}

	/* The surface has been resized, allow the original entry to expire
	 * as it becomes inactive.
	 */
    }

    surface = args->target->create_similar (args->surface, content, width, height);
    s = malloc (sizeof (struct scache));
    if (s == NULL)
	return surface;

    s->entry.hash = uid;
    s->content = content;
    s->width = width;
    s->height = height;
    s->surface = surface;
    if (_comac_hash_table_insert (surface_cache, &s->entry)) {
	free (s);
    } else if (comac_surface_set_user_data
	       (surface,
		(const comac_user_data_key_t *) &surface_cache,
		s, scache_remove))
    {
	scache_remove (s);
    }

    return surface;
}

static comac_surface_t *
_source_image_create (void		*closure,
		      comac_format_t	 format,
		      int		 width,
		      int		 height,
		      long		 uid)
{
    struct trace *args = closure;

    return comac_surface_create_similar_image (args->surface,
					       format, width, height);
}

static comac_t *
_context_create (void		 *closure,
		 comac_surface_t *surface)
{
    scache_mark_active (surface);
    return comac_create (surface);
}

static int user_interrupt;

static void
interrupt (int sig)
{
    if (user_interrupt) {
	signal (sig, SIG_DFL);
	raise (sig);
    }

    user_interrupt = 1;
}

static void
describe (comac_perf_t *perf,
          void *closure)
{
    char *description = NULL;

    if (perf->has_described_backend)
	    return;
    perf->has_described_backend = TRUE;

    if (perf->target->describe)
        description = perf->target->describe (closure);

    if (description == NULL)
        return;

    if (perf->raw) {
        printf ("[ # ] %s: %s\n", perf->target->name, description);
    }

    if (perf->summary) {
        fprintf (perf->summary,
                 "[ # ] %8s: %s\n",
                 perf->target->name,
                 description);
    }

    free (description);
}

static void
usage (const char *argv0)
{
    fprintf (stderr,
"Usage: %s [-clrsv] [-i iterations] [-t tile-size] [-x exclude-file] [test-names ... | traces ...]\n"
"\n"
"Run the comac performance test suite over the given tests (all by default)\n"
"The command-line arguments are interpreted as follows:\n"
"\n"
"  -c	use surface cache; keep a cache of surfaces to be reused\n"
"  -i	iterations; specify the number of iterations per test case\n"
"  -l	list only; just list selected test case names without executing\n"
"  -r	raw; display each time measurement instead of summary statistics\n"
"  -s	sync; only sum the elapsed time of the individual operations\n"
"  -t	tile size; draw to tiled surfaces\n"
"  -v	verbose; in raw mode also show the summaries\n"
"  -x	exclude; specify a file to read a list of traces to exclude\n"
"\n"
"If test names are given they are used as sub-string matches so a command\n"
"such as \"%s firefox\" can be used to run all firefox traces.\n"
"Alternatively, you can specify a list of filenames to execute.\n",
	     argv0, argv0);
}

static comac_bool_t
read_excludes (comac_perf_t *perf,
	       const char   *filename)
{
    FILE *file;
    char *line = NULL;
    size_t line_size = 0;
    char *s, *t;

    file = fopen (filename, "r");
    if (file == NULL)
	return FALSE;

    while (getline (&line, &line_size, file) != -1) {
	/* terminate the line at a comment marker '#' */
	s = strchr (line, '#');
	if (s)
	    *s = '\0';

	/* whitespace delimits */
	s = line;
	while (*s != '\0' && isspace (*s))
	    s++;

	t = s;
	while (*t != '\0' && ! isspace (*t))
	    t++;

	if (s != t) {
	    int i = perf->num_exclude_names;
	    perf->exclude_names = xrealloc (perf->exclude_names,
					    sizeof (char *) * (i+1));
	    perf->exclude_names[i] = strndup (s, t-s);
	    perf->num_exclude_names++;
	}
    }
    free (line);

    fclose (file);

    return TRUE;
}

static void
parse_options (comac_perf_t *perf,
	       int	     argc,
	       char	    *argv[])
{
    int c;
    const char *iters;
    char *end;
    int verbose = 0;
    int use_surface_cache = 0;

    if ((iters = getenv ("COMAC_PERF_ITERATIONS")) && *iters)
	perf->iterations = strtol (iters, NULL, 0);
    else
	perf->iterations = COMAC_PERF_ITERATIONS_DEFAULT;
    perf->exact_iterations = 0;

    perf->raw = FALSE;
    perf->observe = FALSE;
    perf->list_only = FALSE;
    perf->tile_size = 0;
    perf->names = NULL;
    perf->num_names = 0;
    perf->summary = stdout;
    perf->summary_continuous = FALSE;
    perf->exclude_names = NULL;
    perf->num_exclude_names = 0;

    while (1) {
	c = _comac_getopt (argc, argv, "ci:lrst:vx:");
	if (c == -1)
	    break;

	switch (c) {
	case 'c':
	    use_surface_cache = 1;
	    break;
	case 'i':
	    perf->exact_iterations = TRUE;
	    perf->iterations = strtoul (optarg, &end, 10);
	    if (*end != '\0') {
		fprintf (stderr, "Invalid argument for -i (not an integer): %s\n",
			 optarg);
		exit (1);
	    }
	    break;
	case 'l':
	    perf->list_only = TRUE;
	    break;
	case 'r':
	    perf->raw = TRUE;
	    perf->summary = NULL;
	    break;
	case 's':
	    perf->observe = TRUE;
	    break;
	case 't':
	    perf->tile_size = strtoul (optarg, &end, 10);
	    if (*end != '\0') {
		fprintf (stderr, "Invalid argument for -t (not an integer): %s\n",
			 optarg);
		exit (1);
	    }
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'x':
	    if (! read_excludes (perf, optarg)) {
		fprintf (stderr, "Invalid argument for -x (not readable file): %s\n",
			 optarg);
		exit (1);
	    }
	    break;
	default:
	    fprintf (stderr, "Internal error: unhandled option: %c\n", c);
	    /* fall-through */
	case '?':
	    usage (argv[0]);
	    exit (1);
	}
    }

    if (perf->observe && perf->tile_size) {
	fprintf (stderr, "Can't mix observer and tiling. Sorry.\n");
	exit (1);
    }

    if (verbose && perf->summary == NULL)
	perf->summary = stderr;
#if HAVE_UNISTD_H
    if (perf->summary && isatty (fileno (perf->summary)))
	perf->summary_continuous = TRUE;
#endif

    if (optind < argc) {
	perf->names = &argv[optind];
	perf->num_names = argc - optind;
    }

    if (use_surface_cache)
	surface_cache = _comac_hash_table_create (scache_equal);
}

static void
comac_perf_fini (comac_perf_t *perf)
{
    comac_boilerplate_free_targets (perf->targets);
    comac_boilerplate_fini ();

    free (perf->times);
    comac_debug_reset_static_data ();
#if HAVE_FCFINI
    FcFini ();
#endif
}

static comac_bool_t
have_trace_filenames (comac_perf_t *perf)
{
    unsigned int i;

    if (perf->num_names == 0)
	return FALSE;

#if HAVE_UNISTD_H
    for (i = 0; i < perf->num_names; i++)
	if (access (perf->names[i], R_OK) == 0)
	    return TRUE;
#endif

    return FALSE;
}

static void
_tiling_surface_finish (comac_surface_t *observer,
			comac_surface_t *target,
			void *closure)
{
    struct trace *args = closure;
    comac_surface_t *surface;
    comac_content_t content;
    comac_rectangle_t r;
    int width, height;
    int x, y, w, h;

    comac_recording_surface_get_extents (target, &r);
    w = r.width;
    h = r.height;

    content = comac_surface_get_content (target);

    for (y = 0; y < h; y += args->tile_size) {
	height = args->tile_size;
	if (y + height > h)
	    height = h - y;

	for (x = 0; x < w; x += args->tile_size) {
	    comac_t *cr;

	    width = args->tile_size;
	    if (x + width > w)
		width = w - x;

	    /* XXX to correctly observe the playback we would need
	     * to replay the target onto the observer directly.
	     */
	    surface = args->target->create_similar (args->surface,
						    content, width, height);

	    cr = comac_create (surface);
	    comac_set_operator (cr, COMAC_OPERATOR_SOURCE);
	    comac_set_source_surface (cr, target, -x, -y);
	    comac_paint (cr);
	    comac_destroy (cr);

	    comac_surface_destroy (surface);
	}
    }
}

static comac_surface_t *
_tiling_surface_create (void		 *closure,
			comac_content_t  content,
			double		  width,
			double		  height,
			long		  uid)
{
    comac_rectangle_t r;
    comac_surface_t *surface, *observer;

    r.x = r.y = 0;
    r.width = width;
    r.height = height;

    surface = comac_recording_surface_create (content, &r);
    observer = comac_surface_create_observer (surface,
					      COMAC_SURFACE_OBSERVER_NORMAL);
    comac_surface_destroy (surface);

    comac_surface_observer_add_finish_callback (observer,
						_tiling_surface_finish,
						closure);

    return observer;
}

static void
comac_perf_trace (comac_perf_t			   *perf,
		  const comac_boilerplate_target_t *target,
		  const char			   *trace)
{
    static comac_bool_t first_run = TRUE;
    unsigned int i;
    comac_time_t *times, *paint, *mask, *fill, *stroke, *glyphs;
    comac_stats_t stats = {0.0, 0.0};
    struct trace args = { target };
    int low_std_dev_count;
    char *trace_cpy, *name;
    const comac_script_interpreter_hooks_t hooks = {
	&args,
	perf->tile_size ? _tiling_surface_create : _similar_surface_create,
	NULL, /* surface_destroy */
	_context_create,
	NULL, /* context_destroy */
	NULL, /* show_page */
	NULL, /* copy_page */
	_source_image_create,
    };

    args.tile_size = perf->tile_size;
    args.observe = perf->observe;

    trace_cpy = xstrdup (trace);
    name = basename_no_ext (trace_cpy);

    if (perf->list_only) {
	printf ("%s\n", name);
	free (trace_cpy);
	return;
    }

    if (first_run) {
	if (perf->raw) {
	    printf ("[ # ] %s.%-s %s %s %s ...\n",
		    "backend", "content", "test-size", "ticks-per-ms", "time(ticks)");
	}

	if (perf->summary) {
	    if (perf->observe) {
		fprintf (perf->summary,
			 "[ # ] %8s %28s  %9s %9s %9s %9s %9s %9s %5s\n",
			 "backend", "test",
			 "total(s)", "paint(s)", "mask(s)", "fill(s)", "stroke(s)", "glyphs(s)",
			 "count");
	    } else {
		fprintf (perf->summary,
			 "[ # ] %8s %28s %8s %5s %5s %s\n",
			 "backend", "test", "min(s)", "median(s)",
			 "stddev.", "count");
	    }
	}
	first_run = FALSE;
    }

    times = perf->times;
    paint = times + perf->iterations;
    mask = paint + perf->iterations;
    stroke = mask + perf->iterations;
    fill = stroke + perf->iterations;
    glyphs = fill + perf->iterations;

    low_std_dev_count = 0;
    for (i = 0; i < perf->iterations && ! user_interrupt; i++) {
	comac_script_interpreter_t *csi;
	comac_status_t status;
	unsigned int line_no;

	args.surface = target->create_surface (NULL,
					       COMAC_CONTENT_COLOR_ALPHA,
					       1, 1,
					       1, 1,
					       COMAC_BOILERPLATE_MODE_PERF,
					       &args.closure);
	fill_surface(args.surface); /* remove any clear flags */

	if (perf->observe) {
	    comac_surface_t *obs;
	    obs = comac_surface_create_observer (args.surface,
						 COMAC_SURFACE_OBSERVER_NORMAL);
	    comac_surface_destroy (args.surface);
	    args.surface = obs;
	}
	if (comac_surface_status (args.surface)) {
	    fprintf (stderr,
		     "Error: Failed to create target surface: %s\n",
		     target->name);
	    return;
	}

	comac_perf_timer_set_synchronize (target->synchronize, args.closure);

	if (i == 0) {
	    describe (perf, args.closure);
	    if (perf->summary) {
		fprintf (perf->summary,
			 "[%3d] %8s %28s ",
			 perf->test_number,
			 perf->target->name,
			 name);
		fflush (perf->summary);
	    }
	}

	csi = comac_script_interpreter_create ();
	comac_script_interpreter_install_hooks (csi, &hooks);

	if (! perf->observe) {
	    comac_perf_yield ();
	    comac_perf_timer_start ();
	}

	comac_script_interpreter_run (csi, trace);
	line_no = comac_script_interpreter_get_line_number (csi);

	/* Finish before querying timings in case we are using an intermediate
	 * target and so need to destroy all surfaces before rendering
	 * commences.
	 */
	comac_script_interpreter_finish (csi);

	if (perf->observe) {
	    comac_device_t *observer = comac_surface_get_device (args.surface);
	    times[i] = _comac_time_from_s (1.e-9 * comac_device_observer_elapsed (observer));
	    paint[i] = _comac_time_from_s (1.e-9 * comac_device_observer_paint_elapsed (observer));
	    mask[i] = _comac_time_from_s (1.e-9 * comac_device_observer_mask_elapsed (observer));
	    stroke[i] = _comac_time_from_s (1.e-9 * comac_device_observer_stroke_elapsed (observer));
	    fill[i] = _comac_time_from_s (1.e-9 * comac_device_observer_fill_elapsed (observer));
	    glyphs[i] = _comac_time_from_s (1.e-9 * comac_device_observer_glyphs_elapsed (observer));
	} else {
	    fill_surface (args.surface); /* queue a write to the sync'ed surface */
	    comac_perf_timer_stop ();
	    times[i] = comac_perf_timer_elapsed ();
	}

	scache_clear ();

	comac_surface_destroy (args.surface);

	if (target->cleanup)
	    target->cleanup (args.closure);

	status = comac_script_interpreter_destroy (csi);
	if (status) {
	    if (perf->summary) {
		fprintf (perf->summary, "Error during replay, line %d: %s\n",
			 line_no,
			 comac_status_to_string (status));
	    }
	    goto out;
	}

	if (perf->raw) {
	    if (i == 0)
		printf ("[*] %s.%s %s.%d %g",
			perf->target->name,
			"rgba",
			name,
			0,
			_comac_time_to_double (_comac_time_from_s (1)) / 1000.);
	    printf (" %lld", (long long) times[i]);
	    fflush (stdout);
	} else if (! perf->exact_iterations) {
	    if (i > COMAC_PERF_MIN_STD_DEV_COUNT) {
		_comac_stats_compute (&stats, times, i+1);

		if (stats.std_dev <= COMAC_PERF_LOW_STD_DEV) {
		    if (++low_std_dev_count >= COMAC_PERF_STABLE_STD_DEV_COUNT)
			break;
		} else {
		    low_std_dev_count = 0;
		}
	    }
	}

	if (perf->summary && perf->summary_continuous) {
	    _comac_stats_compute (&stats, times, i+1);

	    fprintf (perf->summary,
		     "\r[%3d] %8s %28s ",
		     perf->test_number,
		     perf->target->name,
		     name);
	    if (perf->observe) {
		fprintf (perf->summary,
			 " %#9.3f", _comac_time_to_s (stats.median_ticks));

		_comac_stats_compute (&stats, paint, i+1);
		fprintf (perf->summary,
			 " %#9.3f", _comac_time_to_s (stats.median_ticks));

		_comac_stats_compute (&stats, mask, i+1);
		fprintf (perf->summary,
			 " %#9.3f", _comac_time_to_s (stats.median_ticks));

		_comac_stats_compute (&stats, fill, i+1);
		fprintf (perf->summary,
			 " %#9.3f", _comac_time_to_s (stats.median_ticks));

		_comac_stats_compute (&stats, stroke, i+1);
		fprintf (perf->summary,
			 " %#9.3f", _comac_time_to_s (stats.median_ticks));

		_comac_stats_compute (&stats, glyphs, i+1);
		fprintf (perf->summary,
			 " %#9.3f", _comac_time_to_s (stats.median_ticks));

		fprintf (perf->summary,
			 " %5d", i+1);
	    } else {
		fprintf (perf->summary,
			 "%#8.3f %#8.3f %#6.2f%% %4d/%d",
			 _comac_time_to_s (stats.min_ticks),
			 _comac_time_to_s (stats.median_ticks),
			 stats.std_dev * 100.0,
			 stats.iterations, i+1);
	    }
	    fflush (perf->summary);
	}
    }
    user_interrupt = 0;

    if (perf->summary) {
	_comac_stats_compute (&stats, times, i);
	if (perf->summary_continuous) {
	    fprintf (perf->summary,
		     "\r[%3d] %8s %28s ",
		     perf->test_number,
		     perf->target->name,
		     name);
	}
	if (perf->observe) {
	    fprintf (perf->summary,
		     " %#9.3f", _comac_time_to_s (stats.median_ticks));

	    _comac_stats_compute (&stats, paint, i);
	    fprintf (perf->summary,
		     " %#9.3f", _comac_time_to_s (stats.median_ticks));

	    _comac_stats_compute (&stats, mask, i);
	    fprintf (perf->summary,
		     " %#9.3f", _comac_time_to_s (stats.median_ticks));

	    _comac_stats_compute (&stats, fill, i);
	    fprintf (perf->summary,
		     " %#9.3f", _comac_time_to_s (stats.median_ticks));

	    _comac_stats_compute (&stats, stroke, i);
	    fprintf (perf->summary,
		     " %#9.3f", _comac_time_to_s (stats.median_ticks));

	    _comac_stats_compute (&stats, glyphs, i);
	    fprintf (perf->summary,
		     " %#9.3f", _comac_time_to_s (stats.median_ticks));

	    fprintf (perf->summary,
		     " %5d\n", i);
	} else {
	    fprintf (perf->summary,
		     "%#8.3f %#8.3f %#6.2f%% %4d/%d\n",
		     _comac_time_to_s (stats.min_ticks),
		     _comac_time_to_s (stats.median_ticks),
		     stats.std_dev * 100.0,
		     stats.iterations, i);
	}
	fflush (perf->summary);
    }

out:
    if (perf->raw) {
	printf ("\n");
	fflush (stdout);
    }

    perf->test_number++;
    free (trace_cpy);
}

static void
warn_no_traces (const char *message,
		const char *trace_dir)
{
    fprintf (stderr,
"Error: %s '%s'.\n"
"Have you cloned the comac-traces repository and uncompressed the traces?\n"
"  git clone git://anongit.freedesktop.org/comac-traces\n"
"  cd comac-traces && make\n"
"Or set the env.var COMAC_TRACE_DIR to point to your traces?\n",
	    message, trace_dir);
}

static int
comac_perf_trace_dir (comac_perf_t		       *perf,
		      const comac_boilerplate_target_t *target,
		      const char		       *dirname)
{
    DIR *dir;
    struct dirent *de;
    int num_traces = 0;
    comac_bool_t force;
    comac_bool_t is_explicit;

    dir = opendir (dirname);
    if (dir == NULL)
	return 0;

    force = FALSE;
    if (comac_perf_can_run (perf, dirname, &is_explicit))
	force = is_explicit;

    while ((de = readdir (dir)) != NULL) {
	char *trace;
	struct stat st;

	if (de->d_name[0] == '.')
	    continue;

	xasprintf (&trace, "%s/%s", dirname, de->d_name);
	if (stat (trace, &st) != 0)
	    goto next;

	if (S_ISDIR(st.st_mode)) {
	    num_traces += comac_perf_trace_dir (perf, target, trace);
	} else {
	    const char *dot;

	    dot = strrchr (de->d_name, '.');
	    if (dot == NULL)
		goto next;
	    if (strcmp (dot, ".trace"))
		goto next;

	    num_traces++;
	    if (!force && ! comac_perf_can_run (perf, de->d_name, NULL))
		goto next;

	    comac_perf_trace (perf, target, trace);
	}
next:
	free (trace);

    }
    closedir (dir);

    return num_traces;
}

int
main (int   argc,
      char *argv[])
{
    comac_perf_t perf;
    const char *trace_dir = "comac-traces:/usr/src/comac-traces:/usr/share/comac-traces";
    unsigned int n;
    int i;

    parse_options (&perf, argc, argv);

    signal (SIGINT, interrupt);

    if (getenv ("COMAC_TRACE_DIR") != NULL)
	trace_dir = getenv ("COMAC_TRACE_DIR");

    perf.targets = comac_boilerplate_get_targets (&perf.num_targets, NULL);
    perf.times = xmalloc (6 * perf.iterations * sizeof (comac_time_t));

    /* do we have a list of filenames? */
    perf.exact_names = have_trace_filenames (&perf);

    for (i = 0; i < perf.num_targets; i++) {
	const comac_boilerplate_target_t *target = perf.targets[i];

	if (! perf.list_only && ! target->is_measurable)
	    continue;

	perf.target = target;
	perf.test_number = 0;
	perf.has_described_backend = FALSE;

	if (perf.exact_names) {
	    for (n = 0; n < perf.num_names; n++) {
		struct stat st;

		if (stat (perf.names[n], &st) == 0) {
		    if (S_ISDIR (st.st_mode)) {
			comac_perf_trace_dir (&perf, target, perf.names[n]);
		    } else
			comac_perf_trace (&perf, target, perf.names[n]);
		}
	    }
	} else {
	    int num_traces = 0;
	    const char *dir;

	    dir = trace_dir;
	    do {
		char buf[1024];
		const char *end = strchr (dir, ':');
		if (end != NULL) {
		    memcpy (buf, dir, end-dir);
		    buf[end-dir] = '\0';
		    end++;

		    dir = buf;
		}

		num_traces += comac_perf_trace_dir (&perf, target, dir);
		dir = end;
	    } while (dir != NULL);

	    if (num_traces == 0) {
		warn_no_traces ("Found no traces in", trace_dir);
		return 1;
	    }
	}

	if (perf.list_only)
	    break;
    }

    comac_perf_fini (&perf);

    return 0;
}
