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

#ifndef __VBA_GUI_SDL_H__
#define __VBA_GUI_SDL_H__

#include <glib.h>
#include "DisplaySDL.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Function to be run when a GUI element is actionned */
typedef void (*ActionableAction)(gpointer);

/**
 * Check whether one of the actionables can handle the event, and have it handle it
 *
 * @param event An event that has just occured
 * @return TRUE when the event has been handled
 */
gboolean actionables_handle_event(const SDL_Event *event);

/** Button GUI element */
typedef struct Button Button;

/**
 * Create a new Button entity
 *
 * @param display The display to use to render the rectangle
 * @param caption The caption to display
 * @param x number of pixels away from the left side of the window the rectangle is drawn at
 * @param y number of pixels away from the top side of the window the rectangle is drawn at
 * @param w Maximum width of the rectangle
 * @param h Maximum height of the rectangle
 * @param action Function to be called when the button is triggered
 * @param err return location for a GError, or NULL
 * @return Button entity or NULL if the creation failed
 */
Button *button_create(Display *display, const gchar *caption, gint x, gint y, guint w, guint h, ActionableAction action, GError **err);

/**
 * Free a button entity
 *
 * @param button Button entity
 */
void button_free(Button *button);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_GUI_SDL_H__
