#ifndef FLASH_H
#define FLASH_H

u8 flashRead(u32 address);
void flashWrite(u32 address, u8 byte);
void flashReset();
void flashSetSize(int size);
void flashInit();
bool flashReadBattery(FILE *file, size_t size);
bool flashWriteBattery(FILE *file);

#endif // FLASH_H
