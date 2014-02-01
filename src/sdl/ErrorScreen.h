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

#ifndef __VBA_ERRORSCREEN_SDL_H__
#define __VBA_ERRORSCREEN_SDL_H__

#include "DisplaySDL.h"
#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque ErrorScreen entity
 */
typedef struct ErrorScreen ErrorScreen;

/**
 * Create an error screen entity
 *
 * @param display Display to use for rendering
 * @param message Error message to display
 * @param err return location for a GError, or NULL
 * @return ErrorScreen object, to be freed when removed from the screens stack
 */
ErrorScreen *errorscreen_create(Display *display, const gchar *message, GError **err);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_ERRORSCREEN_SDL_H__
