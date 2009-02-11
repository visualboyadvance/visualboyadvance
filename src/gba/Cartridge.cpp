#include "Globals.h"
#include "Cartridge.h"
#include "EEprom.h"
#include "Flash.h"
#include "RTC.h"
#include "Sram.h"
#include "../common/Util.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace Cartridge
{

Features features = {SaveNone, 0x10000, false, false};

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

static void clearFeatures(Features &features)
{
	features.saveType = SaveNone;
	features.flashSize = 0x10000;
	features.hasRTC = false;
	features.hasMotionSensor = false;
}

static void processFeatureToken(Features &features, const std::string &token)
{
	if (token == "FLASH_V120" || token == "FLASH_V121" ||
		token == "FLASH_V123" || token == "FLASH_V124" ||
		token == "FLASH_V125" || token == "FLASH_V126" ||
		token == "FLASH512_V130" || token == "FLASH512_V131")
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
			 token == "EEPROM_V124")
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

void init()
{
	flashInit();
	sramInit();
	eepromInit();
}

void reset()
{
	Features f;
	GameSerial gs;

	getGameSerial(gs);
	findFeatures(f, gs);

	eepromReset();
	flashReset();
	flashSetSize(f.flashSize);
	rtcReset();
	rtcEnable(f.hasRTC);

	features = f;
}

bool writeBatteryToFile(const char *fileName)
{
	if (features.saveType != SaveNone)
	{
		FILE *file = fopen(fileName, "wb");

		if(!file)
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

	if(!file)
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

} // namespace Cartridge
