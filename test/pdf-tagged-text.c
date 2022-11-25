/*
 * Copyright © 2016 Adrian Johnson
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
 * Author: Adrian Johnson <ajohnson@redneon.com>
 */

#include "comac-test.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#include <comac.h>
#include <comac-pdf.h>

/* This test checks PDF with
 * - tagged text
 * - hyperlinks
 * - document outline
 * - metadata
 * - thumbnails
 * - page labels
 */

#define BASENAME "pdf-tagged-text.out"

#define PAGE_WIDTH 595
#define PAGE_HEIGHT 842

#define HEADING1_SIZE 16
#define HEADING2_SIZE 14
#define HEADING3_SIZE 12
#define TEXT_SIZE 12
#define HEADING_HEIGHT 50
#define MARGIN 50

struct section {
    int level;
    const char *heading;
    int num_paragraphs;
};

static const struct section contents[] = {
    {0, "Chapter 1", 1},     {1, "Section 1.1", 4},
    {2, "Section 1.1.1", 3}, {1, "Section 1.2", 2},
    {2, "Section 1.2.1", 4}, {2, "Section 1.2.2", 4},
    {1, "Section 1.3", 2},   {0, "Chapter 2", 1},
    {1, "Section 2.1", 4},   {2, "Section 2.1.1", 3},
    {1, "Section 2.2", 2},   {2, "Section 2.2.1", 4},
    {2, "Section 2.2.2", 4}, {1, "Section 2.3", 2},
    {0, "Chapter 3", 1},     {1, "Section 3.1", 4},
    {2, "Section 3.1.1", 3}, {1, "Section 3.2", 2},
    {2, "Section 3.2.1", 4}, {2, "Section 3.2.2", 4},
    {1, "Section 3.3", 2},   {0, NULL}};

static const char *ipsum_lorem =
    "Lorem ipsum dolor sit amet, consectetur adipiscing"
    " elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
    " Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi"
    " ut aliquip ex ea commodo consequat. Duis aute irure dolor in"
    " reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla"
    " pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa"
    " qui officia deserunt mollit anim id est laborum.";

static const char *roman_numerals[] = {"i", "ii", "iii", "iv", "v"};

#define MAX_PARAGRAPH_LINES 20

static int paragraph_num_lines;
static char *paragraph_text[MAX_PARAGRAPH_LINES];
static double paragraph_height;
static double line_height;
static double y_pos;
static int outline_parents[10];
static int page_num;

static void
layout_paragraph (comac_t *cr)
{
    char *text, *begin, *end, *prev_end;
    comac_text_extents_t text_extents;
    comac_font_extents_t font_extents;

    comac_select_font_face (cr,
			    "Serif",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);
    comac_font_extents (cr, &font_extents);
    line_height = font_extents.height;
    paragraph_height = 0;
    paragraph_num_lines = 0;
    text = strdup (ipsum_lorem);
    begin = text;
    end = text;
    prev_end = end;
    while (*begin) {
	end = strchr (end, ' ');
	if (! end) {
	    paragraph_text[paragraph_num_lines++] = strdup (begin);
	    break;
	}
	*end = 0;
	comac_text_extents (cr, begin, &text_extents);
	*end = ' ';
	if (text_extents.width + 2 * MARGIN > PAGE_WIDTH) {
	    int len = prev_end - begin;
	    char *s = malloc (len);
	    memcpy (s, begin, len);
	    s[len - 1] = 0;
	    paragraph_text[paragraph_num_lines++] = s;
	    begin = prev_end + 1;
	}
	prev_end = end;
	end++;
    }
    paragraph_height = line_height * (paragraph_num_lines + 1);
    free (text);
}

static void
draw_paragraph (comac_t *cr)
{
    int i;

    comac_select_font_face (cr,
			    "Serif",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);
    comac_tag_begin (cr, "P", NULL);
    for (i = 0; i < paragraph_num_lines; i++) {
	comac_move_to (cr, MARGIN, y_pos);
	comac_show_text (cr, paragraph_text[i]);
	y_pos += line_height;
    }
    comac_tag_end (cr, "P");
    y_pos += line_height;
}

