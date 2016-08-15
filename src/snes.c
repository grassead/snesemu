#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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

snes_t *snes_init(snes_cart_t *cart)
{
	snes_t *snes = malloc(sizeof(snes_t));

	snes->cart = cart;

	snes->wram = snes_ram_init(128*1024);
	if(snes->wram == NULL) {
		printf("Unable to init wram !\n");
		goto error_wram;
	}

	snes->apu = snes_apu_init();
	if(snes->apu == NULL) {
		printf("Unable to init apu!\n");
		goto error_apu;
	}

	snes->bus_a = snes_bus_init(cart, snes->wram, snes->apu);
	if(snes->bus_a == NULL) {
		printf("Unable to init bus_a !\n");
		goto error_bus_a;
	}

	snes->cpu = snes_cpu_init(cart,snes->bus_a);
	if(snes->cpu == NULL) {
		printf("Unable to init cpu !\n");
		goto error_cpu;
	}
	return snes;

error_cpu:
	snes_bus_destroy(snes->bus_a);
error_bus_a:
	snes_apu_destroy(snes->apu);
error_apu:
	snes_ram_destroy(snes->wram);
error_wram:
	free(snes);
	snes = NULL;
	return NULL;
}

void snes_destroy(snes_t *snes)
{
	snes_power_down(snes);
	snes_cpu_destroy(snes->cpu);
	snes_apu_destroy(snes->apu);
	snes_bus_destroy(snes->bus_a);
	snes_ram_destroy(snes->wram);
	free(snes);
}

int snes_power_up(snes_t *snes)
{
	int ret = 0;

	ret = snes_cpu_power_up(snes->cpu);
	if(ret < 0) {
		goto error_cpu;
	}

	ret = snes_apu_power_up(snes->apu);
	if(ret < 0) {
		goto error_apu;
	}

	return ret;

error_apu:
	snes_cpu_power_down(snes->cpu);
error_cpu:
	return ret;
}

void snes_power_down(snes_t *snes)
{
	snes_cpu_power_down(snes->cpu);
	printf("CPU down\n");
	snes_apu_power_down(snes->apu);
	printf("APU down\n");
}

void snes_set_breakpoint(snes_t *snes, snes_breakpoint_type_t breakpoint_type, uint32_t addr)
{
	switch (breakpoint_type) {
		case SNES_BREAKPOINT_TYPE_CPU:
			snes_cpu_set_breakpoint(snes->cpu, addr);
			break;
		default:
			printf("Breakpoint type invalid !\n");
			assert(0);
	}
}

void snes_do_cpu_tick(snes_t *snes)
{
	snes_cpu_set_execution_mode(snes->cpu, SNES_CPU_EXECUTION_MODE_STEP);
}

void snes_run_cpu(snes_t *snes)
{
	snes_cpu_set_execution_mode(snes->cpu, SNES_CPU_EXECUTION_MODE_RUN);
}

void nmi(snes_t *snes)
{
	snes_cpu_nmi(snes->cpu);
}
