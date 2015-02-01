#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "snes_cpu_defs.h"
#include "snes_cpu.h"
#include "snes_cpu_registers.h"
#include "snes_cpu_addressing_mode.h"
#include "snes_cpu_mne.h"
#include "snes_cpu_stack.h"

struct _snes_cpu{
	snes_cpu_registers_t *registers;
	snes_cart_t *cart;
	snes_bus_t *bus;
	snes_cpu_stack_t *stack;
};

typedef struct {
	snes_cpu_mnemonic_t mne;
	snes_cpu_addressing_mode_t addr;
	int cycles;
} snes_cpu_opcode_t;

typedef struct {
	snes_cpu_opcode_t opcode;
	uint8_t operand_size;
	uint8_t operand_low;
	uint8_t operand_high;
	uint8_t operand_bank;
} snes_cpu_instruction_t;

static snes_cpu_opcode_t ops[256] = {
	[0x00] = {
		.mne = BRK,
		.addr = StackInterrupt,
		.cycles = 7,
	},
	[0x01] = {
		.mne = ORA,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0x02] = {
		.mne = COP,
		.addr = StackInterrupt,
		.cycles = 7,
	},
	[0x03] = {
		.mne = ORA,
		.addr = StackRelative,
		.cycles = 4,
	},
	[0x04] = {
		.mne = TSB,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0x05] = {
		.mne = ORA,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x06] = {
		.mne = ASL,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0x07] = {
		.mne = ORA,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0x08] = {
		.mne = PHP,
		.addr = StackPush,
		.cycles = 3,
	},
	[0x09] = {
		.mne = ORA,
		.addr = Immediate,
		.cycles = 2,
	},
	[0x0A] = {
		.mne = ASL,
		.addr = Accumulator,
		.cycles = 2,
	},
	[0x0B] = {
		.mne = PHD,
		.addr = StackPush,
		.cycles = 4,
	},
	[0x0C] = {
		.mne = TSB,
		.addr = Absolute,
		.cycles = 6,
	},
	[0x0D] = {
		.mne = ORA,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x0E] = {
		.mne = ASL,
		.addr = Absolute,
		.cycles = 6,
	},
	[0x0F] = {
		.mne = ORA,
		.addr = AbsoluteLong,
		.cycles = 5,
	},
	[0x10] = {
		.mne = BLP,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0x11] = {
		.mne = ORA,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 6,
	},
	[0x12] = {
		.mne = ORA,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0x13] = {
		.mne = ORA,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0x14] = {
		.mne = TRB,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0x15] = {
		.mne = ORA,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x16] = {
		.mne = ASL,
		.addr = DirectPageIndexedX,
		.cycles = 6,
	},
	[0x17] = {
		.mne = ORA,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 4,
	},
	[0x18] = {
		.mne = CLC,
		.addr = Implied,
		.cycles = 2,
	},
	[0x19] = {
		.mne = ORA,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0x1A] = {
		.mne = INC,
		.addr = Accumulator,
		.cycles = 2,
	},
	[0x1B] = {
		.mne = TCS,
		.addr = Implied,
		.cycles = 2,
	},
	[0x1C] = {
		.mne = TRB,
		.addr = Absolute,
		.cycles = 6,
	},
	[0x1D] = {
		.mne = ORA,
		.addr = AbsoluteIndexedX,
		.cycles = 4,
	},
	[0x1E] = {
		.mne = ASL,
		.addr = AbsoluteIndexedX,
		.cycles = 7,
	},
	[0x1F] = {
		.mne = ORA,
		.addr = AbsoluteLongIndexedX,
		.cycles = 5,
	},
	[0x20] = {
		.mne = JSR,
		.addr = Absolute,
		.cycles = 6,
	},
	[0x21] = {
		.mne = AND,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0x22] = {
		.mne = JSR,
		.addr = AbsoluteLong,
		.cycles = 8,
	},
	[0x23] = {
		.mne = AND,
		.addr = StackRelative,
		.cycles = 4,
	},
	[0x24] = {
		.mne = BIT,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x25] = {
		.mne = AND,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x26] = {
		.mne = ROL,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0x27] = {
		.mne = AND,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0x28] = {
		.mne = PLP,
		.addr = StackPull,
		.cycles = 4,
	},
	[0x29] = {
		.mne = AND,
		.addr = Immediate,
		.cycles = 2,
	},
	[0x2A] = {
		.mne = ROL,
		.addr = Accumulator,
		.cycles = 2,
	},
	[0x2B] = {
		.mne = PLD,
		.addr = StackPull,
		.cycles = 5,
	},
	[0x2C] = {
		.mne = BIT,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x2D] = {
		.mne = AND,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x2E] = {
		.mne = ROL,
		.addr = Absolute,
		.cycles = 6,
	},
	[0x2F] = {
		.mne = AND,
		.addr = AbsoluteLong,
		.cycles = 5,
	},
	[0x30] = {
		.mne = BMI,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0x31] = {
		.mne = AND,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 5,
	},
	[0x32] = {
		.mne = AND,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0x33] = {
		.mne = AND,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0x34] = {
		.mne = BIT,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x35] = {
		.mne = AND,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x36] = {
		.mne = ROL,
		.addr = DirectPageIndexedX,
		.cycles = 6,
	},
	[0x37] = {
		.mne = AND,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 6,
	},
	[0x38] = {
		.mne = SEC,
		.addr = Implied,
		.cycles = 2,
	},
	[0x39] = {
		.mne = AND,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0x3A] = {
		.mne = DEC,
		.addr = Accumulator,
		.cycles = 2,
	},
	[0x3B] = {
		.mne = TSC,
		.addr = Implied,
		.cycles = 2,
	},
	[0x3C] = {
		.mne = BIT,
		.addr = AbsoluteIndexedX,
		.cycles = 4,
	},
	[0x3D] = {
		.mne = AND,
		.addr = AbsoluteIndexedX,
		.cycles = 4,
	},
	[0x3E] = {
		.mne = ROL,
		.addr = AbsoluteIndexedX,
		.cycles = 7,
	},
	[0x3F] = {
		.mne = AND,
		.addr = AbsoluteLongIndexedX,
		.cycles = 5,
	},
	[0x40] = {
		.mne = RTI,
		.addr = StackRTI,
		.cycles = 6,
	},
	[0x41] = {
		.mne = EOR,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0x42] = {
		.mne = WDM,
		.addr = ProgramCounterRelative, //????
		.cycles = 2, //???
	},
	[0x43] = {
		.mne = EOR,
		.addr = StackRelative,
		.cycles = 4,
	},
	[0x44] = {
		.mne = MVP,
		.addr = BlockMove,
		.cycles = 4, //??
	},
	[0x45] = {
		.mne = EOR,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x46] = {
		.mne = LSR,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0x47] = {
		.mne = EOR,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0x48] = {
		.mne = PHA,
		.addr = StackPush,
		.cycles = 3,
	},
	[0x49] = {
		.mne = EOR,
		.addr = Immediate,
		.cycles = 2,
	},
	[0x4A] = {
		.mne = LSR,
		.addr = Accumulator,
		.cycles = 2,
	},
	[0x4B] = {
		.mne = PHK,
		.addr = StackPush,
		.cycles = 3,
	},
	[0x4C] = {
		.mne = JMP,
		.addr = Absolute,
		.cycles = 3,
	},
	[0x4D] = {
		.mne = EOR,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x4E] = {
		.mne = LSR,
		.addr = Absolute,
		.cycles = 6,
	},
	[0x4F] = {
		.mne = EOR,
		.addr = AbsoluteLong,
		.cycles = 5,
	},
	[0x50] = {
		.mne = BVC,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0x51] = {
		.mne = EOR,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 5,
	},
	[0x52] = {
		.mne = EOR,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0x53] = {
		.mne = EOR,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0x54] = {
		.mne = MVN,
		.addr = BlockMove,
		.cycles = 2, //???
	},
	[0x55] = {
		.mne = EOR,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x56] = {
		.mne = LSR,
		.addr = DirectPageIndexedX,
		.cycles = 6,
	},
	[0x57] = {
		.mne = EOR,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 6,
	},
	[0x58] = {
		.mne = CLI,
		.addr = Implied,
		.cycles = 2,
	},
	[0x59] = {
		.mne = EOR,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0x5A] = {
		.mne = PHY,
		.addr = StackPush,
		.cycles = 3,
	},
	[0x5B] = {
		.mne = TCD,
		.addr = Implied,
		.cycles = 2,
	},
	[0x5C] = {
		.mne = JMP,
		.addr = AbsoluteLong,
		.cycles = 4,
	},
	[0x5D] = {
		.mne = EOR,
		.addr = AbsoluteIndexedX,
		.cycles = 4,
	},
	[0x5E] = {
		.mne = LSR,
		.addr = AbsoluteIndexedX,
		.cycles = 7,
	},
	[0x5F] = {
		.mne = EOR,
		.addr = AbsoluteLongIndexedX,
		.cycles = 5,
	},
	[0x60] = {
		.mne = RTS,
		.addr = StackRTS,
		.cycles = 6,
	},
	[0x61] = {
		.mne = ADC,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0x62] = {
		.mne = PER,
		.addr = StackProgramCounterRelativeLong,
		.cycles = 6,
	},
	[0x63] = {
		.mne = ADC,
		.addr = StackRelative,
		.cycles = 4,
	},
	[0x64] = {
		.mne = STZ,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x65] = {
		.mne = ADC,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x66] = {
		.mne = ROR,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0x67] = {
		.mne = ADC,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0x68] = {
		.mne = PLA,
		.addr = StackPull,
		.cycles = 4,
	},
	[0x69] = {
		.mne = ADC,
		.addr = Immediate,
		.cycles = 2,
	},
	[0x6A] = {
		.mne = ROR,
		.addr = Accumulator,
		.cycles = 2,
	},
	[0x6B] = {
		.mne = RTL,
		.addr = StackRTL,
		.cycles = 6,
	},
	[0x6C] = {
		.mne = JMP,
		.addr = AbsoluteIndirect,
		.cycles = 5,
	},
	[0x6D] = {
		.mne = ADC,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x6E] = {
		.mne = ROR,
		.addr = Absolute,
		.cycles = 6,
	},
	[0x6F] = {
		.mne = ADC,
		.addr = AbsoluteLong,
		.cycles = 5,
	},
	[0x70] = {
		.mne = BVS,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0x71] = {
		.mne = ADC,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 5,
	},
	[0x72] = {
		.mne = ADC,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0x73] = {
		.mne = ADC,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0x74] = {
		.mne = STZ,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x75] = {
		.mne = ADC,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x76] = {
		.mne = ROR,
		.addr = DirectPageIndexedX,
		.cycles = 6,
	},
	[0x77] = {
		.mne = ADC,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 6,
	},
	[0x78] = {
		.mne = SEI,
		.addr = Implied,
		.cycles = 2,
	},
	[0x79] = {
		.mne = ADC,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0x7A] = {
		.mne = PLY,
		.addr = StackPull,
		.cycles = 4,
	},
	[0x7B] = {
		.mne = TDC,
		.addr = Implied,
		.cycles = 2,
	},
	[0x7C] = {
		.mne = JMP,
		.addr = AbsoluteIndexedIndirect,
		.cycles = 6,
	},
	[0x7D] = {
		.mne = ADC,
		.addr = AbsoluteIndexedX,
		.cycles = 4,
	},
	[0x7E] = {
		.mne = ROR,
		.addr = AbsoluteIndexedX,
		.cycles = 7,
	},
	[0x7F] = {
		.mne = ADC,
		.addr = AbsoluteLongIndexedX,
		.cycles = 5,
	},
	[0x80] = {
		.mne = BRA,
		.addr = ProgramCounterRelative,
		.cycles = 3,
	},
	[0x81] = {
		.mne = STA,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0x82] = {
		.mne = BRL,
		.addr = ProgramCounterRelativeLong,
		.cycles = 4,
	},
	[0x83] = {
		.mne = STA,
		.addr = StackRelative,
		.cycles = 4,
	},
	[0x84] = {
		.mne = STY,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x85] = {
		.mne = STA,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x86] = {
		.mne = STX,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0x87] = {
		.mne = STA,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0x88] = {
		.mne = DEY,
		.addr = Implied,
		.cycles = 2,
	},
	[0x89] = {
		.mne = BIT,
		.addr = Immediate,
		.cycles = 2,
	},
	[0x8A] = {
		.mne = TXA,
		.addr = Implied,
		.cycles = 2,
	},
	[0x8B] = {
		.mne = PHB,
		.addr = StackPush,
		.cycles = 3,
	},
	[0x8C] = {
		.mne = STY,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x8D] = {
		.mne = STA,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x8E] = {
		.mne = STX,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x8F] = {
		.mne = STA,
		.addr = AbsoluteLong,
		.cycles = 5,
	},
	[0x90] = {
		.mne = BCC,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0x91] = {
		.mne = STA,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 6,
	},
	[0x92] = {
		.mne = STA,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0x93] = {
		.mne = STA,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0x94] = {
		.mne = STY,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x95] = {
		.mne = STA,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0x96] = {
		.mne = STX,
		.addr = DirectPageIndexedY,
		.cycles = 4,
	},
	[0x97] = {
		.mne = STA,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 6,
	},
	[0x98] = {
		.mne = TYA,
		.addr = Implied,
		.cycles = 2,
	},
	[0x99] = {
		.mne = STA,
		.addr = AbsoluteIndexedY,
		.cycles = 5,
	},
	[0x9A] = {
		.mne = TXS,
		.addr = Implied,
		.cycles = 2,
	},
	[0x9B] = {
		.mne = TXY,
		.addr = Implied,
		.cycles = 2,
	},
	[0x9C] = {
		.mne = STZ,
		.addr = Absolute,
		.cycles = 4,
	},
	[0x9D] = {
		.mne = STA,
		.addr = AbsoluteIndexedX,
		.cycles = 5,
	},
	[0x9E] = {
		.mne = STZ,
		.addr = AbsoluteIndexedX,
		.cycles = 5,
	},
	[0x9F] = {
		.mne = STA,
		.addr = AbsoluteLongIndexedX,
		.cycles = 6,
	},
	[0xA0] = {
		.mne = LDY,
		.addr = Immediate,
		.cycles = 2,
	},
	[0xA1] = {
		.mne = LDA,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0xA2] = {
		.mne = LDX,
		.addr = Immediate,
		.cycles = 2,
	},
	[0xA3] = {
		.mne = LDA,
		.addr = StackRelative,
		.cycles = 4,
	},
	[0xA4] = {
		.mne = LDY,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0xA5] = {
		.mne = LDA,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0xA6] = {
		.mne = LDX,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0xA7] = {
		.mne = LDA,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0xA8] = {
		.mne = TAY,
		.addr = Implied,
		.cycles = 2,
	},
	[0xA9] = {
		.mne = LDA,
		.addr = Immediate,
		.cycles = 2,
	},
	[0xAA] = {
		.mne = TAX,
		.addr = Implied,
		.cycles = 2,
	},
	[0xAB] = {
		.mne = PLB,
		.addr = StackPull,
		.cycles = 4,
	},
	[0xAC] = {
		.mne = LDY,
		.addr = Absolute,
		.cycles = 4,
	},
	[0xAD] = {
		.mne = LDA,
		.addr = Absolute,
		.cycles = 4,
	},
	[0xAE] = {
		.mne = LDX,
		.addr = Absolute,
		.cycles = 4,
	},
	[0xAF] = {
		.mne = LDA,
		.addr = AbsoluteLong,
		.cycles = 6,
	},
	[0xB0] = {
		.mne = BCS,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0xB1] = {
		.mne = LDA,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 5,
	},
	[0xB2] = {
		.mne = LDA,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0xB3] = {
		.mne = LDA,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0xB4] = {
		.mne = LDY,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0xB5] = {
		.mne = LDA,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0xB6] = {
		.mne = LDX,
		.addr = DirectPageIndexedY,
		.cycles = 6,
	},
	[0xB7] = {
		.mne = LDA,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 6,
	},
	[0xB8] = {
		.mne = CLV,
		.addr = Implied,
		.cycles = 2,
	},
	[0xB9] = {
		.mne = LDA,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0xBA] = {
		.mne = TSX,
		.addr = Implied,
		.cycles = 2,
	},
	[0xBB] = {
		.mne = TYX,
		.addr = Implied,
		.cycles = 2,
	},
	[0xBC] = {
		.mne = LDY,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0xBD] = {
		.mne = LDA,
		.addr = AbsoluteIndexedX,
		.cycles = 4,
	},
	[0xBE] = {
		.mne = LDX,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0xBF] = {
		.mne = LDA,
		.addr = AbsoluteLongIndexedX,
		.cycles = 6,
	},
	[0xC0] = {
		.mne = CPY,
		.addr = Immediate,
		.cycles = 2,
	},
	[0xC1] = {
		.mne = CMP,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0xC2] = {
		.mne = REP,
		.addr = Immediate,
		.cycles = 3,
	},
	[0xC3] = {
		.mne = CMP,
		.addr = StackRelative,
		.cycles = 4,
	},
	[0xC4] = {
		.mne = CPY,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0xC5] = {
		.mne = CMP,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0xC6] = {
		.mne = DEC,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0xC7] = {
		.mne = CMP,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0xC8] = {
		.mne = INY,
		.addr = Implied,
		.cycles = 2,
	},
	[0xC9] = {
		.mne = CMP,
		.addr = Immediate,
		.cycles = 2,
	},
	[0xCA] = {
		.mne = DEX,
		.addr = Implied,
		.cycles = 2,
	},
	[0xCB] = {
		.mne = WAI,
		.addr = Implied,
		.cycles = 3,
	},
	[0xCC] = {
		.mne = CPY,
		.addr = Absolute,
		.cycles = 4,
	},
	[0xCD] = {
		.mne = CMP,
		.addr = Absolute,
		.cycles = 4,
	},
	[0xCE] = {
		.mne = DEC,
		.addr = Absolute,
		.cycles = 6,
	},
	[0xCF] = {
		.mne = CMP,
		.addr = AbsoluteLong,
		.cycles = 6,
	},
	[0xD0] = {
		.mne = BNE,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0xD1] = {
		.mne = CMP,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 5,
	},
	[0xD2] = {
		.mne = CMP,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0xD3] = {
		.mne = CMP,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0xD4] = {
		.mne = PEI,
		.addr = StackDirectPageIndirect,
		.cycles = 6,
	},
	[0xD5] = {
		.mne = CMP,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0xD6] = {
		.mne = DEC,
		.addr = DirectPageIndexedX,
		.cycles = 6,
	},
	[0xD7] = {
		.mne = CMP,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 6,
	},
	[0xD8] = {
		.mne = CLD,
		.addr = Implied,
		.cycles = 2,
	},
	[0xD9] = {
		.mne = CMP,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0xDA] = {
		.mne = PHX,
		.addr = StackPush,
		.cycles = 3,
	},
	[0xDB] = {
		.mne = STP,
		.addr = Implied,
		.cycles = 3,
	},
	[0xDC] = {
		.mne = JMP,
		.addr = AbsoluteIndirectLong,
		.cycles = 6,
	},
	[0xDD] = {
		.mne = CMP,
		.addr = AbsoluteIndexedX,
		.cycles = 7,
	},
	[0xDE] = {
		.mne = DEC,
		.addr = AbsoluteIndexedX,
		.cycles = 6,
	},
	[0xDF] = {
		.mne = CMP,
		.addr = AbsoluteLongIndexedX,
		.cycles = 5,
	},
	[0xE0] = {
		.mne = CPX,
		.addr = Immediate,
		.cycles = 2,
	},
	[0xE1] = {
		.mne = SBC,
		.addr = DirectPageIndexedIndirectX,
		.cycles = 6,
	},
	[0xE2] = {
		.mne = SEP,
		.addr = Immediate,
		.cycles = 3,
	},
	[0xE3] = {
		.mne = SBC,
		.addr = StackRelative,
		.cycles = 3,
	},
	[0xE4] = {
		.mne = CPX,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0xE5] = {
		.mne = SBC,
		.addr = DirectPage,
		.cycles = 3,
	},
	[0xE6] = {
		.mne = INC,
		.addr = DirectPage,
		.cycles = 5,
	},
	[0xE7] = {
		.mne = SBC,
		.addr = DirectPageIndirectLong,
		.cycles = 6,
	},
	[0xE8] = {
		.mne = INX,
		.addr = Implied,
		.cycles = 2,
	},
	[0xE9] = {
		.mne = SBC,
		.addr = Immediate,
		.cycles = 2,
	},
	[0xEA] = {
		.mne = NOP,
		.addr = Implied,
		.cycles = 2,
	},
	[0xEB] = {
		.mne = XBA,
		.addr = Implied,
		.cycles = 3,
	},
	[0xEC] = {
		.mne = CPX,
		.addr = Absolute,
		.cycles = 4,
	},
	[0xED] = {
		.mne = SBC,
		.addr = Absolute,
		.cycles = 4,
	},
	[0xEE] = {
		.mne = INC,
		.addr = Absolute,
		.cycles = 6,
	},
	[0xEF] = {
		.mne = SBC,
		.addr = AbsoluteLong,
		.cycles = 5,
	},
	[0xF0] = {
		.mne = BEQ,
		.addr = ProgramCounterRelative,
		.cycles = 2,
	},
	[0xF1] = {
		.mne = SBC,
		.addr = DirectPageIndirectIndexedY,
		.cycles = 5,
	},
	[0xF2] = {
		.mne = SBC,
		.addr = DirectPageIndirect,
		.cycles = 5,
	},
	[0xF3] = {
		.mne = SBC,
		.addr = StackRelativeIndirectIndexedY,
		.cycles = 7,
	},
	[0xF4] = {
		.mne = PEA,
		.addr = StackAbsolute,
		.cycles = 5,
	},
	[0xF5] = {
		.mne = SBC,
		.addr = DirectPageIndexedX,
		.cycles = 4,
	},
	[0xF6] = {
		.mne = INC,
		.addr = DirectPageIndexedX,
		.cycles = 6,
	},
	[0xF7] = {
		.mne = SBC,
		.addr = DirectPageIndirectLongIndexedY,
		.cycles = 6,
	},
	[0xF8] = {
		.mne = SED,
		.addr = Implied,
		.cycles = 2,
	},
	[0xF9] = {
		.mne = SBC,
		.addr = AbsoluteIndexedY,
		.cycles = 4,
	},
	[0xFA] = {
		.mne = PLX,
		.addr = StackPull,
		.cycles = 4,
	},
	[0xFB] = {
		.mne = XCE,
		.addr = Implied,
		.cycles = 2,
	},
	[0xFC] = {
		.mne = JSR,
		.addr = AbsoluteIndexedIndirect,
		.cycles = 8,
	},
	[0xFD] = {
		.mne = SBC,
		.addr = AbsoluteIndexedX,
		.cycles = 4,
	},
	[0xFE] = {
		.mne = INC,
		.addr = AbsoluteIndexedX,
		.cycles = 7
	},
	[0xFF] = {
		.mne = SBC,
		.addr = AbsoluteLongIndexedX,
		.cycles = 5,
	},
};

