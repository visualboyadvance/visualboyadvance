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

#include <SDL.h>
#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Display Display;

/**
 * Initialize an SDL window, and returns the associated display display
 *
 * @param err return location for a GError, or NULL
 * @return SDL display display or NULL if case of error
 */
Display *display_sdl_init(GError **err);

/**
 * Free a SDL display display. If display is NULL, it simply returns.
 *
 * @param display SDL display display to be freed
 */
void display_sdl_free(Display *display);

/**
 * Current fullscreen state accessor
 *
 * @param display display display
 * @return whether display is fullscreen
 */
gboolean display_sdl_is_fullscreen(Display *display);

/**
 * Toggle between windowed and fullscreen mode
 *
 * @param display display display
 * @param err return location for a GError, or NULL
 * @return FALSE in case of error
 */
gboolean display_sdl_toggle_fullscreen(Display *display, GError **err);

/**
 * Display an on screen text message
 *
 * @param display display display
 * @param title title to be set
 */
void display_sdl_set_window_title(Display *display, const gchar *title);

/** Available horizontal alignment options */
typedef enum {
	ALIGN_LEFT = 1,
	ALIGN_CENTER,
	ALIGN_RIGHT
} HorizontalAlignment;

/** Available vertical alignment options */
typedef enum {
	ALIGN_TOP = 1,
	ALIGN_MIDDLE,
	ALIGN_BOTTOM
} VerticalAlignment;

/**
 * Component allowing an entity to be rendered on the screen
 */
typedef struct Renderable Renderable;
struct Renderable {
	/** Render function for the entity */
	void (*render)(gpointer entity);

	/** Resize event handler */
	void (*resize)(gpointer entity);

	/** Pointer to the parent entity */
	gpointer entity;

	/** Display display to be used for rendering */
	Display *display;

	Renderable *parent;

	/** Screen position x */
	gint x;

	/** Screen position y */
	gint y;

	/** Width available for the renderable */
	gint width;

	/** Height available for the renderable */
	gint height;

	/** Horizontal alignment within the parent */
	HorizontalAlignment horizontalAlignment;

	/** Vertical alignment within the parent */
	VerticalAlignment verticalAlignment;

	/** Renderer to use for rendering */
	SDL_Renderer *renderer;
};

/**
 * Create a new renderable component and add it to an entity
 *
 * The entity is rendered at each frame until the renderable component is freed
 *
 * @param display Display display to use for rendering
 * @param entity Entity this component applies to
 * @param parent Renderable the renderable to be created renders over
 */
Renderable *display_sdl_renderable_create(Display *display, gpointer entity, Renderable *parent);

/**
 * Set the position of the Renderable on the screen
 *
 * If the x or y parameters are negative the positioning is relative to the
 * opposite side of the parent.
 *
 * @param renderable Renderable component
 * @param x number of pixels away from the left side of the parent the renderable is drawn at
 * @param y number of pixels away from the top side of the parent the renderable is drawn at
 */
void display_sdl_renderable_set_position(Renderable *renderable, gint x, gint y);

/**
 * Set the size of the Renderable
 *
 * @param renderable Renderable component
 * @param width Width of the renderable
 * @param height Height of the renderable
 */
void display_sdl_renderable_set_size(Renderable *renderable, gint width, gint height);

/**
 * Compute the absolute scaled position of a Renderable
 *
 * @param renderable Renderable component
 * @param x Returns the number of pixels to the top of the window
 * @param y Returns the number of pixels to the left of the window
 */
void display_sdl_renderable_get_absolute_position(Renderable *renderable, gint *x, gint *y);

/**
 * Set the alignment of a Renderable within its parent
 *
 * @param renderable Renderable component
 * @param horizontal Horizontal alignment, can be ALIGN_LEFT, ALIGN_CENTER or ALIGN_RIGHT
 * @param vertical Vertical alignment, can be ALIGN_TOP, ALIGN_MIDDLE or ALIGN_BOTTOM
 */
void display_sdl_renderable_set_alignment(Renderable *renderable, HorizontalAlignment horizontal, VerticalAlignment vertical);

/**
 * Free a renderable component
 *
 * This will stop the associated entity from rendering
 *
 * @param renderable teh renderable component to free
 */
void display_sdl_renderable_free(Renderable *renderable);

/**
 * Render all the renderables
 *
 * @param display Display to render
 */
void display_sdl_render(Display *display);

/**
 * Warn the renderables that the viewport has been resized
 *
 * @param display Current Display
 */
void display_sdl_resize(Display *display);

/**
 * Loads a given PNG file pointed to by filename into an SDL_Texture.
 *
 * Call SDL_DestroyTexture on the result to release.
 *
 * @param display The display the texture belongs to
 * @param filename Filename of the PNG image
 * @param err return location for a GError, or NULL
 * @return a pointer to the texture on success, NULL on failure.
 */
SDL_Texture *display_sdl_load_png(Display *display, const gchar *filename, GError **err);

/**
 * Compute the scaled value of an emulated screen dimension
 *
 * The emulated screen width is 240x160. To keep things simple the GUI layouts
 * are defined for that resolution. However the final display can be bigger.
 * This function computes the real scaled value of an emulated screen dimension.
 *
 */
gint display_sdl_scale(Display *display, gint unscaled);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_DISPLAY_SDL_H__
