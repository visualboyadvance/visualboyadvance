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
 * @param parent Renderable the Button to be created renders over
 * @param err return location for a GError, or NULL
 * @return Button entity or NULL if the creation failed
 */
Button *button_create(Display *display, const gchar *caption, gint x, gint y, guint w, guint h, ActionableAction action, const Renderable *parent, GError **err);

/**
 * Free a button entity
 *
 * @param button Button entity
 */
void button_free(Button *button);


/**
 * Component allowing to handle screens in a generic way.
 *
 * All instanciated screens are stored in a stack.
 */
typedef struct Screen Screen;
struct Screen {
	/** Process an SDL event to update the screen */
	gboolean (*process_event)(gpointer entity, const SDL_Event *event);

	/** Main loop element allowing the screen to update */
	void (*update)(gpointer entity);

	/** Free the owning entity */
	void (*free)(gpointer entity);

	/** Type allowing to distinguish screens in the stack */
	GQuark type;

	/** Pointer to the parent entity */
	gpointer entity;
};

/**
 * Create a Screen component for an entity and add on top of the stack
 *
 * @param entity Owning entity
 * @param type Screen type
 * @return The newly instanciated Screen
 */
Screen *screen_create(gpointer entity, GQuark type);

/**
 * Free a Screen component and remove it from the stack
 *
 * @param screen The Screen to free
 */
void screen_free(Screen *screen);

/**
 * Check whether the current Screen is of the specified type
 *
 * @param type The type to test the current screen against
 */
gboolean screens_current_is(GQuark type);

/**
 * Update the current Screen.
 *
 * Called from the main loop
 */
void screens_update_current();

/**
 * Have the current Screen process an event
 *
 * @param event An event that has just occured
 */
void screens_process_event_current(const SDL_Event *event);

/**
 * Remove the current Screen from the stack, making the next one active
 *
 * Also frees the associated entity
 */
void screens_pop();

/**
 * Remove all the Screens from the stack
 *
 * Also frees the associated entities
 */
void screens_free_all();

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_GUI_SDL_H__
