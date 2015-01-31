#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snes_cpu_registers.h"



struct _snes_cpu_registers{
	struct snes_cpu_register_value accumulator;
	uint8_t data_bank;
	struct snes_cpu_register_value x;
	struct snes_cpu_register_value y;
	uint16_t direct_page;
	struct snes_cpu_register_value stack_pointer;
	uint8_t program_bank;
	uint16_t pc;
	uint8_t status;
	uint8_t emulation;
};

snes_cpu_registers_t *snes_cpu_registers_init()
{
	snes_cpu_registers_t *registers = malloc(sizeof(snes_cpu_registers_t));
	memset(registers, 0, sizeof(snes_cpu_registers_t));

	//All other registers will be configured by the emulation
	registers->stack_pointer.len = CPU_REGISTER_16_BIT;

	snes_cpu_registers_emulation_set(registers);
	snes_cpu_registers_status_flag_reset(registers, 0xf);
	return registers;
}

void snes_cpu_registers_destroy(snes_cpu_registers_t *registers)
{
	free(registers);
}

static void snes_cpu_registers_switch_reg_len(snes_cpu_registers_t *registers, enum snes_cpu_register_length len)
{
	registers->x.len = len;
	registers->y.len = len;
	if(len == CPU_REGISTER_8_BIT) {
		registers->x.value8_high = 0;
		registers->y.value8_high = 0;
	}
//	registers->direct_page = 0;
}

static void snes_cpu_registers_switch_mem_len(snes_cpu_registers_t *registers, enum snes_cpu_register_length len)
{
	registers->accumulator.len = len;
}

void snes_cpu_registers_update8(snes_cpu_registers_t *registers, uint8_t value)
{

	if(value == 0)
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_Z);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_Z);
	if((int8_t)value < 0)
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_N);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_N);
}

void snes_cpu_registers_update16(snes_cpu_registers_t *registers, uint16_t value)
{
	if(value == 0)
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_Z);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_Z);
	if((int16_t)value < 0)
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_N);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_N);
}

///////////////////ACCUMULATOR//////////////////////
void snes_cpu_registers_accumulator_set(snes_cpu_registers_t *registers, uint16_t value)
{
	if(registers->accumulator.len == CPU_REGISTER_8_BIT) {
		registers->accumulator.value8_low = (uint8_t)value;
		snes_cpu_registers_update8(registers, (uint8_t)value);
	} else {
		registers->accumulator.value16 = value;
		snes_cpu_registers_update16(registers, value);
	}
}

void snes_cpu_registers_accumulator_set16(snes_cpu_registers_t *registers, uint16_t value)
{
	registers->accumulator.value16 = value;
	snes_cpu_registers_update16(registers, value);
}

struct snes_cpu_register_value snes_cpu_registers_accumulator_get(snes_cpu_registers_t *registers)
{
	return registers->accumulator;
}

///////////////////X REGISTER//////////////////////
void snes_cpu_registers_x_set(snes_cpu_registers_t *registers, uint16_t value)
{
	registers->x.value16 = value;
	if(registers->x.len == CPU_REGISTER_8_BIT)
		snes_cpu_registers_update8(registers, registers->x.value8_low);
	else
		snes_cpu_registers_update16(registers, registers->x.value16);
}

struct snes_cpu_register_value snes_cpu_registers_x_get(snes_cpu_registers_t *registers)
{
	return registers->x;
}

///////////////////Y REGISTER//////////////////////
void snes_cpu_registers_y_set(snes_cpu_registers_t *registers, uint16_t value)
{
	registers->y.value16 = value;
	if(registers->y.len == CPU_REGISTER_8_BIT)
		snes_cpu_registers_update8(registers, registers->y.value8_low);
	else
		snes_cpu_registers_update16(registers, registers->y.value16);
}

struct snes_cpu_register_value snes_cpu_registers_y_get(snes_cpu_registers_t *registers)
{
	return registers->y;
}

///////////////////Direct Page//////////////////////
void snes_cpu_registers_direct_page_set(snes_cpu_registers_t *registers, uint16_t value)
{
	 registers->direct_page = value;
	 snes_cpu_registers_update16(registers, value);
}

uint16_t snes_cpu_registers_direct_page_get(snes_cpu_registers_t *registers)
{
	return registers->direct_page;
}

///////////////////Stack Pointer//////////////////////
void snes_cpu_registers_stack_pointer_set(snes_cpu_registers_t *registers, uint16_t value)
{
	if(registers->stack_pointer.len == CPU_REGISTER_8_BIT) {
		registers->stack_pointer.value8_low = (uint8_t) value;
	} else {
		registers->stack_pointer.value16 = value;
	}
}

struct snes_cpu_register_value snes_cpu_registers_stack_pointer_get(snes_cpu_registers_t *registers)
{
	return registers->stack_pointer;
}

///////////////////Program Counter//////////////////////
void snes_cpu_registers_program_counter_set(snes_cpu_registers_t *registers, uint16_t pc)
{
	registers->pc = pc;
}