static const char* addressing_mode_tostring(snes_cpu_addressing_mode_t mode)
{
	switch (mode) {
		case Absolute:
			return "Absolute";
		case AbsoluteIndexedX:
			return "AbsoluteIndexedX";
		case AbsoluteIndexedY:
			return "AbsoluteIndexedY";
		case AbsoluteIndexedIndirect:
			return "AbsoluteIndexedIndirect";
		case AbsoluteIndirect:
			return "AbsoluteIndirect";
		case AbsoluteIndirectLong:
			return "AbsoluteIndirectLong";
		case AbsoluteLong:
			return "AbsoluteLong";
		case AbsoluteLongIndexedX:
			return "AbsoluteLongIndexedX";
		case Accumulator:
			return "Accumulator";
		case BlockMove:
			return "BlockMove";
		case DirectPage:
			return "DirectPage";
		case DirectPageIndexedX:
			return "DirectPageIndexedX";
		case DirectPageIndexedY:
			return "DirectPageIndexedY";
		case DirectPageIndexedIndirectX:
			return "DirectPageIndexedIndirectX";
		case DirectPageIndirect:
			return "DirectPageIndirect";
		case DirectPageIndirectLong:
			return "DirectPageIndirectLong";
		case DirectPageIndirectIndexedY:
			return "DirectPageIndirectIndexedY";
		case DirectPageIndirectLongIndexedY:
			return "DirectPageIndirectLongIndexedY";
		case Immediate:
			return "Immediate";
		case Implied:
			return "Implied";
		case ProgramCounterRelative:
			return "ProgramCounterRelative";
		case ProgramCounterRelativeLong:
			return "ProgramCounterRelativeLong";
		case StackAbsolute:
			return "StackAbsolute";
		case StackDirectPageIndirect:
			return "StackDirectPageIndirect";
		case StackInterrupt:
			return "StackInterrupt";
		case StackProgramCounterRelativeLong:
			return "StackProgramCounterRelativeLong";
		case StackPull:
			return "StackPull";
		case StackPush:
			return "StackPush";
		case StackRTI:
			return "StackRTI";
		case StackRTL:
			return "StackRTL";
		case StackRTS:
			return "StackRTS";
		case StackRelative:
			return "StackRelative";
		case StackRelativeIndirectIndexedY:
			return "StackRelativeIndirectIndexedY";
		default:
			return "Unknown";
	}
}

