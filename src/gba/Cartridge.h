#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "../common/Types.h"
#include "GameInfos.h"

namespace Cartridge
{

bool init();
void reset();
void uninit();
bool loadGame(const GameInfos &_game);
void unloadGame();
void getGameName(u8 *romname);
const GameInfos &getGame();

bool readBatteryFromFile(const char *fileName);
bool writeBatteryToFile(const char *fileName);

u32 readMemory32(const u32 address);
u16 readMemory16(const u32 address);
u8 readMemory8(const u32 address);
void writeMemory32(const u32 address, const u32 value);
void writeMemory16(const u32 address, const u16 value);
void writeMemory8(const u32 address, const u8 value);

}

#endif // CARTRIDGE_H