uint16_t snes_cpu_registers_program_counter_get(snes_cpu_registers_t *registers)
{
	return registers->pc;
}

void snes_cpu_registers_program_counter_inc(snes_cpu_registers_t *registers)
{
	registers->pc++;
}

///////////////////Program Bank//////////////////////
void snes_cpu_registers_program_bank_set(snes_cpu_registers_t *registers, uint8_t value)
{
	registers->program_bank = value;
}

uint32_t snes_cpu_registers_program_bank_get(snes_cpu_registers_t *registers)
{
	return registers->program_bank << 16;
}


///////////////////Data Bank//////////////////////
void snes_cpu_registers_data_bank_set(snes_cpu_registers_t *registers, uint8_t value)
{
	registers->data_bank = value;
	snes_cpu_registers_update8(registers, value);
}

uint32_t snes_cpu_registers_data_bank_get(snes_cpu_registers_t *registers)
{
	return (registers->data_bank << 16);
}


///////////////////Status register//////////////////////
uint8_t snes_cpu_registers_status_flag_get(snes_cpu_registers_t *registers)
{
	return registers->status;
}

void snes_cpu_registers_status_flag_force(snes_cpu_registers_t *registers, uint8_t flags)
{
	registers->status = flags;
	if(flags & STATUS_FLAG_X)
		snes_cpu_registers_switch_reg_len(registers, CPU_REGISTER_8_BIT);
	if(flags & STATUS_FLAG_M)
		snes_cpu_registers_switch_mem_len(registers, CPU_REGISTER_8_BIT);
	///TODO implement the BCD mode
	if(flags & STATUS_FLAG_D)
		printf("Error BCD mode not supported !\n");
}

void snes_cpu_registers_status_flag_set(snes_cpu_registers_t *registers, uint8_t flags)
{
	registers->status |= flags;
	if(flags & STATUS_FLAG_X)
		snes_cpu_registers_switch_reg_len(registers, CPU_REGISTER_8_BIT);
	if(flags & STATUS_FLAG_M)
		snes_cpu_registers_switch_mem_len(registers, CPU_REGISTER_8_BIT);
	///TODO implement the BCD mode
	if(flags & STATUS_FLAG_D)
		printf("Error BCD mode not supported !\n");
}

void snes_cpu_registers_status_flag_reset(snes_cpu_registers_t *registers, uint8_t flags)
{
	registers->status &= ~flags;
	if(flags & STATUS_FLAG_X)
		snes_cpu_registers_switch_reg_len(registers, CPU_REGISTER_16_BIT);
	if(flags & STATUS_FLAG_M)
		snes_cpu_registers_switch_mem_len(registers, CPU_REGISTER_16_BIT);
}

uint8_t snes_cpu_registers_status_flag_isset(snes_cpu_registers_t *registers, uint8_t flag)
{
	return !!(registers->status & flag);
}

///////////////////Emulation enable disable//////////////////////
void snes_cpu_registers_emulation_set(snes_cpu_registers_t *registers)
{
	registers->emulation = 1;
	snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);

	registers->stack_pointer.value8_high = 0x01;
	snes_cpu_registers_switch_reg_len(registers, CPU_REGISTER_8_BIT);
	snes_cpu_registers_switch_mem_len(registers, CPU_REGISTER_8_BIT);
}

void snes_cpu_registers_emulation_reset(snes_cpu_registers_t *registers)
{
	/* Switch to native Mode */
	registers->emulation = 0;
	snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C | STATUS_FLAG_X | STATUS_FLAG_M);
}

uint8_t snes_cpu_registers_emulation_isset(snes_cpu_registers_t *registers)
{
	return registers->emulation;
}

void snes_cpu_registers_dump(snes_cpu_registers_t *registers)
{
	//Dump status flag
	if(registers->emulation)
		printf("PSR : %c%c%c%c%c%c%c%c%c",
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_N) ? 'N':'n',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_V) ? 'V':'v',
				'-',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_B) ? 'B':'b',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_D) ? 'D':'d',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_I) ? 'I':'i',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_Z) ? 'Z':'z',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C) ? 'C':'c',
				'E');
	else
		printf("PSR : %c%c%c%c%c%c%c%c%c",
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_N) ? 'N':'n',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_V) ? 'V':'v',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_M) ? 'M':'m',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_X) ? 'X':'x',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_D) ? 'D':'d',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_I) ? 'I':'i',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_Z) ? 'Z':'z',
				snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C) ? 'C':'c',
				'e');
	printf(" || ");
	printf("C = 0x%04X, A = 0x%2X, B = 0x%2X ",registers->accumulator.value16, registers->accumulator.value8_low, registers->accumulator.value8_high);
	printf("DP = 0x%04X ", registers->direct_page);
	printf("X = 0x%04X, Y = 0x%4X ",registers->x.len == CPU_REGISTER_8_BIT ? registers->x.value8_low:registers->x.value16, registers->y.len == CPU_REGISTER_8_BIT ? registers->y.value8_low:registers->y.value16);
	printf("SP = 0x%04X",registers->stack_pointer.value16);
}
