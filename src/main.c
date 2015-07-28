#include <stdlib.h>
#include <stdio.h>

#include "snes.h"
#include "snes_cart.h"

int main(int argc, char *argv[])
{
	int i;
	snes_cart_t *cart = snes_cart_power_up(argv[1]);
	if(cart == NULL) {
		printf("Unable to powerup cart !\n");
		goto error_cart;
	}

	snes_t *snes = snes_power_up(cart);
	if(snes == NULL) {
		printf("Unable to power up snes !\n");
		goto error_snes;
	}

	snes_rom_print_header(snes_cart_get_rom(cart));

	for(i=0;i<20000;i++){
		tick(snes);
	}

	snes_power_down(snes);
	snes_cart_power_down(cart);

	return 0;
error_snes:
	snes_cart_power_down(cart);
error_cart:
	return -1;
}
