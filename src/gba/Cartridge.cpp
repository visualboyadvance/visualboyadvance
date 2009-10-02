#include "Globals.h"
#include "Cartridge.h"
#include "CartridgeEEprom.h"
#include "CartridgeFlash.h"
#include "CartridgeRTC.h"
#include "CartridgeSram.h"
#include "../common/Util.h"
#include "../common/Port.h"
#include "../System.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

namespace Cartridge
{

static GameInfos game;
static u8 *rom = 0;

bool loadGame(const GameInfos &_game)
{
	game = _game;
	std::string file = game.getBasePath() + game.getRomDump();
	
	int romSize = 0x2000000;

	return utilLoad(file.c_str(), utilIsGBAImage, rom, romSize);

	// What does this do ?
	/*u16 *temp = (u16 *)(rom+((romSize+1)&~1));
	int i;
	for(i = (romSize+1)&~1; i < 0x2000000; i+=2) {
		WRITE16LE(temp, (i >> 1) & 0xFFFF);
		temp++;
	}*/
}

void unloadGame()
{
	GameInfos g;
	game = g;
}

const GameInfos &getGame()
{
	return game;
}

void getGameName(u8 *romname)
{
	std::copy(&rom[0xa0], &rom[0xa0] + 16, romname);
}

bool init()
{
	rom = (u8 *)malloc(0x2000000);
	if (!rom)
	{
		systemMessage("Failed to allocate memory for %s", "ROM");
		return false;
	}

	flashInit();
	sramInit();
	eepromInit();

	return true;
}

void reset()
{
	eepromReset(game.getEEPROMSize());
	flashReset(game.getFlashSize());

	rtcReset();
	rtcEnable(game.hasRTC());
}

void uninit()
{
	if (rom)
	{
		free(rom);
		rom = 0;
	}
}

bool writeBatteryToFile(const char *fileName)
{
	if (game.hasFlash() || game.hasEEPROM() || game.hasSRAM())
	{
		FILE *file = fopen(fileName, "wb");

		if (!file)
		{
			systemMessage("Error creating file %s", fileName);
			return false;
		}

		bool res = true;

		if (game.hasFlash())
		{
			res = flashWriteBattery(file);
		}
		else if (game.hasEEPROM())
		{
			res = eepromWriteBattery(file);
		}
		else if (game.hasSRAM())
		{
			res = sramWriteBattery(file);
		}

		fclose(file);

		return res;
	}

	return true;
}

bool readBatteryFromFile(const char *fileName)
{
	FILE *file = fopen(fileName, "rb");

	if (!file)
		return false;

	// check file size to know what we should read
	fseek(file, 0, SEEK_END);

	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	bool res = true;

	if (game.hasFlash())
	{
		res = flashReadBattery(file, size);
	}
	else if (game.hasEEPROM())
	{
		res = eepromReadBattery(file, size);
	}
	else if (game.hasSRAM())
	{
		res = sramReadBattery(file, size);
	}

	fclose(file);
	return res;
}

u32 read32(const u32 address)
{
	switch (address >> 24)
	{
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		return READ32LE(((u32 *)&rom[address&0x1FFFFFC]));
		break;
	case 13:
		if (game.hasEEPROM())
			return eepromRead(address);
		break;
	case 14:
		if (game.hasSRAM())
			return sramRead(address);
		else if (game.hasFlash())
			return flashRead(address);
		break;
	default:
		break;
	}

	return 0;
}

u16 read16(const u32 address)
{
	switch (address >> 24)
	{
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		if (rtcIsEnabled() && (address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8))
			return rtcRead(address);
		else
			return READ16LE(((u16 *)&rom[address & 0x1FFFFFE]));
		break;
	case 13:
		if (game.hasEEPROM())
			return eepromRead(address);
		break;
	case 14:
		if (game.hasSRAM())
			return sramRead(address);
		else if (game.hasFlash())
			return flashRead(address);
		break;
	default:
		break;
	}

	return 0;
}

u8 read8(const u32 address)
{
	switch (address >> 24)
	{
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		return rom[address & 0x1FFFFFF];
		break;
	case 13:
		if (game.hasEEPROM())
			return eepromRead(address);
		break;
	case 14:
		if (game.hasSRAM())
			return sramRead(address);
		else if (game.hasFlash())
			return flashRead(address);

		/*if (game.hasMotionSensor())
		{
			switch (address & 0x00008f00)
			{
			case 0x8200:
				return systemGetSensorX() & 255;
			case 0x8300:
				return (systemGetSensorX() >> 8)|0x80;
			case 0x8400:
				return systemGetSensorY() & 255;
			case 0x8500:
				return systemGetSensorY() >> 8;
			}
		}*/
		break;
	default:
		break;
	}

	return 0;
}

void write32(const u32 address, const u32 value)
{
	switch (address >> 24)
	{
	case 13:
		if (game.hasEEPROM())
		{
			eepromWrite(address, value);
		}
		break;
	case 14:
		if (game.hasSRAM())
		{
			sramWrite(address, (u8)value);
		}
		else if (game.hasFlash())
		{
			flashWrite(address, (u8)value);
		}
		break;
	default:
		break;
	}
}

void write16(const u32 address, const u16 value)
{
	switch (address >> 24)
	{
	case 8:
		if (address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8)
		{
			rtcWrite(address, value);
		}
		break;
	case 13:
		if (game.hasEEPROM())
		{
			eepromWrite(address, (u8)value);
		}
		break;
	case 14:
		if (game.hasSRAM())
		{
			sramWrite(address, (u8)value);
		}
		else if (game.hasFlash())
		{
			flashWrite(address, (u8)value);
		}
		break;
	default:
		break;
	}
}

void write8(const u32 address, const u8 value)
{
	switch (address >> 24)
	{
	case 13:
		if (game.hasEEPROM())
		{
			eepromWrite(address, value);
		}
		break;
	case 14:
		if (game.hasSRAM())
		{
			sramWrite(address, value);
		}
		else if (game.hasFlash())
		{
			flashWrite(address, value);
		}
		break;
	default:
		break;
	}
}

} // namespace Cartridge
