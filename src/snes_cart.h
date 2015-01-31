#ifndef SNES_CART_H
#define SNES_CART_H

#include "snes_rom.h"
#include "snes_ram.h"
#include "snes_addrdecoder.h"

typedef struct _snes_cart snes_cart_t;

snes_cart_t *snes_cart_power_up(const char* rom_file_path);
void snes_cart_power_down(snes_cart_t *cart);

snes_rom_t *snes_cart_get_rom(snes_cart_t *cart);
snes_ram_t *snes_cart_get_ram(snes_cart_t *cart);
snes_address_decoder_t *snes_cart_get_decoder(snes_cart_t *cart);


#endif //SNES_CART_H
