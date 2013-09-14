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

#include "CartridgeFlash.h"

#include <string.h>

#define FLASH_READ_ARRAY         0
#define FLASH_CMD_1              1
#define FLASH_CMD_2              2
#define FLASH_AUTOSELECT         3
#define FLASH_CMD_3              4
#define FLASH_CMD_4              5
#define FLASH_CMD_5              6
#define FLASH_ERASE_COMPLETE     7
#define FLASH_PROGRAM            8
#define FLASH_SETBANK            9

static guint8 flashSaveMemory[0x20000];
static int flashState = FLASH_READ_ARRAY;
static int flashReadState = FLASH_READ_ARRAY;
static size_t flashSize = 0x10000;
static int flashDeviceID = 0x1b;
static int flashManufacturerID = 0x32;
static int flashBank = 0;

static void flashSetSize(int size)
{
	if (size == 0x10000)
	{
		flashDeviceID = 0x1b;
		flashManufacturerID = 0x32;
	}
	else
	{
		flashDeviceID = 0x13; //0x09;
		flashManufacturerID = 0x62; //0xc2;
	}

	flashSize = size;
}

void cartridge_flash_init()
{
	memset(flashSaveMemory, 0xFF, 0x20000);
}

void cartridge_flash_reset(int size)
{
	flashState = FLASH_READ_ARRAY;
	flashReadState = FLASH_READ_ARRAY;
	flashBank = 0;
	flashSetSize(size);
}

guint8 cartridge_flash_read(guint32 address)
{
	address &= 0xFFFF;

	switch (flashReadState)
	{
	case FLASH_READ_ARRAY:
		return flashSaveMemory[(flashBank << 16) + address];
	case FLASH_AUTOSELECT:
		switch (address & 0xFF)
		{
		case 0:
			// manufacturer ID
			return flashManufacturerID;
		case 1:
			// device ID
			return flashDeviceID;
		}
		break;
	case FLASH_ERASE_COMPLETE:
		flashState = FLASH_READ_ARRAY;
		flashReadState = FLASH_READ_ARRAY;
		return 0xFF;
	};
	return 0;
}

void cartridge_flash_write(guint32 address, guint8 byte)
{
	//  log("Writing %02x at %08x\n", byte, address);
	//  log("Current state is %d\n", flashState);
	address &= 0xFFFF;
	switch (flashState)
	{
	case FLASH_READ_ARRAY:
		if (address == 0x5555 && byte == 0xAA)
			flashState = FLASH_CMD_1;
		break;
	case FLASH_CMD_1:
		if (address == 0x2AAA && byte == 0x55)
			flashState = FLASH_CMD_2;
		else
			flashState = FLASH_READ_ARRAY;
		break;
	case FLASH_CMD_2:
		if (address == 0x5555)
		{
			if (byte == 0x90)
			{
				flashState = FLASH_AUTOSELECT;
				flashReadState = FLASH_AUTOSELECT;
			}
			else if (byte == 0x80)
			{
				flashState = FLASH_CMD_3;
			}
			else if (byte == 0xF0)
			{
				flashState = FLASH_READ_ARRAY;
				flashReadState = FLASH_READ_ARRAY;
			}
			else if (byte == 0xA0)
			{
				flashState = FLASH_PROGRAM;
			}
			else if (byte == 0xB0 && flashSize == 0x20000)
			{
				flashState = FLASH_SETBANK;
			}
			else
			{
				flashState = FLASH_READ_ARRAY;
				flashReadState = FLASH_READ_ARRAY;
			}
		}
		else
		{
			flashState = FLASH_READ_ARRAY;
			flashReadState = FLASH_READ_ARRAY;
		}
		break;
	case FLASH_CMD_3:
		if (address == 0x5555 && byte == 0xAA)
		{
			flashState = FLASH_CMD_4;
		}
		else
		{
			flashState = FLASH_READ_ARRAY;
			flashReadState = FLASH_READ_ARRAY;
		}
		break;
	case FLASH_CMD_4:
		if (address == 0x2AAA && byte == 0x55)
		{
			flashState = FLASH_CMD_5;
		}
		else
		{
			flashState = FLASH_READ_ARRAY;
			flashReadState = FLASH_READ_ARRAY;
		}
		break;
	case FLASH_CMD_5:
		if (byte == 0x30)
		{
			// SECTOR ERASE
			guint8 *offset = flashSaveMemory + (flashBank << 16) + (address & 0xF000);
			memset(offset, 0, 0x1000);
			flashReadState = FLASH_ERASE_COMPLETE;
		}
		else if (byte == 0x10)
		{
			// CHIP ERASE
			memset(flashSaveMemory, 0, flashSize);
			flashReadState = FLASH_ERASE_COMPLETE;
		}
		else
		{
			flashState = FLASH_READ_ARRAY;
			flashReadState = FLASH_READ_ARRAY;
		}
		break;
	case FLASH_AUTOSELECT:
		if (byte == 0xF0)
		{
			flashState = FLASH_READ_ARRAY;
			flashReadState = FLASH_READ_ARRAY;
		}
		else if (address == 0x5555 && byte == 0xAA)
		{
			flashState = FLASH_CMD_1;
		}
		else
		{
			flashState = FLASH_READ_ARRAY;
			flashReadState = FLASH_READ_ARRAY;
		}
		break;
	case FLASH_PROGRAM:
		flashSaveMemory[(flashBank<<16)+address] = byte;
		flashState = FLASH_READ_ARRAY;
		flashReadState = FLASH_READ_ARRAY;
		break;
	case FLASH_SETBANK:
		if (address == 0)
		{
			flashBank = (byte & 1);
		}
		flashState = FLASH_READ_ARRAY;
		flashReadState = FLASH_READ_ARRAY;
		break;
	}
}

gboolean cartridge_flash_read_battery(FILE *file, size_t size)
{
	if (size != flashSize)
		return FALSE;

	return fread(flashSaveMemory, 1, flashSize, file) == flashSize;
}

gboolean cartridge_flash_write_battery(FILE *file)
{
	return fwrite(flashSaveMemory, 1, flashSize, file) == (size_t)flashSize;
}

