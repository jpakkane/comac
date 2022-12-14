/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright © 2006 Mozilla Corporation
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2009 Chris Wilson
 * Copyright © 2011 Intel Corporation
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

#include "comac-perf.h"
#include "comac-stats.h"

#include "comac-boilerplate-getopt.h"
#include <comac-script-interpreter.h>
#include "comac-missing.h"

/* rudely reuse bits of the library... */
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

    dot = strchr (name, '.');
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

struct trace {
    const comac_boilerplate_target_t *target;
    void *closure;
    comac_surface_t *surface;
};

comac_bool_t
comac_perf_can_run (comac_perf_t *perf,
		    const char *name,
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
    dot = strchr (copy, '.');
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

static comac_surface_t *
surface_create (void *closure,
		comac_content_t content,
		double width,
		double height,
		long uid)
{
    struct trace *args = closure;
    return comac_surface_create_similar (args->surface, content, width, height);
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
describe (comac_perf_t *perf, void *closure)
{
    char *description = NULL;

    if (perf->has_described_backend)
	return;
    perf->has_described_backend = TRUE;

    if (perf->target->describe)
	description = perf->target->describe (closure);

    if (description == NULL)
	return;

    free (description);
}

static void
execute (comac_perf_t *perf, struct trace *args, const char *trace)
{
    char *trace_cpy, *name;
    const comac_script_interpreter_hooks_t hooks = {
	.closure = args,
	.surface_create = surface_create,
    };

    trace_cpy = xstrdup (trace);
    name = basename_no_ext (trace_cpy);

    if (perf->list_only) {
	printf ("%s\n", name);
	free (trace_cpy);
	return;
    }

    describe (perf, args->closure);

    {
	comac_script_interpreter_t *csi;
	comac_status_t status;
	unsigned int line_no;

	csi = comac_script_interpreter_create ();
	comac_script_interpreter_install_hooks (csi, &hooks);

	comac_script_interpreter_run (csi, trace);

	comac_script_interpreter_finish (csi);

	line_no = comac_script_interpreter_get_line_number (csi);
	status = comac_script_interpreter_destroy (csi);
	if (status) {
	    /* XXXX comac_status_to_string is just wrong! */
	    fprintf (stderr,
		     "Error during replay, line %d: %s\n",
		     line_no,
		     comac_status_to_string (status));
	}
    }
    user_interrupt = 0;

    free (trace_cpy);
}

static void
usage (const char *argv0)
{
    fprintf (
	stderr,
	"Usage: %s [-l] [-i iterations] [-x exclude-file] [test-names ... | "
	"traces ...]\n"
	"\n"
	"Run the comac trace analysis suite over the given tests (all by "
	"default)\n"
	"The command-line arguments are interpreted as follows:\n"
	"\n"
	"  -i	iterations; specify the number of iterations per test case\n"
	"  -l	list only; just list selected test case names without "
	"executing\n"
	"  -x	exclude; specify a file to read a list of traces to exclude\n"
	"\n"
	"If test names are given they are used as sub-string matches so a "
	"command\n"
	"such as \"%s firefox\" can be used to run all firefox traces.\n"
	"Alternatively, you can specify a list of filenames to execute.\n",
	argv0,
	argv0);
}

static comac_bool_t
read_excludes (comac_perf_t *perf, const char *filename)
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
	    perf->exclude_names =
		xrealloc (perf->exclude_names, sizeof (char *) * (i + 1));
	    perf->exclude_names[i] = strndup (s, t - s);
	    perf->num_exclude_names++;
	}
    }
    free (line);

    fclose (file);

    return TRUE;
}

