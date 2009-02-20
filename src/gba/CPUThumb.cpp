#include "GBA.h"
#include "CPU.h"
#include "Globals.h"
#include "MMU.h"

namespace CPU
{

///////////////////////////////////////////////////////////////////////////

static int clockTicks;

static INSN_REGPARM void thumbUnknownInsn(u32 opcode)
{
#ifdef GBA_LOGGING
	if(systemVerbose & VERBOSE_UNDEFINED)
		log("Undefined THUMB instruction %04x at %08x\n", opcode, armNextPC-2);
#endif
	CPUUndefinedException();
}

// Common macros //////////////////////////////////////////////////////////

static inline u32 NEG(const u32 i) { return i >> 31; }
static inline u32 POS(const u32 i) { return (~i) >> 31;}


// C core

static inline bool ADDCARRY(const u32 a, const u32 b, const u32 c)
{
	return	(NEG(a) & NEG(b)) |
			(NEG(a) & POS(c)) |
			(NEG(b) & POS(c));
}

static inline bool ADDOVERFLOW(const u32 a, const u32 b, const u32 c)
{
	return	(NEG(a) & NEG(b) & POS(c)) |
			(POS(a) & POS(b) & NEG(c));
}

static inline bool SUBCARRY(const u32 a, const u32 b, const u32 c)
{
	return	(NEG(a) & POS(b)) |
			(NEG(a) & POS(c)) |
			(POS(b) & POS(c));
}

static inline bool SUBOVERFLOW(const u32 a, const u32 b, const u32 c)
{
	return	(NEG(a) & POS(b) & POS(c)) |
			(POS(a) & NEG(b) & NEG(c));
}

// 3-argument ADD/SUB /////////////////////////////////////////////////////

// ADD Rd, Rs, Rn
template <int N>
static INSN_REGPARM void thumb18(u32 opcode)
{
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 lhs = reg[source].I;
	u32 rhs = reg[N].I;
	u32 res = lhs + rhs;
	reg[dest].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = ADDCARRY(lhs, rhs, res);
	V_FLAG = ADDOVERFLOW(lhs, rhs, res);
}

// SUB Rd, Rs, Rn
template <int N>
static INSN_REGPARM void thumb1A(u32 opcode)
{
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 lhs = reg[source].I;
	u32 rhs = reg[N].I;
	u32 res = lhs - rhs;
	reg[dest].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = SUBCARRY(lhs, rhs, res);
	V_FLAG = SUBOVERFLOW(lhs, rhs, res);
}

// ADD Rd, Rs, #Offset3
template <int N>
static INSN_REGPARM void thumb1C(u32 opcode)
{
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 lhs = reg[source].I;
	u32 rhs = N;
	u32 res = lhs + rhs;
	reg[dest].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = ADDCARRY(lhs, rhs, res);
	V_FLAG = ADDOVERFLOW(lhs, rhs, res);
}

// SUB Rd, Rs, #Offset3
template <int N>
static INSN_REGPARM void thumb1E(u32 opcode)
{
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 lhs = reg[source].I;
	u32 rhs = N;
	u32 res = lhs - rhs;
	reg[dest].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = SUBCARRY(lhs, rhs, res);
	V_FLAG = SUBOVERFLOW(lhs, rhs, res);
}

// Shift instructions /////////////////////////////////////////////////////

// LSL Rd, Rm, #Imm 5
template <int N>
static INSN_REGPARM void thumb00(u32 opcode)
{ 
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 value;
	int shift = N;
	C_FLAG = (reg[source].I >> (32 - shift)) & 1 ? true : false;
	value = reg[source].I << shift;
	reg[dest].I = value;
	N_FLAG = (value & 0x80000000 ? true : false);
	Z_FLAG = (value ? false : true);
}

template <>
INSN_REGPARM void thumb00<0>(u32 opcode)
{ 
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 value = reg[source].I;
	reg[dest].I = value;
	N_FLAG = (value & 0x80000000 ? true : false);
	Z_FLAG = (value ? false : true);
}

// LSR Rd, Rm, #Imm 5
template <int N>
static INSN_REGPARM void thumb08(u32 opcode)
{ 
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 value;
	int shift = N;
	C_FLAG = (reg[source].I >> (shift - 1)) & 1 ? true : false;
	value = reg[source].I >> shift;
	reg[dest].I = value;
	N_FLAG = (value & 0x80000000 ? true : false);
	Z_FLAG = (value ? false : true);
}

template <>
INSN_REGPARM void thumb08<0>(u32 opcode)
{ 
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 value = 0;
	C_FLAG = reg[source].I & 0x80000000 ? true : false;
	reg[dest].I = value;
	N_FLAG = (value & 0x80000000 ? true : false);
	Z_FLAG = (value ? false : true);
}

// ASR Rd, Rm, #Imm 5
template <int N>
static INSN_REGPARM void thumb10(u32 opcode)
{ 
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 value;
	int shift = N;
	C_FLAG = ((s32)reg[source].I >> (int)(shift - 1)) & 1 ? true : false;
	value = (s32)reg[source].I >> (int)shift;
	reg[dest].I = value;
	N_FLAG = (value & 0x80000000 ? true : false);
	Z_FLAG = (value ? false : true);
}

template <>
INSN_REGPARM void thumb10<0>(u32 opcode)
{ 
	int dest = opcode & 0x07;
	int source = (opcode >> 3) & 0x07;
	u32 value;
	if(reg[source].I & 0x80000000) {
		value = 0xFFFFFFFF;
		C_FLAG = true;
	} else {
		value = 0;
		C_FLAG = false;
	}
	reg[dest].I = value;
	N_FLAG = (value & 0x80000000 ? true : false);
	Z_FLAG = (value ? false : true);
}

// MOV/CMP/ADD/SUB immediate //////////////////////////////////////////////

// MOV RN, #Offset8
template <int N>
static INSN_REGPARM void thumb20(u32 opcode)
{
	reg[N].I = opcode & 255;
	N_FLAG = false;
	Z_FLAG = (reg[N].I ? false : true);
}

// CMP RN, #Offset8
template <int N>
static INSN_REGPARM void thumb28(u32 opcode)
{
	u32 lhs = reg[N].I;
	u32 rhs = (opcode & 255);
	u32 res = lhs - rhs;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = SUBCARRY(lhs, rhs, res);
	V_FLAG = SUBOVERFLOW(lhs, rhs, res);
}

// ADD RN,#Offset8
template <int N>
static INSN_REGPARM void thumb30(u32 opcode)
{
	u32 lhs = reg[N].I;
	u32 rhs = (opcode & 255);
	u32 res = lhs + rhs;
	reg[N].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = ADDCARRY(lhs, rhs, res);
	V_FLAG = ADDOVERFLOW(lhs, rhs, res);
}

// SUB RN,#Offset8
template <int N>
static INSN_REGPARM void thumb38(u32 opcode)
{
	u32 lhs = reg[N].I;
	u32 rhs = (opcode & 255);
	u32 res = lhs - rhs;
	reg[N].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = SUBCARRY(lhs, rhs, res);
	V_FLAG = SUBOVERFLOW(lhs, rhs, res);
}

// ALU operations /////////////////////////////////////////////////////////

static inline void CMP_RD_RS(int dest, u32 value)
{
	u32 lhs = reg[dest].I;
	u32 rhs = value;
	u32 res = lhs - rhs;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = SUBCARRY(lhs, rhs, res);
	V_FLAG = SUBOVERFLOW(lhs, rhs, res);
} 

// AND Rd, Rs
static INSN_REGPARM void thumb40_0(u32 opcode)
{
	int dest = opcode & 7;
	reg[dest].I &= reg[(opcode >> 3)&7].I;
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
	Z_FLAG = reg[dest].I ? false : true;
}

// EOR Rd, Rs
static INSN_REGPARM void thumb40_1(u32 opcode)
{
	int dest = opcode & 7;
	reg[dest].I ^= reg[(opcode >> 3)&7].I;
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
	Z_FLAG = reg[dest].I ? false : true;
}

// LSL Rd, Rs
static INSN_REGPARM void thumb40_2(u32 opcode)
{
	int dest = opcode & 7;
	u32 value = reg[(opcode >> 3)&7].B.B0;
	if(value) {
		if(value == 32) {
			value = 0;
			C_FLAG = (reg[dest].I & 1 ? true : false);
		} else if(value < 32) {
			C_FLAG = (reg[dest].I >> (32 - value)) & 1 ? true : false;
			value = reg[dest].I << value;
		} else {
			value = 0;
			C_FLAG = false;
		}
		reg[dest].I = value;
	}
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
	Z_FLAG = reg[dest].I ? false : true;
	clockTicks = codeTicksAccess16(armNextPC)+2;
}

// LSR Rd, Rs
static INSN_REGPARM void thumb40_3(u32 opcode)
{
	int dest = opcode & 7;
	u32 value = reg[(opcode >> 3)&7].B.B0;
	if(value) {
		if(value == 32) {
			value = 0;
			C_FLAG = (reg[dest].I & 0x80000000 ? true : false);
		} else if(value < 32) {
			C_FLAG = (reg[dest].I >> (value - 1)) & 1 ? true : false;
			value = reg[dest].I >> value;
		} else {
			value = 0;
			C_FLAG = false;
		}
		reg[dest].I = value;
	}
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
	Z_FLAG = reg[dest].I ? false : true;
	clockTicks = codeTicksAccess16(armNextPC)+2;
}

// ASR Rd, Rs
static INSN_REGPARM void thumb41_0(u32 opcode)
{
	int dest = opcode & 7;
	u32 value = reg[(opcode >> 3)&7].B.B0;
	if(value) {
		if(value < 32) {
			C_FLAG = ((s32)reg[dest].I >> (int)(value - 1)) & 1 ? true : false;
			value = (s32)reg[dest].I >> (int)value;
			reg[dest].I = value;
		} else {
			if(reg[dest].I & 0x80000000) {
				reg[dest].I = 0xFFFFFFFF;
				C_FLAG = true;
			} else {
				reg[dest].I = 0x00000000;
				C_FLAG = false;
			}
		}
	}
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
	Z_FLAG = reg[dest].I ? false : true;
	clockTicks = codeTicksAccess16(armNextPC)+2;
}

// ADC Rd, Rs
static INSN_REGPARM void thumb41_1(u32 opcode)
{
	int dest = opcode & 0x07;
	u32 value = reg[(opcode >> 3)&7].I;
	u32 lhs = reg[dest].I;
	u32 rhs = value;
	u32 res = lhs + rhs + (u32)C_FLAG;
	reg[dest].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = ADDCARRY(lhs, rhs, res);
	V_FLAG = ADDOVERFLOW(lhs, rhs, res);
}

// SBC Rd, Rs
static INSN_REGPARM void thumb41_2(u32 opcode)
{
	int dest = opcode & 0x07;
	u32 value = reg[(opcode >> 3)&7].I;
	u32 lhs = reg[dest].I;
	u32 rhs = value;
	u32 res = lhs - rhs - !((u32)C_FLAG);
	reg[dest].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = SUBCARRY(lhs, rhs, res);
	V_FLAG = SUBOVERFLOW(lhs, rhs, res);
}

// ROR Rd, Rs
static INSN_REGPARM void thumb41_3(u32 opcode)
{
	int dest = opcode & 7;
	u32 value = reg[(opcode >> 3)&7].B.B0;

	if(value) {
		value = value & 0x1f;
		if(value == 0) {
			C_FLAG = (reg[dest].I & 0x80000000 ? true : false);
		} else {
			C_FLAG = (reg[dest].I >> (value - 1)) & 1 ? true : false;
			value = ((reg[dest].I << (32 - value)) |
										(reg[dest].I >> value));
			reg[dest].I = value;
		}
	}
	clockTicks = codeTicksAccess16(armNextPC)+2;
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
	Z_FLAG = reg[dest].I ? false : true;
}

// TST Rd, Rs
static INSN_REGPARM void thumb42_0(u32 opcode)
{
	u32 value = reg[opcode & 7].I & reg[(opcode >> 3) & 7].I;
	N_FLAG = value & 0x80000000 ? true : false;
	Z_FLAG = value ? false : true;
}

// NEG Rd, Rs
static INSN_REGPARM void thumb42_1(u32 opcode)
{
	int dest = opcode & 7;
	int source = (opcode >> 3) & 7;
	u32 lhs = reg[source].I;
	u32 rhs = 0;
	u32 res = rhs - lhs;
	reg[dest].I = res;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = SUBCARRY(rhs, lhs, res);
	V_FLAG = SUBOVERFLOW(rhs, lhs, res);
}

// CMP Rd, Rs
static INSN_REGPARM void thumb42_2(u32 opcode)
{
	int dest = opcode & 7;
	u32 value = reg[(opcode >> 3)&7].I;
	CMP_RD_RS(dest, value);
}

// CMN Rd, Rs
static INSN_REGPARM void thumb42_3(u32 opcode)
{
	int dest = opcode & 7;
	u32 value = reg[(opcode >> 3)&7].I;
	u32 lhs = reg[dest].I;
	u32 rhs = value;
	u32 res = lhs + rhs;
	Z_FLAG = (res == 0) ? true : false;
	N_FLAG = NEG(res) ? true : false;
	C_FLAG = ADDCARRY(lhs, rhs, res);
	V_FLAG = ADDOVERFLOW(lhs, rhs, res);
}

// ORR Rd, Rs
static INSN_REGPARM void thumb43_0(u32 opcode)
{
	int dest = opcode & 7;
	reg[dest].I |= reg[(opcode >> 3) & 7].I;
	Z_FLAG = reg[dest].I ? false : true;
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
}

// MUL Rd, Rs
static INSN_REGPARM void thumb43_1(u32 opcode)
{
	clockTicks = 1;
	int dest = opcode & 7;
	u32 rm = reg[dest].I;
	reg[dest].I = reg[(opcode >> 3) & 7].I * rm;
	if (((s32)rm) < 0)
		rm = ~rm;
	if ((rm & 0xFFFFFF00) == 0)
		clockTicks += 0;
	else if ((rm & 0xFFFF0000) == 0)
		clockTicks += 1;
	else if ((rm & 0xFF000000) == 0)
		clockTicks += 2;
	else
		clockTicks += 3;
	busPrefetchCount = (busPrefetchCount<<clockTicks) | (0xFF>>(8-clockTicks));
	clockTicks += codeTicksAccess16(armNextPC) + 1;
	Z_FLAG = reg[dest].I ? false : true;
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
}

// BIC Rd, Rs
static INSN_REGPARM void thumb43_2(u32 opcode)
{
	int dest = opcode & 7;
	reg[dest].I &= (~reg[(opcode >> 3) & 7].I);
	Z_FLAG = reg[dest].I ? false : true;
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
}

// MVN Rd, Rs
static INSN_REGPARM void thumb43_3(u32 opcode)
{
	int dest = opcode & 7;
	reg[dest].I = ~reg[(opcode >> 3) & 7].I;
	Z_FLAG = reg[dest].I ? false : true;
	N_FLAG = reg[dest].I & 0x80000000 ? true : false;
}

// High-register instructions and BX //////////////////////////////////////

// ADD Rd, Hs
static INSN_REGPARM void thumb44_1(u32 opcode)
{
	reg[opcode&7].I += reg[((opcode>>3)&7)+8].I;
}

// ADD Hd, Rs
static INSN_REGPARM void thumb44_2(u32 opcode)
{
	reg[(opcode&7)+8].I += reg[(opcode>>3)&7].I;
	if((opcode&7) == 7) {
		reg[15].I &= 0xFFFFFFFE;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC)*2
			+ codeTicksAccess16(armNextPC) + 3;
	}
}

