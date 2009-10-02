#ifndef MMU_H
#define MMU_H

#include "../common/Port.h"

namespace MMU
{

bool init();
void uninit();
 
u32 read32(u32 address);
u32 read16(u32 address);
u16 read16s(u32 address);
u8 read8(u32 address);
 
void write32(u32 address, u32 value);
void write16(u32 address, u16 value);
void write8(u32 address, u8 b);

u32 CPUReadMemory(u32 address);
u32 CPUReadHalfWord(u32 address);
u16 CPUReadHalfWordSigned(u32 address);
u8 CPUReadByte(u32 address);

void CPUWriteMemory(u32 address, u32 value);
void CPUWriteHalfWord(u32 address, u16 value);
void CPUWriteByte(u32 address, u8 b);

} // namespace MMU

#endif // MMU_H
