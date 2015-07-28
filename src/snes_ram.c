#include <stdlib.h>
#include <stdio.h>

#include "snes_ram.h"

struct _snes_ram {
	uint32_t size;
	int8_t *data;
};


snes_ram_t *snes_ram_init(uint32_t size)
{
	snes_ram_t *ram = (snes_ram_t *)malloc(sizeof(snes_ram_t));
	if(ram == NULL) {
		printf("Error at allocation time !\n");
		goto error_alloc;
	}
	ram->size = size;
	ram->data = (int8_t *)malloc(size * sizeof(int8_t));
	if(ram->data == NULL) {
		printf("Error when allocating the RAM !\n");
		goto error_alloc_data;
	}
	return ram;
error_alloc_data:
	free(ram);
error_alloc:
	return NULL;
}

void snes_ram_destroy(snes_ram_t *ram)
{
	free(ram->data);
	ram->data = NULL;
	free(ram);
}

uint8_t snes_ram_read(snes_ram_t *ram, uint32_t addr)
{
	return ram->data[addr];
}

void snes_ram_write(snes_ram_t *ram, uint32_t addr, int8_t data)
{
	ram->data[addr] = data;
}
