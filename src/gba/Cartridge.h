#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "../common/Types.h"

namespace Cartridge
{

typedef char GameSerial[4];

enum ESaveType
{
	SaveNone,
	SaveEEPROM,
	SaveSRAM,
	SaveFlash
};

struct Features
{
	ESaveType saveType;
	int eepromSize;
	int flashSize;
	bool hasRTC;
	bool hasMotionSensor;
};

extern Features features;

void init();
void reset();
bool loadDump(const char *file);
bool readBatteryFromFile(const char *fileName);
bool writeBatteryToFile(const char *fileName);

u32 readMemory32(const u32 address);
u16 readMemory16(const u32 address);
u8 readMemory8(const u32 address);

}

#endif // CARTRIDGE_H
