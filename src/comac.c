/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "comacint.h"
#include "comac-private.h"

#include "comac-backend-private.h"
#include "comac-error-private.h"
#include "comac-path-private.h"
#include "comac-pattern-private.h"
#include "comac-surface-private.h"
#include "comac-surface-backend-private.h"

#include <assert.h>

/**
 * SECTION:comac
 * @Title: comac_t
 * @Short_Description: The comac drawing context
 * @See_Also: #comac_surface_t
 *
 * #comac_t is the main object used when drawing with comac. To
 * draw with comac, you create a #comac_t, set the target surface,
 * and drawing options for the #comac_t, create shapes with
 * functions like comac_move_to() and comac_line_to(), and then
 * draw shapes with comac_stroke() or comac_fill().
 *
 * #comac_t<!-- -->'s can be pushed to a stack via comac_save().
 * They may then safely be changed, without losing the current state.
 * Use comac_restore() to restore to the saved state.
 **/

/**
 * SECTION:comac-text
 * @Title: text
 * @Short_Description: Rendering text and glyphs
 * @See_Also: #comac_font_face_t, #comac_scaled_font_t, comac_text_path(),
 *            comac_glyph_path()
 *
 * The functions with <emphasis>text</emphasis> in their name form comac's
 * <firstterm>toy</firstterm> text API.  The toy API takes UTF-8 encoded
 * text and is limited in its functionality to rendering simple
 * left-to-right text with no advanced features.  That means for example
 * that most complex scripts like Hebrew, Arabic, and Indic scripts are
 * out of question.  No kerning or correct positioning of diacritical marks
 * either.  The font selection is pretty limited too and doesn't handle the
 * case that the selected font does not cover the characters in the text.
 * This set of functions are really that, a toy text API, for testing and
 * demonstration purposes.  Any serious application should avoid them.
 *
 * The functions with <emphasis>glyphs</emphasis> in their name form comac's
 * <firstterm>low-level</firstterm> text API.  The low-level API relies on
 * the user to convert text to a set of glyph indexes and positions.  This
 * is a very hard problem and is best handled by external libraries, like
 * the pangocomac that is part of the Pango text layout and rendering library.
 * Pango is available from <ulink
 * url="http://www.pango.org/">http://www.pango.org/</ulink>.
 **/

/**
 * SECTION:comac-transforms
 * @Title: Transformations
 * @Short_Description: Manipulating the current transformation matrix
 * @See_Also: #comac_matrix_t
 *
 * The current transformation matrix, <firstterm>ctm</firstterm>, is a
 * two-dimensional affine transformation that maps all coordinates and other
 * drawing instruments from the <firstterm>user space</firstterm> into the
 * surface's canonical coordinate system, also known as the <firstterm>device
 * space</firstterm>.
 **/

/**
 * SECTION:comac-tag
 * @Title: Tags and Links
 * @Short_Description: Hyperlinks and document structure
 * @See_Also: #comac_pdf_surface_t
 *
 * The tag functions provide the ability to specify hyperlinks and
 * document logical structure on supported backends. The following tags are supported:
 * * [Link][link] - Create a hyperlink
 * * [Destinations][dest] - Create a hyperlink destination
 * * [Document Structure Tags][doc-struct] - Create PDF Document Structure
 *
 * # Link Tags # {#link}
 * A hyperlink is specified by enclosing the hyperlink text with the %COMAC_TAG_LINK tag.
 *
 * For example:
 * <informalexample><programlisting>
 * comac_tag_begin (cr, COMAC_TAG_LINK, "uri='https://cairographics.org'");
 * comac_move_to (cr, 50, 50);
 * comac_show_text (cr, "This is a link to the comac website.");
 * comac_tag_end (cr, COMAC_TAG_LINK);
 * </programlisting></informalexample>
 *
 * The PDF backend uses one or more rectangles to define the clickable
 * area of the link.  By default comac will use the extents of the
 * drawing operations enclosed by the begin/end link tags to define the
 * clickable area. In some cases, such as a link split across two
 * lines, the default rectangle is undesirable.
 *
 * @rect: [optional] The "rect" attribute allows the application to
 * specify one or more rectangles that form the clickable region.  The
 * value of this attribute is an array of floats. Each rectangle is
 * specified by four elements in the array: x, y, width, height. The
 * array size must be a multiple of four.
 *
 * An example of creating a link with user specified clickable region:
 * <informalexample><programlisting>
 * struct text {
 *     const char *s;
 *     double x, y;
 * };
 * const struct text text1 = { "This link is split", 450, 70 };
 * const struct text text2 = { "across two lines", 50, 70 };
 * comac_text_extents_t text1_extents;
 * comac_text_extents_t text2_extents;
 * char attribs[100];
 *
 * comac_text_extents (cr, text1.s, &text1_extents);
 * comac_text_extents (cr, text2.s, &text2_extents);
 * sprintf (attribs,
 *          "rect=[%f %f %f %f %f %f %f %f] uri='https://cairographics.org'",
 *          text1_extents.x_bearing + text1.x,
 *          text1_extents.y_bearing + text1.y,
 *          text1_extents.width,
 *          text1_extents.height,
 *          text2_extents.x_bearing + text2.x,
 *          text2_extents.y_bearing + text2.y,
 *          text2_extents.width,
 *          text2_extents.height);
 *
 * comac_tag_begin (cr, COMAC_TAG_LINK, attribs);
 * comac_move_to (cr, text1.x, text1.y);
 * comac_show_text (cr, text1.s);
 * comac_move_to (cr, text2.x, text2.y);
 * comac_show_text (cr, text2.s);
 * comac_tag_end (cr, COMAC_TAG_LINK);
 * </programlisting></informalexample>
 *
 * There are three types of links. Each type has its own attributes as detailed below.
 * * [Internal Links][internal-link] - A link to a location in the same document
 * * [URI Links][uri-link] - A link to a Uniform resource identifier
 * * [File Links][file-link] - A link to a location in another document
 *
 * ## Internal Links ## {#internal-link}
 * An internal link is a link to a location in the same document. The destination
 * is specified with either:
 *
 * @dest: a UTF-8 string specifying the destination in the PDF file to link
 * to. Destinations are created with the %COMAC_TAG_DEST tag.
 *
 * or the two attributes:
 *
 * @page: An integer specifying the page number in the PDF file to link to.
 *
 * @pos: [optional] An array of two floats specifying the x,y position
 * on the page.
 *
 * An example of the link attributes to link to a page and x,y position:
 * <programlisting>
 * "page=3 pos=[3.1 6.2]"
 * </programlisting>
 *
 * ## URI Links ## {#uri-link}
 * A URI link is a link to a Uniform Resource Identifier ([RFC 2396](http://tools.ietf.org/html/rfc2396)).
 *
 * A URI is specified with the following attribute:
 *
 * @uri: An ASCII string specifying the URI.
 *
 * An example of the link attributes to the comac website:
 * <programlisting>
 * "uri='https://cairographics.org'"
 * </programlisting>
 *
 * ## File Links ## {#file-link}
 * A file link is a link a location in another PDF file.
 *
 * The file attribute (required) specifies the name of the PDF file:
 *
 * @file: File name of PDF file to link to.
 *
 * The position is specified by either:
 *
 *  @dest: a UTF-8 string specifying the named destination in the PDF file.
 *
 * or
 *
 *  @page: An integer specifying the page number in the PDF file.
 *
 *  @pos: [optional] An array of two floats specifying the x,y
 *  position on the page. Position coordinates in external files are in PDF
 *  coordinates (0,0 at bottom left).
 *
 * An example of the link attributes to PDF file:
 * <programlisting>
 * "file='document.pdf' page=16 pos=[25 40]"
 * </programlisting>
 *
 * # Destination Tags # {#dest}
 *
 * A destination is specified by enclosing the destination drawing
 * operations with the %COMAC_TAG_DEST tag.
 *
 * @name: [required] A UTF-8 string specifying the name of this destination.
 *
 * @x: [optional] A float specifying the x coordinate of destination
 *                 position on this page. If not specified the default
 *                 x coordinate is the left side of the extents of the
 *                 operations enclosed by the %COMAC_TAG_DEST begin/end tags. If
 *                 no operations are enclosed, the x coordidate is 0.
 *
 * @y: [optional] A float specifying the y coordinate of destination
 *                 position on this page. If not specified the default
 *                 y coordinate is the top of the extents of the
 *                 operations enclosed by the %COMAC_TAG_DEST begin/end tags. If
 *                 no operations are enclosed, the y coordidate is 0.
 *
 * @internal: A boolean that if true, the destination name may be
 *            omitted from PDF where possible. In this case, links
 *            refer directly to the page and position instead of via
 *            the named destination table. Note that if this
 *            destination is referenced by another PDF (see [File Links][file-link]),
 *            this attribute must be false. Default is false.
 *
 * <informalexample><programlisting>
 * /&ast; Create a hyperlink &ast;/
 * comac_tag_begin (cr, COMAC_TAG_LINK, "dest='mydest' internal");
 * comac_move_to (cr, 50, 50);
 * comac_show_text (cr, "This is a hyperlink.");
 * comac_tag_end (cr, COMAC_TAG_LINK);
 *
 * /&ast; Create a destination &ast;/
 * comac_tag_begin (cr, COMAC_TAG_DEST, "name='mydest'");
 * comac_move_to (cr, 50, 250);
 * comac_show_text (cr, "This paragraph is the destination of the above link.");
 * comac_tag_end (cr, COMAC_TAG_DEST);
 * </programlisting></informalexample>
 *
 * # Document Structure (PDF) # {#doc-struct}
 *
 * The document structure tags provide a means of specifying structural information
 * such as headers, paragraphs, tables, and figures. The inclusion of structural information facilitates:
 * * Extraction of text and graphics for copy and paste
 * * Reflow of text and graphics in the viewer
 * * Processing text eg searching and indexing
 * * Conversion to other formats
 * * Accessability support
 *
 * The list of structure types is specified in section 14.8.4 of the
 * [PDF Reference](http://www.adobe.com/content/dam/Adobe/en/devnet/acrobat/pdfs/PDF32000_2008.pdf).
 *
 * Note the PDF "Link" structure tag is the same as the comac %COMAC_TAG_LINK tag.
 *
 * The following example creates a document structure for a document containing two section, each with
 * a header and a paragraph.
 *
 * <informalexample><programlisting>
 * comac_tag_begin (cr, "Document", NULL);
 *
 * comac_tag_begin (cr, "Sect", NULL);
 * comac_tag_begin (cr, "H1", NULL);
 * comac_show_text (cr, "Heading 1");
 * comac_tag_end (cr, "H1");
 *
 * comac_tag_begin (cr, "P", NULL);
 * comac_show_text (cr, "Paragraph 1");
 * comac_tag_end (cr, "P");
 * comac_tag_end (cr, "Sect");
 *
 * comac_tag_begin (cr, "Sect", NULL);
 * comac_tag_begin (cr, "H1", NULL);
 * comac_show_text (cr, "Heading 2");
 * comac_tag_end (cr, "H1");
 *
 * comac_tag_begin (cr, "P", NULL);
 * comac_show_text (cr, "Paragraph 2");
 * comac_tag_end (cr, "P");
 * comac_tag_end (cr, "Sect");
 *
 * comac_tag_end (cr, "Document");
 * </programlisting></informalexample>
 *
 **/

#define DEFINE_NIL_CONTEXT(status)                                             \
    {                                                                          \
	COMAC_REFERENCE_COUNT_INVALID, /* ref_count */                         \
	    status,		       /* status */                            \
	    {0, 0, 0, NULL},	       /* user_data */                         \
	    NULL                                                               \
    }

static const comac_t _comac_nil[] = {
    DEFINE_NIL_CONTEXT (COMAC_STATUS_NO_MEMORY),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_RESTORE),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_POP_GROUP),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_NO_CURRENT_POINT),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_MATRIX),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_STATUS),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_NULL_POINTER),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_STRING),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_PATH_DATA),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_READ_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_WRITE_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_SURFACE_FINISHED),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_SURFACE_TYPE_MISMATCH),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_PATTERN_TYPE_MISMATCH),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_CONTENT),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_FORMAT),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_VISUAL),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_FILE_NOT_FOUND),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_DASH),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_DSC_COMMENT),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_INDEX),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_CLIP_NOT_REPRESENTABLE),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_TEMP_FILE_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_STRIDE),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_FONT_TYPE_MISMATCH),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_USER_FONT_IMMUTABLE),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_USER_FONT_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_NEGATIVE_COUNT),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_CLUSTERS),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_SLANT),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_WEIGHT),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_SIZE),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_USER_FONT_NOT_IMPLEMENTED),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_DEVICE_TYPE_MISMATCH),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_DEVICE_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_INVALID_MESH_CONSTRUCTION),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_DEVICE_FINISHED),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_JBIG2_GLOBAL_MISSING),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_PNG_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_FREETYPE_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_WIN32_GDI_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_TAG_ERROR),
    DEFINE_NIL_CONTEXT (COMAC_STATUS_DWRITE_ERROR)};
COMPILE_TIME_ASSERT (ARRAY_LENGTH (_comac_nil) == COMAC_STATUS_LAST_STATUS - 1);

/**
 * _comac_set_error:
 * @cr: a comac context
 * @status: a status value indicating an error
 *
 * Atomically sets cr->status to @status and calls _comac_error;
 * Does nothing if status is %COMAC_STATUS_SUCCESS.
 *
 * All assignments of an error status to cr->status should happen
 * through _comac_set_error(). Note that due to the nature of the atomic
 * operation, it is not safe to call this function on the nil objects.
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _comac_error() to generate a stack trace for when the
 * user causes comac to detect an error.
 **/
static void
_comac_set_error (comac_t *cr, comac_status_t status)
{
    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. */
    _comac_status_set_error (&cr->status, _comac_error (status));
}

