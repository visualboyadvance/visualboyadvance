#include "Sram.h"
#include <algorithm>

static const size_t sramSize = 0x10000;
static u8 sramData[sramSize];

void sramInit()
{
	std::fill(sramData, sramData + sramSize, 0xff);
}

u8 sramRead(u32 address)
{
	return sramData[address & 0xFFFF];
}

void sramWrite(u32 address, u8 byte)
{
	sramData[address & 0xFFFF] = byte;
}

bool sramReadBattery(FILE *file, size_t size)
{
	return fread(sramData, 1, size, file) == size;
}

bool sramWriteBattery(FILE *file)
{
	return fwrite(sramData, 1, sramSize, file) == sramSize;
}
