#include "snes_cpu_mne.h"
#include "snes_cpu_registers.h"
#include <stdio.h>


/*For test*/
#include "snes_cpu.h"

static uint16_t fetch_data(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, uint8_t size)
{
	uint16_t data;
	if(eff_addr.type == SNES_ADDRESS_TYPE_DATA) {
		data = eff_addr.simple_address;
	} else if(eff_addr.type == SNES_ADDRESS_TYPE_ACCUMULATOR) {
		struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
		if(acc.len == CPU_REGISTER_8_BIT) {
			data = acc.value8_low;
		} else {
			data = acc.value16;
		}
	} else {
		data = snes_bus_read(bus, eff_addr.simple_address);
		if(size == 2)
			data += snes_bus_read(bus, eff_addr.simple_address + 1) << 8;
	}
	return data;
}

static void store_data(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, uint16_t data, uint8_t size)
{
	if(eff_addr.type == SNES_ADDRESS_TYPE_ACCUMULATOR) {
		snes_cpu_registers_accumulator_set(registers, data);
	} else {
		snes_bus_write(bus, eff_addr.simple_address, data);
		snes_cpu_registers_update8(registers, data);
		if(size == 2) {
			snes_bus_write(bus, eff_addr.simple_address + 1, data >> 8);
			snes_cpu_registers_update16(registers, data);
		}
	}
}

static void snes_cpu_registers_update16_v(snes_cpu_registers_t *registers, uint16_t value1, uint16_t value2, uint16_t result)
{
	if((value1 & 0x8000) && (value2 & 0x8000) && !(result & 0x8000))
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_V);
	else if (!(value1 & 0x8000) && !(value2 & 0x8000) && (result & 0x8000))
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_V);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_V);
}

static void snes_cpu_registers_update8_v(snes_cpu_registers_t *registers, uint8_t value1, uint8_t value2, uint8_t result)
{
	if((value1 & 0x80) && (value2 & 0x80) && !(result & 0x80))
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_V);
	else if (!(value1 & 0x80) && !(value2 & 0x80) && (result & 0x80))
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_V);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_V);
}

static void snes_cpu_registers_update16_c_add(snes_cpu_registers_t *registers, uint16_t value1, uint16_t value2, uint16_t result)
{
	if(((value1 & 0x8000) || (value2 & 0x8000)) && !(result & 0x8000))
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
}

static void snes_cpu_registers_update8_c_add(snes_cpu_registers_t *registers, uint8_t value1, uint8_t value2, uint16_t result)
{
	if(((value1 & 0x80) || (value2 & 0x80)) && !(result & 0x80))
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
}

//Set N -> negative result; V -> signed overflow; Z -> result is zero; C -> unsigned overflow
static void execute_ADC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = 0;
	uint16_t result16 = 0;
	uint8_t result8 = 0;
	uint8_t is_carry_set = 0;
	if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C))
		is_carry_set = 1;

	value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	//ADD
	if(!snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_D)) {
		//Binary addition
		if(acc.len == CPU_REGISTER_16_BIT) {
			result16 = acc.value16 + value + is_carry_set;
			snes_cpu_registers_update16_c_add(registers, acc.value16, value, result16);
			snes_cpu_registers_update16_v(registers, acc.value16, value, result16);
			snes_cpu_registers_accumulator_set(registers, result16);
		} else {
			result8 = acc.value8_low + (uint8_t)value + is_carry_set;
			snes_cpu_registers_update8_c_add(registers, acc.value8_low, value, result8);
			snes_cpu_registers_update8_v(registers, acc.value8_low, value, result8);
			snes_cpu_registers_accumulator_set(registers, result8);
		}
	} else {
		//BCD is not supported !!
		printf("BCD is not supported !!!\n");
	}
}

//Update N (negative) and Z (zero)
static void execute_AND(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = 0;
	uint16_t result = 0;

	//Get the value
	value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	result = acc.value16 & value;

	//This will set N and Z flag
	snes_cpu_registers_accumulator_set(registers, result);
}

