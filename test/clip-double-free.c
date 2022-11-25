/*
 * Copyright © 2011 Uli Schlachter
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
 * Author: Uli Schlachter <psychon@znc.in>
 */

/*
 * This test wants to hit the following double free:
 *
 * ==10517== Invalid free() / delete / delete[]
 * ==10517==    at 0x4C268FE: free (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
 * ==10517==    by 0x87FBE80: _comac_clip_destroy (comac-clip.c:136)
 * ==10517==    by 0x87FE520: _comac_clip_intersect_boxes.part.1 (comac-clip-private.h:92)
 * ==10517==    by 0x87FE79F: _comac_clip_intersect_rectilinear_path (comac-clip-boxes.c:266)
 * ==10517==    by 0x87FC29B: _comac_clip_intersect_path.part.3 (comac-clip.c:242)
 * ==10517==    by 0x8809C3A: _comac_gstate_clip (comac-gstate.c:1518)
 * ==10517==    by 0x8802E40: _comac_default_context_clip (comac-default-context.c:1048)
 * ==10517==    by 0x87FA2C6: comac_clip (comac.c:2380)
 * ==10517==  Address 0x18d44cb0 is 0 bytes inside a block of size 32 free'd
 * ==10517==    at 0x4C268FE: free (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
 * ==10517==    by 0x87FE506: _comac_clip_intersect_boxes.part.1 (comac-clip-boxes.c:295)
 * ==10517==    by 0x87FE79F: _comac_clip_intersect_rectilinear_path (comac-clip-boxes.c:266)
 * ==10517==    by 0x87FC29B: _comac_clip_intersect_path.part.3 (comac-clip.c:242)
 * ==10517==    by 0x8809C3A: _comac_gstate_clip (comac-gstate.c:1518)
 * ==10517==    by 0x8802E40: _comac_default_context_clip (comac-default-context.c:1048)
 * ==10517==    by 0x87FA2C6: comac_clip (comac.c:2380)
 *
 * _comac_clip_intersect_boxes() is called with clip->num_boxes != 0. It then
 * calls _comac_boxes_init_for_array (&clip_boxes, clip->boxes, clip->num_boxes)
 * and free (clip->boxes), stealing the clip's boxes, but leaving a dangling
 * pointer behind.
 * Because this code already intersected the existing boxes and the new ones, we
 * now have num_boxes == 0. This means that _comac_clip_set_all_clipped() gets
 * called and tries to free the clip's boxes again.
 */

#include "comac-test.h"

static comac_test_status_t
draw (comac_t *cr, int width, int height)
{
    comac_set_fill_rule (cr, COMAC_FILL_RULE_EVEN_ODD);

    /* To hit this bug, we first need a clip with
     * clip->boxes != clip->embedded_boxes.
     */
    comac_rectangle (cr, 0, 0, 2, 2);
    comac_rectangle (cr, 0, 0, 1, 1);
    comac_clip (cr);

    /* Then we have to intersect this with a rectilinear path which results in
     * all clipped. This path must consist of at least two boxes or we will hit
     * a different code path.
     */
    comac_rectangle (cr, 10, 10, 2, 2);
    comac_rectangle (cr, 10, 10, 1, 1);
    comac_clip (cr);

    return COMAC_TEST_SUCCESS;
}

COMAC_TEST (clip_double_free,
	    "Test a double free bug in the clipping code",
	    "clip", /* keywords */
	    NULL, /* requirements */
	    0, 0,
	    NULL, draw)
