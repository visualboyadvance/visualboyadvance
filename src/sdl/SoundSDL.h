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

#ifndef __VBA_SOUND_SDL_H__
#define __VBA_SOUND_SDL_H__

#include "../common/SoundDriver.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize an SDL audio output, and returns the associated sound driver
 *
 * @param err return location for a GError, or NULL
 * @return SDL sound driver or NULL if case of error
 */
SoundDriver *sound_sdl_init(GError **err);

/**
 * Free a SDL sound driver. If driver is NULL, it simply returns.
 *
 * @param driver SDL sound driver to be freed
 */
void sound_sdl_free(SoundDriver *driver);

/**
 * Enable or disable sound synchronization
 *
 * Disabling sound synch makes the emulation speed unlimited
 *
 * @param enable value to set
 * @param driver SDL sound driver
 */
void sound_sdl_enable_sync(SoundDriver *driver, gboolean enable);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_SOUND_SDL_H__
