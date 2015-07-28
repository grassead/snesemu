#include <stdlib.h>
#include <stdio.h>

#include "snes_bus.h"
#include "snes_addrdecoder.h"

struct _snes_bus{
	snes_cart_t *cart;
	snes_ram_t *wram;
};

snes_bus_t *snes_bus_power_up(snes_cart_t *cart, snes_ram_t *wram)
{
	snes_bus_t *bus = malloc(sizeof(snes_bus_t));
	if(bus == NULL) {
		goto error_alloc;
	}

	bus->cart = cart;
	if(bus->cart == NULL) {
		goto error_input;
	}

	bus->wram = wram;
	if(bus->wram == NULL) {
		goto error_input;
	}
	return bus;

error_input:
	free(bus);
error_alloc:
	return NULL;
}

void snes_bus_power_down(snes_bus_t *bus)
{
	bus->cart = NULL;
	bus->wram = NULL;
	free(bus);
}

uint8_t snes_bus_read(snes_bus_t *bus, uint32_t addr)
{
	snes_address_decoder_t *decoder = snes_cart_get_decoder(bus->cart);
	snes_address_t *address = snes_addrdecoder_decode(decoder, addr);
	enum snes_memtype type = snes_addrdecoder_get_memtype(address);
	uint8_t data = 0;
	uint32_t translated_addr = snes_addrdecoder_get_transaddr(address);
	switch(type) {
		case ROM :
		{
			snes_rom_t *rom = snes_cart_get_rom(bus->cart);
			data = snes_rom_read(rom,translated_addr);
			break;
		}
		case SRAM:
		{
			snes_ram_t *sram = snes_cart_get_ram(bus->cart);
			data = snes_ram_read(sram,translated_addr);
			break;
		}
		case WRAM:
		{
			data = snes_ram_read(bus->wram,translated_addr);
			break;
		}
		default:
		{
			printf("snes_bus : addr type (%d) not handled in read (addr = 0x%06X)!)\n",type,addr);
			break;
		}
	}

	snes_addrdecoder_destroy_adress(address);
	return data;
}

void snes_bus_write(snes_bus_t *bus, uint32_t addr, uint8_t data)
{
	snes_address_decoder_t *decoder = snes_cart_get_decoder(bus->cart);
	snes_address_t *address = snes_addrdecoder_decode(decoder, addr);
	enum snes_memtype type = snes_addrdecoder_get_memtype(address);
	uint32_t translated_addr = snes_addrdecoder_get_transaddr(address);

	switch(type) {
		case ROM :
		{
			printf("Cant write ROM!!\n");
			break;
		}
		case SRAM:
		{
			snes_ram_t *sram = snes_cart_get_ram(bus->cart);
			snes_ram_write(sram,translated_addr,data);
			break;
		}
		case WRAM:
		{
			snes_ram_write(bus->wram,translated_addr,data);
			break;
		}
		default:
		{
			printf("snes_bus : addr type (%d) not handled in write (addr = 0x%06X; data = 0x%4X!)\n",type,addr,data);
			break;
		}
	}
	snes_addrdecoder_destroy_adress(address);
}
