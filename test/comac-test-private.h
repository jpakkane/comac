/*
 * Copyright © 2004 Red Hat, Inc.
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#ifndef _COMAC_TEST_PRIVATE_H_
#define _COMAC_TEST_PRIVATE_H_

#include "comac-test.h"

/* For communication between the core components of comac-test and not
 * for the tests themselves.
 */

COMAC_BEGIN_DECLS

typedef enum { DIRECT, SIMILAR } comac_test_similar_t;

comac_test_similar_t
comac_test_target_has_similar (const comac_test_context_t *ctx,
			       const comac_boilerplate_target_t *target);

comac_test_status_t
_comac_test_context_run_for_target (comac_test_context_t *ctx,
				    const comac_boilerplate_target_t *target,
				    comac_bool_t similar,
				    int dev_offset,
				    int dev_scale);

void
_comac_test_context_init_for_test (comac_test_context_t *ctx,
				   const comac_test_context_t *parent,
				   const comac_test_t *test);

void
comac_test_init (comac_test_context_t *ctx,
		 const char *test_name,
		 const char *output);

void
comac_test_fini (comac_test_context_t *ctx);

void
_comac_test_runner_register_tests (void);

COMAC_END_DECLS

#endif /* _COMAC_TEST_PRIVATE_H_ */
