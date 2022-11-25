/* comac - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat Inc.
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
 *      Owen Taylor <otaylor@redhat.com>
 */

#include "comacint.h"
#include "comac-error-private.h"

/**
 * SECTION:comac-font-options
 * @Title: comac_font_options_t
 * @Short_Description: How a font should be rendered
 * @See_Also: #comac_scaled_font_t
 *
 * The font options specify how fonts should be rendered.  Most of the 
 * time the font options implied by a surface are just right and do not 
 * need any changes, but for pixel-based targets tweaking font options 
 * may result in superior output on a particular display.
 **/

static const comac_font_options_t _comac_font_options_nil = {
    COMAC_ANTIALIAS_DEFAULT,
    COMAC_SUBPIXEL_ORDER_DEFAULT,
    COMAC_LCD_FILTER_DEFAULT,
    COMAC_HINT_STYLE_DEFAULT,
    COMAC_HINT_METRICS_DEFAULT,
    COMAC_ROUND_GLYPH_POS_DEFAULT,
    NULL, /* variations */
    COMAC_COLOR_MODE_DEFAULT,
    COMAC_COLOR_PALETTE_DEFAULT
};

/**
 * _comac_font_options_init_default:
 * @options: a #comac_font_options_t
 *
 * Initializes all fields of the font options object to default values.
 **/
void
_comac_font_options_init_default (comac_font_options_t *options)
{
    options->antialias = COMAC_ANTIALIAS_DEFAULT;
    options->subpixel_order = COMAC_SUBPIXEL_ORDER_DEFAULT;
    options->lcd_filter = COMAC_LCD_FILTER_DEFAULT;
    options->hint_style = COMAC_HINT_STYLE_DEFAULT;
    options->hint_metrics = COMAC_HINT_METRICS_DEFAULT;
    options->round_glyph_positions = COMAC_ROUND_GLYPH_POS_DEFAULT;
    options->variations = NULL;
    options->color_mode = COMAC_COLOR_MODE_DEFAULT;
    options->palette_index = COMAC_COLOR_PALETTE_DEFAULT;
}

void
_comac_font_options_init_copy (comac_font_options_t		*options,
			       const comac_font_options_t	*other)
{
    options->antialias = other->antialias;
    options->subpixel_order = other->subpixel_order;
    options->lcd_filter = other->lcd_filter;
    options->hint_style = other->hint_style;
    options->hint_metrics = other->hint_metrics;
    options->round_glyph_positions = other->round_glyph_positions;
    options->variations = other->variations ? strdup (other->variations) : NULL;
    options->color_mode = other->color_mode;
    options->palette_index = other->palette_index;
}

/**
 * comac_font_options_create:
 *
 * Allocates a new font options object with all options initialized
 *  to default values.
 *
 * Return value: a newly allocated #comac_font_options_t. Free with
 *   comac_font_options_destroy(). This function always returns a
 *   valid pointer; if memory cannot be allocated, then a special
 *   error object is returned where all operations on the object do nothing.
 *   You can check for this with comac_font_options_status().
 *
 * Since: 1.0
 **/
comac_font_options_t *
comac_font_options_create (void)
{
    comac_font_options_t *options;

    options = _comac_malloc (sizeof (comac_font_options_t));
    if (!options) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_font_options_t *) &_comac_font_options_nil;
    }

    _comac_font_options_init_default (options);

    return options;
}

/**
 * comac_font_options_copy:
 * @original: a #comac_font_options_t
 *
 * Allocates a new font options object copying the option values from
 *  @original.
 *
 * Return value: a newly allocated #comac_font_options_t. Free with
 *   comac_font_options_destroy(). This function always returns a
 *   valid pointer; if memory cannot be allocated, then a special
 *   error object is returned where all operations on the object do nothing.
 *   You can check for this with comac_font_options_status().
 *
 * Since: 1.0
 **/
