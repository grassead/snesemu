#ifndef SNES_CPU_H
#define SNES_CPU_H

#include "snes_cart.h"
#include "snes_bus.h"
#include "snes_cpu_registers.h"
#include "snes_cpu_stack.h"
#include "snes_bus.h"

typedef struct _snes_cpu snes_cpu_t;

typedef enum {
	SNES_CPU_EXECUTION_MODE_STOP = 0,
	SNES_CPU_EXECUTION_MODE_STEP,
	SNES_CPU_EXECUTION_MODE_RUN,
	SNES_CPU_EXECUTION_MODE_UNKNOWN,
} snes_cpu_execution_mode;

snes_cpu_t *snes_cpu_init(snes_cart_t *cart, snes_bus_t *bus);
void snes_cpu_destroy(snes_cpu_t *cpu);

int snes_cpu_power_up(snes_cpu_t *cpu);
void snes_cpu_power_down(snes_cpu_t *cpu);

snes_cpu_registers_t *snes_cpu_get_registers(snes_cpu_t *cpu);
snes_cpu_stack_t *snes_cpu_get_stack(snes_cpu_t *cpu);
snes_bus_t *snes_cpu_get_bus(snes_cpu_t *cpu);

int snes_cpu_set_breakpoint(snes_cpu_t *cpu, uint32_t addr);
void snes_cpu_set_execution_mode(snes_cpu_t *cpu, snes_cpu_execution_mode mode);

#endif //SNES_BUS_H
