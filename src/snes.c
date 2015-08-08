#include <stdlib.h>
#include <stdio.h>

#include "snes.h"
#include "snes_cart.h"
#include "snes_bus.h"
#include "snes_cpu.h"
#include "snes_ram.h"
#include "snes_apu.h"

struct _snes {
	snes_cart_t *cart;
	snes_bus_t *bus_a;
	snes_cpu_t *cpu;
	snes_ram_t *wram;
	snes_apu_t *apu;
};

snes_t *snes_power_up(snes_cart_t *cart)
{
	snes_t *snes = malloc(sizeof(snes_t));

	snes->cart = cart;

	snes->wram = snes_ram_init(128*1024);
	if(snes->wram == NULL) {
		printf("Unable to init wram !\n");
		goto error_wram;
	}

	snes->apu = snes_apu_power_up();
	if(snes->apu == NULL) {
		printf("Unable to power up apu!\n");
		goto error_apu;
	}

	snes->bus_a = snes_bus_power_up(cart, snes->wram, snes->apu);
	if(snes->bus_a == NULL) {
		printf("Unable to power up bus_a !\n");
		goto error_bus_a;
	}

	snes->cpu = snes_cpu_power_up(cart,snes->bus_a);
	if(snes->cpu == NULL) {
		printf("Unable to power cpu !\n");
		goto error_cpu;
	}
	return snes;

error_cpu:
	snes_bus_power_down(snes->bus_a);
error_bus_a:
	snes_apu_power_down(snes->apu);
error_apu:
	snes_ram_destroy(snes->wram);
error_wram:
	return NULL;
}

void snes_power_down(snes_t *snes)
{
	snes_cpu_power_down(snes->cpu);
	snes_apu_power_down(snes->apu);
	snes_bus_power_down(snes->bus_a);
	snes_ram_destroy(snes->wram);
	free(snes);
}

void tick(snes_t *snes)
{
	usleep(500);
	snes_cpu_next_op(snes->cpu);
}

void nmi(snes_t *snes)
{
	snes_cpu_nmi(snes->cpu);
}
