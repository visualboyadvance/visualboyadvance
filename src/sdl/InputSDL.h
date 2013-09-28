// VBA-M, A Nintendo Handheld Console Emulator
// Copyright (C) 2008 VBA-M development team
//
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

#ifndef VBAM_SDL_INPUT_H
#define VBAM_SDL_INPUT_H

#include <SDL.h>
#include <glib.h>
#include "../common/InputDriver.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init the joysticks needed by the keymap. Verify that the keymap is compatible
 * with the joysticks. If it's not the case, revert to the default keymap.
 *
 * @param err return location for a GError, or NULL
 * @return SDL input driver or NULL if case of error
 */
InputDriver *input_sdl_init(GError **err);

/**
 * Free a SDL input driver. If driver is NULL, it simply returns.
 *
 * @param driver SDL input driver to be freed
 */
void input_sdl_free(InputDriver *driver);

/**
 * Define which key controls an emulated joypad button
 * @param pad Emulated joypad index (there may be up to 4 joypads for the SGB)
 * @param key Emulated joypad button
 * @param code Code defining an actual joypad / keyboard button
 */
void input_sdl_set_keymap(EKey key, uint32_t code);

/**
 * Get which key is associated to which emulated joypad button
 * @param pad Emulated joypad index (there may be up to 4 joypads for the SGB)
 * @param key Emulated joypad button
 * @retunr Code defining an actual joypad / keyboard button
 */
uint32_t input_sdl_get_keymap(EKey key);

/**
 * Define which keys control motion detection emulation
 * @param key Emulated joypad button
 * @param code Code defining an actual joypad / keyboard button
 */
void input_sdl_set_motion_keymap(EKey key, uint32_t code);

/**
 * Update the emulated pads state with a SDL event
 * @param SDL_Event An event that has just occured
 */
void input_sdl_process_SDL_event(const SDL_Event *event);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // VBAM_SDL_INPUT_H