static void execute_ASL(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = 0;
	uint16_t result = 0;
	uint8_t high_bit_set = 0;

	value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	if(acc.len == CPU_REGISTER_8_BIT)
		high_bit_set = value & 0x80 >> 7;
	else
		high_bit_set = value & 0x8000 >> 15;

	result = value << 1;

	store_data(eff_addr, registers, bus, result,  acc.len == CPU_REGISTER_8_BIT? 1:2);

	if(high_bit_set)
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
}

static void execute_BCC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(!snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BCS(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BEQ(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(!snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_Z))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BIT(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value;
	uint16_t result;

	value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	if(acc.len == CPU_REGISTER_16_BIT) {
		snes_cpu_registers_update16(registers, value);
		result = acc.value16 & value;
	} else {
		snes_cpu_registers_update8(registers, value);
		result = acc.value8_low & value;
	}

	if(result)
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_Z);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_Z);
}

static void execute_BMI(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_N))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BNE(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(!snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_Z))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

//BPL
static void execute_BLP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(!snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_N))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BRA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BRK(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
 //Call interrupt, do nothing for now
}

static void execute_BRL(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BVC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(!snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_V))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_BVS(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_V))
		snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_CLC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
}

static void execute_CLD(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_D);
}

static void execute_CLI(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_I);
}

static void execute_CLV(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_V);
}

static void execute_CMP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t data;
	int32_t result32;
	int16_t result16;

	data = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	//Execute sub
	if(acc.len == CPU_REGISTER_16_BIT) {
		result32 = acc.value16 - (int32_t)data;
		snes_cpu_registers_update16(registers, result32);
		if(result32 >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
	} else {
		result16 = acc.value8_low - (int16_t)(uint8_t)data;
		snes_cpu_registers_update8(registers, data);
		if(result16 >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
	}
}

static void execute_COP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	//Enable coproc, must ASSERT
}

static void execute_CPX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	uint16_t data;
	int32_t result32;
	int16_t result16;

	data = fetch_data(eff_addr, registers, bus, x.len == CPU_REGISTER_8_BIT? 1:2);

	//Execute sub
	if(x.len == CPU_REGISTER_16_BIT) {
		result32 = x.value16 - (int32_t)data;
		snes_cpu_registers_update16(registers, result32);
		if(result32 >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
	} else {
		result16 = x.value8_low - (int16_t)(uint8_t)data;
		snes_cpu_registers_update8(registers, data);
		if(result16 >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
	}
}

static void execute_CPY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	uint16_t data;
	int32_t result32;
	int16_t result16;

	data = fetch_data(eff_addr, registers, bus, y.len == CPU_REGISTER_8_BIT? 1:2);

	//Execute sub
	if(y.len == CPU_REGISTER_16_BIT) {
		result32 = y.value16 - (int32_t)data;
		snes_cpu_registers_update16(registers, result32);
		if(result32 >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
	} else {
		result16 = y.value8_low - (int16_t)(uint8_t)data;
		snes_cpu_registers_update8(registers, data);
		if(result16 >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
	}
}

static void execute_DEC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t data = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2) - 1;
	store_data(eff_addr, registers, bus, data, acc.len == CPU_REGISTER_8_BIT? 1:2);
}

static void execute_DEX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	snes_cpu_registers_x_set(registers, x.value16 - 1);
}

static void execute_DEY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	snes_cpu_registers_y_set(registers, y.value16 - 1);
}

static void execute_EOR(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t data;
	uint16_t result;

	data = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	result = data ^ acc.value16;
	snes_cpu_registers_accumulator_set(registers, result);
}

static void execute_INC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t data = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2) + 1;
	store_data(eff_addr, registers, bus, data, acc.len == CPU_REGISTER_8_BIT? 1:2);
}

static void execute_INX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	snes_cpu_registers_x_set(registers, x.value16 + 1);
}

static void execute_INY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	snes_cpu_registers_y_set(registers, y.value16 + 1);
}

static void execute_JMP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
	if(eff_addr.simple_address >> 16)
		snes_cpu_registers_program_bank_set(registers, eff_addr.simple_address >> 16);
}

static void execute_JSR(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t pc = snes_cpu_registers_program_counter_get(registers) - 1;
	snes_cpu_stack_push(stack, (uint8_t)pc >> 8);
	snes_cpu_stack_push(stack, (uint8_t)pc);
	snes_cpu_registers_program_counter_set(registers, eff_addr.simple_address);
}