comac_font_options_t *
comac_font_options_copy (const comac_font_options_t *original)
{
    comac_font_options_t *options;

    if (comac_font_options_status ((comac_font_options_t *) original))
	return (comac_font_options_t *) &_comac_font_options_nil;

    options = _comac_malloc (sizeof (comac_font_options_t));
    if (!options) {
	_comac_error_throw (COMAC_STATUS_NO_MEMORY);
	return (comac_font_options_t *) &_comac_font_options_nil;
    }

    _comac_font_options_init_copy (options, original);

    return options;
}

void
_comac_font_options_fini (comac_font_options_t *options)
{
    free (options->variations);
}

/**
 * comac_font_options_destroy:
 * @options: a #comac_font_options_t
 *
 * Destroys a #comac_font_options_t object created with
 * comac_font_options_create() or comac_font_options_copy().
 *
 * Since: 1.0
 **/
void
comac_font_options_destroy (comac_font_options_t *options)
{
    if (comac_font_options_status (options))
	return;

    _comac_font_options_fini (options);
    free (options);
}

/**
 * comac_font_options_status:
 * @options: a #comac_font_options_t
 *
 * Checks whether an error has previously occurred for this
 * font options object
 *
 * Return value: %COMAC_STATUS_SUCCESS, %COMAC_STATUS_NO_MEMORY, or
 *	%COMAC_STATUS_NULL_POINTER.
 *
 * Since: 1.0
 **/
comac_status_t
comac_font_options_status (comac_font_options_t *options)
{
    if (options == NULL)
	return COMAC_STATUS_NULL_POINTER;
    else if (options == (comac_font_options_t *) &_comac_font_options_nil)
	return COMAC_STATUS_NO_MEMORY;
    else
	return COMAC_STATUS_SUCCESS;
}

/**
 * comac_font_options_merge:
 * @options: a #comac_font_options_t
 * @other: another #comac_font_options_t
 *
 * Merges non-default options from @other into @options, replacing
 * existing values. This operation can be thought of as somewhat
 * similar to compositing @other onto @options with the operation
 * of %COMAC_OPERATOR_OVER.
 *
 * Since: 1.0
 **/
void
comac_font_options_merge (comac_font_options_t       *options,
			  const comac_font_options_t *other)
{
    if (comac_font_options_status (options))
	return;

    if (comac_font_options_status ((comac_font_options_t *) other))
	return;

    if (other->antialias != COMAC_ANTIALIAS_DEFAULT)
	options->antialias = other->antialias;
    if (other->subpixel_order != COMAC_SUBPIXEL_ORDER_DEFAULT)
	options->subpixel_order = other->subpixel_order;
    if (other->lcd_filter != COMAC_LCD_FILTER_DEFAULT)
	options->lcd_filter = other->lcd_filter;
    if (other->hint_style != COMAC_HINT_STYLE_DEFAULT)
	options->hint_style = other->hint_style;
    if (other->hint_metrics != COMAC_HINT_METRICS_DEFAULT)
	options->hint_metrics = other->hint_metrics;
    if (other->round_glyph_positions != COMAC_ROUND_GLYPH_POS_DEFAULT)
	options->round_glyph_positions = other->round_glyph_positions;

    if (other->variations) {
      if (options->variations) {
        char *p;

        /* 'merge' variations by concatenating - later entries win */
        p = malloc (strlen (other->variations) + strlen (options->variations) + 2);
        p[0] = 0;
        strcat (p, options->variations);
        strcat (p, ",");
        strcat (p, other->variations);
        free (options->variations);
        options->variations = p;
      }
      else {
        options->variations = strdup (other->variations);
      }
    }

    if (other->color_mode != COMAC_COLOR_MODE_DEFAULT)
	options->color_mode = other->color_mode;
    if (other->palette_index != COMAC_COLOR_PALETTE_DEFAULT)
	options->palette_index = other->palette_index;
}

