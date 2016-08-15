#include <stdlib.h>
#include <stdio.h>
#include "snes_ram.h"
#include "snes_apu_port.h"

struct _snes_apu_port{
	snes_ram_t *input_port;
	snes_ram_t *output_port;
};


snes_apu_port_t *snes_apu_port_init()
{
	snes_apu_port_t *port = malloc(sizeof(snes_apu_port_t));

	if(port == NULL){
		goto error_alloc;
	}

	port->input_port = snes_ram_init(4);
	if(port->input_port == NULL) {
		goto error_input;
	}

	port->output_port = snes_ram_init(4);
	if(port->output_port == NULL) {
		goto error_output;
	}
	return port;

error_output:
	snes_ram_destroy(port->input_port);
error_input:
	free(port);
error_alloc:
	return NULL;
}

void snes_apu_port_destroy(snes_apu_port_t *port)
{
	snes_ram_destroy(port->output_port);
	snes_ram_destroy(port->input_port);
	free(port);
}

uint8_t snes_apu_port_read(snes_apu_port_t *port, uint32_t address)
{
	uint8_t data = snes_ram_read(port->output_port, address);
	//printf("CPU is reading APU port 0x%x : data is 0x%x\n",0x2140 + address,data);
	return data;
}

void snes_apu_port_write(snes_apu_port_t *port, uint32_t address, uint8_t data)
{
	//printf("CPU is writing APU port 0x%x : data is 0x%x\n",0x2140 + address,data);
	snes_ram_write(port->input_port, address, data);
}

uint8_t snes_apu_port_internal_read(snes_apu_port_t *port, uint32_t address)
{
	uint8_t data = snes_ram_read(port->input_port, address);
	//printf("APU is reading APU port 0x%x : data is 0x%x\n",0x2140 + address,data);
	return data;
}

void snes_apu_port_internal_write(snes_apu_port_t *port, uint32_t address, uint8_t data)
{
	//printf("APU is writing APU port 0x%x : data is 0x%x\n",0x2140 + address,data);
	snes_ram_write(port->output_port, address, data);
}
