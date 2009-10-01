#include "../System.h"
#include "../common/Port.h"
#include "Cartridge.h"
#include "CPU.h"
#include "GBA.h"
#include "Globals.h"
#include "Sound.h"

extern bool stopState;
extern bool timer0On;
extern int timer0Ticks;
extern int timer0ClockReload;
extern bool timer1On;
extern int timer1Ticks;
extern int timer1ClockReload;
extern bool timer2On;
extern int timer2Ticks;
extern int timer2ClockReload;
extern bool timer3On;
extern int timer3Ticks;
extern int timer3ClockReload;

static const u32 objTilesAddress [3] = {0x010000, 0x014000, 0x014000};
static bool ioReadable[0x400];

void MMUinit()
{
	for (int i = 0; i < 0x400; i++)
		ioReadable[i] = true;
	for (int i = 0x10; i < 0x48; i++)
		ioReadable[i] = false;
	for (int i = 0x4c; i < 0x50; i++)
		ioReadable[i] = false;
	for (int i = 0x54; i < 0x60; i++)
		ioReadable[i] = false;
	for (int i = 0x8c; i < 0x90; i++)
		ioReadable[i] = false;
	for (int i = 0xa0; i < 0xb8; i++)
		ioReadable[i] = false;
	for (int i = 0xbc; i < 0xc4; i++)
		ioReadable[i] = false;
	for (int i = 0xc8; i < 0xd0; i++)
		ioReadable[i] = false;
	for (int i = 0xd4; i < 0xdc; i++)
		ioReadable[i] = false;
	for (int i = 0xe0; i < 0x100; i++)
		ioReadable[i] = false;
	for (int i = 0x110; i < 0x120; i++)
		ioReadable[i] = false;
	for (int i = 0x12c; i < 0x130; i++)
		ioReadable[i] = false;
	for (int i = 0x138; i < 0x140; i++)
		ioReadable[i] = false;
	for (int i = 0x144; i < 0x150; i++)
		ioReadable[i] = false;
	for (int i = 0x15c; i < 0x200; i++)
		ioReadable[i] = false;
	for (int i = 0x20c; i < 0x300; i++)
		ioReadable[i] = false;
	for (int i = 0x304; i < 0x400; i++)
		ioReadable[i] = false;
}