// ADD Hd, Hs
static INSN_REGPARM void thumb44_3(u32 opcode)
{
	reg[(opcode&7)+8].I += reg[((opcode>>3)&7)+8].I;
	if((opcode&7) == 7) {
		reg[15].I &= 0xFFFFFFFE;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC)*2
			+ codeTicksAccess16(armNextPC) + 3;
	}
}

// CMP Rd, Hs
static INSN_REGPARM void thumb45_1(u32 opcode)
{
	int dest = opcode & 7;
	u32 value = reg[((opcode>>3)&7)+8].I;
	CMP_RD_RS(dest, value);
}

// CMP Hd, Rs
static INSN_REGPARM void thumb45_2(u32 opcode)
{
	int dest = (opcode & 7) + 8;
	u32 value = reg[(opcode>>3)&7].I;
	CMP_RD_RS(dest, value);
}

// CMP Hd, Hs
static INSN_REGPARM void thumb45_3(u32 opcode)
{
	int dest = (opcode & 7) + 8;
	u32 value = reg[((opcode>>3)&7)+8].I;
	CMP_RD_RS(dest, value);
}

// MOV Rd, Hs
static INSN_REGPARM void thumb46_1(u32 opcode)
{
	reg[opcode&7].I = reg[((opcode>>3)&7)+8].I;
}

