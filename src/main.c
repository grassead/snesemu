#include <stdlib.h>
#include <stdio.h>

#include "snes.h"
#include "snes_cart.h"


void handle_user_input(snes_t *snes)
{
	char input[50];
	char command;
	char previous_command = 0;
	uint32_t params;
	for(;;) {
		input[0] = 0;
		command = 0;
		fgets(input,50,stdin);
		sscanf(input, "%c %x", &command, &params);
		if (command == '\n') {
			command = previous_command;
		}
		switch(command) {
			case 'n':
			case 'N':
				printf("next\n");
				snes_do_cpu_tick(snes);
				break;
			case 'b':
			case 'B':
				snes_set_breakpoint(snes, SNES_BREAKPOINT_TYPE_CPU, params);
				break;
			case 'r':
			case 'R':
				printf("run\n");
				snes_run_cpu(snes);
				break;
			case 'q':
			case 'Q':
				printf("Exiting ...\n");
				return;
			default:
				printf("Unkonwn command %c\n",command);
				printf("Valid commands are:\n");
				printf("\tn: next instruction\n");
				printf("\tb: set breakpoint\n");
				printf("\tr: run program\n");
				break;
		}
		previous_command = command;
	}
}

int main(int argc, char *argv[])
{
	snes_cart_t *cart = snes_cart_power_up(argv[1]);
	if(cart == NULL) {
		printf("Unable to powerup cart !\n");
		goto error_cart;
	}

	snes_rom_print_header(snes_cart_get_rom(cart));


	snes_t *snes = snes_init(cart);
	if(snes == NULL) {
		printf("Unable to power up snes !\n");
		goto error_snes;
	}

	//snes_set_breakpoint(snes, SNES_BREAKPOINT_TYPE_CPU, 0x0080D6);
	//snes_set_breakpoint(snes, SNES_BREAKPOINT_TYPE_CPU, 0x0088DC);

	snes_power_up(snes);

	handle_user_input(snes);

	//This function will never return (at this point of the dev)
	printf("Destroying snes\n");
	snes_destroy(snes);
	printf("Stopping cart\n");
	snes_cart_power_down(cart);

	return 0;
error_snes:
	snes_cart_power_down(cart);
error_cart:
	return -1;
}
