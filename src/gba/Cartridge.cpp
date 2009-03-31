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

Features features = {SaveNone, 0x2000, 0x10000, false, false};
static u8 *rom = 0;

static bool areEqual(const GameSerial &x, const GameSerial &y)
{
	return (x[0] == y[0]) && (x[1] == y[1]) && (x[2] == y[2]) && (x[3] == y[3]);
}

static void gameSerialFromString(const std::string &str, GameSerial &gs)
{
	gs[0] = str[0];
	gs[1] = str[1];
	gs[2] = str[2];
	gs[3] = str[3];
}

static void getGameSerial(GameSerial &gs)
{
	gs[0] = rom[0xac];
	gs[1] = rom[0xad];
	gs[2] = rom[0xae];
	gs[3] = rom[0xaf];
}

void getGameName(u8 *romname)
{
	std::copy(&rom[0xa0], &rom[0xa0] + 16, romname);
}

static void clearFeatures(Features &features)
{
	features.saveType = SaveNone;
	features.eepromSize = 0x2000;
	features.flashSize = 0x10000;
	features.hasRTC = false;
	features.hasMotionSensor = false;
}

static void processFeatureToken(Features &features, const std::string &token)
{
	if (token == "FLASH_V120" || token == "FLASH_V121" ||
	        token == "FLASH_V123" || token == "FLASH_V124" ||
	        token == "FLASH_V125" || token == "FLASH_V126" ||
	        token == "FLASH512_V130" || token == "FLASH512_V131" ||
	        token == "FLASH512_V133")
	{
		features.saveType = SaveFlash;
		features.flashSize = 0x10000;
	}
	else if (token == "FLASH1M_V102" || token == "FLASH1M_V103")
	{
		features.saveType = SaveFlash;
		features.flashSize = 0x20000;
	}
	else if (token == "EEPROM_V111" || token == "EEPROM_V120" ||
	         token == "EEPROM_V121" || token == "EEPROM_V122" ||
	         token == "EEPROM_V124" || token == "EEPROM_V125" ||
	         token == "EEPROM_V126")
	{
		features.saveType = SaveEEPROM;
	}
	else if (token == "SRAM_V110" || token == "SRAM_V111" ||
	         token == "SRAM_V112" || token == "SRAM_V113" ||
	         token == "SRAM_F_V100" || token == "SRAM_F_V102" ||
	         token == "SRAM_F_V103")
	{
		features.saveType = SaveSRAM;
	}
	else if (token == "EEPROM_4K")
	{
		features.eepromSize = 0x0200;
	}
	else if (token == "EEPROM_64K")
	{
		features.eepromSize = 0x2000;
	}
	else if (token == "SIIRTC_V001")
	{
		features.hasRTC = true;
	}
}

static void findFeatures(Features &features, const GameSerial &gs)
{
	clearFeatures(features);

	std::ifstream file(PKGDATADIR "/GamesDB.txt");
	if (file.is_open())
	{
		while (!file.eof())
		{
			std::string line;
			GameSerial lineGS;

			std::getline(file, line);

			if (line.length() < 4) continue;

			gameSerialFromString(line, lineGS);

			if (areEqual(lineGS, gs))
			{
				std::string token;
				std::istringstream iss(line);

				while (iss >> token)
				{
					processFeatureToken(features, token);
				}
				break;
			}
		}
		file.close();
	}
	else
	{
		systemMessage("Error reading the game database.");
	}
}

bool loadDump(const char *file)
{
	int romSize = 0x2000000;

	return utilLoad(file, utilIsGBAImage, rom, romSize);

	// What does this do ?
	/*u16 *temp = (u16 *)(rom+((romSize+1)&~1));
	int i;
	for(i = (romSize+1)&~1; i < 0x2000000; i+=2) {
		WRITE16LE(temp, (i >> 1) & 0xFFFF);
		temp++;
	}*/
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
	Features f;
	GameSerial gs;

	getGameSerial(gs);
	findFeatures(f, gs);

	eepromReset(f.eepromSize);
	flashReset(f.flashSize);

	rtcReset();
	rtcEnable(f.hasRTC);

	features = f;
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
	if (features.saveType != SaveNone)
	{
		FILE *file = fopen(fileName, "wb");

		if (!file)
		{
			systemMessage("Error creating file %s", fileName);
			return false;
		}

		bool res = true;

		switch (features.saveType)
		{
		case SaveFlash:
			res = flashWriteBattery(file);
			break;
		case SaveEEPROM:
			res = eepromWriteBattery(file);
			break;
		case SaveSRAM:
			res = sramWriteBattery(file);
			break;
		case SaveNone:
			break;
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

	switch (features.saveType)
	{
	case SaveSRAM:
		res = sramReadBattery(file, size);
		break;
	case SaveFlash:
		res = flashReadBattery(file, size);
		break;
	case SaveEEPROM:
		res = eepromReadBattery(file, size);
		break;
	case SaveNone:
		break;
	}

	fclose(file);
	return res;
}

u32 readMemory32(const u32 address)
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
		if (features.saveType == SaveEEPROM)
			return eepromRead(address);
		break;
	case 14:
		if (features.saveType == SaveSRAM)
			return sramRead(address);
		else if (features.saveType == SaveFlash)
			return flashRead(address);
		break;
	default:
		break;
	}

	return 0;
}

u16 readMemory16(const u32 address)
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
		if (features.saveType == SaveEEPROM)
			return eepromRead(address);
		break;
	case 14:
		if (features.saveType == SaveSRAM)
			return sramRead(address);
		else if (features.saveType == SaveFlash)
			return flashRead(address);
		break;
	default:
		break;
	}

	return 0;
}

u8 readMemory8(const u32 address)
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
		if (features.saveType == SaveEEPROM)
			return eepromRead(address);
		break;
	case 14:
		if (features.saveType == SaveSRAM)
			return sramRead(address);
		else if (features.saveType == SaveFlash)
			return flashRead(address);

		if (features.hasMotionSensor)
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
		}
		break;
	default:
		break;
	}

	return 0;
}

void writeMemory32(const u32 address, const u32 value)
{
	switch (address >> 24)
	{
	case 13:
		if (features.saveType == SaveEEPROM)
		{
			eepromWrite(address, value);
		}
		break;
	case 14:
		if (features.saveType == SaveSRAM)
		{
			sramWrite(address, (u8)value);
		}
		else if (features.saveType == SaveFlash)
		{
			flashWrite(address, (u8)value);
		}
		break;
	default:
		break;
	}
}

void writeMemory16(const u32 address, const u16 value)
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
		if (features.saveType == SaveEEPROM)
		{
			eepromWrite(address, (u8)value);
		}
		break;
	case 14:
		if (features.saveType == SaveSRAM)
		{
			sramWrite(address, (u8)value);
		}
		else if (features.saveType == SaveFlash)
		{
			flashWrite(address, (u8)value);
		}
		break;
	default:
		break;
	}
}

void writeMemory8(const u32 address, const u8 value)
{
	switch (address >> 24)
	{
	case 13:
		if (features.saveType == SaveEEPROM)
		{
			eepromWrite(address, value);
		}
		break;
	case 14:
		if (features.saveType == SaveSRAM)
		{
			sramWrite(address, value);
		}
		else if (features.saveType == SaveFlash)
		{
			flashWrite(address, value);
		}
		break;
	default:
		break;
	}
}

} // namespace Cartridge
