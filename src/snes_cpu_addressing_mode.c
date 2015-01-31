#include "snes_cpu_addressing_mode.h"
#include <string.h>

enum _snes_cpu_mnemonic_type {
	LOCATES_DATA,
	TRANSFER_CONTROL,
};

static enum _snes_cpu_mnemonic_type snes_cpu_mnemonic_get_type(snes_cpu_mnemonic_t mne)
{
	switch(mne) {
		case JMP:
		case JSR:
			return TRANSFER_CONTROL;
		default:
			return LOCATES_DATA;
	}
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute(snes_cpu_registers_t *registers, snes_cpu_mnemonic_t mne, uint16_t addr)
{
	struct snes_effective_address eff_addr;
	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = addr;
	switch(snes_cpu_mnemonic_get_type(mne)) {
		case TRANSFER_CONTROL:
			eff_addr.simple_address += snes_cpu_registers_program_bank_get(registers);
			break;
		default:
			eff_addr.simple_address += snes_cpu_registers_data_bank_get(registers);
			break;
	}

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute_idx_x(snes_cpu_registers_t *registers, uint16_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = addr + snes_cpu_registers_data_bank_get(registers);

	if(x.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += x.value8_low;
	else
		eff_addr.simple_address += x.value16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute_idx_y(snes_cpu_registers_t *registers, uint16_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = addr + (snes_cpu_registers_data_bank_get(registers));

	if(y.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += y.value8_low;
	else
		eff_addr.simple_address += y.value16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute_idx_ind(snes_bus_t *bus, snes_cpu_registers_t *registers, uint16_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	uint32_t ind_addr = addr;

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = snes_cpu_registers_program_bank_get(registers);

	if(x.len == CPU_REGISTER_8_BIT)
		ind_addr += x.value8_low;
	else
		ind_addr += x.value16;

	ind_addr += snes_cpu_registers_program_bank_get(registers);

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += (snes_bus_read(bus, ind_addr + 1) << 8);

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute_ind(snes_bus_t *bus, snes_cpu_registers_t *registers, uint16_t ind_addr)
{
	struct snes_effective_address eff_addr;
	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;

	eff_addr.simple_address = snes_cpu_registers_program_bank_get(registers);

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += (snes_bus_read(bus, ind_addr + 1) << 8);

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute_ind_long(snes_bus_t *bus, uint16_t ind_addr)
{
	struct snes_effective_address eff_addr;
	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += (snes_bus_read(bus, ind_addr + 1) << 8);
	eff_addr.simple_address += (snes_bus_read(bus, ind_addr + 2) << 16);

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute_long(uint32_t addr)
{
	struct snes_effective_address eff_addr;
	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = addr;
	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_absolute_long_idx_x(snes_cpu_registers_t *registers, uint32_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = addr;

	if(x.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += x.value8_low;
	else
		eff_addr.simple_address += x.value16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_acc(snes_cpu_registers_t *registers)
{
	struct snes_effective_address eff_addr;

	eff_addr.type = SNES_ADDRESS_TYPE_ACCUMULATOR;
	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_block_move(snes_cpu_registers_t *registers, uint16_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	struct snes_cpu_register_value acc = snes_cpu_registers_accumulator_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_BLOCK_MOVE;

	if(x.len == CPU_REGISTER_8_BIT)
		eff_addr.src = x.value8_low;
	else
		eff_addr.src = x.value16;

	if(y.len == CPU_REGISTER_8_BIT)
		eff_addr.dest = y.value8_low;
	else
		eff_addr.dest = y.value16;

	eff_addr.count = acc.value16 + 1;

	//Specific method should be called
	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page(snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = addr + snes_cpu_registers_direct_page_get(registers);

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page_idx_x(snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);

	eff_addr = snes_cpu_addressing_mode_resolve_direct_page(registers, addr);
	if(x.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += x.value8_low;
	else
		eff_addr.simple_address += x.value16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page_idx_y(snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);

	eff_addr = snes_cpu_addressing_mode_resolve_direct_page(registers, addr);
	if(y.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += y.value8_low;
	else
		eff_addr.simple_address += y.value16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page_idx_ind_x(snes_bus_t *bus, snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	uint32_t ind_addr;
	struct snes_cpu_register_value x = snes_cpu_registers_x_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = snes_cpu_registers_data_bank_get(registers);

	ind_addr = snes_cpu_registers_direct_page_get(registers);
	
	if(x.len == CPU_REGISTER_8_BIT)
		ind_addr += x.value8_low;
	else
		ind_addr += x.value16;

	ind_addr += addr;

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 1) << 8;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page_ind(snes_bus_t *bus, snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	uint32_t ind_addr = addr + snes_cpu_registers_direct_page_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = snes_cpu_registers_data_bank_get(registers);

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 1) << 8;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page_ind_long(snes_bus_t *bus, snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	uint32_t ind_addr = addr + snes_cpu_registers_direct_page_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 1) << 8;
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 2) << 16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page_ind_idx_y(snes_bus_t *bus, snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	uint32_t ind_addr = addr + snes_cpu_registers_direct_page_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = snes_cpu_registers_data_bank_get(registers);
	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 1) << 8;

	if(y.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += y.value8_low;
	else
		eff_addr.simple_address += y.value16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_direct_page_ind_long_idx_y(snes_bus_t *bus, snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	uint32_t ind_addr = addr + snes_cpu_registers_direct_page_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = snes_cpu_registers_data_bank_get(registers);

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 1) << 8;
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 2) << 16;

	if(y.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += y.value8_low;
	else
		eff_addr.simple_address += y.value16;

	return eff_addr;
}
static struct snes_effective_address snes_cpu_addressing_mode_resolve_immediate(uint16_t addr)
{
	struct snes_effective_address eff_addr;
	eff_addr.type = SNES_ADDRESS_TYPE_DATA;
	eff_addr.simple_address = addr;
	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_program_counter_relative(snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	int8_t i8offset = (int8_t) addr;
	int16_t offset = (int16_t) i8offset;

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = snes_cpu_registers_program_counter_get(registers);
	eff_addr.simple_address += snes_cpu_registers_program_bank_get(registers);
	eff_addr.simple_address += offset;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_program_counter_relative_long(snes_cpu_registers_t *registers, uint16_t addr)
{
	struct snes_effective_address eff_addr;
	int16_t offset = (int16_t) addr;

	eff_addr.type = SNES_ADDRESS_TYPE_SIMPLE;
	eff_addr.simple_address = snes_cpu_registers_program_counter_get(registers);
	eff_addr.simple_address += snes_cpu_registers_program_bank_get(registers);
	eff_addr.simple_address += offset;

	return eff_addr;
}

/*Stack mode*/
static struct snes_effective_address snes_cpu_addressing_mode_resolve_stack_relative_absolute(uint16_t addr)
{
	struct snes_effective_address eff_addr;

	eff_addr.type = SNES_ADDRESS_TYPE_STACK;
	eff_addr.simple_address = addr;
	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_stack_relative_direct_page_ind(snes_bus_t *bus, snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;

	eff_addr = snes_cpu_addressing_mode_resolve_direct_page_ind(bus, registers, addr);
	eff_addr.type = SNES_ADDRESS_TYPE_STACK;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_stack_relative_program_counter_relative_long(snes_cpu_registers_t *registers, uint32_t addr)
{
	struct snes_effective_address eff_addr;
	eff_addr = snes_cpu_addressing_mode_resolve_program_counter_relative_long(registers, addr);
	eff_addr.type = SNES_ADDRESS_TYPE_STACK;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_stack_relative(snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value sp = snes_cpu_registers_stack_pointer_get(registers);

	eff_addr.type = SNES_ADDRESS_TYPE_STACK;
	eff_addr.simple_address = addr + sp.value16;

	return eff_addr;
}

static struct snes_effective_address snes_cpu_addressing_mode_resolve_stack_relative_ind_idx_y(snes_bus_t *bus, snes_cpu_registers_t *registers, uint8_t addr)
{
	struct snes_effective_address eff_addr;
	struct snes_cpu_register_value sp = snes_cpu_registers_stack_pointer_get(registers);
	struct snes_cpu_register_value y = snes_cpu_registers_y_get(registers);
	uint32_t ind_addr = addr + sp.value16;

	eff_addr.type = SNES_ADDRESS_TYPE_STACK;
	eff_addr.simple_address = snes_cpu_registers_data_bank_get(registers);

	eff_addr.simple_address += snes_bus_read(bus, ind_addr);
	eff_addr.simple_address += snes_bus_read(bus, ind_addr + 1) << 8;

	if(y.len == CPU_REGISTER_8_BIT)
		eff_addr.simple_address += y.value8_low;
	else
		eff_addr.simple_address += y.value16;

	return eff_addr;
}

struct snes_effective_address snes_cpu_addressing_mode_decode(snes_bus_t *bus,  snes_cpu_registers_t *registers, snes_cpu_mnemonic_t mne, snes_cpu_addressing_mode_t mode, uint32_t addr)
{
	switch(mode) {
		case Absolute:
			return snes_cpu_addressing_mode_resolve_absolute(registers, mne, addr);
		case AbsoluteIndexedX:
			return snes_cpu_addressing_mode_resolve_absolute_idx_x(registers, addr);
		case AbsoluteIndexedY:
			return snes_cpu_addressing_mode_resolve_absolute_idx_y(registers, addr);
		case AbsoluteIndexedIndirect:
			return snes_cpu_addressing_mode_resolve_absolute_idx_ind(bus, registers, addr);
		case AbsoluteIndirect:
			return snes_cpu_addressing_mode_resolve_absolute_ind(bus, registers, addr);
		case AbsoluteIndirectLong:
			return snes_cpu_addressing_mode_resolve_absolute_ind_long(bus, addr);
		case AbsoluteLong:
			return snes_cpu_addressing_mode_resolve_absolute_long(addr);
		case AbsoluteLongIndexedX:
			return snes_cpu_addressing_mode_resolve_absolute_long_idx_x(registers, addr);
		case Accumulator:
			return snes_cpu_addressing_mode_resolve_acc(registers);
		case BlockMove:
			return snes_cpu_addressing_mode_resolve_block_move(registers, addr);
		case DirectPage:
			return snes_cpu_addressing_mode_resolve_direct_page(registers, addr);
		case DirectPageIndexedX:
			return snes_cpu_addressing_mode_resolve_direct_page_idx_x(registers, addr);
		case DirectPageIndexedY:
			return snes_cpu_addressing_mode_resolve_direct_page_idx_y(registers, addr);
		case DirectPageIndexedIndirectX:
			return snes_cpu_addressing_mode_resolve_direct_page_idx_ind_x(bus, registers, addr);
		case DirectPageIndirect:
			return snes_cpu_addressing_mode_resolve_direct_page_ind(bus, registers, addr);
		case DirectPageIndirectLong:
			return snes_cpu_addressing_mode_resolve_direct_page_ind_long(bus, registers, addr);
		case DirectPageIndirectIndexedY:
			return snes_cpu_addressing_mode_resolve_direct_page_ind_idx_y(bus, registers, addr);
		case DirectPageIndirectLongIndexedY:
			return snes_cpu_addressing_mode_resolve_direct_page_ind_long_idx_y(bus, registers, addr);
		case Immediate:
			return snes_cpu_addressing_mode_resolve_immediate(addr);
		case ProgramCounterRelative:
			return snes_cpu_addressing_mode_resolve_program_counter_relative(registers, addr);
		case ProgramCounterRelativeLong:
			return snes_cpu_addressing_mode_resolve_program_counter_relative_long(registers, addr);
		case StackAbsolute:
			return snes_cpu_addressing_mode_resolve_stack_relative_absolute(addr);
		case StackDirectPageIndirect:
			return snes_cpu_addressing_mode_resolve_stack_relative_direct_page_ind(bus, registers, addr);
		case StackProgramCounterRelativeLong:
			return snes_cpu_addressing_mode_resolve_stack_relative_program_counter_relative_long(registers, addr);
		case StackRelative:
			return snes_cpu_addressing_mode_resolve_stack_relative(registers, addr);
		case StackRelativeIndirectIndexedY:
			return snes_cpu_addressing_mode_resolve_stack_relative_ind_idx_y(bus, registers, addr);

		case Implied:
		case StackInterrupt:
		case StackPull:
		case StackPush:
		case StackRTI:
		case StackRTL:
		case StackRTS:
		default:
		{
			struct snes_effective_address eff_addr;
			memset(&eff_addr, 0, sizeof(struct snes_effective_address));
			return eff_addr;
		}
	}
}
