/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc
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
 *      Graydon Hoare <graydon@redhat.com>
 *	Owen Taylor <otaylor@redhat.com>
 */

#ifndef COMAC_FT_H
#define COMAC_FT_H

#include "comac.h"

#if COMAC_HAS_FT_FONT

/* Fontconfig/Freetype platform-specific font interface */

#include <ft2build.h>
#include FT_FREETYPE_H

#if COMAC_HAS_FC_FONT
#include <fontconfig/fontconfig.h>
#endif

COMAC_BEGIN_DECLS

comac_public comac_font_face_t *
comac_ft_font_face_create_for_ft_face (FT_Face face, int load_flags);

/**
 * comac_ft_synthesize_t:
 * @COMAC_FT_SYNTHESIZE_BOLD: Embolden the glyphs (redraw with a pixel offset)
 * @COMAC_FT_SYNTHESIZE_OBLIQUE: Slant the glyph outline by 12 degrees to the
 * right.
 *
 * A set of synthesis options to control how FreeType renders the glyphs
 * for a particular font face.
 *
 * Individual synthesis features of a #comac_ft_font_face_t can be set
 * using comac_ft_font_face_set_synthesize(), or disabled using
 * comac_ft_font_face_unset_synthesize(). The currently enabled set of
 * synthesis options can be queried with comac_ft_font_face_get_synthesize().
 *
 * Note: that when synthesizing glyphs, the font metrics returned will only
 * be estimates.
 *
 * Since: 1.12
 **/
typedef enum {
    COMAC_FT_SYNTHESIZE_BOLD = 1 << 0,
    COMAC_FT_SYNTHESIZE_OBLIQUE = 1 << 1
} comac_ft_synthesize_t;

comac_public void
comac_ft_font_face_set_synthesize (comac_font_face_t *font_face,
				   unsigned int synth_flags);

comac_public void
comac_ft_font_face_unset_synthesize (comac_font_face_t *font_face,
				     unsigned int synth_flags);

comac_public unsigned int
comac_ft_font_face_get_synthesize (comac_font_face_t *font_face);

comac_public FT_Face
comac_ft_scaled_font_lock_face (comac_scaled_font_t *scaled_font);

comac_public void
comac_ft_scaled_font_unlock_face (comac_scaled_font_t *scaled_font);

#if COMAC_HAS_FC_FONT

comac_public comac_font_face_t *
comac_ft_font_face_create_for_pattern (FcPattern *pattern);

comac_public void
comac_ft_font_options_substitute (const comac_font_options_t *options,
				  FcPattern *pattern);

#endif

COMAC_END_DECLS

#else /* COMAC_HAS_FT_FONT */
#error Comac was not compiled with support for the freetype font backend
#endif /* COMAC_HAS_FT_FONT */

#endif /* COMAC_FT_H */