// MOV Hd, Rs
static INSN_REGPARM void thumb46_2(u32 opcode)
{
	reg[(opcode&7)+8].I = reg[(opcode>>3)&7].I;
	if((opcode&7) == 7) {
		reg[15].I &= 0xFFFFFFFE;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC)*2
			+ codeTicksAccess16(armNextPC) + 3;
	}
}

// MOV Hd, Hs
static INSN_REGPARM void thumb46_3(u32 opcode)
{
	reg[(opcode&7)+8].I = reg[((opcode>>3)&7)+8].I;
	if((opcode&7) == 7) {
		reg[15].I &= 0xFFFFFFFE;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC)*2
			+ codeTicksAccess16(armNextPC) + 3;
	}
}


// BX Rs
static INSN_REGPARM void thumb47(u32 opcode)
{
	int base = (opcode >> 3) & 15;
	busPrefetchCount=0;
	reg[15].I = reg[base].I;
	if(reg[base].I & 1) {
		armState = false;
		reg[15].I &= 0xFFFFFFFE;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC)
			+ codeTicksAccessSeq16(armNextPC) + codeTicksAccess16(armNextPC) + 3;
	} else {
		armState = true;
		reg[15].I &= 0xFFFFFFFC;
		armNextPC = reg[15].I;
		reg[15].I += 4;
		ARM_PREFETCH;
		clockTicks = codeTicksAccessSeq32(armNextPC)
			+ codeTicksAccessSeq32(armNextPC) + codeTicksAccess32(armNextPC) + 3;
	}
}

