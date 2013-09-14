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

#ifndef VBAM_GBA_FLASH_H_
#define VBAM_GBA_FLASH_H_

#include <glib.h>
#include <stdio.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

guint8 cartridge_flash_read(guint32 address);
void cartridge_flash_write(guint32 address, guint8 byte);
void cartridge_flash_reset(int size);
void cartridge_flash_init();
gboolean cartridge_flash_read_battery(FILE *file, size_t size);
gboolean cartridge_flash_write_battery(FILE *file);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* VBAM_GBA_FLASH_H_ */
