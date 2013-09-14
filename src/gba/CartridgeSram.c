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

#include "CartridgeSram.h"
#include <string.h>

static const size_t sramSize = 0x10000;
static guint8 sramData[sramSize];

void cartridge_sram_init()
{
	memset(sramData, 0xFF, sramSize);
}

guint8 cartridge_sram_read(guint32 address)
{
	return sramData[address & 0xFFFF];
}

void cartridge_sram_write(guint32 address, guint8 byte)
{
	sramData[address & 0xFFFF] = byte;
}

gboolean cartridge_sram_read_battery(FILE *file, size_t size)
{
	return fread(sramData, 1, size, file) == size;
}

gboolean cartridge_sram_write_battery(FILE *file)
{
	return fwrite(sramData, 1, sramSize, file) == sramSize;
}