static void
parse_options (comac_perf_t *perf, int argc, char *argv[])
{
    char *end;
    int c;

    perf->list_only = FALSE;
    perf->names = NULL;
    perf->num_names = 0;
    perf->exclude_names = NULL;
    perf->num_exclude_names = 0;

    while (1) {
	c = _comac_getopt (argc, argv, "i:lx:");
	if (c == -1)
	    break;

	switch (c) {
	case 'i':
	    perf->exact_iterations = TRUE;
	    perf->iterations = strtoul (optarg, &end, 10);
	    if (*end != '\0') {
		fprintf (stderr,
			 "Invalid argument for -i (not an integer): %s\n",
			 optarg);
		exit (1);
	    }
	    break;
	case 'l':
	    perf->list_only = TRUE;
	    break;
	case 'x':
	    if (! read_excludes (perf, optarg)) {
		fprintf (stderr,
			 "Invalid argument for -x (not readable file): %s\n",
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

    if (optind < argc) {
	perf->names = &argv[optind];
	perf->num_names = argc - optind;
    }
}

static void
comac_perf_fini (comac_perf_t *perf)
{
    comac_boilerplate_free_targets (perf->targets);
    comac_boilerplate_fini ();

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

static comac_status_t
print (void *closure, const unsigned char *data, unsigned int length)
{
    fwrite (data, length, 1, closure);
    return COMAC_STATUS_SUCCESS;
}

static void
comac_perf_trace (comac_perf_t *perf,
		  const comac_boilerplate_target_t *target,
		  const char *trace)
{
    struct trace args;
    comac_surface_t *real;

    args.target = target;
    real = target->create_surface (NULL,
				   COMAC_CONTENT_COLOR_ALPHA,
				   1,
				   1,
				   1,
				   1,
				   COMAC_BOILERPLATE_MODE_PERF,
				   &args.closure);
    args.surface = comac_surface_create_observer (
	real,
	COMAC_SURFACE_OBSERVER_RECORD_OPERATIONS);
    comac_surface_destroy (real);
    if (comac_surface_status (args.surface)) {
	fprintf (stderr,
		 "Error: Failed to create target surface: %s\n",
		 target->name);
	return;
    }

    printf ("Observing '%s'...", trace);
    fflush (stdout);

    execute (perf, &args, trace);

    printf ("\n");
    comac_device_observer_print (comac_surface_get_device (args.surface),
				 print,
				 stdout);
    fflush (stdout);

    comac_surface_destroy (args.surface);

    if (target->cleanup)
	target->cleanup (args.closure);
}

static void
warn_no_traces (const char *message, const char *trace_dir)
{
    fprintf (stderr,
	     "Error: %s '%s'.\n"
	     "Have you cloned the comac-traces repository and uncompressed the "
	     "traces?\n"
	     "  git clone git://anongit.freedesktop.org/comac-traces\n"
	     "  cd comac-traces && make\n"
	     "Or set the env.var COMAC_TRACE_DIR to point to your traces?\n",
	     message,
	     trace_dir);
}

static int
comac_perf_trace_dir (comac_perf_t *perf,
		      const comac_boilerplate_target_t *target,
		      const char *dirname)
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

	if (S_ISDIR (st.st_mode)) {
	    num_traces += comac_perf_trace_dir (perf, target, trace);
	} else {
	    const char *dot;

	    dot = strrchr (de->d_name, '.');
	    if (dot == NULL)
		goto next;
	    if (strcmp (dot, ".trace"))
		goto next;

	    num_traces++;
	    if (! force && ! comac_perf_can_run (perf, de->d_name, NULL))
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
main (int argc, char *argv[])
{
    comac_perf_t perf;
    const char *trace_dir =
	"comac-traces:/usr/src/comac-traces:/usr/share/comac-traces";
    unsigned int n;
    int i;

    parse_options (&perf, argc, argv);

    signal (SIGINT, interrupt);

    if (getenv ("COMAC_TRACE_DIR") != NULL)
	trace_dir = getenv ("COMAC_TRACE_DIR");

    perf.targets = comac_boilerplate_get_targets (&perf.num_targets, NULL);

    /* do we have a list of filenames? */
    perf.exact_names = have_trace_filenames (&perf);

    for (i = 0; i < perf.num_targets; i++) {
	const comac_boilerplate_target_t *target = perf.targets[i];

	if (! perf.list_only && ! target->is_measurable)
	    continue;

	perf.target = target;
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
		    memcpy (buf, dir, end - dir);
		    buf[end - dir] = '\0';
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
