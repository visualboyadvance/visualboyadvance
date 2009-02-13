#ifndef SRAM_H
#define SRAM_H

#include "../common/Types.h"
#include <iostream>

namespace Cartridge
{

void sramInit();
u8 sramRead(u32 address);
void sramWrite(u32 address, u8 byte);
bool sramReadBattery(FILE *file, size_t size);
bool sramWriteBattery(FILE *file);

} // namespace Cartridge

#endif // SRAM_H
