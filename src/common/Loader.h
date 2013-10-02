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

#ifndef VBAM_COMMON_LOADER_H_
#define VBAM_COMMON_LOADER_H_

#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Loader error domain
 */
#define LOADER_ERROR (loader_error_quark ())
GQuark loader_error_quark();

/**
 * Loader error types
 */
typedef enum
{
	G_LOADER_ERROR_FAILED,
	G_LOADER_ERROR_NOT_IN_DB
} LoaderError;

/**
 * Loader rom types
 */
typedef enum {
	ROM_BIOS,
	ROM_GBA
} RomType;

/**
 * Opaque ROM loader struct
 */
typedef struct RomLoader RomLoader;

/**
 * Initialize a new ROM loader
 *
 * @param type Rom type to load
 * @param filename File to load
 */
RomLoader *loader_new(RomType type, const gchar *filename);

/**
 * Free a ROM loader
 *
 * @param loader a loader
 */
void loader_free(RomLoader *loader);

/**
 * Load a ROM to memory
 *
 * @param loader a loader
 * @param data location to the output buffer
 * @param size input buffer size and output load data size
 * @param err return location for a GError, or NULL
 * @return TRUE if successful, FALSE otherwise
 */
gboolean loader_load(RomLoader *loader, guint8 *data, int *size, GError **err);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* VBAM_COMMON_LOADER_H_ */
