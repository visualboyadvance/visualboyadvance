#include "Globals.h"
#include "Cartridge.h"
#include "CartridgeEEprom.h"
#include "CartridgeFlash.h"
#include "CartridgeRTC.h"
#include "CartridgeSram.h"
#include "../common/GameDB.h"
#include "../common/Loader.h"
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

static GameInfos *game = NULL;
static u8 *rom = 0;

static gchar *getRomCode()
{
	return g_strndup((gchar *) &rom[0xac], 4);
}

bool loadRom(const char *_sFileName, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	int romSize = 0x2000000;
	
	RomLoader *loader = loader_new(ROM_GBA, _sFileName);
	if (!loader_load(loader, rom, &romSize, err)) {
		loader_free(loader);
		return FALSE;
	}
	loader_free(loader);

	// What does this do ?
	/*u16 *temp = (u16 *)(rom+((romSize+1)&~1));
	int i;
	for(i = (romSize+1)&~1; i < 0x2000000; i+=2) {
		WRITE16LE(temp, (i >> 1) & 0xFFFF);
		temp++;
	}*/
	
	gchar *code = getRomCode();
	game = game_db_lookup_code(code, err);
	g_free(code);

	return game != NULL;
}

void unloadGame()
{
	game_infos_free(game);
	game = NULL;
}

void getGameName(u8 *romname)
{
	std::copy(&rom[0xa0], &rom[0xa0] + 16, romname);
}

const gchar *getGameTitle() {
	if (!isPresent()) {
		return NULL;
	}

	return game->title;
}

bool isPresent() {
	return game != NULL;
}

bool init()
{
	rom = (u8 *)malloc(0x2000000);
	if (!rom)
	{
		return false;
	}

	cartridge_flash_init();
	cartridge_sram_init();
	cartridge_eeprom_init();

	return true;
}

void reset()
{
	cartridge_eeprom_reset(game->EEPROMSize);
	cartridge_flash_reset(game->flashSize);

	cartridge_rtc_reset();
	cartridge_rtc_enable(game->hasRTC);
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
	if (game->hasFlash || game->hasEEPROM || game->hasSRAM)
	{
		FILE *file = fopen(fileName, "wb");

		if (!file)
		{
			systemMessage("Error creating file %s", fileName);
			return false;
		}

		bool res = true;

		if (game->hasFlash)
		{
			res = cartridge_flash_write_battery(file);
		}
		else if (game->hasEEPROM)
		{
			res = cartridge_eeprom_write_battery(file);
		}
		else if (game->hasSRAM)
		{
			res = cartridge_sram_write_battery(file);
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

	if (game->hasFlash)
	{
		res = cartridge_flash_read_battery(file, size);
	}
	else if (game->hasEEPROM)
	{
		res = cartridge_eeprom_read_battery(file, size);
	}
	else if (game->hasSRAM)
	{
		res = cartridge_sram_read_battery(file, size);
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
		if (game->hasEEPROM)
			return cartridge_eeprom_read(address);
		break;
	case 14:
		if (game->hasSRAM)
			return cartridge_sram_read(address);
		else if (game->hasFlash)
			return cartridge_flash_read(address);
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
		if (cartridge_rtc_is_enabled() && (address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8))
			return cartridge_rtc_read(address);
		else
			return READ16LE(((u16 *)&rom[address & 0x1FFFFFE]));
		break;
	case 13:
		if (game->hasEEPROM)
			return cartridge_eeprom_read(address);
		break;
	case 14:
		if (game->hasSRAM)
			return cartridge_sram_read(address);
		else if (game->hasFlash)
			return cartridge_flash_read(address);
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
		if (game->hasEEPROM)
			return cartridge_eeprom_read(address);
		break;
	case 14:
		if (game->hasSRAM)
			return cartridge_sram_read(address);
		else if (game->hasFlash)
			return cartridge_flash_read(address);

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
		if (game->hasEEPROM)
		{
			cartridge_eeprom_write(address, value);
		}
		break;
	case 14:
		if (game->hasSRAM)
		{
			cartridge_sram_write(address, (u8)value);
		}
		else if (game->hasFlash)
		{
			cartridge_flash_write(address, (u8)value);
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
			cartridge_rtc_write(address, value);
		}
		break;
	case 13:
		if (game->hasEEPROM)
		{
			cartridge_eeprom_write(address, (u8)value);
		}
		break;
	case 14:
		if (game->hasSRAM)
		{
			cartridge_sram_write(address, (u8)value);
		}
		else if (game->hasFlash)
		{
			cartridge_flash_write(address, (u8)value);
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
		if (game->hasEEPROM)
		{
			cartridge_eeprom_write(address, value);
		}
		break;
	case 14:
		if (game->hasSRAM)
		{
			cartridge_sram_write(address, value);
		}
		else if (game->hasFlash)
		{
			cartridge_flash_write(address, value);
		}
		break;
	default:
		break;
	}
}

} // namespace Cartridge
