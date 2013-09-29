#include "Globals.h"
#include "Cartridge.h"
#include "CartridgeEEprom.h"
#include "CartridgeFlash.h"
#include "CartridgeRTC.h"
#include "CartridgeSram.h"
#include "Savestate.h"
#include "../common/GameDB.h"
#include "../common/Loader.h"
#include "../common/Util.h"
#include "../common/Port.h"
#include "../common/Settings.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

static GameInfos *game = NULL;
static u8 *rom = 0;

static gchar *getRomCode()
{
	return g_strndup((gchar *) &rom[0xac], 4);
}

gboolean cartridge_load_rom(const char *filename, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	int romSize = 0x2000000;
	
	RomLoader *loader = loader_new(ROM_GBA, filename);
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

void cartridge_unload()
{
	game_infos_free(game);
	game = NULL;
}

void cartridge_get_game_name(u8 *romname)
{
	memcpy(romname, &rom[0xa0], 16);
}

const gchar *cartridge_get_game_title() {
	if (!cartridge_is_present()) {
		return NULL;
	}

	return game->title;
}

const gchar *cartridge_get_game_region() {
	if (!cartridge_is_present()) {
		return NULL;
	}

	return game->region;
}

const gchar *cartridge_get_game_publisher() {
	if (!cartridge_is_present()) {
		return NULL;
	}

	return game->publisher;
}

gboolean cartridge_is_present() {
	return game != NULL;
}

gboolean cartridge_init()
{
	rom = (u8 *)malloc(0x2000000);
	if (!rom)
	{
		return FALSE;
	}

	cartridge_flash_init();
	cartridge_sram_init();
	cartridge_eeprom_init();

	return TRUE;
}

void cartridge_reset()
{
	cartridge_eeprom_reset(game->EEPROMSize);
	cartridge_flash_reset(game->flashSize);

	cartridge_rtc_reset();
	cartridge_rtc_enable(game->hasRTC);
}

void cartridge_free()
{
	if (rom)
	{
		free(rom);
		rom = 0;
	}
}

static gchar *get_battery_name() {
	const gchar *batteryDir = settings_get_battery_dir();
	gchar *baseName = g_path_get_basename(cartridge_get_game_title());
	gchar *fileName = g_strconcat(baseName, ".sav", NULL);
	gchar *batteryFile = g_build_filename(batteryDir, fileName, NULL);

	g_free(fileName);
	g_free(baseName);

	return batteryFile;
}

gboolean cartridge_write_battery(GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	if (game->hasFlash || game->hasEEPROM || game->hasSRAM)
	{
		gchar *batteryFile = get_battery_name();
		FILE *file = fopen(batteryFile, "wb");
		g_free(batteryFile);

		if (file == NULL) {
			g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
					"Failed to save battery: %s", g_strerror(errno));
			return FALSE;
		}

		gboolean success = TRUE;

		if (game->hasFlash)
		{
			success = cartridge_flash_write_battery(file);
		}
		else if (game->hasEEPROM)
		{
			success = cartridge_eeprom_write_battery(file);
		}
		else if (game->hasSRAM)
		{
			success = cartridge_sram_write_battery(file);
		}

		fclose(file);

		if (!success) {
			g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
					"Failed to save battery");
			return FALSE;
		}

		return TRUE;
	}

	return TRUE;
}

gboolean cartridge_read_battery(GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	gchar *batteryFile = get_battery_name();
	FILE *file = fopen(batteryFile, "rb");
	g_free(batteryFile);

	if (file == NULL) {
		g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
				"Failed to load battery: %s", g_strerror(errno));
		return FALSE;
	}

	// check file size to know what we should read
	fseek(file, 0, SEEK_END);

	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	gboolean success = TRUE;

	if (game->hasFlash)
	{
		success = cartridge_flash_read_battery(file, size);
	}
	else if (game->hasEEPROM)
	{
		success = cartridge_eeprom_read_battery(file, size);
	}
	else if (game->hasSRAM)
	{
		success = cartridge_sram_read_battery(file, size);
	}

	fclose(file);

	if (!success) {
		g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
				"Failed to load battery");
		return FALSE;
	}

	return TRUE;
}

u32 cartridge_read32(const u32 address)
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

u16 cartridge_read16(const u32 address)
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

u8 cartridge_read8(const u32 address)
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

void cartridge_write32(const u32 address, const u32 value)
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

void cartridge_write16(const u32 address, const u16 value)
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

void cartridge_write8(const u32 address, const u8 value)
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

