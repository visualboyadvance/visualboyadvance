#ifndef SRAM_H
#define SRAM_H

#include "../common/Types.h"
#include <iostream>

void sramInit();
u8 sramRead(u32 address);
void sramWrite(u32 address, u8 byte);
bool sramReadBattery(FILE *file, size_t size);
bool sramWriteBattery(FILE *file);

#endif // SRAM_H
