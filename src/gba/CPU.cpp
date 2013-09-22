#include "CPU.h"
#include "GBA.h"
#include "Globals.h"
#include "MMU.h"
#include "../common/Settings.h"

#include <algorithm>

namespace CPU
{

u32 cpuPrefetch[2];
u8 cpuBitsSet[256];

reg_pair reg[45];
bool N_FLAG = 0;
bool C_FLAG = 0;
bool Z_FLAG = 0;
bool V_FLAG = 0;

bool armState = true;
bool armIrqEnable = true;
u32 armNextPC = 0x00000000;
int armMode = 0x1f;

bool busPrefetch = false; //TODO: never read ?
bool busPrefetchEnable = false;
u32 busPrefetchCount = 0;

void init()
{
	for (int i = 0; i < 256; i++)
	{
		int count = 0;
		for (int j = 0; j < 8; j++)
			if (i & (1 << j))
				count++;

		cpuBitsSet[i] = count;
	}
}

void reset()
{
	// clean registers
	for (int i = 0; i < 45; i++)
		reg[i].I = 0;

	armMode = 0x1F;

	/*if(cpuIsMultiBoot)
	{
	  reg[13].I = 0x03007F00;
	  reg[15].I = 0x02000000;
	  reg[16].I = 0x00000000;
	  reg[R13_IRQ].I = 0x03007FA0;
	  reg[R13_SVC].I = 0x03007FE0;
	  armIrqEnable = true;
	}
	else*/
	{
		reg[15].I = 0x00000000;
		armMode = 0x13;
		armIrqEnable = false;
	}
	armState = true;
	C_FLAG = false;
	V_FLAG = false;
	N_FLAG = false;
	Z_FLAG = false;

	// disable FIQ
	reg[16].I |= 0x40;

	CPUUpdateCPSR();

	armNextPC = reg[15].I;
	reg[15].I += 4;

	ARM_PREFETCH();
}

#ifdef WORDS_BIGENDIAN
static void CPUSwap(volatile u32 *a, volatile u32 *b)
{
	volatile u32 c = *b;
	*b = *a;
	*a = c;
}
#else
static void CPUSwap(u32 *a, u32 *b)
{
	u32 c = *b;
	*b = *a;
	*a = c;
}
#endif

// Waitstates when accessing data
int dataTicksAccess16(u32 address) // DATA 8/16bits NON SEQ
{
	int addr = (address>>24)&15;
	int value =  memoryWait[addr];

	if ((addr>=0x08) || (addr < 0x02))
	{
		busPrefetchCount=0;
		busPrefetch=false;
	}
	else if (busPrefetch)
	{
		int waitState = value;
		if (!waitState)
			waitState = 1;
		busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	}

	return value;
}

int dataTicksAccess32(u32 address) // DATA 32bits NON SEQ
{
	int addr = (address>>24)&15;
	int value = memoryWait32[addr];

	if ((addr>=0x08) || (addr < 0x02))
	{
		busPrefetchCount=0;
		busPrefetch=false;
	}
	else if (busPrefetch)
	{
		int waitState = value;
		if (!waitState)
			waitState = 1;
		busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	}

	return value;
}

int dataTicksAccessSeq16(u32 address)// DATA 8/16bits SEQ
{
	int addr = (address>>24)&15;
	int value = memoryWaitSeq[addr];

	if ((addr>=0x08) || (addr < 0x02))
	{
		busPrefetchCount=0;
		busPrefetch=false;
	}
	else if (busPrefetch)
	{
		int waitState = value;
		if (!waitState)
			waitState = 1;
		busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	}

	return value;
}

int dataTicksAccessSeq32(u32 address)// DATA 32bits SEQ
{
	int addr = (address>>24)&15;
	int value =  memoryWaitSeq32[addr];

	if ((addr>=0x08) || (addr < 0x02))
	{
		busPrefetchCount=0;
		busPrefetch=false;
	}
	else if (busPrefetch)
	{
		int waitState = value;
		if (!waitState)
			waitState = 1;
		busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	}

	return value;
}


// Waitstates when executing opcode
int codeTicksAccess16(u32 address) // THUMB NON SEQ
{
	int addr = (address>>24)&15;

	if ((addr>=0x08) && (addr<=0x0D))
	{
		if (busPrefetchCount&0x1)
		{
			if (busPrefetchCount&0x2)
			{
				busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
				return 0;
			}
			busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
			return memoryWaitSeq[addr]-1;
		}
		else
		{
			busPrefetchCount=0;
			return memoryWait[addr];
		}
	}
	else
	{
		busPrefetchCount = 0;
		return memoryWait[addr];
	}
}

int codeTicksAccess32(u32 address) // ARM NON SEQ
{
	int addr = (address>>24)&15;

	if ((addr>=0x08) && (addr<=0x0D))
	{
		if (busPrefetchCount&0x1)
		{
			if (busPrefetchCount&0x2)
			{
				busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
				return 0;
			}
			busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
			return memoryWaitSeq[addr] - 1;
		}
		else
		{
			busPrefetchCount = 0;
			return memoryWait32[addr];
		}
	}
	else
	{
		busPrefetchCount = 0;
		return memoryWait32[addr];
	}
}

int codeTicksAccessSeq16(u32 address) // THUMB SEQ
{
	int addr = (address>>24)&15;

	if ((addr>=0x08) && (addr<=0x0D))
	{
		if (busPrefetchCount&0x1)
		{
			busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
			return 0;
		}
		else
			if (busPrefetchCount>0xFF)
			{
				busPrefetchCount=0;
				return memoryWait[addr];
			}
			else
				return memoryWaitSeq[addr];
	}
	else
	{
		busPrefetchCount = 0;
		return memoryWaitSeq[addr];
	}
}

int codeTicksAccessSeq32(u32 address) // ARM SEQ
{
	int addr = (address>>24)&15;

	if ((addr>=0x08) && (addr<=0x0D))
	{
		if (busPrefetchCount&0x1)
		{
			if (busPrefetchCount&0x2)
			{
				busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
				return 0;
			}
			busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
			return memoryWaitSeq[addr];
		}
		else
			if (busPrefetchCount>0xFF)
			{
				busPrefetchCount=0;
				return memoryWait32[addr];
			}
			else
				return memoryWaitSeq32[addr];
	}
	else
	{
		return memoryWaitSeq32[addr];
	}
}

void CPUSwitchMode(int mode, bool saveState, bool breakLoop)
{
	//  if(armMode == mode)
	//    return;

	CPUUpdateCPSR();

	switch (armMode)
	{
	case 0x10:
	case 0x1F:
		reg[R13_USR].I = reg[13].I;
		reg[R14_USR].I = reg[14].I;
		reg[17].I = reg[16].I;
		break;
	case 0x11:
		CPUSwap(&reg[R8_FIQ].I, &reg[8].I);
		CPUSwap(&reg[R9_FIQ].I, &reg[9].I);
		CPUSwap(&reg[R10_FIQ].I, &reg[10].I);
		CPUSwap(&reg[R11_FIQ].I, &reg[11].I);
		CPUSwap(&reg[R12_FIQ].I, &reg[12].I);
		reg[R13_FIQ].I = reg[13].I;
		reg[R14_FIQ].I = reg[14].I;
		reg[SPSR_FIQ].I = reg[17].I;
		break;
	case 0x12:
		reg[R13_IRQ].I  = reg[13].I;
		reg[R14_IRQ].I  = reg[14].I;
		reg[SPSR_IRQ].I =  reg[17].I;
		break;
	case 0x13:
		reg[R13_SVC].I  = reg[13].I;
		reg[R14_SVC].I  = reg[14].I;
		reg[SPSR_SVC].I =  reg[17].I;
		break;
	case 0x17:
		reg[R13_ABT].I  = reg[13].I;
		reg[R14_ABT].I  = reg[14].I;
		reg[SPSR_ABT].I =  reg[17].I;
		break;
	case 0x1b:
		reg[R13_UND].I  = reg[13].I;
		reg[R14_UND].I  = reg[14].I;
		reg[SPSR_UND].I =  reg[17].I;
		break;
	}

	u32 CPSR = reg[16].I;
	u32 SPSR = reg[17].I;

	switch (mode)
	{
	case 0x10:
	case 0x1F:
		reg[13].I = reg[R13_USR].I;
		reg[14].I = reg[R14_USR].I;
		reg[16].I = SPSR;
		break;
	case 0x11:
		CPUSwap(&reg[8].I, &reg[R8_FIQ].I);
		CPUSwap(&reg[9].I, &reg[R9_FIQ].I);
		CPUSwap(&reg[10].I, &reg[R10_FIQ].I);
		CPUSwap(&reg[11].I, &reg[R11_FIQ].I);
		CPUSwap(&reg[12].I, &reg[R12_FIQ].I);
		reg[13].I = reg[R13_FIQ].I;
		reg[14].I = reg[R14_FIQ].I;
		if (saveState)
			reg[17].I = CPSR;
		else
			reg[17].I = reg[SPSR_FIQ].I;
		break;
	case 0x12:
		reg[13].I = reg[R13_IRQ].I;
		reg[14].I = reg[R14_IRQ].I;
		reg[16].I = SPSR;
		if (saveState)
			reg[17].I = CPSR;
		else
			reg[17].I = reg[SPSR_IRQ].I;
		break;
	case 0x13:
		reg[13].I = reg[R13_SVC].I;
		reg[14].I = reg[R14_SVC].I;
		reg[16].I = SPSR;
		if (saveState)
			reg[17].I = CPSR;
		else
			reg[17].I = reg[SPSR_SVC].I;
		break;
	case 0x17:
		reg[13].I = reg[R13_ABT].I;
		reg[14].I = reg[R14_ABT].I;
		reg[16].I = SPSR;
		if (saveState)
			reg[17].I = CPSR;
		else
			reg[17].I = reg[SPSR_ABT].I;
		break;
	case 0x1b:
		reg[13].I = reg[R13_UND].I;
		reg[14].I = reg[R14_UND].I;
		reg[16].I = SPSR;
		if (saveState)
			reg[17].I = CPSR;
		else
			reg[17].I = reg[SPSR_UND].I;
		break;
	default:
		g_message("Unsupported ARM mode %02x", mode);
		break;
	}
	armMode = mode;
	CPUUpdateFlags(breakLoop);
	CPUUpdateCPSR();
}

void CPUSwitchMode(int mode, bool saveState)
{
	CPUSwitchMode(mode, saveState, true);
}

void CPUUndefinedException()
{
	u32 PC = reg[15].I;
	bool savedArmState = armState;
	CPUSwitchMode(0x1b, true, false);
	reg[14].I = PC - (savedArmState ? 4 : 2);
	reg[15].I = 0x04;
	armState = true;
	armIrqEnable = false;
	armNextPC = 0x04;
	ARM_PREFETCH();
	reg[15].I += 4;
}

void CPUSoftwareInterrupt()
{
	u32 PC = reg[15].I;
	bool savedArmState = armState;
	CPUSwitchMode(0x13, true, false);
	reg[14].I = PC - (savedArmState ? 4 : 2);
	reg[15].I = 0x08;
	armState = true;
	armIrqEnable = false;
	armNextPC = 0x08;
	ARM_PREFETCH();
	reg[15].I += 4;
}

void CPUSoftwareInterrupt(int comment)
{
	if (armState) comment >>= 16;

#ifdef GBA_LOGGING
	if (settings_log_channel_enabled(LOG_SWI))
	{
		g_message("SWI: %08x at %08x (0x%08x,0x%08x,0x%08x,VCOUNT = %2d)\n", comment,
		    armState ? armNextPC - 4: armNextPC -2,
		    reg[0].I,
		    reg[1].I,
		    reg[2].I,
		    VCOUNT);
	}
#endif
	CPUSoftwareInterrupt();
}

void CPUUpdateCPSR()
{
	u32 CPSR = reg[16].I & 0x40;
	if (N_FLAG)
		CPSR |= 0x80000000;
	if (Z_FLAG)
		CPSR |= 0x40000000;
	if (C_FLAG)
		CPSR |= 0x20000000;
	if (V_FLAG)
		CPSR |= 0x10000000;
	if (!armState)
		CPSR |= 0x00000020;
	if (!armIrqEnable)
		CPSR |= 0x80;
	CPSR |= (armMode & 0x1F);
	reg[16].I = CPSR;
}

void CPUUpdateFlags(bool breakLoop)
{
	u32 CPSR = reg[16].I;

	N_FLAG = (CPSR & 0x80000000) ? true: false;
	Z_FLAG = (CPSR & 0x40000000) ? true: false;
	C_FLAG = (CPSR & 0x20000000) ? true: false;
	V_FLAG = (CPSR & 0x10000000) ? true: false;
	armState = (CPSR & 0x20) ? false : true;
	armIrqEnable = (CPSR & 0x80) ? false : true;
	if (breakLoop)
	{
		if (armIrqEnable && (IF & IE) && (IME & 1))
			cpuNextEvent = cpuTotalTicks;
	}
}

void CPUUpdateFlags()
{
	CPUUpdateFlags(true);
}

void interrupt()
{
	u32 PC = reg[15].I;
	bool savedState = armState;
	CPUSwitchMode(0x12, true, false);
	reg[14].I = PC;
	if (!savedState)
		reg[14].I += 2;
	reg[15].I = 0x18;
	armState = true;
	armIrqEnable = false;

	armNextPC = reg[15].I;
	reg[15].I += 4;
	ARM_PREFETCH();

	//  if(!holdState)
	biosProtected[0] = 0x02;
	biosProtected[1] = 0xc0;
	biosProtected[2] = 0x5e;
	biosProtected[3] = 0xe5;
}

void enableBusPrefetch(bool enable)
{
	if (enable)
	{
		busPrefetchEnable = true;
		busPrefetch = false;
		busPrefetchCount = 0;
	}
	else
	{
		busPrefetchEnable = false;
		busPrefetch = false;
		busPrefetchCount = 0;
	}
}

} // namespace CPU

