#ifndef FLASH_H
#define FLASH_H

extern u8 flashRead(u32 address);
extern void flashWrite(u32 address, u8 byte);
extern void flashDelayedWrite(u32 address, u8 byte);
extern u8 flashSaveMemory[0x20000];
extern void flashSaveDecide(u32 address, u8 byte);
extern void flashReset();
extern void flashSetSize(int size);
extern void flashInit();

extern int flashSize;

#endif // FLASH_H
