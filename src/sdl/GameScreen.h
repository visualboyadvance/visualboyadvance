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

#ifndef __VBA_GAMESCREEN_SDL_H__
#define __VBA_GAMESCREEN_SDL_H__

#include "../common/DisplayDriver.h"
#include "DisplaySDL.h"
#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque GameScreen entity
 */
typedef struct GameScreen GameScreen;

/** Game Screen type, identifies the game screen in the screens stack */
#define PAUSE_SCREEN (pausescreen_quark())
GQuark pausescreen_quark();

/**
 * Create a game screen entity
 *
 * @param display Display to use for rendering
 * @param err return location for a GError, or NULL
 * @return GameScreen object, to be freed when removed from the screens stack
 */
GameScreen *gamescreen_create(Display *display, GError **err);

/**
 * Display an on screen text message
 *
 * @param game Game screen
 * @param msg message to be displayed
 */
void gamescreen_show_status_message(GameScreen *game, const gchar *msg);

/**
 * Create a display driver to update the screen
 *
 * @param game Game screen
 */
const DisplayDriver *gamescreen_get_display_driver(GameScreen *game);

/**
 * Write the battery and display a status message
 *
 * @param game Game screen
 */
void gamescreen_write_battery(GameScreen *game);

/**
 * Load the battery and display a status message
 *
 * @param game Game screen
 */
void gamescreen_read_battery(GameScreen *game);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_GAMESCREEN_SDL_H__