/**
 * comac_font_options_equal:
 * @options: a #comac_font_options_t
 * @other: another #comac_font_options_t
 *
 * Compares two font options objects for equality.
 *
 * Return value: %TRUE if all fields of the two font options objects match.
 *	Note that this function will return %FALSE if either object is in
 *	error.
 *
 * Since: 1.0
 **/
comac_bool_t
comac_font_options_equal (const comac_font_options_t *options,
			  const comac_font_options_t *other)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return FALSE;
    if (comac_font_options_status ((comac_font_options_t *) other))
	return FALSE;

    if (options == other)
	return TRUE;

    return (options->antialias == other->antialias &&
	    options->subpixel_order == other->subpixel_order &&
	    options->lcd_filter == other->lcd_filter &&
	    options->hint_style == other->hint_style &&
	    options->hint_metrics == other->hint_metrics &&
	    options->round_glyph_positions == other->round_glyph_positions &&
            ((options->variations == NULL && other->variations == NULL) ||
             (options->variations != NULL && other->variations != NULL &&
              strcmp (options->variations, other->variations) == 0)) &&
	    options->color_mode == other->color_mode &&
	    options->palette_index == other->palette_index);
}

/**
 * comac_font_options_hash:
 * @options: a #comac_font_options_t
 *
 * Compute a hash for the font options object; this value will
 * be useful when storing an object containing a #comac_font_options_t
 * in a hash table.
 *
 * Return value: the hash value for the font options object.
 *   The return value can be cast to a 32-bit type if a
 *   32-bit hash value is needed.
 *
 * Since: 1.0
 **/
unsigned long
comac_font_options_hash (const comac_font_options_t *options)
{
    unsigned long hash = 0;

    if (comac_font_options_status ((comac_font_options_t *) options))
	options = &_comac_font_options_nil; /* force default values */

    if (options->variations)
      hash = _comac_string_hash (options->variations, strlen (options->variations));

    hash ^= options->palette_index;

    return ((options->antialias) |
	    (options->subpixel_order << 4) |
	    (options->lcd_filter << 8) |
	    (options->hint_style << 12) |
	    (options->hint_metrics << 16) |
            (options->color_mode << 20)) ^ hash;
}

/**
 * comac_font_options_set_antialias:
 * @options: a #comac_font_options_t
 * @antialias: the new antialiasing mode
 *
 * Sets the antialiasing mode for the font options object. This
 * specifies the type of antialiasing to do when rendering text.
 *
 * Since: 1.0
 **/
void
comac_font_options_set_antialias (comac_font_options_t *options,
				  comac_antialias_t     antialias)
{
    if (comac_font_options_status (options))
	return;

    options->antialias = antialias;
}

/**
 * comac_font_options_get_antialias:
 * @options: a #comac_font_options_t
 *
 * Gets the antialiasing mode for the font options object.
 *
 * Return value: the antialiasing mode
 *
 * Since: 1.0
 **/
comac_antialias_t
comac_font_options_get_antialias (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_ANTIALIAS_DEFAULT;

    return options->antialias;
}

/**
 * comac_font_options_set_subpixel_order:
 * @options: a #comac_font_options_t
 * @subpixel_order: the new subpixel order
 *
 * Sets the subpixel order for the font options object. The subpixel
 * order specifies the order of color elements within each pixel on
 * the display device when rendering with an antialiasing mode of
 * %COMAC_ANTIALIAS_SUBPIXEL. See the documentation for
 * #comac_subpixel_order_t for full details.
 *
 * Since: 1.0
 **/
void
comac_font_options_set_subpixel_order (comac_font_options_t   *options,
				       comac_subpixel_order_t  subpixel_order)
{
    if (comac_font_options_status (options))
	return;

    options->subpixel_order = subpixel_order;
}

/**
 * comac_font_options_get_subpixel_order:
 * @options: a #comac_font_options_t
 *
 * Gets the subpixel order for the font options object.
 * See the documentation for #comac_subpixel_order_t for full details.
 *
 * Return value: the subpixel order for the font options object
 *
 * Since: 1.0
 **/
