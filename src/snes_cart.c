#include <stdlib.h>
#include <stdio.h>
#include "snes_cart.h"

struct _snes_cart{
	snes_rom_t *rom;
	snes_ram_t *sram;
	snes_address_decoder_t *decoder;
};


snes_cart_t *snes_cart_power_up(const char* rom_file_path)
{
	snes_cart_t *cart = malloc(sizeof(snes_cart_t));
	if(cart == NULL) {
		printf("Unable to alloc cart !\n");
		goto error_alloc;
	}

	cart->rom = snes_rom_init(rom_file_path);
	if(cart->rom == NULL) {
		printf("Error at rom init !\n");
		goto error_rom;
	}

	cart->sram = snes_ram_init(snes_rom_get_sram_size(cart->rom));
	if(cart->sram == NULL) {
		printf("Error at ram init !\n");
		goto error_ram;
	}

	cart->decoder = snes_addrdecoder_init(cart->rom);
	if(cart->decoder == NULL) {
		printf("Error at decoder init !\n");
		goto error_decoder;
	}

	return cart;
error_decoder:
	snes_ram_destroy(cart->sram);
error_ram:
	snes_rom_destroy(cart->rom);
error_rom:
	free(cart);
error_alloc:
	return NULL;
}

void snes_cart_power_down(snes_cart_t *cart)
{
	snes_addrdecoder_destroy(cart->decoder);
	snes_ram_destroy(cart->sram);
	snes_rom_destroy(cart->rom);
	free(cart);
}

snes_rom_t *snes_cart_get_rom(snes_cart_t *cart)
{
	return cart->rom;
}

snes_ram_t *snes_cart_get_ram(snes_cart_t *cart)
{
	return cart->sram;
}

snes_address_decoder_t *snes_cart_get_decoder(snes_cart_t *cart)
{
	return cart->decoder;
}
