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

#ifndef __VBA_INPUT_DRIVER_H__
#define __VBA_INPUT_DRIVER_H__

#include "glib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Input driver abstract interface for the core to use to get input.
 */
typedef struct InputDriver InputDriver;
struct InputDriver {

	/**
	 * Read the state of an emulated joypad
	 * @return Joypad state
	 */
	guint32 (*read_joypad)(InputDriver *driver);

	/**
	 * Compute the motion sensor X and Y values
	 */
	void (*update_motion_sensor)(InputDriver *driver);

	/**
	 * Get the motion sensor X value
	 * @return motion sensor X value
	 */
	int (*read_sensor_x)(InputDriver *driver);

	/**
	 * Get the motion sensor Y value
	 * @return motion sensor Y value
	 */
	int (*read_sensor_y)(InputDriver *driver);

	/**
	 * Opaque driver specific data
	 */
	gpointer driverData;
};

/**
 * Emulated buttons
 */
typedef enum {
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_BUTTON_A,
	KEY_BUTTON_B,
	KEY_BUTTON_START,
	KEY_BUTTON_SELECT,
	KEY_BUTTON_L,
	KEY_BUTTON_R
} EKey;

static EKey settings_buttons[] = {
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_BUTTON_A,
	KEY_BUTTON_B,
	KEY_BUTTON_START,
	KEY_BUTTON_SELECT,
	KEY_BUTTON_L,
	KEY_BUTTON_R
};


/**
 * Input error domain
 */
#define INPUT_ERROR (input_error_quark ())
GQuark input_error_quark();

/**
 * Input error types
 */
typedef enum
{
	G_INPUT_ERROR_FAILED
} InputError;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_INPUT_DRIVER_H__