static void execute_LDA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t data = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);
	snes_cpu_registers_accumulator_set(registers, data);
}

static void execute_LDX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	uint16_t data = fetch_data(eff_addr, registers, bus, x.len == CPU_REGISTER_8_BIT? 1:2);
	snes_cpu_registers_x_set(registers, data);
}

static void execute_LDY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	uint16_t data = fetch_data(eff_addr, registers, bus, y.len == CPU_REGISTER_8_BIT? 1:2);
	snes_cpu_registers_y_set(registers, data);
}

static void execute_LSR(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = 0;
	uint16_t result = 0;

	value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);
	result = value >> 1;
	store_data(eff_addr, registers, bus, result, acc.len == CPU_REGISTER_8_BIT? 1:2);

	if(value & 1)
		snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
	else
		snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
}

static void execute_MVN(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{

}

static void execute_MVP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{

}

static void execute_NOP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	return;
}

static void execute_ORA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = 0;
	uint16_t result = 0;

	//Get the value
	value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	result = acc.value16 | value;

	//This will set N and Z flag
	snes_cpu_registers_accumulator_set(registers, result);
}

static void execute_PEA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t value;
	value = fetch_data(eff_addr, registers, bus, 2);
	snes_cpu_stack_push(stack, value >> 8);
	snes_cpu_stack_push(stack, value); 
}

static void execute_PEI(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t value;
	value = fetch_data(eff_addr, registers, bus, 2);
	snes_cpu_stack_push(stack, value >> 8);
	snes_cpu_stack_push(stack, value); 
}

static void execute_PER(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t value;
	value = fetch_data(eff_addr, registers, bus, 2);
	snes_cpu_stack_push(stack, value >> 8);
	snes_cpu_stack_push(stack, value); 
}

static void execute_PHA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	if(acc.len == CPU_REGISTER_16_BIT)
		snes_cpu_stack_push(stack, acc.value8_high);
	snes_cpu_stack_push(stack, acc.value8_low);
}

static void execute_PHB(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint32_t dbr = snes_cpu_registers_data_bank_get(registers);
	snes_cpu_stack_push(stack,dbr >> 16);
}

static void execute_PHD(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t dpr = snes_cpu_registers_direct_page_get(registers);
	snes_cpu_stack_push(stack, dpr >> 8);
	snes_cpu_stack_push(stack, dpr);
}

static void execute_PHK(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint32_t pbr = snes_cpu_registers_program_bank_get(registers);
	snes_cpu_stack_push(stack,pbr >> 16);
}

static void execute_PHP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint8_t p = snes_cpu_registers_status_flag_get(registers);
	snes_cpu_stack_push(stack,p);
}

static void execute_PHX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	if(x.len == CPU_REGISTER_16_BIT)
		snes_cpu_stack_push(stack, x.value8_high);
	snes_cpu_stack_push(stack, x.value8_low);
}

static void execute_PHY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	if(y.len == CPU_REGISTER_16_BIT)
		snes_cpu_stack_push(stack, y.value8_high);
	snes_cpu_stack_push(stack, y.value8_low);
}

static void execute_PLA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value;
	value = snes_cpu_stack_pull(stack);
	if(acc.len == CPU_REGISTER_16_BIT)
		value += snes_cpu_stack_pull(stack) << 8;
	snes_cpu_registers_accumulator_set(registers, value);
}

static void execute_PLB(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint8_t value = snes_cpu_stack_pull(stack);
	snes_cpu_registers_data_bank_set(registers, value);
}

static void execute_PLD(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t value = snes_cpu_stack_pull(stack);
	value += snes_cpu_stack_pull(stack) << 8;
	snes_cpu_registers_direct_page_set(registers, value);
}

static void execute_PLP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint8_t value = snes_cpu_stack_pull(stack);
	snes_cpu_registers_status_flag_force(registers, value);
}

static void execute_PLX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	uint16_t value = snes_cpu_stack_pull(stack);
	if(x.len == CPU_REGISTER_16_BIT)
		value += snes_cpu_stack_pull(stack) << 8;
	snes_cpu_registers_x_set(registers, value);
}

