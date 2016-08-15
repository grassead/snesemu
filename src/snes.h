#ifndef SNES_H
#define SNES_H

#include "snes_cart.h"

typedef struct _snes snes_t;

typedef enum _snes_breakpoint_type {
	SNES_BREAKPOINT_TYPE_CPU,
}snes_breakpoint_type_t;

snes_t *snes_init(snes_cart_t *cart);
void snes_destroy(snes_t *snes);

int snes_power_up(snes_t *snes);
void snes_power_down(snes_t *snes);

void snes_set_breakpoint(snes_t *snes,
						 snes_breakpoint_type_t breakpoint_type,
						 uint32_t addr);

void snes_do_cpu_tick(snes_t *snes);
void snes_run_cpu(snes_t *snes);

void nmi(snes_t *snes);


#endif //SNES_H
