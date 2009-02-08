#include "Globals.h"
#include "Cartridge.h"
#include "../Util.h"

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
		systemMessage("Error reading game database.");
	}
}

bool loadDump(const char *file)
{
	int romSize = 0x2000000;
	Features f;
	GameSerial gs;

	if (!utilLoad(file, utilIsGBAImage, rom, romSize))
		return false;

	getGameSerial(gs);
	findFeatures(f, gs);

	features = f;

	return true;

	// What does this do ?
	/*u16 *temp = (u16 *)(rom+((romSize+1)&~1));
	int i;
	for(i = (romSize+1)&~1; i < 0x2000000; i+=2) {
		WRITE16LE(temp, (i >> 1) & 0xFFFF);
		temp++;
	}*/
}

} // namespace Cartridge