static void execute_PLY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	uint16_t value = snes_cpu_stack_pull(stack);
	if(y.len == CPU_REGISTER_16_BIT)
		value += snes_cpu_stack_pull(stack) << 8;
	snes_cpu_registers_y_set(registers, value);
}

static void execute_REP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_reset(registers, eff_addr.simple_address);
}

static void execute_ROL(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT ? 1:2);
	if(acc.len == CPU_REGISTER_8_BIT) {
		uint8_t result = (uint8_t)value << 1;
		if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C))
			result++;
		if(value & 0x80)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
		snes_cpu_registers_accumulator_set(registers, result);
	} else {
		uint16_t result = value << 1;
		if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C))
			result++;
		if(value & 0x8000)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
		snes_cpu_registers_accumulator_set(registers, result);
	}
}

static void execute_ROR(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT ? 1:2);
	uint8_t c_wasset = 0;
	if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C))
			c_wasset = 1;
	if(acc.len == CPU_REGISTER_8_BIT) {
		uint8_t result = (uint8_t)value >> 1;
		if(value & 0x80)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
		if(c_wasset)
			result += 0x80;
		snes_cpu_registers_accumulator_set(registers, result);
	} else {
		uint16_t result = value >> 1;
		if(value & 0x8000)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);
		if(c_wasset)
			result += 0x8000;
		snes_cpu_registers_accumulator_set(registers, result);
	}
}

static void execute_RTI(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t pc;
	uint8_t pbr;
	uint8_t p;

	pc = snes_cpu_stack_pull(stack);
	pc += snes_cpu_stack_pull(stack) << 8;
	pbr = snes_cpu_stack_pull(stack);
	p = snes_cpu_stack_pull(stack);

	snes_cpu_registers_program_counter_set(registers, pc);
	snes_cpu_registers_program_bank_set(registers, pbr);
	snes_cpu_registers_status_flag_force(registers, p);
}

static void execute_RTL(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t pc;
	uint8_t pbr;
	pc = snes_cpu_stack_pull(stack);
	pc += snes_cpu_stack_pull(stack) << 8;
	pc++;
	pbr = snes_cpu_stack_pull(stack);
	snes_cpu_registers_program_counter_set(registers, pc);
	snes_cpu_registers_program_bank_set(registers, pbr);
}

static void execute_RTS(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t pc;
	pc = snes_cpu_stack_pull(stack);
	pc += snes_cpu_stack_pull(stack) << 8;
	pc++;
	snes_cpu_registers_program_counter_set(registers, pc);
}

static void execute_SBC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t data;
	int8_t carry = 0;
	if(snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C))
		carry = 1;

	data = fetch_data(eff_addr, registers, bus, acc.len == CPU_REGISTER_8_BIT? 1:2);

	if(acc.len == CPU_REGISTER_16_BIT) {
		int32_t result = (int32_t)acc.value16 - (int32_t)data + (int32_t)carry -1;
		snes_cpu_registers_accumulator_set(registers, result);

		if(result >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);

		if ((acc.value16 ^ data) & (acc.value16 ^ (uint16_t) result) & 0x8000)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_V);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_V);

	} else {
		int16_t result = (int16_t)acc.value8_low - (int16_t)data + (int16_t)carry -1;
		if(result >= 0)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_C);

		if ((acc.value8_low ^ data) & (acc.value8_low ^ (uint16_t) result) & 0x8000)
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_V);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_V);
	}
}

static void execute_SEC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_C);
}

static void execute_SED(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_D);
}

static void execute_SEI(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_I);
}

static void execute_SEP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_status_flag_set(registers, eff_addr.simple_address);
}

static void execute_STA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	snes_bus_write(bus, eff_addr.simple_address, acc.value8_low);
	if(acc.len == CPU_REGISTER_16_BIT)
		snes_bus_write(bus, eff_addr.simple_address + 1, acc.value8_high);
}

static void execute_STP(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint8_t value = snes_cpu_registers_status_flag_get(registers);
	snes_bus_write(bus, eff_addr.simple_address, value);
}

static void execute_STX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	snes_bus_write(bus, eff_addr.simple_address, x.value8_low);
	if(x.len == CPU_REGISTER_16_BIT)
		snes_bus_write(bus, eff_addr.simple_address + 1, x.value8_high);
}

