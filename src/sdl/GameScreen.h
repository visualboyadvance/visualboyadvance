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
#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque GameScreen entity
 */
typedef struct GameScreen GameScreen;

/**
 * Create a game screen entity
 *
 * @param driver display driver to use for rendering
 */
GameScreen *gamescreen_create(DisplayDriver *driver);

/**
 * Free a game screen
 *
 * @param screen Game screen to free
 */
void gamescreen_free(GameScreen *screen);

/**
 * Update a game screen with pixel data
 *
 * @param screen Game screen to update
 * @param pix 240x160px RGB555 pixel data
 */
void gamescreen_update(GameScreen *screen, guint16 *pix);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_GAMESCREEN_SDL_H__
