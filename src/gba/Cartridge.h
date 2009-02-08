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

bool loadDump(const char *file);

}

#endif // CARTRIDGE_H