static void execute_STY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	snes_bus_write(bus, eff_addr.simple_address, y.value8_low);
	if(y.len == CPU_REGISTER_16_BIT)
		snes_bus_write(bus, eff_addr.simple_address + 1, y.value8_high);
}

static void execute_STZ(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_bus_write(bus, eff_addr.simple_address, 0);
	if(!snes_cpu_registers_emulation_isset(registers) && !snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_M))
		snes_bus_write(bus, eff_addr.simple_address + 1, 0);
}

static void execute_TAX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	if(x.len == CPU_REGISTER_16_BIT)
		snes_cpu_registers_x_set(registers, acc.value16);
	else
		snes_cpu_registers_x_set(registers, acc.value8_low);
}

static void execute_TAY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	if(y.len == CPU_REGISTER_16_BIT)
		snes_cpu_registers_y_set(registers, acc.value16);
	else
		snes_cpu_registers_y_set(registers, acc.value8_low);
}

static void execute_TCD(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	snes_cpu_registers_direct_page_set(registers, acc.value16);
}

static void execute_TCS(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	snes_cpu_registers_stack_pointer_set(registers, acc.value16);
}

static void execute_TDC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	snes_cpu_registers_accumulator_set16(registers, snes_cpu_registers_direct_page_get(registers));
}

static void execute_TRB(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t value = snes_bus_read(bus, eff_addr.simple_address);
	uint16_t result = 0;
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);

	//First test and store
	if(acc.len == CPU_REGISTER_16_BIT)
		value += snes_bus_read(bus, eff_addr.simple_address + 1) << 8;

	result = value & ~acc.value16;

	snes_bus_write(bus, eff_addr.simple_address, (uint8_t) result);
	if(acc.len == CPU_REGISTER_16_BIT)
		snes_bus_write(bus, eff_addr.simple_address + 1, (uint8_t) (result >> 8));

	//Second test and set Z flag
	if(acc.len == CPU_REGISTER_8_BIT) {
		if(!(value & acc.value8_low))
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_Z);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_Z);
	} else {
		if(!(value & acc.value16))
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_Z);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_Z);
	}
}

static void execute_TSB(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint16_t value = snes_bus_read(bus, eff_addr.simple_address);
	uint16_t result = 0;
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);

	//First test and store
	if(acc.len == CPU_REGISTER_16_BIT)
		value += snes_bus_read(bus, eff_addr.simple_address + 1) << 8;

	result = value | acc.value16;

	snes_bus_write(bus, eff_addr.simple_address, (uint8_t) result);
	if(acc.len == CPU_REGISTER_16_BIT)
		snes_bus_write(bus, eff_addr.simple_address + 1, (uint8_t) (result >> 8));

	//Second test and set Z flag
	if(acc.len == CPU_REGISTER_8_BIT) {
		if(!(value & acc.value8_low))
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_Z);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_Z);
	} else {
		if(!(value & acc.value16))
			snes_cpu_registers_status_flag_set(registers, STATUS_FLAG_Z);
		else
			snes_cpu_registers_status_flag_reset(registers, STATUS_FLAG_Z);
	}
}

static void execute_TSC(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value sp = snes_cpu_registers_stack_pointer_get(registers);
	snes_cpu_registers_accumulator_set16(registers, sp.value16);
}

static void execute_TSX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value sp = snes_cpu_registers_stack_pointer_get(registers);
	snes_cpu_registers_x_set(registers, sp.value16);
}

static void execute_TXA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	snes_cpu_registers_accumulator_set(registers, x.value16);
}

static void execute_TXS(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	snes_cpu_registers_stack_pointer_set(registers, x.value16);
}

static void execute_TXY(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	snes_cpu_registers_y_set(registers, x.value16);
}

static void execute_TYA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	snes_cpu_registers_accumulator_set(registers, y.value16);
}

static void execute_TYX(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	snes_cpu_registers_x_set(registers, y.value16);
}

static void execute_WAI(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	//wait for interrupt, must assert
}

static void execute_WDM(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	//reserved, must assert
}

