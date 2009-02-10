#ifndef FLASH_H
#define FLASH_H

namespace Cartridge
{

u8 flashRead(u32 address);
void flashWrite(u32 address, u8 byte);
void flashReset();
void flashSetSize(int size);
void flashInit();
bool flashReadBattery(FILE *file, size_t size);
bool flashWriteBattery(FILE *file);

} // namespace Cartridge

#endif // FLASH_H
