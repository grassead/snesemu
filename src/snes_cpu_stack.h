#ifndef SNES_CPU_STACK_H
#define SNES_CPU_STACK_H

#include <stdint.h>
#include "snes_cpu_registers.h"
#include "snes_bus.h"

typedef struct _snes_cpu_stack snes_cpu_stack_t;


snes_cpu_stack_t *snes_cpu_stack_init(snes_cpu_registers_t *registers, snes_bus_t *bus);
void snes_cpu_stack_destroy(snes_cpu_stack_t *stack);

void snes_cpu_stack_push(snes_cpu_stack_t *stack, uint8_t value);
uint8_t snes_cpu_stack_pull(snes_cpu_stack_t *stack);


#endif //SNES_CPU_STACK_H
