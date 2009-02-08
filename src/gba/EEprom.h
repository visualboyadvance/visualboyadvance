#ifndef EEPROM_H
#define EEPROM_H

#include <iostream>

int eepromRead(u32 address);
void eepromWrite(u32 address, u8 value);
void eepromInit();
void eepromReset();
bool eepromReadBattery(FILE *file, size_t size);
bool eepromWriteBattery(FILE *file);

#endif // EEPROM_H