static const char* mnemonics_tostring(snes_cpu_mnemonic_t mne)
{
	switch (mne) {
		case ADC :
			return "ADC";
		case AND :
			return "AND";
		case ASL :
			return "ASL";
		case BCC :
			return "BCC";
		case BCS :
			return "BCS";
		case BEQ :
			return "BEQ";
		case BIT :
			return "BIT";
		case BMI :
			return "BMI";
		case BNE :
			return "BNE";
		case BLP :
			return "BLP";
		case BRA :
			return "BRA";
		case BRK :
			return "BRK";
		case BRL :
			return "BRL";
		case BVC :
			return "BVC";
		case BVS :
			return "BVS";
		case CLC :
			return "CLC";
		case CLD :
			return "CLD";
		case CLI :
			return "CLI";
		case CLV :
			return "CLV";
		case CMP :
			return "CMP";
		case COP :
			return "COP";
		case CPX :
			return "CPX";
		case CPY :
			return "CPY";
		case DEC :
			return "DEC";
		case DEX :
			return "DEX";
		case DEY :
			return "DEY";
		case EOR :
			return "EOR";
		case INC :
			return "INC";
		case INX :
			return "INX";
		case INY :
			return "INY";
		case JMP :
			return "JMP";
		case JSR :
			return "JSR";
		case LDA :
			return "LDA";
		case LDX :
			return "LDX";
		case LDY :
			return "LDY";
		case LSR :
			return "LSR";
		case MVN :
			return "MVN";
		case MVP :
			return "MVP";
		case NOP :
			return "NOP";
		case ORA :
			return "ORA";
		case PEA :
			return "PEA";
		case PEI :
			return "PEI";
		case PER :
			return "PER";
		case PHA :
			return "PHA";
		case PHB :
			return "PHB";
		case PHD :
			return "PHD";
		case PHK :
			return "PHK";
		case PHP :
			return "PHP";
		case PHX :
			return "PHX";
		case PHY :
			return "PHY";
		case PLA :
			return "PLA";
		case PLB :
			return "PLB";
		case PLD :
			return "PLD";
		case PLP :
			return "PLP";
		case PLX :
			return "PLX";
		case PLY :
			return "PLY";
		case REP :
			return "REP";
		case ROL :
			return "ROL";
		case ROR :
			return "ROR";
		case RTI :
			return "RTI";
		case RTL :
			return "RTL";
		case RTS :
			return "RTS";
		case SBC :
			return "SBC";
		case SEC :
			return "SEC";
		case SED :
			return "SED";
		case SEI :
			return "SEI";
		case SEP :
			return "SEP";
		case STA :
			return "STA";
		case STP :
			return "STP";
		case STX :
			return "STX";
		case STY :
			return "STY";
		case STZ :
			return "STZ";
		case TAX :
			return "TAX";
		case TAY :
			return "TAY";
		case TCD :
			return "TCD";
		case TCS :
			return "TCS";
		case TDC :
			return "TDC";
		case TRB :
			return "TRB";
		case TSB :
			return "TSB";
		case TSC :
			return "TSC";
		case TSX :
			return "TSX";
		case TXA :
			return "TXA";
		case TXS :
			return "TXS";
		case TXY :
			return "TXY";
		case TYA :
			return "TYA";
		case TYX :
			return "TYX";
		case WAI :
			return "WAI";
		case WDM :
			return "WDM";
		case XBA :
			return "XBA";
		case XCE :
			return "XCE";
		default :
			return "Unknown";
	}
}
static uint8_t snes_cpu_mne_is_m_sensitive(snes_cpu_mnemonic_t mne)
{
	switch(mne) {
		case ORA:
		case AND:
		case EOR:
		case ADC:
		case BIT:
		case LDA:
		case CMP:
		case SBC:
			return 1;
		default:
			return 0;
	}
}