u32 CPUReadMemory(u32 address)
{
#ifdef GBA_LOGGING
	if (address & 3)
	{
		if (systemVerbose & VERBOSE_UNALIGNED_MEMORY)
		{
			log("Unaligned word read: %08x at %08x\n", address, CPU::armMode ?
			    CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
	}
#endif

	u32 value;
	switch (address >> 24)
	{
	case 0:
		if (CPU::reg[15].I >> 24)
		{
			if (address < 0x4000)
			{
#ifdef GBA_LOGGING
				if (systemVerbose & VERBOSE_ILLEGAL_READ)
				{
					log("Illegal word read: %08x at %08x\n", address, CPU::armMode ?
					    CPU::armNextPC - 4 : CPU::armNextPC - 2);
				}
#endif

				value = READ32LE(((u32 *)&biosProtected));
				log("bios protected read %d", value);
			}
			else goto unreadable;
		}
		else
			value = READ32LE(((u32 *)&bios[address & 0x3FFC]));
		break;
	case 2:
		value = READ32LE(((u32 *)&workRAM[address & 0x3FFFC]));
		break;
	case 3:
		value = READ32LE(((u32 *)&internalRAM[address & 0x7ffC]));
		break;
	case 4:
		if ((address < 0x4000400) && ioReadable[address & 0x3fc])
		{
			if (ioReadable[(address & 0x3fc) + 2])
				value = READ32LE(((u32 *)&ioMem[address & 0x3fC]));
			else
				value = READ16LE(((u16 *)&ioMem[address & 0x3fc]));
		}
		else goto unreadable;
		break;
	case 5:
		value = READ32LE(((u32 *)&paletteRAM[address & 0x3fC]));
		break;
	case 6:
		address = (address & 0x1fffc);
		if (((DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
		{
			value = 0;
			break;
		}
		if ((address & 0x18000) == 0x18000)
			address &= 0x17fff;
		value = READ32LE(((u32 *)&vram[address]));
		break;
	case 7:
		value = READ32LE(((u32 *)&oam[address & 0x3FC]));
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		value = Cartridge::readMemory32(address);
		break;
		// default
	default:
unreadable:
#ifdef GBA_LOGGING
		if (systemVerbose & VERBOSE_ILLEGAL_READ)
		{
			log("Illegal word read: %08x at %08x\n", address, CPU::armMode ?
			    CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
#endif

		value = 0;
	}

	if (address & 3)
	{
		int shift = (address & 3) << 3;
		value = (value >> shift) | (value << (32 - shift));
	}
	return value;
}

u32 CPUReadHalfWord(u32 address)
{
#ifdef GBA_LOGGING
	if (address & 1)
	{
		if (systemVerbose & VERBOSE_UNALIGNED_MEMORY)
		{
			log("Unaligned halfword read: %08x at %08x\n", address, CPU::armMode ?
			    CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
	}
#endif

	u32 value;

	switch (address >> 24)
	{
	case 0:
		if (CPU::reg[15].I >> 24)
		{
			if (address < 0x4000)
			{
#ifdef GBA_LOGGING
				if (systemVerbose & VERBOSE_ILLEGAL_READ)
				{
					log("Illegal halfword read: %08x at %08x\n", address, CPU::armMode ?
					    CPU::armNextPC - 4 : CPU::armNextPC - 2);
				}
#endif
				value = READ16LE(((u16 *)&biosProtected[address&2]));
			}
			else goto unreadable;
		}
		else
			value = READ16LE(((u16 *)&bios[address & 0x3FFE]));
		break;
	case 2:
		value = READ16LE(((u16 *)&workRAM[address & 0x3FFFE]));
		break;
	case 3:
		value = READ16LE(((u16 *)&internalRAM[address & 0x7ffe]));
		break;
	case 4:
		if ((address < 0x4000400) && ioReadable[address & 0x3fe])
		{
			value =  READ16LE(((u16 *)&ioMem[address & 0x3fe]));
			if (((address & 0x3fe)>0xFF) && ((address & 0x3fe)<0x10E))
			{
				if (((address & 0x3fe) == 0x100) && timer0On)
					value = 0xFFFF - ((timer0Ticks-cpuTotalTicks) >> timer0ClockReload);
				else
					if (((address & 0x3fe) == 0x104) && timer1On && !(TM1CNT & 4))
						value = 0xFFFF - ((timer1Ticks-cpuTotalTicks) >> timer1ClockReload);
					else
						if (((address & 0x3fe) == 0x108) && timer2On && !(TM2CNT & 4))
							value = 0xFFFF - ((timer2Ticks-cpuTotalTicks) >> timer2ClockReload);
						else
							if (((address & 0x3fe) == 0x10C) && timer3On && !(TM3CNT & 4))
								value = 0xFFFF - ((timer3Ticks-cpuTotalTicks) >> timer3ClockReload);
			}
		}
		else goto unreadable;
		break;
	case 5:
		value = READ16LE(((u16 *)&paletteRAM[address & 0x3fe]));
		break;
	case 6:
		address = (address & 0x1fffe);
		if (((DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
		{
			value = 0;
			break;
		}
		if ((address & 0x18000) == 0x18000)
			address &= 0x17fff;
		value = READ16LE(((u16 *)&vram[address]));
		break;
	case 7:
		value = READ16LE(((u16 *)&oam[address & 0x3fe]));
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		value = Cartridge::readMemory16(address);
		break;
	default:
unreadable:
#ifdef GBA_LOGGING
		if (systemVerbose & VERBOSE_ILLEGAL_READ)
		{
			log("Illegal halfword read: %08x at %08x\n", address, CPU::armMode ?
			    CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
#endif
		value = 0;
		break;
	}

	if (address & 1)
	{
		value = (value >> 8) | (value << 24);
	}

	return value;
}

u16 CPUReadHalfWordSigned(u32 address)
{
	u16 value = CPUReadHalfWord(address);
	if ((address & 1))
		value = (s8)value;
	return value;
}

u8 CPUReadByte(u32 address)
{
	switch (address >> 24)
	{
	case 0:
		if (CPU::reg[15].I >> 24)
		{
			if (address < 0x4000)
			{
#ifdef GBA_LOGGING
				if (systemVerbose & VERBOSE_ILLEGAL_READ)
				{
					log("Illegal byte read: %08x at %08x\n", address, CPU::armMode ?
					    CPU::armNextPC - 4 : CPU::armNextPC - 2);
				}
#endif
				return biosProtected[address & 3];
			}
			else goto unreadable;
		}
		return bios[address & 0x3FFF];
	case 2:
		return workRAM[address & 0x3FFFF];
	case 3:
		return internalRAM[address & 0x7fff];
	case 4:
		if ((address < 0x4000400) && ioReadable[address & 0x3ff])
			return ioMem[address & 0x3ff];
		else goto unreadable;
	case 5:
		return paletteRAM[address & 0x3ff];
	case 6:
		address = (address & 0x1ffff);
		if (((DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
			return 0;
		if ((address & 0x18000) == 0x18000)
			address &= 0x17fff;
		return vram[address];
	case 7:
		return oam[address & 0x3ff];
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		return Cartridge::readMemory8(address);
		// default
	default:
unreadable:
#ifdef GBA_LOGGING
		if (systemVerbose & VERBOSE_ILLEGAL_READ)
		{
			log("Illegal byte read: %08x at %08x\n", address, CPU::armMode ?
			    CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
#endif
		return 0;
		break;
	}
}

void CPUWriteMemory(u32 address, u32 value)
{

#ifdef GBA_LOGGING
	if (address & 3)
	{
		if (systemVerbose & VERBOSE_UNALIGNED_MEMORY)
		{
			log("Unaligned word write: %08x to %08x from %08x\n",
			    value,
			    address,
			    CPU::armMode ? CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
	}
#endif

	switch (address >> 24)
	{
	case 0x02:
		WRITE32LE(((u32 *)&workRAM[address & 0x3FFFC]), value);
		break;
	case 0x03:
		WRITE32LE(((u32 *)&internalRAM[address & 0x7ffC]), value);
		break;
	case 0x04:
		if (address < 0x4000400)
		{
			CPUUpdateRegister((address & 0x3FC), value & 0xFFFF);
			CPUUpdateRegister((address & 0x3FC) + 2, (value >> 16));
		}
		else goto unwritable;
		break;
	case 0x05:
		WRITE32LE(((u32 *)&paletteRAM[address & 0x3FC]), value);
		break;
	case 0x06:
		address = (address & 0x1fffc);
		if (((DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
			return;
		if ((address & 0x18000) == 0x18000)
			address &= 0x17fff;
		WRITE32LE(((u32 *)&vram[address]), value);
		break;
	case 0x07:
		WRITE32LE(((u32 *)&oam[address & 0x3fc]), value);
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		Cartridge::writeMemory32(address, value);
		break;

		// default
	default:
unwritable:
#ifdef GBA_LOGGING
		if (systemVerbose & VERBOSE_ILLEGAL_WRITE)
		{
			log("Illegal word write: %08x to %08x from %08x\n",
			    value,
			    address,
			    CPU::armMode ? CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
#endif
		break;
	}
}

void CPUWriteHalfWord(u32 address, u16 value)
{
#ifdef GBA_LOGGING
	if (address & 1)
	{
		if (systemVerbose & VERBOSE_UNALIGNED_MEMORY)
		{
			log("Unaligned halfword write: %04x to %08x from %08x\n",
			    value,
			    address,
			    CPU::armMode ? CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
	}
#endif

	switch (address >> 24)
	{
	case 2:
		WRITE16LE(((u16 *)&workRAM[address & 0x3FFFE]),value);
		break;
	case 3:
		WRITE16LE(((u16 *)&internalRAM[address & 0x7ffe]), value);
		break;
	case 4:
		if (address < 0x4000400)
			CPUUpdateRegister(address & 0x3fe, value);
		else goto unwritable;
		break;
	case 5:
		WRITE16LE(((u16 *)&paletteRAM[address & 0x3fe]), value);
		break;
	case 6:
		address = (address & 0x1fffe);
		if (((DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
			return;
		if ((address & 0x18000) == 0x18000)
			address &= 0x17fff;
		WRITE16LE(((u16 *)&vram[address]), value);
		break;
	case 7:
		WRITE16LE(((u16 *)&oam[address & 0x3fe]), value);
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		Cartridge::writeMemory16(address, value);
		break;
	default:
unwritable:
#ifdef GBA_LOGGING
		if (systemVerbose & VERBOSE_ILLEGAL_WRITE)
		{
			log("Illegal halfword write: %04x to %08x from %08x\n",
			    value,
			    address,
			    CPU::armMode ? CPU::armNextPC - 4 : CPU::armNextPC - 2);
		}
#endif
		break;
	}
}

void CPUWriteByte(u32 address, u8 b)
{
	switch (address >> 24)
	{
	case 2:
		workRAM[address & 0x3FFFF] = b;
		break;
	case 3:
		internalRAM[address & 0x7fff] = b;
		break;
	case 4:
		if (address < 0x4000400)
		{
			switch (address & 0x3FF)
			{
			case 0x60:
			case 0x61:
			case 0x62:
			case 0x63:
			case 0x64:
			case 0x65:
			case 0x68:
			case 0x69:
			case 0x6c:
			case 0x6d:
			case 0x70:
			case 0x71:
			case 0x72:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x78:
			case 0x79:
			case 0x7c:
			case 0x7d:
			case 0x80:
			case 0x81:
			case 0x84:
			case 0x85:
			case 0x90:
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x96:
			case 0x97:
			case 0x98:
			case 0x99:
			case 0x9a:
			case 0x9b:
			case 0x9c:
			case 0x9d:
			case 0x9e:
			case 0x9f:
				soundEvent(address&0xFF, b);
				break;
			case 0x301: // HALTCNT, undocumented
				if (b == 0x80)
					stopState = true;
				holdState = 1;
				cpuNextEvent = cpuTotalTicks;
				break;
			default: // every other register
				u32 lowerBits = address & 0x3fe;
				if (address & 1)
				{
					CPUUpdateRegister(lowerBits, (READ16LE(&ioMem[lowerBits]) & 0x00FF) | (b << 8));
				}
				else
				{
					CPUUpdateRegister(lowerBits, (READ16LE(&ioMem[lowerBits]) & 0xFF00) | b);
				}
			}
			break;
		}
		else goto unwritable;
		break;
	case 5:
		// no need to switch
		*((u16 *)&paletteRAM[address & 0x3FE]) = (b << 8) | b;
		break;
	case 6:
		address = (address & 0x1fffe);
		if (((DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
			return;
		if ((address & 0x18000) == 0x18000)
			address &= 0x17fff;

		// no need to switch
		// byte writes to OBJ VRAM are ignored
		if ((address) < objTilesAddress[((DISPCNT&7)+1)>>2])
		{
			*((u16 *)&vram[address]) = (b << 8) | b;
		}
		break;
	case 7:
		// no need to switch
		// byte writes to OAM are ignored
		//    *((u16 *)&oam[address & 0x3FE]) = (b << 8) | b;
		break;
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		Cartridge::writeMemory8(address, b);
		break;
		// default
	default:
unwritable:
#ifdef GBA_LOGGING
		if (systemVerbose & VERBOSE_ILLEGAL_WRITE)
		{
			log("Illegal byte write: %02x to %08x from %08x\n",
			    b,
			    address,
			    CPU::armMode ? CPU::armNextPC - 4 : CPU::armNextPC -2 );
		}
#endif
		break;
	}
}
