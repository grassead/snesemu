#include <stdlib.h>
#include "snes_cpu_stack.h"

struct _snes_cpu_stack {
	snes_cpu_registers_t *registers;
	snes_bus_t *bus;
};

snes_cpu_stack_t *snes_cpu_stack_init(snes_cpu_registers_t *registers, snes_bus_t *bus)
{
	snes_cpu_stack_t *stack = malloc(sizeof(snes_cpu_stack_t));
	stack->registers = registers;
	stack->bus = bus;
	return stack;
}

void snes_cpu_stack_destroy(snes_cpu_stack_t *stack)
{
	free(stack);
}

void snes_cpu_stack_push(snes_cpu_stack_t *stack, uint8_t value)
{
	struct snes_cpu_register_value sp = snes_cpu_registers_stack_pointer_get(stack->registers);
	snes_bus_write(stack->bus, sp.value16, value);
	snes_cpu_registers_stack_pointer_set(stack->registers, sp.value16 - 1);
}

uint8_t snes_cpu_stack_pull(snes_cpu_stack_t *stack)
{
	struct snes_cpu_register_value sp = snes_cpu_registers_stack_pointer_get(stack->registers);
	snes_cpu_registers_stack_pointer_set(stack->registers, sp.value16 + 1);
	return snes_bus_read(stack->bus, sp.value16 + 1);
}