static uint8_t snes_cpu_mne_is_x_sensitive(snes_cpu_mnemonic_t mne)
{
	switch(mne) {
		case LDY:
		case LDX:
		case CPY:
		case CPX:
			return 1;
		default:
			return 0;
	}
}

static uint8_t snes_cpu_mne_is_always_one(snes_cpu_mnemonic_t mne)
{
	switch(mne) {
		case SEP:
		case REP:
			return 1;
		default:
			return 0;
	}
}

static uint8_t snes_cpu_get_opcode_size(snes_cpu_t *cpu, snes_cpu_opcode_t opcode)
{
	switch (opcode.addr) {
		case Accumulator:
		case Implied:
		case StackPull:
		case StackPush:
		case StackRTI:
		case StackRTL:
		case StackRTS:
			return 0;

		case DirectPage:
		case DirectPageIndexedX:
		case DirectPageIndexedY:
		case DirectPageIndexedIndirectX:
		case DirectPageIndirect:
		case DirectPageIndirectLong:
		case DirectPageIndirectIndexedY:
		case DirectPageIndirectLongIndexedY:
		case ProgramCounterRelative:
		case StackDirectPageIndirect:
		case StackInterrupt:
		case StackRelative:
		case StackRelativeIndirectIndexedY:
			return 1;

		case Absolute:
		case AbsoluteIndexedX:
		case AbsoluteIndexedY:
		case AbsoluteIndexedIndirect:
		case AbsoluteIndirect:
		case AbsoluteIndirectLong:
		case BlockMove:
		case ProgramCounterRelativeLong:
		case StackAbsolute:
		case StackProgramCounterRelativeLong:
			return 2;

		case AbsoluteLong:
		case AbsoluteLongIndexedX:
			return 3;

		case Immediate:
		{
			if(snes_cpu_registers_emulation_isset(cpu->registers))
				return 1;
			if(snes_cpu_mne_is_m_sensitive(opcode.mne) &&
			   snes_cpu_registers_status_flag_isset(cpu->registers, STATUS_FLAG_M))
				return 1;
			if(snes_cpu_mne_is_x_sensitive(opcode.mne) &&
				  snes_cpu_registers_status_flag_isset(cpu->registers, STATUS_FLAG_X))
				return 1;
			if(snes_cpu_mne_is_always_one(opcode.mne))
				return 1;
			return 2;
		}
		default:
			return 0;
	}
}

