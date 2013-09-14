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

#ifndef VBAM_GBA_CARTRIDGE_H_
#define VBAM_GBA_CARTRIDGE_H_

#include <glib.h>
#include "../common/Types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

gboolean cartridge_init();
void cartridge_reset();
void cartridge_free();
gboolean cartridge_load_rom(const gchar *filename, GError **err);
void cartridge_unload();
void cartridge_get_game_name(u8 *romname);
const gchar *cartridge_get_game_title();
gboolean cartridge_is_present();

gboolean cartridge_read_battery(GError **err);
gboolean cartridge_write_battery(GError **err);

u32 cartridge_read32(const u32 address);
u16 cartridge_read16(const u32 address);
u8 cartridge_read8(const u32 address);
void cartridge_write32(const u32 address, const u32 value);
void cartridge_write16(const u32 address, const u16 value);
void cartridge_write8(const u32 address, const u8 value);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // VBAM_GBA_CARTRIDGE_H_
