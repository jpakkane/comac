/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright 2011 Andrea Canciani
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Andrea Canciani <ranma42@gmail.com>
 */

#include "comac-test.h"

#define CHECK_STATUS(status)                                                   \
    do {                                                                       \
	if (comac_status (cr) != (status)) {                                   \
	    comac_test_log (ctx,                                               \
			    "Expected status: %s\n",                           \
			    comac_status_to_string (status));                  \
	    comac_test_log (ctx,                                               \
			    "Actual status: %s\n",                             \
			    comac_status_to_string (comac_status (cr)));       \
	    result = COMAC_TEST_FAILURE;                                       \
	}                                                                      \
    } while (0)

static void
reinit_comac (comac_t **cr)
{
    if (*cr)
	comac_destroy (*cr);

    *cr = comac_create (comac_image_surface_create (COMAC_FORMAT_ARGB32, 1, 1));
    comac_surface_destroy (comac_get_target (*cr));
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_t *cr;
    comac_test_status_t result = COMAC_TEST_SUCCESS;

    cr = NULL;

    reinit_comac (&cr);

    /* comac_restore() must fail with COMAC_STATUS_INVALID_RESTORE if
     * no matching comac_save() call has been performed. */
    comac_test_log (ctx, "Checking save(); push(); restore();\n");
    comac_save (cr);
    CHECK_STATUS (COMAC_STATUS_SUCCESS);
    comac_push_group (cr);
    CHECK_STATUS (COMAC_STATUS_SUCCESS);
    comac_restore (cr);
    CHECK_STATUS (COMAC_STATUS_INVALID_RESTORE);

    reinit_comac (&cr);

    /* comac_restore() must fail with COMAC_STATUS_INVALID_RESTORE if
     * no matching comac_save() call has been performed. */
    comac_test_log (ctx, "Checking push(); save(); pop();\n");
    comac_push_group (cr);
    CHECK_STATUS (COMAC_STATUS_SUCCESS);
    comac_save (cr);
    CHECK_STATUS (COMAC_STATUS_SUCCESS);
    comac_pop_group_to_source (cr);
    CHECK_STATUS (COMAC_STATUS_INVALID_POP_GROUP);

    comac_destroy (cr);

    return result;
}

COMAC_TEST (group_state,
	    "Tests the interaction between state (comac_save, comac_restore) "
	    "and group (comac_push_group/comac_pop_group) API",
	    "api", /* keywords */
	    NULL,  /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