static void
draw_page_num (comac_surface_t *surface,
	       comac_t *cr,
	       const char *prefix,
	       int num)
{
    char buf[100];

    buf[0] = 0;
    if (prefix)
	strcat (buf, prefix);

    if (num)
	sprintf (buf + strlen (buf), "%d", num);

    comac_save (cr);
    comac_select_font_face (cr,
			    "Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, 12);
    comac_set_source_rgb (cr, 0, 0, 0);
    comac_move_to (cr, PAGE_WIDTH / 2, PAGE_HEIGHT - MARGIN);
    comac_show_text (cr, buf);
    comac_restore (cr);
    comac_pdf_surface_set_page_label (surface, buf);
}

static void
draw_contents (comac_surface_t *surface,
	       comac_t *cr,
	       const struct section *section)
{
    char buf[100];

    sprintf (buf, "dest='%s'", section->heading);
    comac_select_font_face (cr,
			    "Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    switch (section->level) {
    case 0:
	comac_set_font_size (cr, HEADING1_SIZE);
	break;
    case 1:
	comac_set_font_size (cr, HEADING2_SIZE);
	break;
    case 2:
	comac_set_font_size (cr, HEADING3_SIZE);
	break;
    }

    if (y_pos + HEADING_HEIGHT + MARGIN > PAGE_HEIGHT) {
	comac_show_page (cr);
	draw_page_num (surface, cr, roman_numerals[page_num++], 0);
	y_pos = MARGIN;
    }
    comac_move_to (cr, MARGIN, y_pos);
    comac_save (cr);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_tag_begin (cr, "TOCI", NULL);
    comac_tag_begin (cr, "Reference", NULL);
    comac_tag_begin (cr, COMAC_TAG_LINK, buf);
    comac_show_text (cr, section->heading);
    comac_tag_end (cr, COMAC_TAG_LINK);
    comac_tag_end (cr, "Reference");
    comac_tag_end (cr, "TOCI");
    comac_restore (cr);
    y_pos += HEADING_HEIGHT;
}

static void
draw_section (comac_surface_t *surface,
	      comac_t *cr,
	      const struct section *section)
{
    int flags, i;
    char buf[100];
    char buf2[100];

    comac_tag_begin (cr, "Sect", NULL);
    sprintf (buf, "name='%s'", section->heading);
    sprintf (buf2, "dest='%s'", section->heading);
    comac_select_font_face (cr,
			    "Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_BOLD);
    if (section->level == 0) {
	comac_show_page (cr);
	draw_page_num (surface, cr, NULL, page_num++);
	comac_set_font_size (cr, HEADING1_SIZE);
	comac_move_to (cr, MARGIN, MARGIN);
	comac_tag_begin (cr, "H1", NULL);
	comac_tag_begin (cr, COMAC_TAG_DEST, buf);
	comac_show_text (cr, section->heading);
	comac_tag_end (cr, COMAC_TAG_DEST);
	comac_tag_end (cr, "H1");
	y_pos = MARGIN + HEADING_HEIGHT;
	flags = COMAC_PDF_OUTLINE_FLAG_BOLD | COMAC_PDF_OUTLINE_FLAG_OPEN;
	outline_parents[0] =
	    comac_pdf_surface_add_outline (surface,
					   COMAC_PDF_OUTLINE_ROOT,
					   section->heading,
					   buf2,
					   flags);
    } else {
	if (section->level == 1) {
	    comac_set_font_size (cr, HEADING2_SIZE);
	    flags = 0;
	} else {
	    comac_set_font_size (cr, HEADING3_SIZE);
	    flags = COMAC_PDF_OUTLINE_FLAG_ITALIC;
	}

	if (y_pos + HEADING_HEIGHT + paragraph_height + MARGIN > PAGE_HEIGHT) {
	    comac_show_page (cr);
	    draw_page_num (surface, cr, NULL, page_num++);
	    y_pos = MARGIN;
	}
	comac_move_to (cr, MARGIN, y_pos);
	if (section->level == 1)
	    comac_tag_begin (cr, "H2", NULL);
	else
	    comac_tag_begin (cr, "H3", NULL);
	comac_tag_begin (cr, COMAC_TAG_DEST, buf);
	comac_show_text (cr, section->heading);
	comac_tag_end (cr, COMAC_TAG_DEST);
	if (section->level == 1)
	    comac_tag_end (cr, "H2");
	else
	    comac_tag_end (cr, "H3");
	y_pos += HEADING_HEIGHT;
	outline_parents[section->level] =
	    comac_pdf_surface_add_outline (surface,
					   outline_parents[section->level - 1],
					   section->heading,
					   buf2,
					   flags);
    }

    for (i = 0; i < section->num_paragraphs; i++) {
	if (y_pos + paragraph_height + MARGIN > PAGE_HEIGHT) {
	    comac_show_page (cr);
	    draw_page_num (surface, cr, NULL, page_num++);
	    y_pos = MARGIN;
	}
	draw_paragraph (cr);
    }
    comac_tag_end (cr, "Sect");
}

static void
draw_cover (comac_surface_t *surface, comac_t *cr)
{
    comac_text_extents_t text_extents;
    char buf[200];
    comac_rectangle_t url_box;
    const char *comac_url = "https://www.comacgraphics.org/";
    const double url_box_margin = 20.0;

    comac_tag_begin (cr, COMAC_TAG_DEST, "name='cover'  internal");
    comac_tag_end (cr, COMAC_TAG_DEST);

    comac_select_font_face (cr,
			    "Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_BOLD);
    comac_set_font_size (cr, 16);
    comac_move_to (cr, PAGE_WIDTH / 3, PAGE_HEIGHT / 3);
    comac_tag_begin (cr, "Span", NULL);
    comac_show_text (cr, "PDF Features Test");
    comac_tag_end (cr, "Span");

    /* Test URL link using "rect" attribute. The entire rectangle surrounding the URL should be a clickable link.  */
    comac_move_to (cr, PAGE_WIDTH / 3, 2 * PAGE_HEIGHT / 3);
    comac_select_font_face (cr,
			    "Sans",
			    COMAC_FONT_SLANT_NORMAL,
			    COMAC_FONT_WEIGHT_NORMAL);
    comac_set_font_size (cr, TEXT_SIZE);
    comac_set_source_rgb (cr, 0, 0, 1);
    comac_show_text (cr, comac_url);
    comac_text_extents (cr, comac_url, &text_extents);
    url_box.x = PAGE_WIDTH / 3 - url_box_margin;
    url_box.y = 2 * PAGE_HEIGHT / 3 - url_box_margin;
    url_box.width = text_extents.width + 2 * url_box_margin;
    url_box.height = -text_extents.height + 2 * url_box_margin;
    comac_rectangle (cr, url_box.x, url_box.y, url_box.width, url_box.height);
    comac_stroke (cr);
    snprintf (buf,
	      sizeof (buf),
	      "rect=[%f %f %f %f] uri=\'%s\'",
	      url_box.x,
	      url_box.y,
	      url_box.width,
	      url_box.height,
	      comac_url);
    comac_tag_begin (cr, COMAC_TAG_LINK, buf);
    comac_tag_end (cr, COMAC_TAG_LINK);

    /* Create link to not yet emmited page number */
    comac_tag_begin (cr, COMAC_TAG_LINK, "page=5");
    comac_move_to (cr, PAGE_WIDTH / 3, 4 * PAGE_HEIGHT / 5);
    comac_show_text (cr, "link to page 5");
    comac_tag_end (cr, COMAC_TAG_LINK);

    /* Create link to not yet emmited destination */
    comac_tag_begin (cr, COMAC_TAG_LINK, "dest='Section 3.3'");
    comac_move_to (cr, PAGE_WIDTH / 3, 4.2 * PAGE_HEIGHT / 5);
    comac_show_text (cr, "link to page section 3.3");
    comac_tag_end (cr, COMAC_TAG_LINK);

    /* Create link to external file */
    comac_tag_begin (cr, COMAC_TAG_LINK, "file='foo.pdf' page=1");
    comac_move_to (cr, PAGE_WIDTH / 3, 4.4 * PAGE_HEIGHT / 5);
    comac_show_text (cr, "link file 'foo.pdf'");
    comac_tag_end (cr, COMAC_TAG_LINK);

    draw_page_num (surface, cr, "cover", 0);
}

static void
create_document (comac_surface_t *surface, comac_t *cr)
{
    layout_paragraph (cr);

    comac_pdf_surface_set_thumbnail_size (surface,
					  PAGE_WIDTH / 10,
					  PAGE_HEIGHT / 10);

    comac_pdf_surface_set_metadata (surface,
				    COMAC_PDF_METADATA_TITLE,
				    "PDF Features Test");
    comac_pdf_surface_set_metadata (surface,
				    COMAC_PDF_METADATA_AUTHOR,
				    "comac test suite");
    comac_pdf_surface_set_metadata (surface,
				    COMAC_PDF_METADATA_SUBJECT,
				    "comac test");
    comac_pdf_surface_set_metadata (
	surface,
	COMAC_PDF_METADATA_KEYWORDS,
	"tags, links, outline, page labels, metadata, thumbnails");
    comac_pdf_surface_set_metadata (surface,
				    COMAC_PDF_METADATA_CREATOR,
				    "pdf-features");
    comac_pdf_surface_set_metadata (surface,
				    COMAC_PDF_METADATA_CREATE_DATE,
				    "2016-01-01T12:34:56+10:30");
    comac_pdf_surface_set_metadata (surface,
				    COMAC_PDF_METADATA_MOD_DATE,
				    "2016-06-21T05:43:21Z");

    comac_pdf_surface_set_custom_metadata (surface, "DocumentNumber", "12345");
    /* Include some non ASCII characters */
    comac_pdf_surface_set_custom_metadata (surface,
					   "Document Name",
					   "\xc2\xab"
					   "comac test\xc2\xbb");
    /* Test unsetting custom metadata. "DocumentNumber" should not be emitted. */
    comac_pdf_surface_set_custom_metadata (surface, "DocumentNumber", "");

    comac_tag_begin (cr, "Document", NULL);

    draw_cover (surface, cr);
    comac_pdf_surface_add_outline (surface,
				   COMAC_PDF_OUTLINE_ROOT,
				   "Cover",
				   "page=1",
				   COMAC_PDF_OUTLINE_FLAG_BOLD);

    /* Create a simple link annotation. */
    comac_tag_begin (cr,
		     COMAC_TAG_LINK,
		     "uri='http://example.org' rect=[10 10 20 20]");
    comac_tag_end (cr, COMAC_TAG_LINK);

    /* Try to create a link annotation while the clip is empty;
     * it will still be emitted.
     */
    comac_save (cr);
    comac_new_path (cr);
    comac_rectangle (cr, 100, 100, 50, 0);
    comac_clip (cr);
    comac_tag_begin (cr,
		     COMAC_TAG_LINK,
		     "uri='http://example.com' rect=[100 100 20 20]");
    comac_tag_end (cr, COMAC_TAG_LINK);
    comac_restore (cr);

    /* An annotation whose rect has a negative coordinate. */
    comac_tag_begin (cr,
		     COMAC_TAG_LINK,
		     "uri='http://127.0.0.1/' rect=[10.0 -10.0 100.0 100.0]");
    comac_tag_end (cr, COMAC_TAG_LINK);

    comac_show_page (cr);

    page_num = 0;
    draw_page_num (surface, cr, roman_numerals[page_num++], 0);
    y_pos = MARGIN;

    comac_pdf_surface_add_outline (surface,
				   COMAC_PDF_OUTLINE_ROOT,
				   "Contents",
				   "dest='TOC'",
				   COMAC_PDF_OUTLINE_FLAG_BOLD);

    comac_tag_begin (cr, COMAC_TAG_DEST, "name='TOC' internal");
    comac_tag_begin (cr, "TOC", NULL);
    const struct section *sect = contents;
    while (sect->heading) {
	draw_contents (surface, cr, sect);
	sect++;
    }
    comac_tag_end (cr, "TOC");
    comac_tag_end (cr, COMAC_TAG_DEST);

    page_num = 1;
    sect = contents;
    while (sect->heading) {
	draw_section (surface, cr, sect);
	sect++;
    }

    comac_show_page (cr);

    comac_tag_begin (cr, COMAC_TAG_LINK, "dest='cover'");
    comac_move_to (cr, PAGE_WIDTH / 3, 2 * PAGE_HEIGHT / 5);
    comac_show_text (cr, "link to cover");
    comac_tag_end (cr, COMAC_TAG_LINK);

    comac_tag_begin (cr, COMAC_TAG_LINK, "page=3");
    comac_move_to (cr, PAGE_WIDTH / 3, 3 * PAGE_HEIGHT / 5);
    comac_show_text (cr, "link to page 3");
    comac_tag_end (cr, COMAC_TAG_LINK);

    comac_tag_end (cr, "Document");
}

#ifdef HAVE_MMAP
static comac_test_status_t
check_contains_string (comac_test_context_t *ctx,
		       const void *hay,
		       size_t size,
		       const char *needle)
{
    if (memmem (hay, size, needle, strlen (needle)))
	return COMAC_TEST_SUCCESS;

    comac_test_log (ctx,
		    "Failed to find expected string in generated PDF: %s\n",
		    needle);
    return COMAC_TEST_FAILURE;
}
#endif

static comac_test_status_t
check_created_pdf (comac_test_context_t *ctx, const char *filename)
{
    comac_test_status_t result = COMAC_TEST_SUCCESS;
    int fd;
    struct stat st;
#ifdef HAVE_MMAP
    void *contents;
#endif

    fd = open (filename, O_RDONLY, 0);
    if (fd < 0) {
	comac_test_log (ctx,
			"Failed to open generated PDF file %s: %s\n",
			filename,
			strerror (errno));
	return COMAC_TEST_FAILURE;
    }

    if (fstat (fd, &st) == -1) {
	comac_test_log (ctx,
			"Failed to stat generated PDF file %s: %s\n",
			filename,
			strerror (errno));
	close (fd);
	return COMAC_TEST_FAILURE;
    }

    if (st.st_size == 0) {
	comac_test_log (ctx, "Generated PDF file %s is empty\n", filename);
	close (fd);
	return COMAC_TEST_FAILURE;
    }

#ifdef HAVE_MMAP
    contents = mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (contents == NULL) {
	comac_test_log (ctx,
			"Failed to mmap generated PDF file %s: %s\n",
			filename,
			strerror (errno));
	close (fd);
	return COMAC_TEST_FAILURE;
    }

    /* check metadata */
    result |= check_contains_string (ctx,
				     contents,
				     st.st_size,
				     "/Title (PDF Features Test)");
    result |= check_contains_string (ctx,
				     contents,
				     st.st_size,
				     "/Author (comac test suite)");
    result |= check_contains_string (ctx,
				     contents,
				     st.st_size,
				     "/Creator (pdf-features)");
    result |= check_contains_string (ctx,
				     contents,
				     st.st_size,
				     "/CreationDate (20160101123456+10'30')");
    result |= check_contains_string (ctx,
				     contents,
				     st.st_size,
				     "/ModDate (20160621054321Z)");

    /* check that both the example.org and example.com links were generated */
    result |=
	check_contains_string (ctx, contents, st.st_size, "http://example.org");
    result |=
	check_contains_string (ctx, contents, st.st_size, "http://example.com");

    // TODO: add more checks

    munmap (contents, st.st_size);
#endif

    close (fd);

    return result;
}

static comac_test_status_t
create_pdf (comac_test_context_t *ctx, comac_bool_t check_output)
{
    comac_surface_t *surface;
    comac_t *cr;
    comac_status_t status, status2;
    comac_test_status_t result;
    comac_pdf_version_t version;
    char *filename;
    const char *path =
	comac_test_mkdir (COMAC_TEST_OUTPUT_DIR) ? COMAC_TEST_OUTPUT_DIR : ".";

    /* check_created_pdf() only works with version 1.4. In version 1.5
     * the text that is searched for is compressed. */
    version = check_output ? COMAC_PDF_VERSION_1_4 : COMAC_PDF_VERSION_1_5;

    xasprintf (&filename,
	       "%s/%s-%s.pdf",
	       path,
	       BASENAME,
	       check_output ? "1.4" : "1.5");
    surface = comac_pdf_surface_create (filename, PAGE_WIDTH, PAGE_HEIGHT);

    comac_pdf_surface_restrict_to_version (surface, version);

    cr = comac_create (surface);
    create_document (surface, cr);

    status = comac_status (cr);
    comac_destroy (cr);
    comac_surface_finish (surface);
    status2 = comac_surface_status (surface);
    if (status == COMAC_STATUS_SUCCESS)
	status = status2;

    comac_surface_destroy (surface);
    if (status) {
	comac_test_log (ctx,
			"Failed to create pdf surface for file %s: %s\n",
			filename,
			comac_status_to_string (status));
	return COMAC_TEST_FAILURE;
    }

    result = COMAC_TEST_SUCCESS;
    if (check_output)
	result = check_created_pdf (ctx, filename);

    free (filename);

    return result;
}

static comac_test_status_t
preamble (comac_test_context_t *ctx)
{
    comac_test_status_t result;

    if (! comac_test_is_target_enabled (ctx, "pdf"))
	return COMAC_TEST_UNTESTED;

    /* Create version 1.5 PDF. This can only be manually checked */
    create_pdf (ctx, FALSE);

    /* Create version 1.4 PDF and checkout output */
    result = create_pdf (ctx, TRUE);

    return result;
}

COMAC_TEST (pdf_tagged_text,
	    "Check tagged text, hyperlinks and PDF document features",
	    "pdf", /* keywords */
	    NULL,  /* requirements */
	    0,
	    0,
	    preamble,
	    NULL)
