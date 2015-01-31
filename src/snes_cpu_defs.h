#ifndef SNES_CPU_DEFS_H
#define SNES_CPU_DEFS_H

#include <stdint.h>

typedef enum {
	ADC,
	AND,
	ASL,
	BCC,
	BCS,
	BEQ,
	BIT,
	BMI,
	BNE,
	BLP,
	BRA,
	BRK,
	BRL,
	BVC,
	BVS,
	CLC,
	CLD,
	CLI,
	CLV,
	CMP,
	COP,
	CPX,
	CPY,
	DEC,
	DEX,
	DEY,
	EOR,
	INC,
	INX,
	INY,
	JMP,
	JSR,
	LDA,
	LDX,
	LDY,
	LSR,
	MVN,
	MVP,
	NOP,
	ORA,
	PEA,
	PEI,
	PER,
	PHA,
	PHB,
	PHD,
	PHK,
	PHP,
	PHX,
	PHY,
	PLA,
	PLB,
	PLD,
	PLP,
	PLX,
	PLY,
	REP,
	ROL,
	ROR,
	RTI,
	RTL,
	RTS,
	SBC,
	SEC,
	SED,
	SEI,
	SEP,
	STA,
	STP,
	STX,
	STY,
	STZ,
	TAX,
	TAY,
	TCD,
	TCS,
	TDC,
	TRB,
	TSB,
	TSC,
	TSX,
	TXA,
	TXS,
	TXY,
	TYA,
	TYX,
	WAI,
	WDM,
	XBA,
	XCE,
	MAXMNE,
} snes_cpu_mnemonic_t;



typedef enum {
	Absolute = 0,
	AbsoluteIndexedX,
	AbsoluteIndexedY,
	AbsoluteIndexedIndirect,
	AbsoluteIndirect,
	AbsoluteIndirectLong,
	AbsoluteLong,
	AbsoluteLongIndexedX,
	Accumulator,
	BlockMove,
	DirectPage,
	DirectPageIndexedX,
	DirectPageIndexedY,
	DirectPageIndexedIndirectX,
	DirectPageIndirect,
	DirectPageIndirectLong,
	DirectPageIndirectIndexedY,
	DirectPageIndirectLongIndexedY,
	Immediate,
	Implied,
	ProgramCounterRelative,
	ProgramCounterRelativeLong,
	StackAbsolute,
	StackDirectPageIndirect,
	StackInterrupt,
	StackProgramCounterRelativeLong,
	StackPull,
	StackPush,
	StackRTI,
	StackRTL,
	StackRTS,
	StackRelative,
	StackRelativeIndirectIndexedY
} snes_cpu_addressing_mode_t;


enum snes_address_type{
	SNES_ADDRESS_TYPE_SIMPLE,
	SNES_ADDRESS_TYPE_BLOCK_MOVE,
	SNES_ADDRESS_TYPE_STACK,
	SNES_ADDRESS_TYPE_DATA,
	SNES_ADDRESS_TYPE_ACCUMULATOR,
};

struct snes_effective_address{
	enum snes_address_type type;
	union {
		uint32_t simple_address;
		struct{
			uint32_t dest;
			uint32_t src;
			uint16_t count;
		};
	};
};

#endif //SNES_CPU_DEFS_H
