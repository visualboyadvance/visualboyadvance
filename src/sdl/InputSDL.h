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
#include "../common/Settings.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init the joysticks needed by the keymap. Verify that the keymap is compatible
 * with the joysticks. If it's not the case, revert to the default keymap.
 */
void input_init_joysticks();

/**
 * Define which key controls an emulated joypad button
 * @param pad Emulated joypad index (there may be up to 4 joypads for the SGB)
 * @param key Emulated joypad button
 * @param code Code defining an actual joypad / keyboard button
 */
void input_set_keymap(EKey key, uint32_t code);

/**
 * Get which key is associated to which emulated joypad button
 * @param pad Emulated joypad index (there may be up to 4 joypads for the SGB)
 * @param key Emulated joypad button
 * @retunr Code defining an actual joypad / keyboard button
 */
uint32_t input_get_keymap(EKey key);

/**
 * Define which keys control motion detection emulation
 * @param key Emulated joypad button
 * @param code Code defining an actual joypad / keyboard button
 */
void input_set_motion_keymap(EKey key, uint32_t code);

/**
 * Toggle Auto fire for the specified button. Only A, B, R, L are supported.
 * @param key Emulated joypad button
 * @return Auto fire enabled
 */
gboolean input_toggle_autofire(EKey key);

/**
 * Get Auto fire status for the specified button. Only A, B, R, L are supported.
 * @param key Emulated joypad button
 * @return Auto fire enabled
 */
gboolean input_get_autofire(EKey key);

/**
 * Update the emulated pads state with a SDL event
 * @param SDL_Event An event that has just occured
 */
void input_process_SDL_event(const SDL_Event *event);

/**
 * Get the keymap code corresponding to a SDL event
 * @param SDL_Event An event that has just occured
 * @return Keymap code
 */
uint32_t input_get_event_code(const SDL_Event *event);

/**
 * Read the state of an emulated joypad
 * @param which Emulated joypad index
 * @return Joypad state
 */
uint32_t input_read_joypad();

/**
 * Compute the motion sensor X and Y values
 */
void input_update_motion_sensor();

/**
 * Get the motion sensor X value
 * @return motion sensor X value
 */
int input_get_sensor_x();

/**
 * Get the motion sensor Y value
 * @return motion sensor Y value
 */
int input_get_sensor_y();

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // VBAM_SDL_INPUT_H
