#ifndef GBACPU_H
#define GBACPU_H

#include "../common/Types.h"

namespace CPU
{

#ifdef __GNUC__
# define INSN_REGPARM __attribute__((regparm(1)))
# define LIKELY(x) __builtin_expect(!!(x),1)
# define UNLIKELY(x) __builtin_expect(!!(x),0)
#else
# define INSN_REGPARM /*nothing*/
# define LIKELY(x) (x)
# define UNLIKELY(x) (x)
#endif

#define ARM_PREFETCH \
  {\
    CPU::cpuPrefetch[0] = CPUReadMemory(armNextPC);\
    CPU::cpuPrefetch[1] = CPUReadMemory(armNextPC+4);\
  }

#define THUMB_PREFETCH \
  {\
    CPU::cpuPrefetch[0] = CPUReadHalfWord(armNextPC);\
    CPU::cpuPrefetch[1] = CPUReadHalfWord(armNextPC+2);\
  }

#define ARM_PREFETCH_NEXT \
  CPU::cpuPrefetch[1] = CPUReadMemory(armNextPC+4);

#define THUMB_PREFETCH_NEXT\
  CPU::cpuPrefetch[1] = CPUReadHalfWord(armNextPC+2);

extern bool N_FLAG;
extern bool C_FLAG;
extern bool Z_FLAG;
extern bool V_FLAG;

extern u32 cpuPrefetch[2];
extern u8 cpuBitsSet[256];

int armExecute();
int thumbExecute();

int dataTicksAccess16(u32 address);
int dataTicksAccess32(u32 address);
int dataTicksAccessSeq16(u32 address);
int dataTicksAccessSeq32(u32 address);
int codeTicksAccess16(u32 address);
int codeTicksAccess32(u32 address);
int codeTicksAccessSeq16(u32 address);
int codeTicksAccessSeq32(u32 address);

void CPUSwitchMode(int mode, bool saveState);
void CPUSwitchMode(int mode, bool saveState, bool breakLoop);
void CPUUpdateCPSR();
void CPUUpdateFlags(bool breakLoop);
void CPUUpdateFlags();
void CPUUndefinedException();
void CPUSoftwareInterrupt();
void CPUSoftwareInterrupt(int comment);

} // namespace CPU

#endif // GBACPU_H
