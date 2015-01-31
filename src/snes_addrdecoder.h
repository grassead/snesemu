#ifndef SNES_ADDRDECODER_H
#define SNES_ADDRDECODER_H

#include <stdint.h>
#include "snes_rom.h"

typedef struct _snes_address_decoder snes_address_decoder_t;
typedef struct _snes_address snes_address_t;

enum snes_memtype {
	ROM = 0,
	SRAM,
	WRAM,
	PPU1_APU,
	PPU2_DMA,
	OLD_PAD,
	CART_SPECIFIC,
	OTHER,
};

const char *snes_memtype_to_string(enum snes_memtype type);

snes_address_decoder_t *snes_addrdecoder_init(snes_rom_t *rom);
void snes_addrdecoder_destroy(snes_address_decoder_t *decoder);


snes_address_t *snes_addrdecoder_decode(snes_address_decoder_t *decoder, uint32_t addr);
enum snes_memtype snes_addrdecoder_get_memtype(snes_address_t *address);
uint32_t snes_addrdecoder_get_transaddr(snes_address_t *address);
void snes_addrdecoder_destroy_adress(snes_address_t *address);


#endif //SNES_ADDRDECODER_H
