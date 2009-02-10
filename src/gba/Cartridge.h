#ifndef CARTRIDGE_H
#define CARTRIDGE_H

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

}

#endif // CARTRIDGE_H