comac_t *
_comac_create_in_error (comac_status_t status)
{
    comac_t *cr;

    assert (status != COMAC_STATUS_SUCCESS);

    cr = (comac_t *) &_comac_nil[status - COMAC_STATUS_NO_MEMORY];
    assert (status == cr->status);

    return cr;
}

/**
 * comac_create:
 * @target: target surface for the context
 *
 * Creates a new #comac_t with all graphics state parameters set to
 * default values and with @target as a target surface. The target
 * surface should be constructed with a backend-specific function such
 * as comac_image_surface_create() (or any other
 * <function>comac_<emphasis>backend</emphasis>_surface_create(<!-- -->)</function>
 * variant).
 *
 * This function references @target, so you can immediately
 * call comac_surface_destroy() on it if you don't need to
 * maintain a separate reference to it.
 *
 * Return value: a newly allocated #comac_t with a reference
 *  count of 1. The initial reference count should be released
 *  with comac_destroy() when you are done using the #comac_t.
 *  This function never returns %NULL. If memory cannot be
 *  allocated, a special #comac_t object will be returned on
 *  which comac_status() returns %COMAC_STATUS_NO_MEMORY. If
 *  you attempt to target a surface which does not support
 *  writing (such as #comac_mime_surface_t) then a
 *  %COMAC_STATUS_WRITE_ERROR will be raised.  You can use this
 *  object normally, but no drawing will be done.
 *
 * Since: 1.0
 **/
comac_t *
comac_create (comac_surface_t *target)
{
    if (unlikely (target == NULL))
	return _comac_create_in_error (
	    _comac_error (COMAC_STATUS_NULL_POINTER));
    if (unlikely (target->status))
	return _comac_create_in_error (target->status);
    if (unlikely (target->finished))
	return _comac_create_in_error (
	    _comac_error (COMAC_STATUS_SURFACE_FINISHED));

    if (target->backend->create_context == NULL)
	return _comac_create_in_error (_comac_error (COMAC_STATUS_WRITE_ERROR));

    return target->backend->create_context (target);
}

void
_comac_init (comac_t *cr, const comac_backend_t *backend)
{
    COMAC_REFERENCE_COUNT_INIT (&cr->ref_count, 1);
    cr->status = COMAC_STATUS_SUCCESS;
    _comac_user_data_array_init (&cr->user_data);

    cr->backend = backend;
}

/**
 * comac_reference:
 * @cr: a #comac_t
 *
 * Increases the reference count on @cr by one. This prevents
 * @cr from being destroyed until a matching call to comac_destroy()
 * is made.
 *
 * Use comac_get_reference_count() to get the number of references to
 * a #comac_t.
 *
 * Return value: the referenced #comac_t.
 *
 * Since: 1.0
 **/
comac_t *
comac_reference (comac_t *cr)
{
    if (cr == NULL || COMAC_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return cr;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&cr->ref_count));

    _comac_reference_count_inc (&cr->ref_count);

    return cr;
}

void
_comac_fini (comac_t *cr)
{
    _comac_user_data_array_fini (&cr->user_data);
}

/**
 * comac_destroy:
 * @cr: a #comac_t
 *
 * Decreases the reference count on @cr by one. If the result
 * is zero, then @cr and all associated resources are freed.
 * See comac_reference().
 *
 * Since: 1.0
 **/