comac_subpixel_order_t
comac_font_options_get_subpixel_order (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_SUBPIXEL_ORDER_DEFAULT;

    return options->subpixel_order;
}

/**
 * _comac_font_options_set_lcd_filter:
 * @options: a #comac_font_options_t
 * @lcd_filter: the new LCD filter
 *
 * Sets the LCD filter for the font options object. The LCD filter
 * specifies how pixels are filtered when rendered with an antialiasing
 * mode of %COMAC_ANTIALIAS_SUBPIXEL. See the documentation for
 * #comac_lcd_filter_t for full details.
 **/
void
_comac_font_options_set_lcd_filter (comac_font_options_t *options,
				    comac_lcd_filter_t    lcd_filter)
{
    if (comac_font_options_status (options))
	return;

    options->lcd_filter = lcd_filter;
}

/**
 * _comac_font_options_get_lcd_filter:
 * @options: a #comac_font_options_t
 *
 * Gets the LCD filter for the font options object.
 * See the documentation for #comac_lcd_filter_t for full details.
 *
 * Return value: the LCD filter for the font options object
 **/
comac_lcd_filter_t
_comac_font_options_get_lcd_filter (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_LCD_FILTER_DEFAULT;

    return options->lcd_filter;
}

/**
 * _comac_font_options_set_round_glyph_positions:
 * @options: a #comac_font_options_t
 * @round: the new rounding value
 *
 * Sets the rounding options for the font options object. If rounding is set, a
 * glyph's position will be rounded to integer values.
 **/
void
_comac_font_options_set_round_glyph_positions (comac_font_options_t *options,
					       comac_round_glyph_positions_t  round)
{
    if (comac_font_options_status (options))
	return;

    options->round_glyph_positions = round;
}

/**
 * _comac_font_options_get_round_glyph_positions:
 * @options: a #comac_font_options_t
 *
 * Gets the glyph position rounding option for the font options object.
 *
 * Return value: The round glyph posistions flag for the font options object.
 **/
comac_round_glyph_positions_t
_comac_font_options_get_round_glyph_positions (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_ROUND_GLYPH_POS_DEFAULT;

    return options->round_glyph_positions;
}

/**
 * comac_font_options_set_hint_style:
 * @options: a #comac_font_options_t
 * @hint_style: the new hint style
 *
 * Sets the hint style for font outlines for the font options object.
 * This controls whether to fit font outlines to the pixel grid,
 * and if so, whether to optimize for fidelity or contrast.
 * See the documentation for #comac_hint_style_t for full details.
 *
 * Since: 1.0
 **/
void
comac_font_options_set_hint_style (comac_font_options_t *options,
				   comac_hint_style_t    hint_style)
{
    if (comac_font_options_status (options))
	return;

    options->hint_style = hint_style;
}

/**
 * comac_font_options_get_hint_style:
 * @options: a #comac_font_options_t
 *
 * Gets the hint style for font outlines for the font options object.
 * See the documentation for #comac_hint_style_t for full details.
 *
 * Return value: the hint style for the font options object
 *
 * Since: 1.0
 **/
comac_hint_style_t
comac_font_options_get_hint_style (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_HINT_STYLE_DEFAULT;

    return options->hint_style;
}

/**
 * comac_font_options_set_hint_metrics:
 * @options: a #comac_font_options_t
 * @hint_metrics: the new metrics hinting mode
 *
 * Sets the metrics hinting mode for the font options object. This
 * controls whether metrics are quantized to integer values in
 * device units.
 * See the documentation for #comac_hint_metrics_t for full details.
 *
 * Since: 1.0
 **/
void
comac_font_options_set_hint_metrics (comac_font_options_t *options,
				     comac_hint_metrics_t  hint_metrics)
{
    if (comac_font_options_status (options))
	return;

    options->hint_metrics = hint_metrics;
}

