#include <stdlib.h>
#include <stdio.h>

#include "snes.h"
#include "snes_cart.h"

int main(int argc, char *argv[])
{
	snes_cart_t *cart = snes_cart_power_up(argv[1]);
	snes_t *snes = snes_power_up(cart);
	int i;
	snes_rom_print_header(snes_cart_get_rom(cart));

	for(i=0;i<20000;i++){
		tick(snes);
	}
	snes_power_down(snes);
	snes_cart_power_down(cart);

	return 0;
}