// Load/store instructions ////////////////////////////////////////////////

// LDR R0~R7,[PC, #Imm]
static INSN_REGPARM void thumb48(u32 opcode)
{
	u8 regist = (opcode >> 8) & 7;
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = (reg[15].I & 0xFFFFFFFC) + ((opcode & 0xFF) << 2);
	reg[regist].I = CPUReadMemory(address);
	busPrefetchCount=0;
	clockTicks = 3 + dataTicksAccess32(address) + codeTicksAccess16(armNextPC);
}

// STR Rd, [Rs, Rn]
static INSN_REGPARM void thumb50(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
	CPUWriteMemory(address, reg[opcode & 7].I);
	clockTicks = dataTicksAccess32(address) + codeTicksAccess16(armNextPC) + 2;
}

// STRH Rd, [Rs, Rn]
static INSN_REGPARM void thumb52(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
	CPUWriteHalfWord(address, reg[opcode&7].W.W0);
	clockTicks = dataTicksAccess16(address) + codeTicksAccess16(armNextPC) + 2;
}

// STRB Rd, [Rs, Rn]
static INSN_REGPARM void thumb54(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode >>6)&7].I;
	CPUWriteByte(address, reg[opcode & 7].B.B0);
	clockTicks = dataTicksAccess16(address) + codeTicksAccess16(armNextPC) + 2;
}

// LDSB Rd, [Rs, Rn]
static INSN_REGPARM void thumb56(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
	reg[opcode&7].I = (s8)CPUReadByte(address);
	clockTicks = 3 + dataTicksAccess16(address) + codeTicksAccess16(armNextPC);
}

// LDR Rd, [Rs, Rn]
static INSN_REGPARM void thumb58(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
	reg[opcode&7].I = CPUReadMemory(address);
	clockTicks = 3 + dataTicksAccess32(address) + codeTicksAccess16(armNextPC);
}

// LDRH Rd, [Rs, Rn]
static INSN_REGPARM void thumb5A(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
	reg[opcode&7].I = CPUReadHalfWord(address);
	clockTicks = 3 + dataTicksAccess32(address) + codeTicksAccess16(armNextPC);
}

// LDRB Rd, [Rs, Rn]
static INSN_REGPARM void thumb5C(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
	reg[opcode&7].I = CPUReadByte(address);
	clockTicks = 3 + dataTicksAccess16(address) + codeTicksAccess16(armNextPC);
}

// LDSH Rd, [Rs, Rn]
static INSN_REGPARM void thumb5E(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
	reg[opcode&7].I = (s16)CPUReadHalfWordSigned(address);
	clockTicks = 3 + dataTicksAccess16(address) + codeTicksAccess16(armNextPC);
}

// STR Rd, [Rs, #Imm]
static INSN_REGPARM void thumb60(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<2);
	CPUWriteMemory(address, reg[opcode&7].I);
	clockTicks = dataTicksAccess32(address) + codeTicksAccess16(armNextPC) + 2;
}

// LDR Rd, [Rs, #Imm]
static INSN_REGPARM void thumb68(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<2);
	reg[opcode&7].I = CPUReadMemory(address);
	clockTicks = 3 + dataTicksAccess32(address) + codeTicksAccess16(armNextPC);
}

// STRB Rd, [Rs, #Imm]
static INSN_REGPARM void thumb70(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31));
	CPUWriteByte(address, reg[opcode&7].B.B0);
	clockTicks = dataTicksAccess16(address) + codeTicksAccess16(armNextPC) + 2;
}

// LDRB Rd, [Rs, #Imm]
static INSN_REGPARM void thumb78(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31));
	reg[opcode&7].I = CPUReadByte(address);
	clockTicks = 3 + dataTicksAccess16(address) + codeTicksAccess16(armNextPC);
}

// STRH Rd, [Rs, #Imm]
static INSN_REGPARM void thumb80(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<1);
	CPUWriteHalfWord(address, reg[opcode&7].W.W0);
	clockTicks = dataTicksAccess16(address) + codeTicksAccess16(armNextPC) + 2;
}

// LDRH Rd, [Rs, #Imm]
static INSN_REGPARM void thumb88(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<1);
	reg[opcode&7].I = CPUReadHalfWord(address);
	clockTicks = 3 + dataTicksAccess16(address) + codeTicksAccess16(armNextPC);
}

