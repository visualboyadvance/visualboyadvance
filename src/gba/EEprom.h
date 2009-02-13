#ifndef EEPROM_H
#define EEPROM_H

#include <iostream>

namespace Cartridge
{

int eepromRead(u32 address);
void eepromWrite(u32 address, u8 value);
void eepromInit();
void eepromReset(int size);
bool eepromReadBattery(FILE *file, size_t size);
bool eepromWriteBattery(FILE *file);

} // namespace Cartridge

#endif // EEPROM_H