static void execute_XBA(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);
	uint16_t value = acc.value8_high;
	value += acc.value8_low << 8;
	snes_cpu_registers_accumulator_set16(registers, value);
	snes_cpu_registers_update8(registers, acc.value8_high);
}

static void execute_XCE(struct snes_effective_address eff_addr, snes_cpu_registers_t *registers, snes_bus_t *bus, snes_cpu_stack_t *stack)
{
	uint8_t carry = snes_cpu_registers_status_flag_isset(registers, STATUS_FLAG_C);
	uint8_t emu = snes_cpu_registers_emulation_isset(registers);
	if(carry != emu) {
		if(carry)
			snes_cpu_registers_emulation_set(registers);
		else
			snes_cpu_registers_emulation_reset(registers);
	}
}

void snes_cpu_mne_execute(snes_cpu_mnemonic_t mne, struct snes_effective_address eff_addr, snes_cpu_t *cpu)
{
	snes_cpu_registers_t *registers = snes_cpu_get_registers(cpu);
	snes_bus_t *bus = snes_cpu_get_bus(cpu);
	snes_cpu_stack_t *stack = snes_cpu_get_stack(cpu);

	switch(mne){
		case ADC:
			return execute_ADC(eff_addr, registers, bus, stack);
		case AND:
			return execute_AND(eff_addr, registers, bus, stack);
		case ASL:
			return execute_ASL(eff_addr, registers, bus, stack);
		case BCC:
			return execute_BCC(eff_addr, registers, bus, stack);
		case BCS:
			return execute_BCS(eff_addr, registers, bus, stack);
		case BEQ:
			return execute_BEQ(eff_addr, registers, bus, stack);
		case BIT:
			return execute_BIT(eff_addr, registers, bus, stack);
		case BMI:
			return execute_BMI(eff_addr, registers, bus, stack);
		case BNE:
			return execute_BNE(eff_addr, registers, bus, stack);
		case BLP:
			return execute_BLP(eff_addr, registers, bus, stack);
		case BRA:
			return execute_BRA(eff_addr, registers, bus, stack);
		case BRK:
			return execute_BRK(eff_addr, registers, bus, stack);
		case BRL:
			return execute_BRL(eff_addr, registers, bus, stack);
		case BVC:
			return execute_BVC(eff_addr, registers, bus, stack);
		case BVS:
			return execute_BVS(eff_addr, registers, bus, stack);
		case CLC:
			return execute_CLC(eff_addr, registers, bus, stack);
		case CLD:
			return execute_CLD(eff_addr, registers, bus, stack);
		case CLI:
			return execute_CLI(eff_addr, registers, bus, stack);
		case CLV:
			return execute_CLV(eff_addr, registers, bus, stack);
		case CMP:
			return execute_CMP(eff_addr, registers, bus, stack);
		case COP:
			return execute_COP(eff_addr, registers, bus, stack);
		case CPX:
			return execute_CPX(eff_addr, registers, bus, stack);
		case CPY:
			return execute_CPY(eff_addr, registers, bus, stack);
		case DEC:
			return execute_DEC(eff_addr, registers, bus, stack);
		case DEX:
			return execute_DEX(eff_addr, registers, bus, stack);
		case DEY:
			return execute_DEY(eff_addr, registers, bus, stack);
		case EOR:
			return execute_EOR(eff_addr, registers, bus, stack);
		case INC:
			return execute_INC(eff_addr, registers, bus, stack);
		case INX:
			return execute_INX(eff_addr, registers, bus, stack);
		case INY:
			return execute_INY(eff_addr, registers, bus, stack);
		case JMP:
			return execute_JMP(eff_addr, registers, bus, stack);
		case JSR:
			return execute_JSR(eff_addr, registers, bus, stack);
		case LDA:
			return execute_LDA(eff_addr, registers, bus, stack);
		case LDX:
			return execute_LDX(eff_addr, registers, bus, stack);
		case LDY:
			return execute_LDY(eff_addr, registers, bus, stack);
		case LSR:
			return execute_LSR(eff_addr, registers, bus, stack);
		case MVN:
			return execute_MVN(eff_addr, registers, bus, stack);
		case MVP:
			return execute_MVP(eff_addr, registers, bus, stack);
		case NOP:
			return execute_NOP(eff_addr, registers, bus, stack);
		case ORA:
			return execute_ORA(eff_addr, registers, bus, stack);
		case PEA:
			return execute_PEA(eff_addr, registers, bus, stack);
		case PEI:
			return execute_PEI(eff_addr, registers, bus, stack);
		case PER:
			return execute_PER(eff_addr, registers, bus, stack);
		case PHA:
			return execute_PHA(eff_addr, registers, bus, stack);
		case PHB:
			return execute_PHB(eff_addr, registers, bus, stack);
		case PHD:
			return execute_PHD(eff_addr, registers, bus, stack);
		case PHK:
			return execute_PHK(eff_addr, registers, bus, stack);
		case PHP:
			return execute_PHP(eff_addr, registers, bus, stack);
		case PHX:
			return execute_PHX(eff_addr, registers, bus, stack);
		case PHY:
			return execute_PHY(eff_addr, registers, bus, stack);
		case PLA:
			return execute_PLA(eff_addr, registers, bus, stack);
		case PLB:
			return execute_PLB(eff_addr, registers, bus, stack);
		case PLD:
			return execute_PLD(eff_addr, registers, bus, stack);
		case PLP:
			return execute_PLP(eff_addr, registers, bus, stack);
		case PLX:
			return execute_PLX(eff_addr, registers, bus, stack);
		case PLY:
			return execute_PLY(eff_addr, registers, bus, stack);
		case REP:
			return execute_REP(eff_addr, registers, bus, stack);
		case ROL:
			return execute_ROL(eff_addr, registers, bus, stack);
		case ROR:
			return execute_ROR(eff_addr, registers, bus, stack);
		case RTI:
			return execute_RTI(eff_addr, registers, bus, stack);
		case RTL:
			return execute_RTL(eff_addr, registers, bus, stack);
		case RTS:
			return execute_RTS(eff_addr, registers, bus, stack);
		case SBC:
			return execute_SBC(eff_addr, registers, bus, stack);
		case SEC:
			return execute_SEC(eff_addr, registers, bus, stack);
		case SED:
			return execute_SED(eff_addr, registers, bus, stack);
		case SEI:
			return execute_SEI(eff_addr, registers, bus, stack);
		case SEP:
			return execute_SEP(eff_addr, registers, bus, stack);
		case STA:
			return execute_STA(eff_addr, registers, bus, stack);
		case STP:
			return execute_STP(eff_addr, registers, bus, stack);
		case STX:
			return execute_STX(eff_addr, registers, bus, stack);
		case STY:
			return execute_STY(eff_addr, registers, bus, stack);
		case STZ:
			return execute_STZ(eff_addr, registers, bus, stack);
		case TAX:
			return execute_TAX(eff_addr, registers, bus, stack);
		case TAY:
			return execute_TAY(eff_addr, registers, bus, stack);
		case TCD:
			return execute_TCD(eff_addr, registers, bus, stack);
		case TCS:
			return execute_TCS(eff_addr, registers, bus, stack);
		case TDC:
			return execute_TDC(eff_addr, registers, bus, stack);
		case TRB:
			return execute_TRB(eff_addr, registers, bus, stack);
		case TSB:
			return execute_TSB(eff_addr, registers, bus, stack);
		case TSC:
			return execute_TSC(eff_addr, registers, bus, stack);
		case TSX:
			return execute_TSX(eff_addr, registers, bus, stack);
		case TXA:
			return execute_TXA(eff_addr, registers, bus, stack);
		case TXS:
			return execute_TXS(eff_addr, registers, bus, stack);
		case TXY:
			return execute_TXY(eff_addr, registers, bus, stack);
		case TYA:
			return execute_TYA(eff_addr, registers, bus, stack);
		case TYX:
			return execute_TYX(eff_addr, registers, bus, stack);
		case WAI:
			return execute_WAI(eff_addr, registers, bus, stack);
		case WDM:
			return execute_WDM(eff_addr, registers, bus, stack);
		case XBA:
			return execute_XBA(eff_addr, registers, bus, stack);
		case XCE:
			return execute_XCE(eff_addr, registers, bus, stack);
		default:
			return;
	}
}