// STR R0~R7, [SP, #Imm]
static INSN_REGPARM void thumb90(u32 opcode)
{
	u8 regist = (opcode >> 8) & 7;
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[13].I + ((opcode&255)<<2);
	CPUWriteMemory(address, reg[regist].I);
	clockTicks = dataTicksAccess32(address) + codeTicksAccess16(armNextPC) + 2;
}

// LDR R0~R7, [SP, #Imm]
static INSN_REGPARM void thumb98(u32 opcode)
{
	u8 regist = (opcode >> 8) & 7;
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[13].I + ((opcode&255)<<2);
	reg[regist].I = CPUReadMemory(address);
	clockTicks = 3 + dataTicksAccess32(address) + codeTicksAccess16(armNextPC);
}

// PC/stack-related ///////////////////////////////////////////////////////

// ADD R0~R7, PC, Imm
static INSN_REGPARM void thumbA0(u32 opcode)
{
	u8 regist = (opcode >> 8) & 7;
	reg[regist].I = (reg[15].I & 0xFFFFFFFC) + ((opcode&255)<<2);
}

// ADD R0~R7, SP, Imm
static INSN_REGPARM void thumbA8(u32 opcode)
{
	u8 regist = (opcode >> 8) & 7;
	reg[regist].I = reg[13].I + ((opcode&255)<<2);
}

// ADD SP, Imm
static INSN_REGPARM void thumbB0(u32 opcode)
{
	int offset = (opcode & 127) << 2;
	if(opcode & 0x80)
		offset = -offset;
	reg[13].I += offset;
}

// Push and pop ///////////////////////////////////////////////////////////

static inline void PUSH_REG(u32 opcode, int &count, u32 &address, int val, int r)
{
	if (opcode & val) {
		CPUWriteMemory(address, reg[r].I);
		if (!count) {
			clockTicks += 1 + dataTicksAccess32(address);
		} else {
			clockTicks += 1 + dataTicksAccessSeq32(address);
		}
		count++;
		address += 4;
	}
}

static inline void POP_REG(u32 opcode, int &count, u32 &address, int val, int r)
{
	if (opcode & val) {
		reg[r].I = CPUReadMemory(address);
		if (!count) {
			clockTicks += 1 + dataTicksAccess32(address);
		} else {
			clockTicks += 1 + dataTicksAccessSeq32(address);
		}
		count++;
		address += 4;
	}
}

// PUSH {Rlist}
static INSN_REGPARM void thumbB4(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	int count = 0;
	u32 temp = reg[13].I - 4 * cpuBitsSet[opcode & 0xff];
	u32 address = temp & 0xFFFFFFFC;
	PUSH_REG(opcode, count, address, 1, 0);
	PUSH_REG(opcode, count, address, 2, 1);
	PUSH_REG(opcode, count, address, 4, 2);
	PUSH_REG(opcode, count, address, 8, 3);
	PUSH_REG(opcode, count, address, 16, 4);
	PUSH_REG(opcode, count, address, 32, 5);
	PUSH_REG(opcode, count, address, 64, 6);
	PUSH_REG(opcode, count, address, 128, 7);
	clockTicks += 1 + codeTicksAccess16(armNextPC);
	reg[13].I = temp;
}

// PUSH {Rlist, LR}
static INSN_REGPARM void thumbB5(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	int count = 0;
	u32 temp = reg[13].I - 4 - 4 * cpuBitsSet[opcode & 0xff];
	u32 address = temp & 0xFFFFFFFC;
	PUSH_REG(opcode, count, address, 1, 0);
	PUSH_REG(opcode, count, address, 2, 1);
	PUSH_REG(opcode, count, address, 4, 2);
	PUSH_REG(opcode, count, address, 8, 3);
	PUSH_REG(opcode, count, address, 16, 4);
	PUSH_REG(opcode, count, address, 32, 5);
	PUSH_REG(opcode, count, address, 64, 6);
	PUSH_REG(opcode, count, address, 128, 7);
	PUSH_REG(opcode, count, address, 256, 14);
	clockTicks += 1 + codeTicksAccess16(armNextPC);
	reg[13].I = temp;
}

// POP {Rlist}
static INSN_REGPARM void thumbBC(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	int count = 0;
	u32 address = reg[13].I & 0xFFFFFFFC;
	u32 temp = reg[13].I + 4*cpuBitsSet[opcode & 0xFF];
	POP_REG(opcode, count, address, 1, 0);
	POP_REG(opcode, count, address, 2, 1);
	POP_REG(opcode, count, address, 4, 2);
	POP_REG(opcode, count, address, 8, 3);
	POP_REG(opcode, count, address, 16, 4);
	POP_REG(opcode, count, address, 32, 5);
	POP_REG(opcode, count, address, 64, 6);
	POP_REG(opcode, count, address, 128, 7);
	reg[13].I = temp;
	clockTicks = 2 + codeTicksAccess16(armNextPC);
}

// POP {Rlist, PC}
static INSN_REGPARM void thumbBD(u32 opcode)
{
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	int count = 0;
	u32 address = reg[13].I & 0xFFFFFFFC;
	u32 temp = reg[13].I + 4 + 4*cpuBitsSet[opcode & 0xFF];
	POP_REG(opcode, count, address, 1, 0);
	POP_REG(opcode, count, address, 2, 1);
	POP_REG(opcode, count, address, 4, 2);
	POP_REG(opcode, count, address, 8, 3);
	POP_REG(opcode, count, address, 16, 4);
	POP_REG(opcode, count, address, 32, 5);
	POP_REG(opcode, count, address, 64, 6);
	POP_REG(opcode, count, address, 128, 7);
	reg[15].I = (CPUReadMemory(address) & 0xFFFFFFFE);
	if (!count) {
		clockTicks += 1 + dataTicksAccess32(address);
	} else {
		clockTicks += 1 + dataTicksAccessSeq32(address);
	}
	count++;
	armNextPC = reg[15].I;
	reg[15].I += 2;
	reg[13].I = temp;
	THUMB_PREFETCH;
	busPrefetchCount = 0;
	clockTicks += 3 + codeTicksAccess16(armNextPC) + codeTicksAccess16(armNextPC);
}

