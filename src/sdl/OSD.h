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
 * @param driver The driver to use to render the text
 * @param message The message to display
 * @return TextOSD entity or NULL if the creation failed
 */
TextOSD *text_osd_create(DisplayDriver *driver, const gchar *message, GError **err);

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
 * Free a text OSD entity
 *
 * @param text TextOSD entity
 */
void text_osd_free(TextOSD *text);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_OSD_SDL_H__
