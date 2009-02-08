#ifndef EEPROM_H
#define EEPROM_H

extern int eepromRead(u32 address);
extern void eepromWrite(u32 address, u8 value);
extern void eepromInit();
extern void eepromReset();
extern u8 eepromData[0x2000];
extern int eepromSize;

#endif // EEPROM_H