// Load/store multiple ////////////////////////////////////////////////////

static inline void THUMB_STM_REG(u32 opcode, int &count, u32 &address, u32 temp, int val, int r, int b)
{
	if(opcode & val) {
		CPUWriteMemory(address, reg[r].I);
		reg[b].I = temp;
		if (!count) {
			clockTicks += 1 + dataTicksAccess32(address);
		} else {
			clockTicks += 1 + dataTicksAccessSeq32(address);
		}
		count++;
		address += 4;
	}
}

static inline void THUMB_LDM_REG(u32 opcode, int &count, u32 &address, int val, int r)
{
	if(opcode & (val)) {
		reg[(r)].I = CPUReadMemory(address);
		if (!count) {
			clockTicks += 1 + dataTicksAccess32(address);
		} else {
			clockTicks += 1 + dataTicksAccessSeq32(address);
		}
		count++;
		address += 4;
	}
}

// STM R0~7!, {Rlist}
static INSN_REGPARM void thumbC0(u32 opcode)
{
	u8 regist = (opcode >> 8) & 7;
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[regist].I & 0xFFFFFFFC;
	u32 temp = reg[regist].I + 4*cpuBitsSet[opcode & 0xff];
	int count = 0;
	// store
	THUMB_STM_REG(opcode, count, address, temp, 1, 0, regist);
	THUMB_STM_REG(opcode, count, address, temp, 2, 1, regist);
	THUMB_STM_REG(opcode, count, address, temp, 4, 2, regist);
	THUMB_STM_REG(opcode, count, address, temp, 8, 3, regist);
	THUMB_STM_REG(opcode, count, address, temp, 16, 4, regist);
	THUMB_STM_REG(opcode, count, address, temp, 32, 5, regist);
	THUMB_STM_REG(opcode, count, address, temp, 64, 6, regist);
	THUMB_STM_REG(opcode, count, address, temp, 128, 7, regist);
	clockTicks = 1 + codeTicksAccess16(armNextPC);
}

// LDM R0~R7!, {Rlist}
static INSN_REGPARM void thumbC8(u32 opcode)
{
	u8 regist = (opcode >> 8) & 7;
	if (busPrefetchCount == 0)
		busPrefetch = busPrefetchEnable;
	u32 address = reg[regist].I & 0xFFFFFFFC;
	//u32 temp = reg[regist].I + 4*cpuBitsSet[opcode & 0xFF];
	int count = 0;
	// load
	THUMB_LDM_REG(opcode, count, address, 1, 0);
	THUMB_LDM_REG(opcode, count, address, 2, 1);
	THUMB_LDM_REG(opcode, count, address, 4, 2);
	THUMB_LDM_REG(opcode, count, address, 8, 3);
	THUMB_LDM_REG(opcode, count, address, 16, 4);
	THUMB_LDM_REG(opcode, count, address, 32, 5);
	THUMB_LDM_REG(opcode, count, address, 64, 6);
	THUMB_LDM_REG(opcode, count, address, 128, 7);
	clockTicks = 2 + codeTicksAccess16(armNextPC);
	if(!(opcode & (1<<regist)))
		reg[regist].I = address;
}

// Conditional branches ///////////////////////////////////////////////////