snes_cpu_t *snes_cpu_power_up(snes_cart_t *cart, snes_bus_t *bus)
{
	snes_cpu_t *cpu = malloc(sizeof(snes_cpu_t));
	cpu->cart = cart;
	cpu->bus = bus;
	cpu->registers = snes_cpu_registers_init();
	cpu->stack = snes_cpu_stack_init(cpu->registers, cpu->bus);
	snes_cpu_registers_program_counter_set(cpu->registers, snes_rom_get_emu_interrupt_vectors(snes_cart_get_rom(cart)).reset);
	return cpu;
}

void snes_cpu_power_down(snes_cpu_t *cpu)
{
	cpu->cart = NULL;
	cpu->bus = NULL;
	snes_cpu_registers_destroy(cpu->registers);
	snes_cpu_stack_destroy(cpu->stack);
	free(cpu);
}

snes_cpu_registers_t *snes_cpu_get_registers(snes_cpu_t *cpu)
{
	return cpu->registers;
}

snes_cpu_stack_t *snes_cpu_get_stack(snes_cpu_t *cpu)
{
	return cpu->stack;
}

snes_bus_t *snes_cpu_get_bus(snes_cpu_t *cpu)
{
	return cpu->bus;
}


void snes_cpu_next_op(snes_cpu_t *cpu)
{
	snes_cpu_instruction_t instruction;
	struct snes_effective_address eff_addr;
	int i;
	uint8_t fetch_size;
	uint16_t pc = snes_cpu_registers_program_counter_get(cpu->registers);
	uint32_t pbr = snes_cpu_registers_program_bank_get(cpu->registers);
	uint8_t word = snes_bus_read(cpu->bus, pc + pbr);
	uint32_t operand = 0;
	snes_cpu_registers_program_counter_inc(cpu->registers);
	instruction.opcode = ops[word];

	fetch_size = snes_cpu_get_opcode_size(cpu, instruction.opcode);
	for(i = 0; i < fetch_size; i++)
	{
		operand += (snes_bus_read(cpu->bus,snes_cpu_registers_program_counter_get(cpu->registers)) << i*8);
		snes_cpu_registers_program_counter_inc(cpu->registers);
	}

	printf("0x%06X : 0x%02X %s (0x%06X) || ",pc + pbr , word, mnemonics_tostring(instruction.opcode.mne),operand);

	snes_cpu_registers_dump(cpu->registers);
	printf("\n");

	//Construct address
	eff_addr = snes_cpu_addressing_mode_decode(cpu->bus, cpu->registers, instruction.opcode.mne, instruction.opcode.addr, operand);
	//Execute instruction
	snes_cpu_mne_execute(instruction.opcode.mne, eff_addr, cpu);
}

void snes_cpu_nmi(snes_cpu_t *cpu)
{
	snes_cpu_registers_program_counter_set(cpu->registers, snes_rom_get_nat_interrupt_vectors(snes_cart_get_rom(cpu->cart)).nmi);
}
