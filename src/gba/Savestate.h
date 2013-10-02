// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2005-2006 Forgotten and the VBA development team

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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef VBAM_GBA_SAVESTATE_H_
#define VBAM_GBA_SAVESTATE_H_

#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Savestate error domain
 */
#define SAVESTATE_ERROR (savestate_error_quark())
GQuark savestate_error_quark();

/**
 * Savestate error types
 */
typedef enum
{
	G_SAVESTATE_ERROR_FAILED,
	G_SAVESTATE_WRONG_GAME,
	G_SAVESTATE_UNSUPPORTED_VERSION,
	G_SAVESTATE_NOT_FOUND
} SaveStateError;

/**
 * Load a save state from file
 * @param file file name
 * @param err return location for a GError, or NULL
 * @return success
 */
gboolean savestate_load_from_file(const gchar *file, GError **err);

/**
 * Save a save state to file
 * @param file file name
 * @param err return location for a GError, or NULL
 * @return success
 */
gboolean savestate_save_to_file(const gchar *file, GError **err);

/**
 * Load a save state from a slot
 * @param num slot number
 * @param err return location for a GError, or NULL
 * @return success
 */
gboolean savestate_load_slot(gint num, GError **err);

/**
 * Save a save state to a slot
 * @param num slot number
 * @param err return location for a GError, or NULL
 * @return success
 */
gboolean savestate_save_slot(gint num, GError **err);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* VBAM_GBA_SAVESTATE_H_ */
