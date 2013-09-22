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

#ifndef __VBA_DISPLAY_DRIVER_H__
#define __VBA_DISPLAY_DRIVER_H__

#include "glib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sound driver abstract interface for the core to use to output sound.
 */
typedef struct DisplayDriver DisplayDriver;
struct DisplayDriver {

	/**
	 * Tell the driver the screen needs to be updated with new data
	 */
	void (*drawScreen)(DisplayDriver *driver, guint32 *pix);

	/**
	 * Opaque driver specific data
	 */
	gpointer driverData;
};

/**
 * Sound error domain
 */
#define DISPLAY_ERROR (display_error_quark ())
GQuark display_error_quark();

/**
 * Loader error types
 */
typedef enum
{
	G_DISPLAY_ERROR_FAILED
} DisplayError;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_DISPLAY_DRIVER_H__
