#ifndef GBA_H
#define GBA_H

#include "../System.h"

#define SAVE_GAME_VERSION_11 11
#define SAVE_GAME_VERSION  SAVE_GAME_VERSION_11

extern u8 biosProtected[4];
extern int cpuNextEvent;
extern int cpuTotalTicks;
extern bool holdState;
extern u8 memoryWait[16];
extern u8 memoryWait32[16];
extern u8 memoryWaitSeq[16];
extern u8 memoryWaitSeq32[16];

extern void CPUUpdateRender();
extern void CPUUpdateRegister(u32, u16);
extern void CPUInit();
bool CPUInitMemory();
bool CPULoadBios(const char *biosFileName);
void CPUCleanUp();
extern void CPUReset();
extern void CPULoop(int ticks = 250000);
extern void CPUCheckDMA(int,int);
bool CPUReadState(const char * file);
bool CPUWriteState(const char *file);

#define R13_IRQ  18
#define R14_IRQ  19
#define SPSR_IRQ 20
#define R13_USR  26
#define R14_USR  27
#define R13_SVC  28
#define R14_SVC  29
#define SPSR_SVC 30
#define R13_ABT  31
#define R14_ABT  32
#define SPSR_ABT 33
#define R13_UND  34
#define R14_UND  35
#define SPSR_UND 36
#define R8_FIQ   37
#define R9_FIQ   38
#define R10_FIQ  39
#define R11_FIQ  40
#define R12_FIQ  41
#define R13_FIQ  42
#define R14_FIQ  43
#define SPSR_FIQ 44

#endif // GBA_H
