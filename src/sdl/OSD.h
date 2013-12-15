// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 2008 VBA-M development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef __VBA_OSD_SDL_H__
#define __VBA_OSD_SDL_H__

#include "DisplaySDL.h"
#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Opaque TextOSD entity */
typedef struct TextOSD TextOSD;

/**
 * Create a new TextOSD entity
 *
 * @param display The display to use to render the text
 * @param message The message to display
 * @param parent Renderable the TextOSD to be created renders over
 * @param err return location for a GError, or NULL
 * @return TextOSD entity or NULL if the creation failed
 */
TextOSD *text_osd_create(Display *display, const gchar *message, const Renderable *parent, GError **err);

/**
 * Update the message of a TextOSD
 *
 * @param text TextOSD entity
 * @param message The message to display
 */
void text_osd_set_message(TextOSD *text, const gchar *message);

/**
 * Set the color of a TextOSD
 *
 * @param text TextOSD entity
 * @param r Red color comoponent
 * @param g Green color comoponent
 * @param b Blue color comoponent
 */
void text_osd_set_color(TextOSD *text, guint8 r, guint8 g, guint8 b);

/**
 * Tells the TextOSD to empty itself after a delay
 *
 * Useful for status messages
 *
 * @param text TextOSD entity
 * @param duration Delay in milliseconds before the TextOSD clears itself
 */
void text_osd_set_auto_clear(TextOSD *text, guint duration);

/**
 * Set the position of the TextOSD on the screen
 *
 * If the x or y parameters are negative the positioning is relative to the
 * opposite side of the window.
 *
 * @param text TextOSD entity
 * @param x number of pixels away from the left side of the window the text is drawn at
 * @param y number of pixels away from the top side of the window the text is drawn at
 */
void text_osd_set_position(TextOSD *text, gint x, gint y);

/**
 * Set the opacity of the TextOSD
 *
 * @param text TextOSD entity
 * @param opacity Opacity between 0 and 100
 */
void text_osd_set_opacity(TextOSD *text, gint opacity);

/**
 * Set the size of the TextOSD
 *
 * @param text TextOSD entity
 * @param width Maximum width of the text message
 * @param height Height of the text message, used to compute the font size
 */
void text_osd_set_size(TextOSD *text, gint width, gint height);

/**
 * Set the alignment of the TextOSD within its parent
 *
 * @param text TextOSD entity
 * @param horizontal Horizontal alignment, can be ALIGN_LEFT, ALIGN_CENTER or ALIGN_RIGHT
 * @param vertical Vertical alignment, can be ALIGN_TOP, ALIGN_MIDDLE or ALIGN_BOTTOM
 */
void text_osd_set_alignment(TextOSD *text, HorizontalAlignment horizontal, VerticalAlignment vertical);

/**
 * Free a text OSD entity
 *
 * @param text TextOSD entity
 */
void text_osd_free(TextOSD *text);

/** Opaque ImageOSD entity */
typedef struct ImageOSD ImageOSD;

/**
 * Create a new ImageOSD entity
 *
 * @param display The display to use to render the image
 * @param file Filename of the image file to open. Only PNG images are supported.
 * @param err return location for a GError, or NULL
 * @return ImageOSD entity or NULL if the creation failed
 */
ImageOSD *image_osd_create(Display *display, const gchar *file, GError **err);

/**
 * Set the position of the ImageOSD on the screen
 *
 * If the x or y parameters are negative the positioning is relative to the
 * opposite side of the window.
 *
 * @param image ImageOSD entity
 * @param x number of pixels away from the left side of the window the image is drawn at
 * @param y number of pixels away from the top side of the window the image is drawn at
 */
void image_osd_set_position(ImageOSD *image, gint x, gint y);

/**
 * Free an image OSD entity
 *
 * @param image ImageOSD entity
 */
void image_osd_free(ImageOSD *image);

/** Opaque RectOSD entity */
typedef struct RectOSD RectOSD;

/**
 * Create a new RectOSD entity
 *
 * @param display The display to use to render the rectangle
 * @param x number of pixels away from the left side of the window the rectangle is drawn at
 * @param y number of pixels away from the top side of the window the rectangle is drawn at
 * @param err return location for a GError, or NULL
 * @return RectOSD entity or NULL if the creation failed
 */
RectOSD *rect_osd_create(Display *display, gint x, gint y, guint w, guint h, GError **err);

/**
 * Set the opacity of the RectOSD
 *
 * @param rect RectOSD entity
 * @param opacity Opacity between 0 and 100
 */
void rect_osd_set_opacity(RectOSD *rect, gint opacity);

/**
 * Return the Renderable component of a RectOSD
 *
 * @param rect RectOSD entity
 */
const Renderable *rect_osd_get_renderable(RectOSD *rect);

/**
 * Set the alignment of the RectOSD within its parent
 *
 * @param rect RectOSD entity
 * @param horizontal Horizontal alignment, can be ALIGN_LEFT, ALIGN_CENTER or ALIGN_RIGHT
 * @param vertical Vertical alignment, can be ALIGN_TOP, ALIGN_MIDDLE or ALIGN_BOTTOM
 */
void rect_osd_set_alignment(RectOSD *rect, HorizontalAlignment horizontal, VerticalAlignment vertical);

/**
 * Free a rectangle OSD entity
 *
 * @param rect RectOSD entity
 */
void rect_osd_free(RectOSD *rect);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_OSD_SDL_H__
