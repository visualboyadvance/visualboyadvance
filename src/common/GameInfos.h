// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 2009 VBA-M development team

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

#ifndef __VBA_GAMEINFOS_H__
#define __VBA_GAMEINFOS_H__

#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	gboolean hasSRAM;
	gboolean hasEEPROM;
	gboolean hasFlash;
	gboolean hasRTC;
	int EEPROMSize;
	int flashSize;

	gchar *title;
	gchar *code;
} GameInfos;

/**
 * Create a new empty cartridge infos object initialized with default values
 */
GameInfos *game_infos_new();

/**
 * Free cartridge infos
 *
 * @param game GameInfos to free
 */
void game_infos_free(GameInfos *game);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_GAMEINFOS_H__
