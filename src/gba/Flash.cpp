#include <cstdio>
#include <algorithm>

#include "GBA.h"
#include "Globals.h"
#include "Flash.h"

namespace Cartridge
{

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

static u8 flashSaveMemory[0x20000];
static int flashState = FLASH_READ_ARRAY;
static int flashReadState = FLASH_READ_ARRAY;
static size_t flashSize = 0x10000;
static int flashDeviceID = 0x1b;
static int flashManufacturerID = 0x32;
static int flashBank = 0;

static void flashSetSize(int size)
{
	if(size == 0x10000)
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

void flashInit()
{
	std::fill(flashSaveMemory, flashSaveMemory + 0x20000, 0xff);
}

void flashReset(int size)
{
	flashState = FLASH_READ_ARRAY;
	flashReadState = FLASH_READ_ARRAY;
	flashBank = 0;
	flashSetSize(size);
}

u8 flashRead(u32 address)
{
	address &= 0xFFFF;

	switch(flashReadState)
	{
		case FLASH_READ_ARRAY:
			return flashSaveMemory[(flashBank << 16) + address];
		case FLASH_AUTOSELECT:
			switch(address & 0xFF)
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

void flashWrite(u32 address, u8 byte)
{
	//  log("Writing %02x at %08x\n", byte, address);
	//  log("Current state is %d\n", flashState);
	address &= 0xFFFF;
	switch(flashState)
	{
		case FLASH_READ_ARRAY:
			if(address == 0x5555 && byte == 0xAA)
			flashState = FLASH_CMD_1;
			break;
		case FLASH_CMD_1:
			if(address == 0x2AAA && byte == 0x55)
				flashState = FLASH_CMD_2;
			else
				flashState = FLASH_READ_ARRAY;
			break;
		case FLASH_CMD_2:
			if(address == 0x5555)
			{
				if(byte == 0x90)
				{
					flashState = FLASH_AUTOSELECT;
					flashReadState = FLASH_AUTOSELECT;
				}
				else if(byte == 0x80)
				{
					flashState = FLASH_CMD_3;
				}
				else if(byte == 0xF0)
				{
					flashState = FLASH_READ_ARRAY;
					flashReadState = FLASH_READ_ARRAY;
				}
				else if(byte == 0xA0)
				{
					flashState = FLASH_PROGRAM;
				}
				else if(byte == 0xB0 && flashSize == 0x20000)
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
			if(address == 0x5555 && byte == 0xAA)
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
			if(address == 0x2AAA && byte == 0x55)
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
			if(byte == 0x30)
			{
				// SECTOR ERASE
				u8 *offset = flashSaveMemory + (flashBank << 16) + (address & 0xF000);
				std::fill(offset, offset + 0x1000, 0);
				flashReadState = FLASH_ERASE_COMPLETE;
			}
			else if(byte == 0x10)
			{
				// CHIP ERASE
				std::fill(flashSaveMemory, flashSaveMemory + flashSize, 0);
				flashReadState = FLASH_ERASE_COMPLETE;
			}
			else
			{
				flashState = FLASH_READ_ARRAY;
				flashReadState = FLASH_READ_ARRAY;
			}
			break;
		case FLASH_AUTOSELECT:
			if(byte == 0xF0)
			{
				flashState = FLASH_READ_ARRAY;
				flashReadState = FLASH_READ_ARRAY;
			}
			else if(address == 0x5555 && byte == 0xAA)
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
			if(address == 0)
			{
				flashBank = (byte & 1);
			}
			flashState = FLASH_READ_ARRAY;
			flashReadState = FLASH_READ_ARRAY;
			break;
	}
}

bool flashReadBattery(FILE *file, size_t size)
{
	if (size != flashSize)
		return false;

	return fread(flashSaveMemory, 1, flashSize, file) == flashSize;
}

bool flashWriteBattery(FILE *file)
{
	return fwrite(flashSaveMemory, 1, flashSize, file) == (size_t)flashSize;
}

} // namespace Cartridge
