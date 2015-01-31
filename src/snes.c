#include <stdlib.h>
#include "snes.h"
#include "snes_cart.h"
#include "snes_bus.h"
#include "snes_cpu.h"
#include "snes_ram.h"

struct _snes {
	snes_cart_t *cart;
	snes_bus_t *bus_a;
	snes_cpu_t *cpu;
	snes_ram_t *wram;
};

snes_t *snes_power_up(snes_cart_t *cart)
{
	snes_t *snes = malloc(sizeof(snes_t));
	snes->cart = cart;
	snes->wram = snes_ram_init(128*1024);
	snes->bus_a = snes_bus_power_up(cart,snes->wram);
	snes->cpu = snes_cpu_power_up(cart,snes->bus_a);
	return snes;
}

void snes_power_down(snes_t *snes)
{
	snes_cpu_power_down(snes->cpu);
	snes_bus_power_down(snes->bus_a);
	snes_ram_destroy(snes->wram);
	free(snes);
}

void tick(snes_t *snes)
{
	snes_cpu_next_op(snes->cpu);
}

void nmi(snes_t *snes)
{
	snes_cpu_nmi(snes->cpu);
}