/**
 * comac_font_options_get_hint_metrics:
 * @options: a #comac_font_options_t
 *
 * Gets the metrics hinting mode for the font options object.
 * See the documentation for #comac_hint_metrics_t for full details.
 *
 * Return value: the metrics hinting mode for the font options object
 *
 * Since: 1.0
 **/
comac_hint_metrics_t
comac_font_options_get_hint_metrics (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_HINT_METRICS_DEFAULT;

    return options->hint_metrics;
}

/**
 * comac_font_options_set_variations:
 * @options: a #comac_font_options_t
 * @variations: the new font variations, or %NULL
 *
 * Sets the OpenType font variations for the font options object.
 * Font variations are specified as a string with a format that
 * is similar to the CSS font-variation-settings. The string contains
 * a comma-separated list of axis assignments, which each assignment
 * consists of a 4-character axis name and a value, separated by
 * whitespace and optional equals sign.
 *
 * Examples:
 *
 * wght=200,wdth=140.5
 *
 * wght 200 , wdth 140.5
 *
 * Since: 1.16
 **/
void
comac_font_options_set_variations (comac_font_options_t *options,
                                   const char           *variations)
{
  char *tmp = variations ? strdup (variations) : NULL;
  free (options->variations);
  options->variations = tmp;
}

/**
 * comac_font_options_get_variations:
 * @options: a #comac_font_options_t
 *
 * Gets the OpenType font variations for the font options object.
 * See comac_font_options_set_variations() for details about the
 * string format.
 *
 * Return value: the font variations for the font options object. The
 *   returned string belongs to the @options and must not be modified.
 *   It is valid until either the font options object is destroyed or
 *   the font variations in this object is modified with
 *   comac_font_options_set_variations().
 *
 * Since: 1.16
 **/
const char *
comac_font_options_get_variations (comac_font_options_t *options)
{
  return options->variations;
}

/**
 * comac_font_options_set_color_mode:
 * @options: a #comac_font_options_t
 * @font_color: the new color mode
 *
 * Sets the color mode for the font options object. This controls
 * whether color fonts are to be rendered in color or as outlines.
 * See the documentation for #comac_color_mode_t for full details.
 *
 * Since: 1.18
 **/
comac_public void
comac_font_options_set_color_mode (comac_font_options_t *options,
                                   comac_color_mode_t    color_mode)
{
    if (comac_font_options_status (options))
	return;

    options->color_mode = color_mode;
}

/**
 * comac_font_options_get_color_mode:
 * @options: a #comac_font_options_t
 *
 * Gets the color mode for the font options object.
 * See the documentation for #comac_color_mode_t for full details.
 *
 * Return value: the color mode for the font options object
 *
 * Since: 1.18
 **/
comac_public comac_color_mode_t
comac_font_options_get_color_mode (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_COLOR_MODE_DEFAULT;

    return options->color_mode;
}

/**
 * comac_font_options_set_color_palette:
 * @options: a #comac_font_options_t
 * @palette_index: the palette index in the CPAL table
 *
 * Sets the OpenType font color palette for the font options
 * object. OpenType color fonts with a CPAL table may contain multiple
 * palettes. The default color palette index is %COMAC_COLOR_PALETTE_DEFAULT. If
 * @palette_index is invalid, the default palette is used.
 *
 * Since: 1.18
 **/
void
comac_font_options_set_color_palette (comac_font_options_t *options,
                                      unsigned int          palette_index)
{
    if (comac_font_options_status (options))
	return;

    options->palette_index = palette_index;
}

/**
 * comac_font_options_get_color_palette:
 * @options: a #comac_font_options_t
 *
 * Gets the OpenType color font palette for the font options object.
 *
 * Return value: the palette index
 *
 * Since: 1.18
 **/
unsigned int
comac_font_options_get_color_palette (const comac_font_options_t *options)
{
    if (comac_font_options_status ((comac_font_options_t *) options))
	return COMAC_COLOR_PALETTE_DEFAULT;

    return options->palette_index;
}
