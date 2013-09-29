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

#ifndef __VBA_DISPLAY_SDL_H__
#define __VBA_DISPLAY_SDL_H__

#include "../common/DisplayDriver.h"
#include <SDL.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize an SDL window, and returns the associated display driver
 *
 * @param err return location for a GError, or NULL
 * @return SDL display driver or NULL if case of error
 */
DisplayDriver *display_sdl_init(GError **err);

/**
 * Free a SDL display driver. If driver is NULL, it simply returns.
 *
 * @param driver SDL display driver to be freed
 */
void display_sdl_free(DisplayDriver *driver);

/**
 * Current fullscreen state accessor
 *
 * @param driver display driver
 * @return whether display is fullscreen
 */
gboolean display_sdl_is_fullscreen(DisplayDriver *driver);

/**
 * Toggle between windowed and fullscreen mode
 *
 * @param driver display driver
 * @param err return location for a GError, or NULL
 * @return FALSE in case of error
 */
gboolean display_sdl_toggle_fullscreen(DisplayDriver *driver, GError **err);

/**
 * Display an on screen text message
 *
 * @param driver display driver
 * @param msg message to be displayed
 */
void display_sdl_show_screen_message(DisplayDriver *driver, const gchar *msg);

/**
 * Display an on screen text message
 *
 * @param driver display driver
 * @param title title to be set
 */
void display_sdl_set_window_title(DisplayDriver *driver, const gchar *title);

/**
 * Component allowing an entity to be rendered on the screen
 */
typedef struct Renderable Renderable;
struct Renderable {
	/** Render function for the entity */
	void (*render)(gpointer entity);

	/** Pointer to the parent entity */
	gpointer entity;

	/** Display driver to be used for rendering */
	DisplayDriver *driver;

	/** Renderer to use for rendering */
	SDL_Renderer *renderer;
};

/**
 * Create a new renderable component and add it to an entity
 *
 * The entity is rendered at each frame until the renderable component is freed
 *
 * @param driver Display driver to use for rendering
 * @param entity Entity this component applies to
 */
Renderable *display_sdl_renderable_create(DisplayDriver *driver, gpointer entity);

/**
 * Free a renderable component
 *
 * This will stop the associated entity from rendering
 *
 * @param renderable teh renderable component to free
 */
void display_sdl_renderable_free(Renderable *renderable);

/**
 * Loads a given PNG file pointed to by filename into an SDL_Texture.
 *
 * Call SDL_DestroyTexture on the result to release.
 *
 * @param driver The driver the texture belongs to
 * @param filename Filename of the PNG image
 * @param err return location for a GError, or NULL
 * @return a pointer to the texture on success, NULL on failure.
 */
SDL_Texture *display_sdl_load_png(DisplayDriver *driver, const gchar *filename, GError **err);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_DISPLAY_SDL_H__