// BEQ offset
static INSN_REGPARM void thumbD0(u32 opcode)
{
	if(Z_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BNE offset
static INSN_REGPARM void thumbD1(u32 opcode)
{
	if(!Z_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BCS offset
static INSN_REGPARM void thumbD2(u32 opcode)
{
	if(C_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BCC offset
static INSN_REGPARM void thumbD3(u32 opcode)
{
	if(!C_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BMI offset
static INSN_REGPARM void thumbD4(u32 opcode)
{
	if(N_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BPL offset
static INSN_REGPARM void thumbD5(u32 opcode)
{
	if(!N_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BVS offset
static INSN_REGPARM void thumbD6(u32 opcode)
{
	if(V_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BVC offset
static INSN_REGPARM void thumbD7(u32 opcode)
{
	if(!V_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BHI offset
static INSN_REGPARM void thumbD8(u32 opcode)
{
	if(C_FLAG && !Z_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BLS offset
static INSN_REGPARM void thumbD9(u32 opcode)
{
	if(!C_FLAG || Z_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BGE offset
static INSN_REGPARM void thumbDA(u32 opcode)
{
	if(N_FLAG == V_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BLT offset
static INSN_REGPARM void thumbDB(u32 opcode)
{
	if(N_FLAG != V_FLAG) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BGT offset
static INSN_REGPARM void thumbDC(u32 opcode)
{
	if(!Z_FLAG && (N_FLAG == V_FLAG)) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// BLE offset
static INSN_REGPARM void thumbDD(u32 opcode)
{
	if(Z_FLAG || (N_FLAG != V_FLAG)) {
		reg[15].I += ((s8)(opcode & 0xFF)) << 1;
		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH;
		clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
			codeTicksAccess16(armNextPC)+3;
		busPrefetchCount=0;
	}
}

// SWI, B, BL /////////////////////////////////////////////////////////////

// SWI #comment
static INSN_REGPARM void thumbDF(u32 opcode)
{
	u32 address = 0;
	clockTicks = codeTicksAccessSeq16(address) + codeTicksAccessSeq16(address) +
		codeTicksAccess16(address)+3;
	busPrefetchCount=0;
	CPUSoftwareInterrupt(opcode & 0xFF);
}

// B offset
static INSN_REGPARM void thumbE0(u32 opcode)
{
	int offset = (opcode & 0x3FF) << 1;
	if(opcode & 0x0400)
		offset |= 0xFFFFF800;
	reg[15].I += offset;
	armNextPC = reg[15].I;
	reg[15].I += 2;
	THUMB_PREFETCH;
	clockTicks = codeTicksAccessSeq16(armNextPC) + codeTicksAccessSeq16(armNextPC) +
		codeTicksAccess16(armNextPC) + 3;
	busPrefetchCount=0;
}

// BLL #offset (forward)
static INSN_REGPARM void thumbF0(u32 opcode)
{
	int offset = (opcode & 0x7FF);
	reg[14].I = reg[15].I + (offset << 12);
	clockTicks = codeTicksAccessSeq16(armNextPC) + 1;
}

// BLL #offset (backward)
static INSN_REGPARM void thumbF4(u32 opcode)
{
	int offset = (opcode & 0x7FF);
	reg[14].I = reg[15].I + ((offset << 12) | 0xFF800000);
	clockTicks = codeTicksAccessSeq16(armNextPC) + 1;
}

// BLH #offset
static INSN_REGPARM void thumbF8(u32 opcode)
{
	int offset = (opcode & 0x7FF);
	u32 temp = reg[15].I-2;
	reg[15].I = (reg[14].I + (offset<<1))&0xFFFFFFFE;
	armNextPC = reg[15].I;
	reg[15].I += 2;
	reg[14].I = temp|1;
	THUMB_PREFETCH;
	clockTicks = codeTicksAccessSeq16(armNextPC) +
		codeTicksAccess16(armNextPC) + codeTicksAccessSeq16(armNextPC) + 3;
	busPrefetchCount = 0;
}

// Instruction table //////////////////////////////////////////////////////

typedef INSN_REGPARM void (*insnfunc_t)(u32 opcode);
#define thumbUI thumbUnknownInsn
#define thumbBP thumbUnknownInsn
static insnfunc_t thumbInsnTable[1024] = {
	thumb00<0>, thumb00<1>, thumb00<2>, thumb00<3>, thumb00<4>, thumb00<5>, thumb00<6>, thumb00<7>,  // 00
	thumb00<8>, thumb00<9>, thumb00<10>,thumb00<11>,thumb00<12>,thumb00<13>,thumb00<14>,thumb00<15>,
	thumb00<16>,thumb00<17>,thumb00<18>,thumb00<19>,thumb00<20>,thumb00<21>,thumb00<22>,thumb00<23>,
	thumb00<24>,thumb00<25>,thumb00<26>,thumb00<27>,thumb00<28>,thumb00<29>,thumb00<30>,thumb00<31>,
	thumb08<0>, thumb08<1>, thumb08<2>, thumb08<3>, thumb08<4>, thumb08<5>, thumb08<6>, thumb08<7>,  // 08
	thumb08<8>, thumb08<9>, thumb08<10>,thumb08<11>,thumb08<12>,thumb08<13>,thumb08<14>,thumb08<15>,
	thumb08<16>,thumb08<17>,thumb08<18>,thumb08<19>,thumb08<20>,thumb08<21>,thumb08<22>,thumb08<23>,
	thumb08<24>,thumb08<25>,thumb08<26>,thumb08<27>,thumb08<28>,thumb08<29>,thumb08<30>,thumb08<31>,
	thumb10<0>, thumb10<1>, thumb10<2>, thumb10<3>, thumb10<4>, thumb10<5>, thumb10<6>, thumb10<7>,  // 10
	thumb10<8>, thumb10<9>, thumb10<10>,thumb10<11>,thumb10<12>,thumb10<13>,thumb10<14>,thumb10<15>,
	thumb10<16>,thumb10<17>,thumb10<18>,thumb10<19>,thumb10<20>,thumb10<21>,thumb10<22>,thumb10<23>,
	thumb10<24>,thumb10<25>,thumb10<26>,thumb10<27>,thumb10<28>,thumb10<29>,thumb10<30>,thumb10<31>,
	thumb18<0>,thumb18<1>,thumb18<2>,thumb18<3>,thumb18<4>,thumb18<5>,thumb18<6>,thumb18<7>,  // 18
	thumb1A<0>,thumb1A<1>,thumb1A<2>,thumb1A<3>,thumb1A<4>,thumb1A<5>,thumb1A<6>,thumb1A<7>,
	thumb1C<0>,thumb1C<1>,thumb1C<2>,thumb1C<3>,thumb1C<4>,thumb1C<5>,thumb1C<6>,thumb1C<7>,
	thumb1E<0>,thumb1E<1>,thumb1E<2>,thumb1E<3>,thumb1E<4>,thumb1E<5>,thumb1E<6>,thumb1E<7>,
	thumb20<0>,thumb20<0>,thumb20<0>,thumb20<0>,thumb20<1>,thumb20<1>,thumb20<1>,thumb20<1>,  // 20
	thumb20<2>,thumb20<2>,thumb20<2>,thumb20<2>,thumb20<3>,thumb20<3>,thumb20<3>,thumb20<3>,
	thumb20<4>,thumb20<4>,thumb20<4>,thumb20<4>,thumb20<5>,thumb20<5>,thumb20<5>,thumb20<5>,
	thumb20<6>,thumb20<6>,thumb20<6>,thumb20<6>,thumb20<7>,thumb20<7>,thumb20<7>,thumb20<7>,
	thumb28<0>,thumb28<0>,thumb28<0>,thumb28<0>,thumb28<1>,thumb28<1>,thumb28<1>,thumb28<1>,  // 28
	thumb28<2>,thumb28<2>,thumb28<2>,thumb28<2>,thumb28<3>,thumb28<3>,thumb28<3>,thumb28<3>,
	thumb28<4>,thumb28<4>,thumb28<4>,thumb28<4>,thumb28<5>,thumb28<5>,thumb28<5>,thumb28<5>,
	thumb28<6>,thumb28<6>,thumb28<6>,thumb28<6>,thumb28<7>,thumb28<7>,thumb28<7>,thumb28<7>,
	thumb30<0>,thumb30<0>,thumb30<0>,thumb30<0>,thumb30<1>,thumb30<1>,thumb30<1>,thumb30<1>,  // 30
	thumb30<2>,thumb30<2>,thumb30<2>,thumb30<2>,thumb30<3>,thumb30<3>,thumb30<3>,thumb30<3>,
	thumb30<4>,thumb30<4>,thumb30<4>,thumb30<4>,thumb30<5>,thumb30<5>,thumb30<5>,thumb30<5>,
	thumb30<6>,thumb30<6>,thumb30<6>,thumb30<6>,thumb30<7>,thumb30<7>,thumb30<7>,thumb30<7>,
	thumb38<0>,thumb38<0>,thumb38<0>,thumb38<0>,thumb38<1>,thumb38<1>,thumb38<1>,thumb38<1>,  // 38
	thumb38<2>,thumb38<2>,thumb38<2>,thumb38<2>,thumb38<3>,thumb38<3>,thumb38<3>,thumb38<3>,
	thumb38<4>,thumb38<4>,thumb38<4>,thumb38<4>,thumb38<5>,thumb38<5>,thumb38<5>,thumb38<5>,
	thumb38<6>,thumb38<6>,thumb38<6>,thumb38<6>,thumb38<7>,thumb38<7>,thumb38<7>,thumb38<7>,
	thumb40_0,thumb40_1,thumb40_2,thumb40_3,thumb41_0,thumb41_1,thumb41_2,thumb41_3,  // 40
	thumb42_0,thumb42_1,thumb42_2,thumb42_3,thumb43_0,thumb43_1,thumb43_2,thumb43_3,
	thumbUI,thumb44_1,thumb44_2,thumb44_3,thumbUI,thumb45_1,thumb45_2,thumb45_3,
	thumbUI,thumb46_1,thumb46_2,thumb46_3,thumb47,thumb47,thumbUI,thumbUI,
	thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,  // 48
	thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
	thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
	thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
	thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,  // 50
	thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,
	thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,
	thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,
	thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,  // 58
	thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,
	thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,
	thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,
	thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,  // 60
	thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
	thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
	thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
	thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,  // 68
	thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
	thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
	thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
	thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,  // 70
	thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
	thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
	thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
	thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,  // 78
	thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
	thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
	thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
	thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,  // 80
	thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
	thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
	thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
	thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,  // 88
	thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
	thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
	thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
	thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,  // 90
	thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
	thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
	thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
	thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,  // 98
	thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
	thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
	thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
	thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,  // A0
	thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
	thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
	thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
	thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,  // A8
	thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
	thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
	thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
	thumbB0,thumbB0,thumbB0,thumbB0,thumbUI,thumbUI,thumbUI,thumbUI,  // B0
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
	thumbB4,thumbB4,thumbB4,thumbB4,thumbB5,thumbB5,thumbB5,thumbB5,
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,  // B8
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
	thumbBC,thumbBC,thumbBC,thumbBC,thumbBD,thumbBD,thumbBD,thumbBD,
	thumbBP,thumbBP,thumbBP,thumbBP,thumbUI,thumbUI,thumbUI,thumbUI,
	thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,  // C0
	thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
	thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
	thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
	thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,  // C8
	thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
	thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
	thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
	thumbD0,thumbD0,thumbD0,thumbD0,thumbD1,thumbD1,thumbD1,thumbD1,  // D0
	thumbD2,thumbD2,thumbD2,thumbD2,thumbD3,thumbD3,thumbD3,thumbD3,
	thumbD4,thumbD4,thumbD4,thumbD4,thumbD5,thumbD5,thumbD5,thumbD5,
	thumbD6,thumbD6,thumbD6,thumbD6,thumbD7,thumbD7,thumbD7,thumbD7,
	thumbD8,thumbD8,thumbD8,thumbD8,thumbD9,thumbD9,thumbD9,thumbD9,  // D8
	thumbDA,thumbDA,thumbDA,thumbDA,thumbDB,thumbDB,thumbDB,thumbDB,
	thumbDC,thumbDC,thumbDC,thumbDC,thumbDD,thumbDD,thumbDD,thumbDD,
	thumbUI,thumbUI,thumbUI,thumbUI,thumbDF,thumbDF,thumbDF,thumbDF,
	thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,  // E0
	thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
	thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
	thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,  // E8
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
	thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
	thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,  // F0
	thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,
	thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,
	thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,
	thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,  // F8
	thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
	thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
	thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
};

// Wrapper routine (execution loop) ///////////////////////////////////////

int thumbExecute()
{
	do {
		u32 opcode = cpuPrefetch[0];
		cpuPrefetch[0] = cpuPrefetch[1];

		busPrefetch = false;
		if (busPrefetchCount & 0xFFFFFF00)
			busPrefetchCount = 0x100 | (busPrefetchCount & 0xFF);
		clockTicks = 0;
		u32 oldArmNextPC = armNextPC;

		armNextPC = reg[15].I;
		reg[15].I += 2;
		THUMB_PREFETCH_NEXT;

		(*thumbInsnTable[opcode>>6])(opcode);

		if (clockTicks < 0)
			return 0;

		if (clockTicks == 0)
			clockTicks = codeTicksAccessSeq16(oldArmNextPC) + 1;

		cpuTotalTicks += clockTicks;

	} while (cpuTotalTicks < cpuNextEvent && !armState && !holdState && !SWITicks);

	return 1;
}

} // namespace CPU
