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

#ifndef __VBA_SOUND_DRIVER_H__
#define __VBA_SOUND_DRIVER_H__

#include "glib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sound driver abstract interface for the core to use to output sound.
 */
typedef struct SoundDriver SoundDriver;
struct SoundDriver {

	/**
	 * Tell the driver that the sound stream has paused or resumed
	 */
	void (*pause)(SoundDriver *driver, gboolean pause);

	/**
	 * Reset the sound driver
	 */
	void (*reset)(SoundDriver *driver);

	/**
	 * Write length bytes of data from the finalWave buffer to the driver output buffer.
	 */
	void (*write)(SoundDriver *driver, guint16 *finalWave, int length);

	/**
	 * Opaque driver specific data
	 */
	gpointer driverData;
};

/**
 * Sound error domain
 */
#define SOUND_ERROR (sound_error_quark ())
GQuark sound_error_quark();

/**
 * Loader error types
 */
typedef enum
{
	G_SOUND_ERROR_FAILED
} SoundError;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_SOUND_DRIVER_H__