void
comac_destroy (comac_t *cr)
{
    if (cr == NULL || COMAC_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return;

    assert (COMAC_REFERENCE_COUNT_HAS_REFERENCE (&cr->ref_count));

    if (! _comac_reference_count_dec_and_test (&cr->ref_count))
	return;

    cr->backend->destroy (cr);
}

/**
 * comac_get_user_data:
 * @cr: a #comac_t
 * @key: the address of the #comac_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @cr using the specified
 * key.  If no user data has been attached with the given key this
 * function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 *
 * Since: 1.4
 **/
void *
comac_get_user_data (comac_t *cr, const comac_user_data_key_t *key)
{
    return _comac_user_data_array_get_data (&cr->user_data, key);
}

/**
 * comac_set_user_data:
 * @cr: a #comac_t
 * @key: the address of a #comac_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the #comac_t
 * @destroy: a #comac_destroy_func_t which will be called when the
 * #comac_t is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @cr.  To remove user data from a surface,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %COMAC_STATUS_SUCCESS or %COMAC_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 *
 * Since: 1.4
 **/
comac_status_t
comac_set_user_data (comac_t *cr,
		     const comac_user_data_key_t *key,
		     void *user_data,
		     comac_destroy_func_t destroy)
{
    if (COMAC_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return cr->status;

    return _comac_user_data_array_set_data (&cr->user_data,
					    key,
					    user_data,
					    destroy);
}

/**
 * comac_get_reference_count:
 * @cr: a #comac_t
 *
 * Returns the current reference count of @cr.
 *
 * Return value: the current reference count of @cr.  If the
 * object is a nil object, 0 will be returned.
 *
 * Since: 1.4
 **/
unsigned int
comac_get_reference_count (comac_t *cr)
{
    if (cr == NULL || COMAC_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
	return 0;

    return COMAC_REFERENCE_COUNT_GET_VALUE (&cr->ref_count);
}

/**
 * comac_save:
 * @cr: a #comac_t
 *
 * Makes a copy of the current state of @cr and saves it
 * on an internal stack of saved states for @cr. When
 * comac_restore() is called, @cr will be restored to
 * the saved state. Multiple calls to comac_save() and
 * comac_restore() can be nested; each call to comac_restore()
 * restores the state from the matching paired comac_save().
 *
 * It isn't necessary to clear all saved states before
 * a #comac_t is freed. If the reference count of a #comac_t
 * drops to zero in response to a call to comac_destroy(),
 * any saved states will be freed along with the #comac_t.
 *
 * Since: 1.0
 **/
void
comac_save (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->save (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_restore:
 * @cr: a #comac_t
 *
 * Restores @cr to the state saved by a preceding call to
 * comac_save() and removes that state from the stack of
 * saved states.
 *
 * Since: 1.0
 **/
void
comac_restore (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->restore (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_push_group:
 * @cr: a comac context
 *
 * Temporarily redirects drawing to an intermediate surface known as a
 * group. The redirection lasts until the group is completed by a call
 * to comac_pop_group() or comac_pop_group_to_source(). These calls
 * provide the result of any drawing to the group as a pattern,
 * (either as an explicit object, or set as the source pattern).
 *
 * This group functionality can be convenient for performing
 * intermediate compositing. One common use of a group is to render
 * objects as opaque within the group, (so that they occlude each
 * other), and then blend the result with translucence onto the
 * destination.
 *
 * Groups can be nested arbitrarily deep by making balanced calls to
 * comac_push_group()/comac_pop_group(). Each call pushes/pops the new
 * target group onto/from a stack.
 *
 * The comac_push_group() function calls comac_save() so that any
 * changes to the graphics state will not be visible outside the
 * group, (the pop_group functions call comac_restore()).
 *
 * By default the intermediate group will have a content type of
 * %COMAC_CONTENT_COLOR_ALPHA. Other content types can be chosen for
 * the group by using comac_push_group_with_content() instead.
 *
 * As an example, here is how one might fill and stroke a path with
 * translucence, but without any portion of the fill being visible
 * under the stroke:
 *
 * <informalexample><programlisting>
 * comac_push_group (cr);
 * comac_set_source (cr, fill_pattern);
 * comac_fill_preserve (cr);
 * comac_set_source (cr, stroke_pattern);
 * comac_stroke (cr);
 * comac_pop_group_to_source (cr);
 * comac_paint_with_alpha (cr, alpha);
 * </programlisting></informalexample>
 *
 * Since: 1.2
 **/
void
comac_push_group (comac_t *cr)
{
    comac_push_group_with_content (cr, COMAC_CONTENT_COLOR_ALPHA);
}

/**
 * comac_push_group_with_content:
 * @cr: a comac context
 * @content: a #comac_content_t indicating the type of group that
 *           will be created
 *
 * Temporarily redirects drawing to an intermediate surface known as a
 * group. The redirection lasts until the group is completed by a call
 * to comac_pop_group() or comac_pop_group_to_source(). These calls
 * provide the result of any drawing to the group as a pattern,
 * (either as an explicit object, or set as the source pattern).
 *
 * The group will have a content type of @content. The ability to
 * control this content type is the only distinction between this
 * function and comac_push_group() which you should see for a more
 * detailed description of group rendering.
 *
 * Since: 1.2
 **/
void
comac_push_group_with_content (comac_t *cr, comac_content_t content)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->push_group (cr, content);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_pop_group:
 * @cr: a comac context
 *
 * Terminates the redirection begun by a call to comac_push_group() or
 * comac_push_group_with_content() and returns a new pattern
 * containing the results of all drawing operations performed to the
 * group.
 *
 * The comac_pop_group() function calls comac_restore(), (balancing a
 * call to comac_save() by the push_group function), so that any
 * changes to the graphics state will not be visible outside the
 * group.
 *
 * Return value: a newly created (surface) pattern containing the
 * results of all drawing operations performed to the group. The
 * caller owns the returned object and should call
 * comac_pattern_destroy() when finished with it.
 *
 * Since: 1.2
 **/
comac_pattern_t *
comac_pop_group (comac_t *cr)
{
    comac_pattern_t *group_pattern;

    if (unlikely (cr->status))
	return _comac_pattern_create_in_error (cr->status);

    group_pattern = cr->backend->pop_group (cr);
    if (unlikely (group_pattern->status))
	_comac_set_error (cr, group_pattern->status);

    return group_pattern;
}

/**
 * comac_pop_group_to_source:
 * @cr: a comac context
 *
 * Terminates the redirection begun by a call to comac_push_group() or
 * comac_push_group_with_content() and installs the resulting pattern
 * as the source pattern in the given comac context.
 *
 * The behavior of this function is equivalent to the sequence of
 * operations:
 *
 * <informalexample><programlisting>
 * comac_pattern_t *group = comac_pop_group (cr);
 * comac_set_source (cr, group);
 * comac_pattern_destroy (group);
 * </programlisting></informalexample>
 *
 * but is more convenient as their is no need for a variable to store
 * the short-lived pointer to the pattern.
 *
 * The comac_pop_group() function calls comac_restore(), (balancing a
 * call to comac_save() by the push_group function), so that any
 * changes to the graphics state will not be visible outside the
 * group.
 *
 * Since: 1.2
 **/
void
comac_pop_group_to_source (comac_t *cr)
{
    comac_pattern_t *group_pattern;

    group_pattern = comac_pop_group (cr);
    comac_set_source (cr, group_pattern);
    comac_pattern_destroy (group_pattern);
}

/**
 * comac_set_operator:
 * @cr: a #comac_t
 * @op: a compositing operator, specified as a #comac_operator_t
 *
 * Sets the compositing operator to be used for all drawing
 * operations. See #comac_operator_t for details on the semantics of
 * each available compositing operator.
 *
 * The default operator is %COMAC_OPERATOR_OVER.
 *
 * Since: 1.0
 **/
void
comac_set_operator (comac_t *cr, comac_operator_t op)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_operator (cr, op);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

#if 0
/*
 * comac_set_opacity:
 * @cr: a #comac_t
 * @opacity: the level of opacity to use when compositing
 *
 * Sets the compositing opacity to be used for all drawing
 * operations. The effect is to fade out the operations
 * using the alpha value.
 *
 * The default opacity is 1.
 */
void
comac_set_opacity (comac_t *cr, double opacity)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_opacity (cr, opacity);
    if (unlikely (status))
	_comac_set_error (cr, status);
}
#endif

/**
 * comac_set_source_rgb:
 * @cr: a comac context
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 *
 * Sets the source pattern within @cr to an opaque color. This opaque
 * color will then be used for any subsequent drawing operation until
 * a new source pattern is set.
 *
 * The color components are floating point numbers in the range 0 to
 * 1. If the values passed in are outside that range, they will be
 * clamped.
 *
 * The default source pattern is opaque black, (that is, it is
 * equivalent to comac_set_source_rgb(cr, 0.0, 0.0, 0.0)).
 *
 * Since: 1.0
 **/
void
comac_set_source_rgb (comac_t *cr, double red, double green, double blue)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_source_rgba (cr, red, green, blue, 1.);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_source_rgba:
 * @cr: a comac context
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 * @alpha: alpha component of color
 *
 * Sets the source pattern within @cr to a translucent color. This
 * color will then be used for any subsequent drawing operation until
 * a new source pattern is set.
 *
 * The color and alpha components are floating point numbers in the
 * range 0 to 1. If the values passed in are outside that range, they
 * will be clamped.
 *
 * The default source pattern is opaque black, (that is, it is
 * equivalent to comac_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0)).
 *
 * Since: 1.0
 **/
void
comac_set_source_rgba (
    comac_t *cr, double red, double green, double blue, double alpha)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_source_rgba (cr, red, green, blue, alpha);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

comac_public void
comac_set_source_gray (comac_t *cr, double graylevel)
{
    comac_set_source_rgb (cr, graylevel, graylevel, graylevel);
}

/**
 * comac_set_source_surface:
 * @cr: a comac context
 * @surface: a surface to be used to set the source pattern
 * @x: User-space X coordinate for surface origin
 * @y: User-space Y coordinate for surface origin
 *
 * This is a convenience function for creating a pattern from @surface
 * and setting it as the source in @cr with comac_set_source().
 *
 * The @x and @y parameters give the user-space coordinate at which
 * the surface origin should appear. (The surface origin is its
 * upper-left corner before any transformation has been applied.) The
 * @x and @y parameters are negated and then set as translation values
 * in the pattern matrix.
 *
 * Other than the initial translation pattern matrix, as described
 * above, all other pattern attributes, (such as its extend mode), are
 * set to the default values as in comac_pattern_create_for_surface().
 * The resulting pattern can be queried with comac_get_source() so
 * that these attributes can be modified if desired, (eg. to create a
 * repeating pattern with comac_pattern_set_extend()).
 *
 * Since: 1.0
 **/
void
comac_set_source_surface (comac_t *cr,
			  comac_surface_t *surface,
			  double x,
			  double y)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (unlikely (surface == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    status = cr->backend->set_source_surface (cr, surface, x, y);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_source:
 * @cr: a comac context
 * @source: a #comac_pattern_t to be used as the source for
 * subsequent drawing operations.
 *
 * Sets the source pattern within @cr to @source. This pattern
 * will then be used for any subsequent drawing operation until a new
 * source pattern is set.
 *
 * Note: The pattern's transformation matrix will be locked to the
 * user space in effect at the time of comac_set_source(). This means
 * that further modifications of the current transformation matrix
 * will not affect the source pattern. See comac_pattern_set_matrix().
 *
 * The default source pattern is a solid pattern that is opaque black,
 * (that is, it is equivalent to comac_set_source_rgb(cr, 0.0, 0.0,
 * 0.0)).
 *
 * Since: 1.0
 **/
void
comac_set_source (comac_t *cr, comac_pattern_t *source)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (unlikely (source == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    if (unlikely (source->status)) {
	_comac_set_error (cr, source->status);
	return;
    }

    status = cr->backend->set_source (cr, source);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_get_source:
 * @cr: a comac context
 *
 * Gets the current source pattern for @cr.
 *
 * Return value: the current source pattern. This object is owned by
 * comac. To keep a reference to it, you must call
 * comac_pattern_reference().
 *
 * Since: 1.0
 **/
comac_pattern_t *
comac_get_source (comac_t *cr)
{
    if (unlikely (cr->status))
	return _comac_pattern_create_in_error (cr->status);

    return cr->backend->get_source (cr);
}

/**
 * comac_set_tolerance:
 * @cr: a #comac_t
 * @tolerance: the tolerance, in device units (typically pixels)
 *
 * Sets the tolerance used when converting paths into trapezoids.
 * Curved segments of the path will be subdivided until the maximum
 * deviation between the original path and the polygonal approximation
 * is less than @tolerance. The default value is 0.1. A larger
 * value will give better performance, a smaller value, better
 * appearance. (Reducing the value from the default value of 0.1
 * is unlikely to improve appearance significantly.)  The accuracy of paths
 * within Comac is limited by the precision of its internal arithmetic, and
 * the prescribed @tolerance is restricted to the smallest
 * representable internal value.
 *
 * Since: 1.0
 **/
void
comac_set_tolerance (comac_t *cr, double tolerance)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_tolerance (cr, tolerance);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_antialias:
 * @cr: a #comac_t
 * @antialias: the new antialiasing mode
 *
 * Set the antialiasing mode of the rasterizer used for drawing shapes.
 * This value is a hint, and a particular backend may or may not support
 * a particular value.  At the current time, no backend supports
 * %COMAC_ANTIALIAS_SUBPIXEL when drawing shapes.
 *
 * Note that this option does not affect text rendering, instead see
 * comac_font_options_set_antialias().
 *
 * Since: 1.0
 **/
void
comac_set_antialias (comac_t *cr, comac_antialias_t antialias)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_antialias (cr, antialias);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_fill_rule:
 * @cr: a #comac_t
 * @fill_rule: a fill rule, specified as a #comac_fill_rule_t
 *
 * Set the current fill rule within the comac context. The fill rule
 * is used to determine which regions are inside or outside a complex
 * (potentially self-intersecting) path. The current fill rule affects
 * both comac_fill() and comac_clip(). See #comac_fill_rule_t for details
 * on the semantics of each available fill rule.
 *
 * The default fill rule is %COMAC_FILL_RULE_WINDING.
 *
 * Since: 1.0
 **/
void
comac_set_fill_rule (comac_t *cr, comac_fill_rule_t fill_rule)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_fill_rule (cr, fill_rule);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_line_width:
 * @cr: a #comac_t
 * @width: a line width
 *
 * Sets the current line width within the comac context. The line
 * width value specifies the diameter of a pen that is circular in
 * user space, (though device-space pen may be an ellipse in general
 * due to scaling/shear/rotation of the CTM).
 *
 * Note: When the description above refers to user space and CTM it
 * refers to the user space and CTM in effect at the time of the
 * stroking operation, not the user space and CTM in effect at the
 * time of the call to comac_set_line_width(). The simplest usage
 * makes both of these spaces identical. That is, if there is no
 * change to the CTM between a call to comac_set_line_width() and the
 * stroking operation, then one can just pass user-space values to
 * comac_set_line_width() and ignore this note.
 *
 * As with the other stroke parameters, the current line width is
 * examined by comac_stroke(), comac_stroke_extents(), and
 * comac_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default line width value is 2.0.
 *
 * Since: 1.0
 **/
void
comac_set_line_width (comac_t *cr, double width)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (width < 0.)
	width = 0.;

    status = cr->backend->set_line_width (cr, width);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_hairline:
 * @cr: a #comac_t
 * @set_hairline: whether or not to set hairline mode
 *
 * Sets lines within the comac context to be hairlines.
 * Hairlines are logically zero-width lines that are drawn at the
 * thinnest renderable width possible in the current context.
 *
 * On surfaces with native hairline support, the native hairline
 * functionality will be used. Surfaces that support hairlines include:
 * - pdf/ps: Encoded as 0-width line.
 * - win32_printing: Rendered with PS_COSMETIC pen.
 * - svg: Encoded as 1px non-scaling-stroke.
 * - script: Encoded with set-hairline function.
 *
 * Comac will always render hairlines at 1 device unit wide, even if
 * an anisotropic scaling was applied to the stroke width. In the wild,
 * handling of this situation is not well-defined. Some PDF, PS, and SVG
 * renderers match Comac's output, but some very popular implementations
 * (Acrobat, Chrome, rsvg) will scale the hairline unevenly.
 * As such, best practice is to reset any anisotropic scaling before calling
 * comac_stroke(). See https://cairographics.org/cookbook/ellipses/
 * for an example.
 *
 * Since: 1.18
 **/
void
comac_set_hairline (comac_t *cr, comac_bool_t set_hairline)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_hairline (cr, set_hairline);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_line_cap:
 * @cr: a comac context
 * @line_cap: a line cap style
 *
 * Sets the current line cap style within the comac context. See
 * #comac_line_cap_t for details about how the available line cap
 * styles are drawn.
 *
 * As with the other stroke parameters, the current line cap style is
 * examined by comac_stroke(), comac_stroke_extents(), and
 * comac_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default line cap style is %COMAC_LINE_CAP_BUTT.
 *
 * Since: 1.0
 **/
void
comac_set_line_cap (comac_t *cr, comac_line_cap_t line_cap)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_line_cap (cr, line_cap);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_line_join:
 * @cr: a comac context
 * @line_join: a line join style
 *
 * Sets the current line join style within the comac context. See
 * #comac_line_join_t for details about how the available line join
 * styles are drawn.
 *
 * As with the other stroke parameters, the current line join style is
 * examined by comac_stroke(), comac_stroke_extents(), and
 * comac_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default line join style is %COMAC_LINE_JOIN_MITER.
 *
 * Since: 1.0
 **/
void
comac_set_line_join (comac_t *cr, comac_line_join_t line_join)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_line_join (cr, line_join);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_dash:
 * @cr: a comac context
 * @dashes: an array specifying alternate lengths of on and off stroke portions
 * @num_dashes: the length of the dashes array
 * @offset: an offset into the dash pattern at which the stroke should start
 *
 * Sets the dash pattern to be used by comac_stroke(). A dash pattern
 * is specified by @dashes, an array of positive values. Each value
 * provides the length of alternate "on" and "off" portions of the
 * stroke. The @offset specifies an offset into the pattern at which
 * the stroke begins.
 *
 * Each "on" segment will have caps applied as if the segment were a
 * separate sub-path. In particular, it is valid to use an "on" length
 * of 0.0 with %COMAC_LINE_CAP_ROUND or %COMAC_LINE_CAP_SQUARE in order
 * to distributed dots or squares along a path.
 *
 * Note: The length values are in user-space units as evaluated at the
 * time of stroking. This is not necessarily the same as the user
 * space at the time of comac_set_dash().
 *
 * If @num_dashes is 0 dashing is disabled.
 *
 * If @num_dashes is 1 a symmetric pattern is assumed with alternating
 * on and off portions of the size specified by the single value in
 * @dashes.
 *
 * If any value in @dashes is negative, or if all values are 0, then
 * @cr will be put into an error state with a status of
 * %COMAC_STATUS_INVALID_DASH.
 *
 * Since: 1.0
 **/
void
comac_set_dash (comac_t *cr,
		const double *dashes,
		int num_dashes,
		double offset)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_dash (cr, dashes, num_dashes, offset);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_get_dash_count:
 * @cr: a #comac_t
 *
 * This function returns the length of the dash array in @cr (0 if dashing
 * is not currently in effect).
 *
 * See also comac_set_dash() and comac_get_dash().
 *
 * Return value: the length of the dash array, or 0 if no dash array set.
 *
 * Since: 1.4
 **/
int
comac_get_dash_count (comac_t *cr)
{
    int num_dashes;

    if (unlikely (cr->status))
	return 0;

    cr->backend->get_dash (cr, NULL, &num_dashes, NULL);

    return num_dashes;
}

/**
 * comac_get_dash:
 * @cr: a #comac_t
 * @dashes: return value for the dash array, or %NULL
 * @offset: return value for the current dash offset, or %NULL
 *
 * Gets the current dash array.  If not %NULL, @dashes should be big
 * enough to hold at least the number of values returned by
 * comac_get_dash_count().
 *
 * Since: 1.4
 **/
void
comac_get_dash (comac_t *cr, double *dashes, double *offset)
{
    if (unlikely (cr->status))
	return;

    cr->backend->get_dash (cr, dashes, NULL, offset);
}

/**
 * comac_set_miter_limit:
 * @cr: a comac context
 * @limit: miter limit to set
 *
 * Sets the current miter limit within the comac context.
 *
 * If the current line join style is set to %COMAC_LINE_JOIN_MITER
 * (see comac_set_line_join()), the miter limit is used to determine
 * whether the lines should be joined with a bevel instead of a miter.
 * Comac divides the length of the miter by the line width.
 * If the result is greater than the miter limit, the style is
 * converted to a bevel.
 *
 * As with the other stroke parameters, the current line miter limit is
 * examined by comac_stroke(), comac_stroke_extents(), and
 * comac_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default miter limit value is 10.0, which will convert joins
 * with interior angles less than 11 degrees to bevels instead of
 * miters. For reference, a miter limit of 2.0 makes the miter cutoff
 * at 60 degrees, and a miter limit of 1.414 makes the cutoff at 90
 * degrees.
 *
 * A miter limit for a desired angle can be computed as: miter limit =
 * 1/sin(angle/2)
 *
 * Since: 1.0
 **/
void
comac_set_miter_limit (comac_t *cr, double limit)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_miter_limit (cr, limit);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_translate:
 * @cr: a comac context
 * @tx: amount to translate in the X direction
 * @ty: amount to translate in the Y direction
 *
 * Modifies the current transformation matrix (CTM) by translating the
 * user-space origin by (@tx, @ty). This offset is interpreted as a
 * user-space coordinate according to the CTM in place before the new
 * call to comac_translate(). In other words, the translation of the
 * user-space origin takes place after any existing transformation.
 *
 * Since: 1.0
 **/
void
comac_translate (comac_t *cr, double tx, double ty)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->translate (cr, tx, ty);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_scale:
 * @cr: a comac context
 * @sx: scale factor for the X dimension
 * @sy: scale factor for the Y dimension
 *
 * Modifies the current transformation matrix (CTM) by scaling the X
 * and Y user-space axes by @sx and @sy respectively. The scaling of
 * the axes takes place after any existing transformation of user
 * space.
 *
 * Since: 1.0
 **/
void
comac_scale (comac_t *cr, double sx, double sy)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->scale (cr, sx, sy);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_rotate:
 * @cr: a comac context
 * @angle: angle (in radians) by which the user-space axes will be
 * rotated
 *
 * Modifies the current transformation matrix (CTM) by rotating the
 * user-space axes by @angle radians. The rotation of the axes takes
 * places after any existing transformation of user space. The
 * rotation direction for positive angles is from the positive X axis
 * toward the positive Y axis.
 *
 * Since: 1.0
 **/
void
comac_rotate (comac_t *cr, double angle)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->rotate (cr, angle);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_transform:
 * @cr: a comac context
 * @matrix: a transformation to be applied to the user-space axes
 *
 * Modifies the current transformation matrix (CTM) by applying
 * @matrix as an additional transformation. The new transformation of
 * user space takes place after any existing transformation.
 *
 * Since: 1.0
 **/
void
comac_transform (comac_t *cr, const comac_matrix_t *matrix)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->transform (cr, matrix);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_matrix:
 * @cr: a comac context
 * @matrix: a transformation matrix from user space to device space
 *
 * Modifies the current transformation matrix (CTM) by setting it
 * equal to @matrix.
 *
 * Since: 1.0
 **/
void
comac_set_matrix (comac_t *cr, const comac_matrix_t *matrix)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_matrix (cr, matrix);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_identity_matrix:
 * @cr: a comac context
 *
 * Resets the current transformation matrix (CTM) by setting it equal
 * to the identity matrix. That is, the user-space and device-space
 * axes will be aligned and one user-space unit will transform to one
 * device-space unit.
 *
 * Since: 1.0
 **/
void
comac_identity_matrix (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_identity_matrix (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_user_to_device:
 * @cr: a comac context
 * @x: X value of coordinate (in/out parameter)
 * @y: Y value of coordinate (in/out parameter)
 *
 * Transform a coordinate from user space to device space by
 * multiplying the given point by the current transformation matrix
 * (CTM).
 *
 * Since: 1.0
 **/
void
comac_user_to_device (comac_t *cr, double *x, double *y)
{
    if (unlikely (cr->status))
	return;

    cr->backend->user_to_device (cr, x, y);
}

/**
 * comac_user_to_device_distance:
 * @cr: a comac context
 * @dx: X component of a distance vector (in/out parameter)
 * @dy: Y component of a distance vector (in/out parameter)
 *
 * Transform a distance vector from user space to device space. This
 * function is similar to comac_user_to_device() except that the
 * translation components of the CTM will be ignored when transforming
 * (@dx,@dy).
 *
 * Since: 1.0
 **/
void
comac_user_to_device_distance (comac_t *cr, double *dx, double *dy)
{
    if (unlikely (cr->status))
	return;

    cr->backend->user_to_device_distance (cr, dx, dy);
}

/**
 * comac_device_to_user:
 * @cr: a comac
 * @x: X value of coordinate (in/out parameter)
 * @y: Y value of coordinate (in/out parameter)
 *
 * Transform a coordinate from device space to user space by
 * multiplying the given point by the inverse of the current
 * transformation matrix (CTM).
 *
 * Since: 1.0
 **/
void
comac_device_to_user (comac_t *cr, double *x, double *y)
{
    if (unlikely (cr->status))
	return;

    cr->backend->device_to_user (cr, x, y);
}

/**
 * comac_device_to_user_distance:
 * @cr: a comac context
 * @dx: X component of a distance vector (in/out parameter)
 * @dy: Y component of a distance vector (in/out parameter)
 *
 * Transform a distance vector from device space to user space. This
 * function is similar to comac_device_to_user() except that the
 * translation components of the inverse CTM will be ignored when
 * transforming (@dx,@dy).
 *
 * Since: 1.0
 **/
void
comac_device_to_user_distance (comac_t *cr, double *dx, double *dy)
{
    if (unlikely (cr->status))
	return;

    cr->backend->device_to_user_distance (cr, dx, dy);
}

/**
 * comac_new_path:
 * @cr: a comac context
 *
 * Clears the current path. After this call there will be no path and
 * no current point.
 *
 * Since: 1.0
 **/
void
comac_new_path (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->new_path (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_new_sub_path:
 * @cr: a comac context
 *
 * Begin a new sub-path. Note that the existing path is not
 * affected. After this call there will be no current point.
 *
 * In many cases, this call is not needed since new sub-paths are
 * frequently started with comac_move_to().
 *
 * A call to comac_new_sub_path() is particularly useful when
 * beginning a new sub-path with one of the comac_arc() calls. This
 * makes things easier as it is no longer necessary to manually
 * compute the arc's initial coordinates for a call to
 * comac_move_to().
 *
 * Since: 1.2
 **/
void
comac_new_sub_path (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->new_sub_path (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_move_to:
 * @cr: a comac context
 * @x: the X coordinate of the new position
 * @y: the Y coordinate of the new position
 *
 * Begin a new sub-path. After this call the current point will be (@x,
 * @y).
 *
 * Since: 1.0
 **/
void
comac_move_to (comac_t *cr, double x, double y)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->move_to (cr, x, y);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_line_to:
 * @cr: a comac context
 * @x: the X coordinate of the end of the new line
 * @y: the Y coordinate of the end of the new line
 *
 * Adds a line to the path from the current point to position (@x, @y)
 * in user-space coordinates. After this call the current point
 * will be (@x, @y).
 *
 * If there is no current point before the call to comac_line_to()
 * this function will behave as comac_move_to(@cr, @x, @y).
 *
 * Since: 1.0
 **/
void
comac_line_to (comac_t *cr, double x, double y)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->line_to (cr, x, y);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_curve_to:
 * @cr: a comac context
 * @x1: the X coordinate of the first control point
 * @y1: the Y coordinate of the first control point
 * @x2: the X coordinate of the second control point
 * @y2: the Y coordinate of the second control point
 * @x3: the X coordinate of the end of the curve
 * @y3: the Y coordinate of the end of the curve
 *
 * Adds a cubic Bézier spline to the path from the current point to
 * position (@x3, @y3) in user-space coordinates, using (@x1, @y1) and
 * (@x2, @y2) as the control points. After this call the current point
 * will be (@x3, @y3).
 *
 * If there is no current point before the call to comac_curve_to()
 * this function will behave as if preceded by a call to
 * comac_move_to(@cr, @x1, @y1).
 *
 * Since: 1.0
 **/
void
comac_curve_to (comac_t *cr,
		double x1,
		double y1,
		double x2,
		double y2,
		double x3,
		double y3)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->curve_to (cr, x1, y1, x2, y2, x3, y3);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_arc:
 * @cr: a comac context
 * @xc: X position of the center of the arc
 * @yc: Y position of the center of the arc
 * @radius: the radius of the arc
 * @angle1: the start angle, in radians
 * @angle2: the end angle, in radians
 *
 * Adds a circular arc of the given @radius to the current path.  The
 * arc is centered at (@xc, @yc), begins at @angle1 and proceeds in
 * the direction of increasing angles to end at @angle2. If @angle2 is
 * less than @angle1 it will be progressively increased by
 * <literal>2*M_PI</literal> until it is greater than @angle1.
 *
 * If there is a current point, an initial line segment will be added
 * to the path to connect the current point to the beginning of the
 * arc. If this initial line is undesired, it can be avoided by
 * calling comac_new_sub_path() before calling comac_arc().
 *
 * Angles are measured in radians. An angle of 0.0 is in the direction
 * of the positive X axis (in user space). An angle of
 * <literal>M_PI/2.0</literal> radians (90 degrees) is in the
 * direction of the positive Y axis (in user space). Angles increase
 * in the direction from the positive X axis toward the positive Y
 * axis. So with the default transformation matrix, angles increase in
 * a clockwise direction.
 *
 * (To convert from degrees to radians, use <literal>degrees * (M_PI /
 * 180.)</literal>.)
 *
 * This function gives the arc in the direction of increasing angles;
 * see comac_arc_negative() to get the arc in the direction of
 * decreasing angles.
 *
 * The arc is circular in user space. To achieve an elliptical arc,
 * you can scale the current transformation matrix by different
 * amounts in the X and Y directions. For example, to draw an ellipse
 * in the box given by @x, @y, @width, @height:
 *
 * <informalexample><programlisting>
 * comac_save (cr);
 * comac_translate (cr, x + width / 2., y + height / 2.);
 * comac_scale (cr, width / 2., height / 2.);
 * comac_arc (cr, 0., 0., 1., 0., 2 * M_PI);
 * comac_restore (cr);
 * </programlisting></informalexample>
 *
 * Since: 1.0
 **/
void
comac_arc (comac_t *cr,
	   double xc,
	   double yc,
	   double radius,
	   double angle1,
	   double angle2)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (angle2 < angle1) {
	/* increase angle2 by multiples of full circle until it
	 * satisfies angle2 >= angle1 */
	angle2 = fmod (angle2 - angle1, 2 * M_PI);
	if (angle2 < 0)
	    angle2 += 2 * M_PI;
	angle2 += angle1;
    }

    status = cr->backend->arc (cr, xc, yc, radius, angle1, angle2, TRUE);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_arc_negative:
 * @cr: a comac context
 * @xc: X position of the center of the arc
 * @yc: Y position of the center of the arc
 * @radius: the radius of the arc
 * @angle1: the start angle, in radians
 * @angle2: the end angle, in radians
 *
 * Adds a circular arc of the given @radius to the current path.  The
 * arc is centered at (@xc, @yc), begins at @angle1 and proceeds in
 * the direction of decreasing angles to end at @angle2. If @angle2 is
 * greater than @angle1 it will be progressively decreased by
 * <literal>2*M_PI</literal> until it is less than @angle1.
 *
 * See comac_arc() for more details. This function differs only in the
 * direction of the arc between the two angles.
 *
 * Since: 1.0
 **/
void
comac_arc_negative (comac_t *cr,
		    double xc,
		    double yc,
		    double radius,
		    double angle1,
		    double angle2)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (angle2 > angle1) {
	/* decrease angle2 by multiples of full circle until it
	 * satisfies angle2 <= angle1 */
	angle2 = fmod (angle2 - angle1, 2 * M_PI);
	if (angle2 > 0)
	    angle2 -= 2 * M_PI;
	angle2 += angle1;
    }

    status = cr->backend->arc (cr, xc, yc, radius, angle1, angle2, FALSE);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/* XXX: NYI
void
comac_arc_to (comac_t *cr,
	      double x1, double y1,
	      double x2, double y2,
	      double radius)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->arc_to (cr, x1, y1, x2, y2, radius);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

void
comac_rel_arc_to (comac_t *cr,
	      double dx1, double dy1,
	      double dx2, double dy2,
	      double radius)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->rel_arc_to (cr, dx1, dy1, dx2, dy2, radius);
    if (unlikely (status))
	_comac_set_error (cr, status);
}
*/

/**
 * comac_rel_move_to:
 * @cr: a comac context
 * @dx: the X offset
 * @dy: the Y offset
 *
 * Begin a new sub-path. After this call the current point will offset
 * by (@x, @y).
 *
 * Given a current point of (x, y), comac_rel_move_to(@cr, @dx, @dy)
 * is logically equivalent to comac_move_to(@cr, x + @dx, y + @dy).
 *
 * It is an error to call this function with no current point. Doing
 * so will cause @cr to shutdown with a status of
 * %COMAC_STATUS_NO_CURRENT_POINT.
 *
 * Since: 1.0
 **/
void
comac_rel_move_to (comac_t *cr, double dx, double dy)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->rel_move_to (cr, dx, dy);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_rel_line_to:
 * @cr: a comac context
 * @dx: the X offset to the end of the new line
 * @dy: the Y offset to the end of the new line
 *
 * Relative-coordinate version of comac_line_to(). Adds a line to the
 * path from the current point to a point that is offset from the
 * current point by (@dx, @dy) in user space. After this call the
 * current point will be offset by (@dx, @dy).
 *
 * Given a current point of (x, y), comac_rel_line_to(@cr, @dx, @dy)
 * is logically equivalent to comac_line_to(@cr, x + @dx, y + @dy).
 *
 * It is an error to call this function with no current point. Doing
 * so will cause @cr to shutdown with a status of
 * %COMAC_STATUS_NO_CURRENT_POINT.
 *
 * Since: 1.0
 **/
void
comac_rel_line_to (comac_t *cr, double dx, double dy)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->rel_line_to (cr, dx, dy);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_rel_curve_to:
 * @cr: a comac context
 * @dx1: the X offset to the first control point
 * @dy1: the Y offset to the first control point
 * @dx2: the X offset to the second control point
 * @dy2: the Y offset to the second control point
 * @dx3: the X offset to the end of the curve
 * @dy3: the Y offset to the end of the curve
 *
 * Relative-coordinate version of comac_curve_to(). All offsets are
 * relative to the current point. Adds a cubic Bézier spline to the
 * path from the current point to a point offset from the current
 * point by (@dx3, @dy3), using points offset by (@dx1, @dy1) and
 * (@dx2, @dy2) as the control points. After this call the current
 * point will be offset by (@dx3, @dy3).
 *
 * Given a current point of (x, y), comac_rel_curve_to(@cr, @dx1,
 * @dy1, @dx2, @dy2, @dx3, @dy3) is logically equivalent to
 * comac_curve_to(@cr, x+@dx1, y+@dy1, x+@dx2, y+@dy2, x+@dx3, y+@dy3).
 *
 * It is an error to call this function with no current point. Doing
 * so will cause @cr to shutdown with a status of
 * %COMAC_STATUS_NO_CURRENT_POINT.
 *
 * Since: 1.0
 **/
void
comac_rel_curve_to (comac_t *cr,
		    double dx1,
		    double dy1,
		    double dx2,
		    double dy2,
		    double dx3,
		    double dy3)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->rel_curve_to (cr, dx1, dy1, dx2, dy2, dx3, dy3);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_rectangle:
 * @cr: a comac context
 * @x: the X coordinate of the top left corner of the rectangle
 * @y: the Y coordinate to the top left corner of the rectangle
 * @width: the width of the rectangle
 * @height: the height of the rectangle
 *
 * Adds a closed sub-path rectangle of the given size to the current
 * path at position (@x, @y) in user-space coordinates.
 *
 * This function is logically equivalent to:
 * <informalexample><programlisting>
 * comac_move_to (cr, x, y);
 * comac_rel_line_to (cr, width, 0);
 * comac_rel_line_to (cr, 0, height);
 * comac_rel_line_to (cr, -width, 0);
 * comac_close_path (cr);
 * </programlisting></informalexample>
 *
 * Since: 1.0
 **/
void
comac_rectangle (comac_t *cr, double x, double y, double width, double height)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->rectangle (cr, x, y, width, height);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

#if 0
/* XXX: NYI */
void
comac_stroke_to_path (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    /* The code in _comac_recording_surface_get_path has a poorman's stroke_to_path */

    status = _comac_gstate_stroke_path (cr->gstate);
    if (unlikely (status))
	_comac_set_error (cr, status);
}
#endif

/**
 * comac_close_path:
 * @cr: a comac context
 *
 * Adds a line segment to the path from the current point to the
 * beginning of the current sub-path, (the most recent point passed to
 * comac_move_to()), and closes this sub-path. After this call the
 * current point will be at the joined endpoint of the sub-path.
 *
 * The behavior of comac_close_path() is distinct from simply calling
 * comac_line_to() with the equivalent coordinate in the case of
 * stroking. When a closed sub-path is stroked, there are no caps on
 * the ends of the sub-path. Instead, there is a line join connecting
 * the final and initial segments of the sub-path.
 *
 * If there is no current point before the call to comac_close_path(),
 * this function will have no effect.
 *
 * Note: As of comac version 1.2.4 any call to comac_close_path() will
 * place an explicit MOVE_TO element into the path immediately after
 * the CLOSE_PATH element, (which can be seen in comac_copy_path() for
 * example). This can simplify path processing in some cases as it may
 * not be necessary to save the "last move_to point" during processing
 * as the MOVE_TO immediately after the CLOSE_PATH will provide that
 * point.
 *
 * Since: 1.0
 **/
void
comac_close_path (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->close_path (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_path_extents:
 * @cr: a comac context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user-space coordinates covering the
 * points on the current path. If the current path is empty, returns
 * an empty rectangle ((0,0), (0,0)). Stroke parameters, fill rule,
 * surface dimensions and clipping are not taken into account.
 *
 * Contrast with comac_fill_extents() and comac_stroke_extents() which
 * return the extents of only the area that would be "inked" by
 * the corresponding drawing operations.
 *
 * The result of comac_path_extents() is defined as equivalent to the
 * limit of comac_stroke_extents() with %COMAC_LINE_CAP_ROUND as the
 * line width approaches 0.0, (but never reaching the empty-rectangle
 * returned by comac_stroke_extents() for a line width of 0.0).
 *
 * Specifically, this means that zero-area sub-paths such as
 * comac_move_to();comac_line_to() segments, (even degenerate cases
 * where the coordinates to both calls are identical), will be
 * considered as contributing to the extents. However, a lone
 * comac_move_to() will not contribute to the results of
 * comac_path_extents().
 *
 * Since: 1.6
 **/
void
comac_path_extents (comac_t *cr, double *x1, double *y1, double *x2, double *y2)
{
    if (unlikely (cr->status)) {
	if (x1)
	    *x1 = 0.0;
	if (y1)
	    *y1 = 0.0;
	if (x2)
	    *x2 = 0.0;
	if (y2)
	    *y2 = 0.0;

	return;
    }

    cr->backend->path_extents (cr, x1, y1, x2, y2);
}

/**
 * comac_paint:
 * @cr: a comac context
 *
 * A drawing operator that paints the current source everywhere within
 * the current clip region.
 *
 * Since: 1.0
 **/
void
comac_paint (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->paint (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_paint_with_alpha:
 * @cr: a comac context
 * @alpha: alpha value, between 0 (transparent) and 1 (opaque)
 *
 * A drawing operator that paints the current source everywhere within
 * the current clip region using a mask of constant alpha value
 * @alpha. The effect is similar to comac_paint(), but the drawing
 * is faded out using the alpha value.
 *
 * Since: 1.0
 **/
void
comac_paint_with_alpha (comac_t *cr, double alpha)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->paint_with_alpha (cr, alpha);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_mask:
 * @cr: a comac context
 * @pattern: a #comac_pattern_t
 *
 * A drawing operator that paints the current source
 * using the alpha channel of @pattern as a mask. (Opaque
 * areas of @pattern are painted with the source, transparent
 * areas are not painted.)
 *
 * Since: 1.0
 **/
void
comac_mask (comac_t *cr, comac_pattern_t *pattern)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (unlikely (pattern == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    if (unlikely (pattern->status)) {
	_comac_set_error (cr, pattern->status);
	return;
    }

    status = cr->backend->mask (cr, pattern);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_mask_surface:
 * @cr: a comac context
 * @surface: a #comac_surface_t
 * @surface_x: X coordinate at which to place the origin of @surface
 * @surface_y: Y coordinate at which to place the origin of @surface
 *
 * A drawing operator that paints the current source
 * using the alpha channel of @surface as a mask. (Opaque
 * areas of @surface are painted with the source, transparent
 * areas are not painted.)
 *
 * Since: 1.0
 **/
void
comac_mask_surface (comac_t *cr,
		    comac_surface_t *surface,
		    double surface_x,
		    double surface_y)
{
    comac_pattern_t *pattern;
    comac_matrix_t matrix;

    if (unlikely (cr->status))
	return;

    pattern = comac_pattern_create_for_surface (surface);

    comac_matrix_init_translate (&matrix, -surface_x, -surface_y);
    comac_pattern_set_matrix (pattern, &matrix);

    comac_mask (cr, pattern);

    comac_pattern_destroy (pattern);
}

/**
 * comac_stroke:
 * @cr: a comac context
 *
 * A drawing operator that strokes the current path according to the
 * current line width, line join, line cap, and dash settings. After
 * comac_stroke(), the current path will be cleared from the comac
 * context. See comac_set_line_width(), comac_set_line_join(),
 * comac_set_line_cap(), comac_set_dash(), and
 * comac_stroke_preserve().
 *
 * Note: Degenerate segments and sub-paths are treated specially and
 * provide a useful result. These can result in two different
 * situations:
 *
 * 1. Zero-length "on" segments set in comac_set_dash(). If the cap
 * style is %COMAC_LINE_CAP_ROUND or %COMAC_LINE_CAP_SQUARE then these
 * segments will be drawn as circular dots or squares respectively. In
 * the case of %COMAC_LINE_CAP_SQUARE, the orientation of the squares
 * is determined by the direction of the underlying path.
 *
 * 2. A sub-path created by comac_move_to() followed by either a
 * comac_close_path() or one or more calls to comac_line_to() to the
 * same coordinate as the comac_move_to(). If the cap style is
 * %COMAC_LINE_CAP_ROUND then these sub-paths will be drawn as circular
 * dots. Note that in the case of %COMAC_LINE_CAP_SQUARE a degenerate
 * sub-path will not be drawn at all, (since the correct orientation
 * is indeterminate).
 *
 * In no case will a cap style of %COMAC_LINE_CAP_BUTT cause anything
 * to be drawn in the case of either degenerate segments or sub-paths.
 *
 * Since: 1.0
 **/
void
comac_stroke (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->stroke (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_stroke_preserve:
 * @cr: a comac context
 *
 * A drawing operator that strokes the current path according to the
 * current line width, line join, line cap, and dash settings. Unlike
 * comac_stroke(), comac_stroke_preserve() preserves the path within the
 * comac context.
 *
 * See comac_set_line_width(), comac_set_line_join(),
 * comac_set_line_cap(), comac_set_dash(), and
 * comac_stroke_preserve().
 *
 * Since: 1.0
 **/
void
comac_stroke_preserve (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->stroke_preserve (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_fill:
 * @cr: a comac context
 *
 * A drawing operator that fills the current path according to the
 * current fill rule, (each sub-path is implicitly closed before being
 * filled). After comac_fill(), the current path will be cleared from
 * the comac context. See comac_set_fill_rule() and
 * comac_fill_preserve().
 *
 * Since: 1.0
 **/
void
comac_fill (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->fill (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_fill_preserve:
 * @cr: a comac context
 *
 * A drawing operator that fills the current path according to the
 * current fill rule, (each sub-path is implicitly closed before being
 * filled). Unlike comac_fill(), comac_fill_preserve() preserves the
 * path within the comac context.
 *
 * See comac_set_fill_rule() and comac_fill().
 *
 * Since: 1.0
 **/
void
comac_fill_preserve (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->fill_preserve (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_copy_page:
 * @cr: a comac context
 *
 * Emits the current page for backends that support multiple pages, but
 * doesn't clear it, so, the contents of the current page will be retained
 * for the next page too.  Use comac_show_page() if you want to get an
 * empty page after the emission.
 *
 * This is a convenience function that simply calls
 * comac_surface_copy_page() on @cr's target.
 *
 * Since: 1.0
 **/
void
comac_copy_page (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->copy_page (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_show_page:
 * @cr: a comac context
 *
 * Emits and clears the current page for backends that support multiple
 * pages.  Use comac_copy_page() if you don't want to clear the page.
 *
 * This is a convenience function that simply calls
 * comac_surface_show_page() on @cr's target.
 *
 * Since: 1.0
 **/
void
comac_show_page (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->show_page (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_in_stroke:
 * @cr: a comac context
 * @x: X coordinate of the point to test
 * @y: Y coordinate of the point to test
 *
 * Tests whether the given point is inside the area that would be
 * affected by a comac_stroke() operation given the current path and
 * stroking parameters. Surface dimensions and clipping are not taken
 * into account.
 *
 * See comac_stroke(), comac_set_line_width(), comac_set_line_join(),
 * comac_set_line_cap(), comac_set_dash(), and
 * comac_stroke_preserve().
 *
 * Return value: A non-zero value if the point is inside, or zero if
 * outside.
 *
 * Since: 1.0
 **/
comac_bool_t
comac_in_stroke (comac_t *cr, double x, double y)
{
    comac_status_t status;
    comac_bool_t inside = FALSE;

    if (unlikely (cr->status))
	return FALSE;

    status = cr->backend->in_stroke (cr, x, y, &inside);
    if (unlikely (status))
	_comac_set_error (cr, status);

    return inside;
}

/**
 * comac_in_fill:
 * @cr: a comac context
 * @x: X coordinate of the point to test
 * @y: Y coordinate of the point to test
 *
 * Tests whether the given point is inside the area that would be
 * affected by a comac_fill() operation given the current path and
 * filling parameters. Surface dimensions and clipping are not taken
 * into account.
 *
 * See comac_fill(), comac_set_fill_rule() and comac_fill_preserve().
 *
 * Return value: A non-zero value if the point is inside, or zero if
 * outside.
 *
 * Since: 1.0
 **/
comac_bool_t
comac_in_fill (comac_t *cr, double x, double y)
{
    comac_status_t status;
    comac_bool_t inside = FALSE;

    if (unlikely (cr->status))
	return FALSE;

    status = cr->backend->in_fill (cr, x, y, &inside);
    if (unlikely (status))
	_comac_set_error (cr, status);

    return inside;
}

/**
 * comac_stroke_extents:
 * @cr: a comac context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user coordinates covering the area that
 * would be affected, (the "inked" area), by a comac_stroke()
 * operation given the current path and stroke parameters.
 * If the current path is empty, returns an empty rectangle ((0,0), (0,0)).
 * Surface dimensions and clipping are not taken into account.
 *
 * Note that if the line width is set to exactly zero, then
 * comac_stroke_extents() will return an empty rectangle. Contrast with
 * comac_path_extents() which can be used to compute the non-empty
 * bounds as the line width approaches zero.
 *
 * Note that comac_stroke_extents() must necessarily do more work to
 * compute the precise inked areas in light of the stroke parameters,
 * so comac_path_extents() may be more desirable for sake of
 * performance if non-inked path extents are desired.
 *
 * See comac_stroke(), comac_set_line_width(), comac_set_line_join(),
 * comac_set_line_cap(), comac_set_dash(), and
 * comac_stroke_preserve().
 *
 * Since: 1.0
 **/
void
comac_stroke_extents (
    comac_t *cr, double *x1, double *y1, double *x2, double *y2)
{
    comac_status_t status;

    if (unlikely (cr->status)) {
	if (x1)
	    *x1 = 0.0;
	if (y1)
	    *y1 = 0.0;
	if (x2)
	    *x2 = 0.0;
	if (y2)
	    *y2 = 0.0;

	return;
    }

    status = cr->backend->stroke_extents (cr, x1, y1, x2, y2);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_fill_extents:
 * @cr: a comac context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user coordinates covering the area that
 * would be affected, (the "inked" area), by a comac_fill() operation
 * given the current path and fill parameters. If the current path is
 * empty, returns an empty rectangle ((0,0), (0,0)). Surface
 * dimensions and clipping are not taken into account.
 *
 * Contrast with comac_path_extents(), which is similar, but returns
 * non-zero extents for some paths with no inked area, (such as a
 * simple line segment).
 *
 * Note that comac_fill_extents() must necessarily do more work to
 * compute the precise inked areas in light of the fill rule, so
 * comac_path_extents() may be more desirable for sake of performance
 * if the non-inked path extents are desired.
 *
 * See comac_fill(), comac_set_fill_rule() and comac_fill_preserve().
 *
 * Since: 1.0
 **/
void
comac_fill_extents (comac_t *cr, double *x1, double *y1, double *x2, double *y2)
{
    comac_status_t status;

    if (unlikely (cr->status)) {
	if (x1)
	    *x1 = 0.0;
	if (y1)
	    *y1 = 0.0;
	if (x2)
	    *x2 = 0.0;
	if (y2)
	    *y2 = 0.0;

	return;
    }

    status = cr->backend->fill_extents (cr, x1, y1, x2, y2);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_clip:
 * @cr: a comac context
 *
 * Establishes a new clip region by intersecting the current clip
 * region with the current path as it would be filled by comac_fill()
 * and according to the current fill rule (see comac_set_fill_rule()).
 *
 * After comac_clip(), the current path will be cleared from the comac
 * context.
 *
 * The current clip region affects all drawing operations by
 * effectively masking out any changes to the surface that are outside
 * the current clip region.
 *
 * Calling comac_clip() can only make the clip region smaller, never
 * larger. But the current clip is part of the graphics state, so a
 * temporary restriction of the clip region can be achieved by
 * calling comac_clip() within a comac_save()/comac_restore()
 * pair. The only other means of increasing the size of the clip
 * region is comac_reset_clip().
 *
 * Since: 1.0
 **/
void
comac_clip (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->clip (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_clip_preserve:
 * @cr: a comac context
 *
 * Establishes a new clip region by intersecting the current clip
 * region with the current path as it would be filled by comac_fill()
 * and according to the current fill rule (see comac_set_fill_rule()).
 *
 * Unlike comac_clip(), comac_clip_preserve() preserves the path within
 * the comac context.
 *
 * The current clip region affects all drawing operations by
 * effectively masking out any changes to the surface that are outside
 * the current clip region.
 *
 * Calling comac_clip_preserve() can only make the clip region smaller, never
 * larger. But the current clip is part of the graphics state, so a
 * temporary restriction of the clip region can be achieved by
 * calling comac_clip_preserve() within a comac_save()/comac_restore()
 * pair. The only other means of increasing the size of the clip
 * region is comac_reset_clip().
 *
 * Since: 1.0
 **/
void
comac_clip_preserve (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->clip_preserve (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_reset_clip:
 * @cr: a comac context
 *
 * Reset the current clip region to its original, unrestricted
 * state. That is, set the clip region to an infinitely large shape
 * containing the target surface. Equivalently, if infinity is too
 * hard to grasp, one can imagine the clip region being reset to the
 * exact bounds of the target surface.
 *
 * Note that code meant to be reusable should not call
 * comac_reset_clip() as it will cause results unexpected by
 * higher-level code which calls comac_clip(). Consider using
 * comac_save() and comac_restore() around comac_clip() as a more
 * robust means of temporarily restricting the clip region.
 *
 * Since: 1.0
 **/
void
comac_reset_clip (comac_t *cr)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->reset_clip (cr);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_clip_extents:
 * @cr: a comac context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user coordinates covering the area inside the
 * current clip.
 *
 * Since: 1.4
 **/
void
comac_clip_extents (comac_t *cr, double *x1, double *y1, double *x2, double *y2)
{
    comac_status_t status;

    if (x1)
	*x1 = 0.0;
    if (y1)
	*y1 = 0.0;
    if (x2)
	*x2 = 0.0;
    if (y2)
	*y2 = 0.0;

    if (unlikely (cr->status))
	return;

    status = cr->backend->clip_extents (cr, x1, y1, x2, y2);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_in_clip:
 * @cr: a comac context
 * @x: X coordinate of the point to test
 * @y: Y coordinate of the point to test
 *
 * Tests whether the given point is inside the area that would be
 * visible through the current clip, i.e. the area that would be filled by
 * a comac_paint() operation.
 *
 * See comac_clip(), and comac_clip_preserve().
 *
 * Return value: A non-zero value if the point is inside, or zero if
 * outside.
 *
 * Since: 1.10
 **/
comac_bool_t
comac_in_clip (comac_t *cr, double x, double y)
{
    comac_status_t status;
    comac_bool_t inside = FALSE;

    if (unlikely (cr->status))
	return FALSE;

    status = cr->backend->in_clip (cr, x, y, &inside);
    if (unlikely (status))
	_comac_set_error (cr, status);

    return inside;
}

/**
 * comac_copy_clip_rectangle_list:
 * @cr: a comac context
 *
 * Gets the current clip region as a list of rectangles in user coordinates.
 * Never returns %NULL.
 *
 * The status in the list may be %COMAC_STATUS_CLIP_NOT_REPRESENTABLE to
 * indicate that the clip region cannot be represented as a list of
 * user-space rectangles. The status may have other values to indicate
 * other errors.
 *
 * Returns: the current clip region as a list of rectangles in user coordinates,
 * which should be destroyed using comac_rectangle_list_destroy().
 *
 * Since: 1.4
 **/
comac_rectangle_list_t *
comac_copy_clip_rectangle_list (comac_t *cr)
{
    if (unlikely (cr->status))
	return _comac_rectangle_list_create_in_error (cr->status);

    return cr->backend->clip_copy_rectangle_list (cr);
}

/**
 * COMAC_TAG_DEST:
 *
 * Create a destination for a hyperlink. Destination tag attributes
 * are detailed at [Destinations][dests].
 *
 * Since: 1.16
 **/

/**
 * COMAC_TAG_LINK:
 *
 * Create hyperlink. Link tag attributes are detailed at
 * [Links][links].
 *
 * Since: 1.16
 **/

/**
 * comac_tag_begin:
 * @cr: a comac context
 * @tag_name: tag name
 * @attributes: tag attributes
 *
 * Marks the beginning of the @tag_name structure. Call
 * comac_tag_end() with the same @tag_name to mark the end of the
 * structure.
 *
 * The attributes string is of the form "key1=value2 key2=value2 ...".
 * Values may be boolean (true/false or 1/0), integer, float, string,
 * or an array.
 *
 * String values are enclosed in single quotes
 * ('). Single quotes and backslashes inside the string should be
 * escaped with a backslash.
 *
 * Boolean values may be set to true by only
 * specifying the key. eg the attribute string "key" is the equivalent
 * to "key=true".
 *
 * Arrays are enclosed in '[]'. eg "rect=[1.2 4.3 2.0 3.0]".
 *
 * If no attributes are required, @attributes can be an empty string or NULL.
 *
 * See [Tags and Links Description][comac-Tags-and-Links.description]
 * for the list of tags and attributes.
 *
 * Invalid nesting of tags or invalid attributes will cause @cr to
 * shutdown with a status of %COMAC_STATUS_TAG_ERROR.
 *
 * See comac_tag_end().
 *
 * Since: 1.16
 **/
void
comac_tag_begin (comac_t *cr, const char *tag_name, const char *attributes)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->tag_begin (cr, tag_name, attributes);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_tag_end:
 * @cr: a comac context
 * @tag_name: tag name
 *
 * Marks the end of the @tag_name structure.
 *
 * Invalid nesting of tags will cause @cr to shutdown with a status of
 * %COMAC_STATUS_TAG_ERROR.
 *
 * See comac_tag_begin().
 *
 * Since: 1.16
 **/
comac_public void
comac_tag_end (comac_t *cr, const char *tag_name)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->tag_end (cr, tag_name);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_select_font_face:
 * @cr: a #comac_t
 * @family: a font family name, encoded in UTF-8
 * @slant: the slant for the font
 * @weight: the weight for the font
 *
 * Note: The comac_select_font_face() function call is part of what
 * the comac designers call the "toy" text API. It is convenient for
 * short demos and simple programs, but it is not expected to be
 * adequate for serious text-using applications.
 *
 * Selects a family and style of font from a simplified description as
 * a family name, slant and weight. Comac provides no operation to
 * list available family names on the system (this is a "toy",
 * remember), but the standard CSS2 generic family names, ("serif",
 * "sans-serif", "cursive", "fantasy", "monospace"), are likely to
 * work as expected.
 *
 * If @family starts with the string "@comac:", or if no native font
 * backends are compiled in, comac will use an internal font family.
 * The internal font family recognizes many modifiers in the @family
 * string, most notably, it recognizes the string "monospace".  That is,
 * the family name "@comac:monospace" will use the monospace version of
 * the internal font family.
 *
 * For "real" font selection, see the font-backend-specific
 * font_face_create functions for the font backend you are using. (For
 * example, if you are using the freetype-based comac-ft font backend,
 * see comac_ft_font_face_create_for_ft_face() or
 * comac_ft_font_face_create_for_pattern().) The resulting font face
 * could then be used with comac_scaled_font_create() and
 * comac_set_scaled_font().
 *
 * Similarly, when using the "real" font support, you can call
 * directly into the underlying font system, (such as fontconfig or
 * freetype), for operations such as listing available fonts, etc.
 *
 * It is expected that most applications will need to use a more
 * comprehensive font handling and text layout library, (for example,
 * pango), in conjunction with comac.
 *
 * If text is drawn without a call to comac_select_font_face(), (nor
 * comac_set_font_face() nor comac_set_scaled_font()), the default
 * family is platform-specific, but is essentially "sans-serif".
 * Default slant is %COMAC_FONT_SLANT_NORMAL, and default weight is
 * %COMAC_FONT_WEIGHT_NORMAL.
 *
 * This function is equivalent to a call to comac_toy_font_face_create()
 * followed by comac_set_font_face().
 *
 * Since: 1.0
 **/
void
comac_select_font_face (comac_t *cr,
			const char *family,
			comac_font_slant_t slant,
			comac_font_weight_t weight)
{
    comac_font_face_t *font_face;
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    font_face = comac_toy_font_face_create (family, slant, weight);
    if (unlikely (font_face->status)) {
	_comac_set_error (cr, font_face->status);
	return;
    }

    status = cr->backend->set_font_face (cr, font_face);
    comac_font_face_destroy (font_face);

    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_font_extents:
 * @cr: a #comac_t
 * @extents: a #comac_font_extents_t object into which the results
 * will be stored.
 *
 * Gets the font extents for the currently selected font.
 *
 * Since: 1.0
 **/
void
comac_font_extents (comac_t *cr, comac_font_extents_t *extents)
{
    comac_status_t status;

    extents->ascent = 0.0;
    extents->descent = 0.0;
    extents->height = 0.0;
    extents->max_x_advance = 0.0;
    extents->max_y_advance = 0.0;

    if (unlikely (cr->status))
	return;

    status = cr->backend->font_extents (cr, extents);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_font_face:
 * @cr: a #comac_t
 * @font_face: a #comac_font_face_t, or %NULL to restore to the default font
 *
 * Replaces the current #comac_font_face_t object in the #comac_t with
 * @font_face. The replaced font face in the #comac_t will be
 * destroyed if there are no other references to it.
 *
 * Since: 1.0
 **/
void
comac_set_font_face (comac_t *cr, comac_font_face_t *font_face)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_font_face (cr, font_face);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_get_font_face:
 * @cr: a #comac_t
 *
 * Gets the current font face for a #comac_t.
 *
 * Return value: the current font face.  This object is owned by
 * comac. To keep a reference to it, you must call
 * comac_font_face_reference().
 *
 * This function never returns %NULL. If memory cannot be allocated, a
 * special "nil" #comac_font_face_t object will be returned on which
 * comac_font_face_status() returns %COMAC_STATUS_NO_MEMORY. Using
 * this nil object will cause its error state to propagate to other
 * objects it is passed to, (for example, calling
 * comac_set_font_face() with a nil font will trigger an error that
 * will shutdown the #comac_t object).
 *
 * Since: 1.0
 **/
comac_font_face_t *
comac_get_font_face (comac_t *cr)
{
    if (unlikely (cr->status))
	return (comac_font_face_t *) &_comac_font_face_nil;

    return cr->backend->get_font_face (cr);
}

/**
 * comac_set_font_size:
 * @cr: a #comac_t
 * @size: the new font size, in user space units
 *
 * Sets the current font matrix to a scale by a factor of @size, replacing
 * any font matrix previously set with comac_set_font_size() or
 * comac_set_font_matrix(). This results in a font size of @size user space
 * units. (More precisely, this matrix will result in the font's
 * em-square being a @size by @size square in user space.)
 *
 * If text is drawn without a call to comac_set_font_size(), (nor
 * comac_set_font_matrix() nor comac_set_scaled_font()), the default
 * font size is 10.0.
 *
 * Since: 1.0
 **/
void
comac_set_font_size (comac_t *cr, double size)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_font_size (cr, size);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_set_font_matrix:
 * @cr: a #comac_t
 * @matrix: a #comac_matrix_t describing a transform to be applied to
 * the current font.
 *
 * Sets the current font matrix to @matrix. The font matrix gives a
 * transformation from the design space of the font (in this space,
 * the em-square is 1 unit by 1 unit) to user space. Normally, a
 * simple scale is used (see comac_set_font_size()), but a more
 * complex font matrix can be used to shear the font
 * or stretch it unequally along the two axes
 *
 * Since: 1.0
 **/
void
comac_set_font_matrix (comac_t *cr, const comac_matrix_t *matrix)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = cr->backend->set_font_matrix (cr, matrix);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_get_font_matrix:
 * @cr: a #comac_t
 * @matrix: return value for the matrix
 *
 * Stores the current font matrix into @matrix. See
 * comac_set_font_matrix().
 *
 * Since: 1.0
 **/
void
comac_get_font_matrix (comac_t *cr, comac_matrix_t *matrix)
{
    if (unlikely (cr->status)) {
	comac_matrix_init_identity (matrix);
	return;
    }

    cr->backend->get_font_matrix (cr, matrix);
}

/**
 * comac_set_font_options:
 * @cr: a #comac_t
 * @options: font options to use
 *
 * Sets a set of custom font rendering options for the #comac_t.
 * Rendering options are derived by merging these options with the
 * options derived from underlying surface; if the value in @options
 * has a default value (like %COMAC_ANTIALIAS_DEFAULT), then the value
 * from the surface is used.
 *
 * Since: 1.0
 **/
void
comac_set_font_options (comac_t *cr, const comac_font_options_t *options)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    status = comac_font_options_status ((comac_font_options_t *) options);
    if (unlikely (status)) {
	_comac_set_error (cr, status);
	return;
    }

    status = cr->backend->set_font_options (cr, options);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_get_font_options:
 * @cr: a #comac_t
 * @options: a #comac_font_options_t object into which to store
 *   the retrieved options. All existing values are overwritten
 *
 * Retrieves font rendering options set via #comac_set_font_options.
 * Note that the returned options do not include any options derived
 * from the underlying surface; they are literally the options
 * passed to comac_set_font_options().
 *
 * Since: 1.0
 **/
void
comac_get_font_options (comac_t *cr, comac_font_options_t *options)
{
    /* check that we aren't trying to overwrite the nil object */
    if (comac_font_options_status (options))
	return;

    if (unlikely (cr->status)) {
	_comac_font_options_init_default (options);
	return;
    }

    cr->backend->get_font_options (cr, options);
}

/**
 * comac_set_scaled_font:
 * @cr: a #comac_t
 * @scaled_font: a #comac_scaled_font_t
 *
 * Replaces the current font face, font matrix, and font options in
 * the #comac_t with those of the #comac_scaled_font_t.  Except for
 * some translation, the current CTM of the #comac_t should be the
 * same as that of the #comac_scaled_font_t, which can be accessed
 * using comac_scaled_font_get_ctm().
 *
 * Since: 1.2
 **/
void
comac_set_scaled_font (comac_t *cr, const comac_scaled_font_t *scaled_font)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if ((scaled_font == NULL)) {
	_comac_set_error (cr, _comac_error (COMAC_STATUS_NULL_POINTER));
	return;
    }

    status = scaled_font->status;
    if (unlikely (status)) {
	_comac_set_error (cr, status);
	return;
    }

    status =
	cr->backend->set_scaled_font (cr, (comac_scaled_font_t *) scaled_font);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_get_scaled_font:
 * @cr: a #comac_t
 *
 * Gets the current scaled font for a #comac_t.
 *
 * Return value: the current scaled font. This object is owned by
 * comac. To keep a reference to it, you must call
 * comac_scaled_font_reference().
 *
 * This function never returns %NULL. If memory cannot be allocated, a
 * special "nil" #comac_scaled_font_t object will be returned on which
 * comac_scaled_font_status() returns %COMAC_STATUS_NO_MEMORY. Using
 * this nil object will cause its error state to propagate to other
 * objects it is passed to, (for example, calling
 * comac_set_scaled_font() with a nil font will trigger an error that
 * will shutdown the #comac_t object).
 *
 * Since: 1.4
 **/
comac_scaled_font_t *
comac_get_scaled_font (comac_t *cr)
{
    if (unlikely (cr->status))
	return _comac_scaled_font_create_in_error (cr->status);

    return cr->backend->get_scaled_font (cr);
}

/**
 * comac_text_extents:
 * @cr: a #comac_t
 * @utf8: a NUL-terminated string of text encoded in UTF-8, or %NULL
 * @extents: a #comac_text_extents_t object into which the results
 * will be stored
 *
 * Gets the extents for a string of text. The extents describe a
 * user-space rectangle that encloses the "inked" portion of the text,
 * (as it would be drawn by comac_show_text()). Additionally, the
 * x_advance and y_advance values indicate the amount by which the
 * current point would be advanced by comac_show_text().
 *
 * Note that whitespace characters do not directly contribute to the
 * size of the rectangle (extents.width and extents.height). They do
 * contribute indirectly by changing the position of non-whitespace
 * characters. In particular, trailing whitespace characters are
 * likely to not affect the size of the rectangle, though they will
 * affect the x_advance and y_advance values.
 *
 * Since: 1.0
 **/
void
comac_text_extents (comac_t *cr,
		    const char *utf8,
		    comac_text_extents_t *extents)
{
    comac_status_t status;
    comac_scaled_font_t *scaled_font;
    comac_glyph_t *glyphs = NULL;
    int num_glyphs = 0;
    double x, y;

    extents->x_bearing = 0.0;
    extents->y_bearing = 0.0;
    extents->width = 0.0;
    extents->height = 0.0;
    extents->x_advance = 0.0;
    extents->y_advance = 0.0;

    if (unlikely (cr->status))
	return;

    if (utf8 == NULL)
	return;

    scaled_font = comac_get_scaled_font (cr);
    if (unlikely (scaled_font->status)) {
	_comac_set_error (cr, scaled_font->status);
	return;
    }

    comac_get_current_point (cr, &x, &y);
    status = comac_scaled_font_text_to_glyphs (scaled_font,
					       x,
					       y,
					       utf8,
					       -1,
					       &glyphs,
					       &num_glyphs,
					       NULL,
					       NULL,
					       NULL);

    if (likely (status == COMAC_STATUS_SUCCESS)) {
	status = cr->backend->glyph_extents (cr, glyphs, num_glyphs, extents);
    }
    comac_glyph_free (glyphs);

    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_glyph_extents:
 * @cr: a #comac_t
 * @glyphs: an array of #comac_glyph_t objects
 * @num_glyphs: the number of elements in @glyphs
 * @extents: a #comac_text_extents_t object into which the results
 * will be stored
 *
 * Gets the extents for an array of glyphs. The extents describe a
 * user-space rectangle that encloses the "inked" portion of the
 * glyphs, (as they would be drawn by comac_show_glyphs()).
 * Additionally, the x_advance and y_advance values indicate the
 * amount by which the current point would be advanced by
 * comac_show_glyphs().
 *
 * Note that whitespace glyphs do not contribute to the size of the
 * rectangle (extents.width and extents.height).
 *
 * Since: 1.0
 **/
void
comac_glyph_extents (comac_t *cr,
		     const comac_glyph_t *glyphs,
		     int num_glyphs,
		     comac_text_extents_t *extents)
{
    comac_status_t status;

    extents->x_bearing = 0.0;
    extents->y_bearing = 0.0;
    extents->width = 0.0;
    extents->height = 0.0;
    extents->x_advance = 0.0;
    extents->y_advance = 0.0;

    if (unlikely (cr->status))
	return;

    if (num_glyphs == 0)
	return;

    if (unlikely (num_glyphs < 0)) {
	_comac_set_error (cr, COMAC_STATUS_NEGATIVE_COUNT);
	return;
    }

    if (unlikely (glyphs == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    status = cr->backend->glyph_extents (cr, glyphs, num_glyphs, extents);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_show_text:
 * @cr: a comac context
 * @utf8: a NUL-terminated string of text encoded in UTF-8, or %NULL
 *
 * A drawing operator that generates the shape from a string of UTF-8
 * characters, rendered according to the current font_face, font_size
 * (font_matrix), and font_options.
 *
 * This function first computes a set of glyphs for the string of
 * text. The first glyph is placed so that its origin is at the
 * current point. The origin of each subsequent glyph is offset from
 * that of the previous glyph by the advance values of the previous
 * glyph.
 *
 * After this call the current point is moved to the origin of where
 * the next glyph would be placed in this same progression. That is,
 * the current point will be at the origin of the final glyph offset
 * by its advance values. This allows for easy display of a single
 * logical string with multiple calls to comac_show_text().
 *
 * Note: The comac_show_text() function call is part of what the comac
 * designers call the "toy" text API. It is convenient for short demos
 * and simple programs, but it is not expected to be adequate for
 * serious text-using applications. See comac_show_glyphs() for the
 * "real" text display API in comac.
 *
 * Since: 1.0
 **/
void
comac_show_text (comac_t *cr, const char *utf8)
{
    comac_text_extents_t extents;
    comac_status_t status;
    comac_glyph_t *glyphs, *last_glyph;
    comac_text_cluster_t *clusters;
    int utf8_len, num_glyphs, num_clusters;
    comac_text_cluster_flags_t cluster_flags;
    double x, y;
    comac_bool_t has_show_text_glyphs;
    comac_glyph_t stack_glyphs[COMAC_STACK_ARRAY_LENGTH (comac_glyph_t)];
    comac_text_cluster_t
	stack_clusters[COMAC_STACK_ARRAY_LENGTH (comac_text_cluster_t)];
    comac_scaled_font_t *scaled_font;
    comac_glyph_text_info_t info, *i;

    if (unlikely (cr->status))
	return;

    if (utf8 == NULL)
	return;

    scaled_font = comac_get_scaled_font (cr);
    if (unlikely (scaled_font->status)) {
	_comac_set_error (cr, scaled_font->status);
	return;
    }

    utf8_len = strlen (utf8);

    has_show_text_glyphs =
	comac_surface_has_show_text_glyphs (comac_get_target (cr));

    glyphs = stack_glyphs;
    num_glyphs = ARRAY_LENGTH (stack_glyphs);

    if (has_show_text_glyphs) {
	clusters = stack_clusters;
	num_clusters = ARRAY_LENGTH (stack_clusters);
    } else {
	clusters = NULL;
	num_clusters = 0;
    }

    comac_get_current_point (cr, &x, &y);
    status = comac_scaled_font_text_to_glyphs (scaled_font,
					       x,
					       y,
					       utf8,
					       utf8_len,
					       &glyphs,
					       &num_glyphs,
					       has_show_text_glyphs ? &clusters
								    : NULL,
					       &num_clusters,
					       &cluster_flags);
    if (unlikely (status))
	goto BAIL;

    if (num_glyphs == 0)
	return;

    i = NULL;
    if (has_show_text_glyphs) {
	info.utf8 = utf8;
	info.utf8_len = utf8_len;
	info.clusters = clusters;
	info.num_clusters = num_clusters;
	info.cluster_flags = cluster_flags;
	i = &info;
    }

    status = cr->backend->glyphs (cr, glyphs, num_glyphs, i);
    if (unlikely (status))
	goto BAIL;

    last_glyph = &glyphs[num_glyphs - 1];
    status = cr->backend->glyph_extents (cr, last_glyph, 1, &extents);
    if (unlikely (status))
	goto BAIL;

    x = last_glyph->x + extents.x_advance;
    y = last_glyph->y + extents.y_advance;
    cr->backend->move_to (cr, x, y);

BAIL:
    if (glyphs != stack_glyphs)
	comac_glyph_free (glyphs);
    if (clusters != stack_clusters)
	comac_text_cluster_free (clusters);

    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_show_glyphs:
 * @cr: a comac context
 * @glyphs: array of glyphs to show
 * @num_glyphs: number of glyphs to show
 *
 * A drawing operator that generates the shape from an array of glyphs,
 * rendered according to the current font face, font size
 * (font matrix), and font options.
 *
 * Since: 1.0
 **/
void
comac_show_glyphs (comac_t *cr, const comac_glyph_t *glyphs, int num_glyphs)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (num_glyphs == 0)
	return;

    if (num_glyphs < 0) {
	_comac_set_error (cr, COMAC_STATUS_NEGATIVE_COUNT);
	return;
    }

    if (glyphs == NULL) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    status = cr->backend->glyphs (cr, glyphs, num_glyphs, NULL);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_show_text_glyphs:
 * @cr: a comac context
 * @utf8: a string of text encoded in UTF-8
 * @utf8_len: length of @utf8 in bytes, or -1 if it is NUL-terminated
 * @glyphs: array of glyphs to show
 * @num_glyphs: number of glyphs to show
 * @clusters: array of cluster mapping information
 * @num_clusters: number of clusters in the mapping
 * @cluster_flags: cluster mapping flags
 *
 * This operation has rendering effects similar to comac_show_glyphs()
 * but, if the target surface supports it, uses the provided text and
 * cluster mapping to embed the text for the glyphs shown in the output.
 * If the target does not support the extended attributes, this function
 * acts like the basic comac_show_glyphs() as if it had been passed
 * @glyphs and @num_glyphs.
 *
 * The mapping between @utf8 and @glyphs is provided by an array of
 * <firstterm>clusters</firstterm>.  Each cluster covers a number of
 * text bytes and glyphs, and neighboring clusters cover neighboring
 * areas of @utf8 and @glyphs.  The clusters should collectively cover @utf8
 * and @glyphs in entirety.
 *
 * The first cluster always covers bytes from the beginning of @utf8.
 * If @cluster_flags do not have the %COMAC_TEXT_CLUSTER_FLAG_BACKWARD
 * set, the first cluster also covers the beginning
 * of @glyphs, otherwise it covers the end of the @glyphs array and
 * following clusters move backward.
 *
 * See #comac_text_cluster_t for constraints on valid clusters.
 *
 * Since: 1.8
 **/
void
comac_show_text_glyphs (comac_t *cr,
			const char *utf8,
			int utf8_len,
			const comac_glyph_t *glyphs,
			int num_glyphs,
			const comac_text_cluster_t *clusters,
			int num_clusters,
			comac_text_cluster_flags_t cluster_flags)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    /* A slew of sanity checks */

    /* Special case for NULL and -1 */
    if (utf8 == NULL && utf8_len == -1)
	utf8_len = 0;

    /* No NULLs for non-zeros */
    if ((num_glyphs && glyphs == NULL) || (utf8_len && utf8 == NULL) ||
	(num_clusters && clusters == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    /* A -1 for utf8_len means NUL-terminated */
    if (utf8_len == -1)
	utf8_len = strlen (utf8);

    /* Apart from that, no negatives */
    if (num_glyphs < 0 || utf8_len < 0 || num_clusters < 0) {
	_comac_set_error (cr, COMAC_STATUS_NEGATIVE_COUNT);
	return;
    }

    if (num_glyphs == 0 && utf8_len == 0)
	return;

    if (utf8) {
	/* Make sure clusters cover the entire glyphs and utf8 arrays,
	 * and that cluster boundaries are UTF-8 boundaries. */
	status = _comac_validate_text_clusters (utf8,
						utf8_len,
						glyphs,
						num_glyphs,
						clusters,
						num_clusters,
						cluster_flags);
	if (status == COMAC_STATUS_INVALID_CLUSTERS) {
	    /* Either got invalid UTF-8 text, or cluster mapping is bad.
	     * Differentiate those. */

	    comac_status_t status2;

	    status2 = _comac_utf8_to_ucs4 (utf8, utf8_len, NULL, NULL);
	    if (status2)
		status = status2;
	} else {
	    comac_glyph_text_info_t info;

	    info.utf8 = utf8;
	    info.utf8_len = utf8_len;
	    info.clusters = clusters;
	    info.num_clusters = num_clusters;
	    info.cluster_flags = cluster_flags;

	    status = cr->backend->glyphs (cr, glyphs, num_glyphs, &info);
	}
    } else {
	status = cr->backend->glyphs (cr, glyphs, num_glyphs, NULL);
    }
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_text_path:
 * @cr: a comac context
 * @utf8: a NUL-terminated string of text encoded in UTF-8, or %NULL
 *
 * Adds closed paths for text to the current path.  The generated
 * path if filled, achieves an effect similar to that of
 * comac_show_text().
 *
 * Text conversion and positioning is done similar to comac_show_text().
 *
 * Like comac_show_text(), After this call the current point is
 * moved to the origin of where the next glyph would be placed in
 * this same progression.  That is, the current point will be at
 * the origin of the final glyph offset by its advance values.
 * This allows for chaining multiple calls to to comac_text_path()
 * without having to set current point in between.
 *
 * Note: The comac_text_path() function call is part of what the comac
 * designers call the "toy" text API. It is convenient for short demos
 * and simple programs, but it is not expected to be adequate for
 * serious text-using applications. See comac_glyph_path() for the
 * "real" text path API in comac.
 *
 * Since: 1.0
 **/
void
comac_text_path (comac_t *cr, const char *utf8)
{
    comac_status_t status;
    comac_text_extents_t extents;
    comac_glyph_t stack_glyphs[COMAC_STACK_ARRAY_LENGTH (comac_glyph_t)];
    comac_glyph_t *glyphs, *last_glyph;
    comac_scaled_font_t *scaled_font;
    int num_glyphs;
    double x, y;

    if (unlikely (cr->status))
	return;

    if (utf8 == NULL)
	return;

    glyphs = stack_glyphs;
    num_glyphs = ARRAY_LENGTH (stack_glyphs);

    scaled_font = comac_get_scaled_font (cr);
    if (unlikely (scaled_font->status)) {
	_comac_set_error (cr, scaled_font->status);
	return;
    }

    comac_get_current_point (cr, &x, &y);
    status = comac_scaled_font_text_to_glyphs (scaled_font,
					       x,
					       y,
					       utf8,
					       -1,
					       &glyphs,
					       &num_glyphs,
					       NULL,
					       NULL,
					       NULL);

    if (num_glyphs == 0)
	return;

    status = cr->backend->glyph_path (cr, glyphs, num_glyphs);

    if (unlikely (status))
	goto BAIL;

    last_glyph = &glyphs[num_glyphs - 1];
    status = cr->backend->glyph_extents (cr, last_glyph, 1, &extents);

    if (unlikely (status))
	goto BAIL;

    x = last_glyph->x + extents.x_advance;
    y = last_glyph->y + extents.y_advance;
    cr->backend->move_to (cr, x, y);

BAIL:
    if (glyphs != stack_glyphs)
	comac_glyph_free (glyphs);

    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_glyph_path:
 * @cr: a comac context
 * @glyphs: array of glyphs to show
 * @num_glyphs: number of glyphs to show
 *
 * Adds closed paths for the glyphs to the current path.  The generated
 * path if filled, achieves an effect similar to that of
 * comac_show_glyphs().
 *
 * Since: 1.0
 **/
void
comac_glyph_path (comac_t *cr, const comac_glyph_t *glyphs, int num_glyphs)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (num_glyphs == 0)
	return;

    if (unlikely (num_glyphs < 0)) {
	_comac_set_error (cr, COMAC_STATUS_NEGATIVE_COUNT);
	return;
    }

    if (unlikely (glyphs == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    status = cr->backend->glyph_path (cr, glyphs, num_glyphs);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_get_operator:
 * @cr: a comac context
 *
 * Gets the current compositing operator for a comac context.
 *
 * Return value: the current compositing operator.
 *
 * Since: 1.0
 **/
comac_operator_t
comac_get_operator (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_GSTATE_OPERATOR_DEFAULT;

    return cr->backend->get_operator (cr);
}

#if 0
/**
 * comac_get_opacity:
 * @cr: a comac context
 *
 * Gets the current compositing opacity for a comac context.
 *
 * Return value: the current compositing opacity.
 *
 * Since: TBD
 **/
double
comac_get_opacity (comac_t *cr)
{
    if (unlikely (cr->status))
        return 1.;

    return cr->backend->get_opacity (cr);
}
#endif

/**
 * comac_get_tolerance:
 * @cr: a comac context
 *
 * Gets the current tolerance value, as set by comac_set_tolerance().
 *
 * Return value: the current tolerance value.
 *
 * Since: 1.0
 **/
double
comac_get_tolerance (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_GSTATE_TOLERANCE_DEFAULT;

    return cr->backend->get_tolerance (cr);
}

/**
 * comac_get_antialias:
 * @cr: a comac context
 *
 * Gets the current shape antialiasing mode, as set by
 * comac_set_antialias().
 *
 * Return value: the current shape antialiasing mode.
 *
 * Since: 1.0
 **/
comac_antialias_t
comac_get_antialias (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_ANTIALIAS_DEFAULT;

    return cr->backend->get_antialias (cr);
}

/**
 * comac_has_current_point:
 * @cr: a comac context
 *
 * Returns whether a current point is defined on the current path.
 * See comac_get_current_point() for details on the current point.
 *
 * Return value: whether a current point is defined.
 *
 * Since: 1.6
 **/
comac_bool_t
comac_has_current_point (comac_t *cr)
{
    if (unlikely (cr->status))
	return FALSE;

    return cr->backend->has_current_point (cr);
}

/**
 * comac_get_current_point:
 * @cr: a comac context
 * @x: return value for X coordinate of the current point
 * @y: return value for Y coordinate of the current point
 *
 * Gets the current point of the current path, which is
 * conceptually the final point reached by the path so far.
 *
 * The current point is returned in the user-space coordinate
 * system. If there is no defined current point or if @cr is in an
 * error status, @x and @y will both be set to 0.0. It is possible to
 * check this in advance with comac_has_current_point().
 *
 * Most path construction functions alter the current point. See the
 * following for details on how they affect the current point:
 * comac_new_path(), comac_new_sub_path(),
 * comac_append_path(), comac_close_path(),
 * comac_move_to(), comac_line_to(), comac_curve_to(),
 * comac_rel_move_to(), comac_rel_line_to(), comac_rel_curve_to(),
 * comac_arc(), comac_arc_negative(), comac_rectangle(),
 * comac_text_path(), comac_glyph_path(), comac_stroke_to_path().
 *
 * Some functions use and alter the current point but do not
 * otherwise change current path:
 * comac_show_text().
 *
 * Some functions unset the current path and as a result, current point:
 * comac_fill(), comac_stroke().
 *
 * Since: 1.0
 **/
void
comac_get_current_point (comac_t *cr, double *x_ret, double *y_ret)
{
    double x, y;

    x = y = 0;
    if (cr->status == COMAC_STATUS_SUCCESS &&
	cr->backend->has_current_point (cr)) {
	cr->backend->get_current_point (cr, &x, &y);
    }

    if (x_ret)
	*x_ret = x;
    if (y_ret)
	*y_ret = y;
}

/**
 * comac_get_fill_rule:
 * @cr: a comac context
 *
 * Gets the current fill rule, as set by comac_set_fill_rule().
 *
 * Return value: the current fill rule.
 *
 * Since: 1.0
 **/
comac_fill_rule_t
comac_get_fill_rule (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_GSTATE_FILL_RULE_DEFAULT;

    return cr->backend->get_fill_rule (cr);
}

/**
 * comac_get_line_width:
 * @cr: a comac context
 *
 * This function returns the current line width value exactly as set by
 * comac_set_line_width(). Note that the value is unchanged even if
 * the CTM has changed between the calls to comac_set_line_width() and
 * comac_get_line_width().
 *
 * Return value: the current line width.
 *
 * Since: 1.0
 **/
double
comac_get_line_width (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_GSTATE_LINE_WIDTH_DEFAULT;

    return cr->backend->get_line_width (cr);
}

/**
 * comac_get_hairline:
 * @cr: a comac context
 *
 * Returns whether or not hairline mode is set, as set by comac_set_hairline().
 *
 * Return value: whether hairline mode is set.
 *
 * Since: 1.18
 **/
comac_bool_t
comac_get_hairline (comac_t *cr)
{
    if (unlikely (cr->status))
	return FALSE;

    return cr->backend->get_hairline (cr);
}

/**
 * comac_get_line_cap:
 * @cr: a comac context
 *
 * Gets the current line cap style, as set by comac_set_line_cap().
 *
 * Return value: the current line cap style.
 *
 * Since: 1.0
 **/
comac_line_cap_t
comac_get_line_cap (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_GSTATE_LINE_CAP_DEFAULT;

    return cr->backend->get_line_cap (cr);
}

/**
 * comac_get_line_join:
 * @cr: a comac context
 *
 * Gets the current line join style, as set by comac_set_line_join().
 *
 * Return value: the current line join style.
 *
 * Since: 1.0
 **/
comac_line_join_t
comac_get_line_join (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_GSTATE_LINE_JOIN_DEFAULT;

    return cr->backend->get_line_join (cr);
}

/**
 * comac_get_miter_limit:
 * @cr: a comac context
 *
 * Gets the current miter limit, as set by comac_set_miter_limit().
 *
 * Return value: the current miter limit.
 *
 * Since: 1.0
 **/
double
comac_get_miter_limit (comac_t *cr)
{
    if (unlikely (cr->status))
	return COMAC_GSTATE_MITER_LIMIT_DEFAULT;

    return cr->backend->get_miter_limit (cr);
}

/**
 * comac_get_matrix:
 * @cr: a comac context
 * @matrix: return value for the matrix
 *
 * Stores the current transformation matrix (CTM) into @matrix.
 *
 * Since: 1.0
 **/
void
comac_get_matrix (comac_t *cr, comac_matrix_t *matrix)
{
    if (unlikely (cr->status)) {
	comac_matrix_init_identity (matrix);
	return;
    }

    cr->backend->get_matrix (cr, matrix);
}

/**
 * comac_get_target:
 * @cr: a comac context
 *
 * Gets the target surface for the comac context as passed to
 * comac_create().
 *
 * This function will always return a valid pointer, but the result
 * can be a "nil" surface if @cr is already in an error state,
 * (ie. comac_status() <literal>!=</literal> %COMAC_STATUS_SUCCESS).
 * A nil surface is indicated by comac_surface_status()
 * <literal>!=</literal> %COMAC_STATUS_SUCCESS.
 *
 * Return value: the target surface. This object is owned by comac. To
 * keep a reference to it, you must call comac_surface_reference().
 *
 * Since: 1.0
 **/
comac_surface_t *
comac_get_target (comac_t *cr)
{
    if (unlikely (cr->status))
	return _comac_surface_create_in_error (cr->status);

    return cr->backend->get_original_target (cr);
}

/**
 * comac_get_group_target:
 * @cr: a comac context
 *
 * Gets the current destination surface for the context. This is either
 * the original target surface as passed to comac_create() or the target
 * surface for the current group as started by the most recent call to
 * comac_push_group() or comac_push_group_with_content().
 *
 * This function will always return a valid pointer, but the result
 * can be a "nil" surface if @cr is already in an error state,
 * (ie. comac_status() <literal>!=</literal> %COMAC_STATUS_SUCCESS).
 * A nil surface is indicated by comac_surface_status()
 * <literal>!=</literal> %COMAC_STATUS_SUCCESS.
 *
 * Return value: the target surface. This object is owned by comac. To
 * keep a reference to it, you must call comac_surface_reference().
 *
 * Since: 1.2
 **/
comac_surface_t *
comac_get_group_target (comac_t *cr)
{
    if (unlikely (cr->status))
	return _comac_surface_create_in_error (cr->status);

    return cr->backend->get_current_target (cr);
}

/**
 * comac_copy_path:
 * @cr: a comac context
 *
 * Creates a copy of the current path and returns it to the user as a
 * #comac_path_t. See #comac_path_data_t for hints on how to iterate
 * over the returned data structure.
 *
 * This function will always return a valid pointer, but the result
 * will have no data (<literal>data==%NULL</literal> and
 * <literal>num_data==0</literal>), if either of the following
 * conditions hold:
 *
 * <orderedlist>
 * <listitem>If there is insufficient memory to copy the path. In this
 *     case <literal>path->status</literal> will be set to
 *     %COMAC_STATUS_NO_MEMORY.</listitem>
 * <listitem>If @cr is already in an error state. In this case
 *    <literal>path->status</literal> will contain the same status that
 *    would be returned by comac_status().</listitem>
 * </orderedlist>
 *
 * Return value: the copy of the current path. The caller owns the
 * returned object and should call comac_path_destroy() when finished
 * with it.
 *
 * Since: 1.0
 **/
comac_path_t *
comac_copy_path (comac_t *cr)
{
    if (unlikely (cr->status))
	return _comac_path_create_in_error (cr->status);

    return cr->backend->copy_path (cr);
}

/**
 * comac_copy_path_flat:
 * @cr: a comac context
 *
 * Gets a flattened copy of the current path and returns it to the
 * user as a #comac_path_t. See #comac_path_data_t for hints on
 * how to iterate over the returned data structure.
 *
 * This function is like comac_copy_path() except that any curves
 * in the path will be approximated with piecewise-linear
 * approximations, (accurate to within the current tolerance
 * value). That is, the result is guaranteed to not have any elements
 * of type %COMAC_PATH_CURVE_TO which will instead be replaced by a
 * series of %COMAC_PATH_LINE_TO elements.
 *
 * This function will always return a valid pointer, but the result
 * will have no data (<literal>data==%NULL</literal> and
 * <literal>num_data==0</literal>), if either of the following
 * conditions hold:
 *
 * <orderedlist>
 * <listitem>If there is insufficient memory to copy the path. In this
 *     case <literal>path->status</literal> will be set to
 *     %COMAC_STATUS_NO_MEMORY.</listitem>
 * <listitem>If @cr is already in an error state. In this case
 *    <literal>path->status</literal> will contain the same status that
 *    would be returned by comac_status().</listitem>
 * </orderedlist>
 *
 * Return value: the copy of the current path. The caller owns the
 * returned object and should call comac_path_destroy() when finished
 * with it.
 *
 * Since: 1.0
 **/
comac_path_t *
comac_copy_path_flat (comac_t *cr)
{
    if (unlikely (cr->status))
	return _comac_path_create_in_error (cr->status);

    return cr->backend->copy_path_flat (cr);
}

/**
 * comac_append_path:
 * @cr: a comac context
 * @path: path to be appended
 *
 * Append the @path onto the current path. The @path may be either the
 * return value from one of comac_copy_path() or
 * comac_copy_path_flat() or it may be constructed manually.  See
 * #comac_path_t for details on how the path data structure should be
 * initialized, and note that <literal>path->status</literal> must be
 * initialized to %COMAC_STATUS_SUCCESS.
 *
 * Since: 1.0
 **/
void
comac_append_path (comac_t *cr, const comac_path_t *path)
{
    comac_status_t status;

    if (unlikely (cr->status))
	return;

    if (unlikely (path == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    if (unlikely (path->status)) {
	if (path->status > COMAC_STATUS_SUCCESS &&
	    path->status <= COMAC_STATUS_LAST_STATUS)
	    _comac_set_error (cr, path->status);
	else
	    _comac_set_error (cr, COMAC_STATUS_INVALID_STATUS);
	return;
    }

    if (path->num_data == 0)
	return;

    if (unlikely (path->data == NULL)) {
	_comac_set_error (cr, COMAC_STATUS_NULL_POINTER);
	return;
    }

    status = cr->backend->append_path (cr, path);
    if (unlikely (status))
	_comac_set_error (cr, status);
}

/**
 * comac_status:
 * @cr: a comac context
 *
 * Checks whether an error has previously occurred for this context.
 *
 * Returns: the current status of this context, see #comac_status_t
 *
 * Since: 1.0
 **/
comac_status_t
comac_status (comac_t *cr)
{
    return cr->status;
}
