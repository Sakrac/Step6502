#pragma once

#include <stdint.h>

// complete representation of 6502 registers
typedef struct Regs {
	uint16_t PC;			// program counter
	uint8_t A, X, Y, P, S;	// registers
	uint8_t T;				// time for previous instruction, 0xff means JAM

	Regs() : PC(0x1000), A(0), X(0), Y(0), P(0), S(0), T(0) {}
} Regs;

enum StatusFlags {
	F_C = 1,
	F_Z = 2,
	F_I = 4,
	F_D = 8,
	F_B = 16,
	F_U = 32,
	F_V = 64,
	F_N = 128,
};

enum AddressModes {
	// address mode bit index

	// 6502

	AM_ZP_REL_X,	// 0 ($12,x)
	AM_ZP,			// 1 $12
	AM_IMM,			// 2 #$12
	AM_ABS,			// 3 $1234
	AM_ZP_Y_REL,	// 4 ($12),y
	AM_ZP_X,		// 5 $12,x
	AM_ABS_Y,		// 6 $1234,y
	AM_ABS_X,		// 7 $1234,x
	AM_REL,			// 8 ($1234)
	AM_ACC,			// 9 A
	AM_NON,			// a
	AM_BRANCH,		// b $1234
	AM_ZP_REL_Y,	// c
	AM_ZP_Y,		// d
	AM_COUNT,
};

void Initialize6502();
void Shutdown6502();
void ResetUndoBuffer();
Regs& GetRegs();
void SetRegs(const Regs &r);
uint32_t GetCycles();
uint8_t *Get6502Mem(uint16_t addr);
uint8_t Get6502Byte(uint16_t addr);
void Set6502Byte(uint16_t addr, uint8_t value);
int Disassemble(uint16_t addr, char *dest, int left, int &chars, bool illegals = false);
int Assemble(char *cmd, uint16_t addr);
int InstructionBytes(uint16_t addr, bool illegals = false);
bool IsCPURunning();
void CPUAddUndoRegs(Regs &regs);
void CPUGo();
void CPUReverse();
void CPUStop();
void CPUStep();
void CPUStepOver();
bool CPUStepBack();	// false if no reverse steps left
void CPUReset();
void CPUIRQ();
void CPUNMI();

bool SetBPCondition(uint32_t id, const uint8_t *condition, uint16_t length);
void ClearBPCondition(uint32_t id);
uint16_t GetNumPCBreakpoints();
uint16_t* GetPCBreakpoints();
uint16_t GetPCBreakpointsID(uint16_t **pBP, uint32_t **pID, uint16_t &nDS);
uint32_t TogglePCBreakpoint(uint16_t addr);
uint32_t SetPCBreakpoint(uint16_t addr); // returns ID of breakpoint
void RemoveBreakpointByID(uint32_t id);
bool GetBreakpointAddrByID(uint32_t id, uint16_t &addr);
bool EnableBPByID(uint32_t id, bool enable);

