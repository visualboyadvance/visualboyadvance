#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "../common/Types.h"
#include <glib.h>

namespace Cartridge
{

bool init();
void reset();
void uninit();
bool loadRom(const char *_sFileName, GError **err);
void unloadGame();
void getGameName(u8 *romname);
const gchar *getGameTitle();
bool isPresent();

bool readBatteryFromFile(const char *fileName);
bool writeBatteryToFile(const char *fileName);

u32 read32(const u32 address);
u16 read16(const u32 address);
u8 read8(const u32 address);
void write32(const u32 address, const u32 value);
void write16(const u32 address, const u16 value);
void write8(const u32 address, const u8 value);

}

#endif // CARTRIDGE_H
