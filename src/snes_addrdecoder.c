#include <stdlib.h>
#include <stdio.h>

#include "snes_addrdecoder.h"


struct _snes_address{
	enum snes_memtype type;
	uint32_t dec_addr;
};

struct _snes_address_decoder{
	enum snes_rom_type rom_type;
};

snes_address_decoder_t *snes_addrdecoder_init(snes_rom_t *rom)
{
	snes_address_decoder_t *decoder = (snes_address_decoder_t *)malloc(sizeof(snes_address_decoder_t));
	decoder->rom_type = snes_rom_get_type(rom);
	return decoder;
}

void snes_addrdecoder_destroy(snes_address_decoder_t *decoder)
{
	free(decoder);
}

static uint32_t snes_addrdecoder_trans_sram_addr(uint8_t bank, uint16_t offset)
{
	if(bank >= 0x70 && bank <= 0x7D) {
		return ((bank - 0x70) * 0x8000 + offset);
	} else {
			return 0x70000 + (bank - 0xFE) * 0x8000 + offset;
	}
}

static uint32_t snes_addrdecoder_trans_wram_addr(uint8_t bank, uint16_t offset)
{
	uint32_t addr = ((uint32_t)bank << 16) + offset;

	/*Low pages*/
	if(bank >= 0x00 && bank <= 0x3F)
			return (uint32_t) offset;

	return addr - 0x7E0000;
}

static uint32_t snes_addrdecoder_trans_rom_addr(uint8_t bank, uint16_t offset)
{
	return (bank & 0x7F) * 0x8000 + (offset - 0x8000);
}

snes_address_t *snes_addrdecoder_decode(snes_address_decoder_t *decoder, uint32_t addr)
{
	snes_address_t *address = malloc(sizeof(snes_address_t));
	uint8_t bank = addr >> 16;
	uint16_t offset = addr;

	address->type = OTHER;
	address->dec_addr = addr;

	//Only for lorom
	if(decoder->rom_type == SNES_ROM_TYPE_HIROM) {
		printf("snses_addrdecoder : HIROM is not supported !\n");
		free(address);
		return NULL;
	}

	if ((bank >= 0x00 && bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) {
		if(offset >= 0x0000 && offset <= 0x1FFF) {
			address->type = WRAM;
			address->dec_addr = snes_addrdecoder_trans_wram_addr(bank, offset);
		} else if (offset >= 0x2000 && offset <= 0x20FF) {
			// Nor used !
		} else if (offset >= 0x2100 && offset <= 0x21FF) {
			address->type = PPU1_APU;
		} else if (offset >= 0x2200 && offset <= 0x2FFF) {
			// Nor used !
		} else if (offset >= 0x3000 && offset <= 0x3FFF) {
			// Nor used ! (specific)
			address->type = CART_SPECIFIC;
		} else if (offset >= 0x4000 && offset <= 0x40FF) {
			address->type = OLD_PAD;
		} else if (offset >= 0x4100 && offset <= 0x41FF) {
			// Nor used !
		} else if (offset >= 0x4200 && offset <= 0x44FF) {
			address->type = PPU2_DMA;
		} else if (offset >= 0x4500 && offset <= 0x5FFF) {
			// Nor used !
		} else if (offset >= 0x6000 && offset <= 0x7FFF) {
			address->type = CART_SPECIFIC;
		} else if (offset >= 0x8000 && offset <= 0xFFFF) {
			address->type = ROM;
			address->dec_addr = snes_addrdecoder_trans_rom_addr(bank, offset);
		}
	} else if((bank >= 0x40 && bank <= 0x6F) || (bank >= 0xC0 && bank <= 0xEF)) {
		address->type = ROM;
		address->dec_addr = snes_addrdecoder_trans_rom_addr(bank,offset);
	} else if ((bank >= 0x70 && bank <= 0x7D) || (bank >= 0xF0 && bank <= 0xFD)) {
		if(offset >= 0x0000 && offset <= 0x7FFF) {
			address->type = SRAM;
			address->dec_addr = snes_addrdecoder_trans_sram_addr(bank,offset);
		} else {
			address->type = ROM;
			address->dec_addr = snes_addrdecoder_trans_rom_addr(bank,offset);
		}
	} else if(bank == 0x7E || bank == 0x7F) {
		address->type = WRAM;
		address->dec_addr = snes_addrdecoder_trans_wram_addr(bank, offset);
	} else if(bank == 0xFE || bank == 0xFF) {
		if(offset >= 0x0000 && offset <= 0x7fff) {
			address->type = SRAM;
			address->dec_addr = snes_addrdecoder_trans_sram_addr(bank,offset);
		} else {
			address->type = ROM;
			address->dec_addr = snes_addrdecoder_trans_rom_addr(bank,offset);
		}
	}
	return address;
}

void snes_addrdecoder_destroy_adress(snes_address_t *address)
{
	free(address);
}

enum snes_memtype snes_addrdecoder_get_memtype(snes_address_t *address)
{
	return address->type;
}

uint32_t snes_addrdecoder_get_transaddr(snes_address_t *address)
{
	return address->dec_addr;
}

const char *snes_memtype_to_string(enum snes_memtype type)
{
	switch (type) {
		case ROM:
			return "ROM";
		case SRAM:
			return "SRAM";
		case WRAM:
			return "WRAM";
		case PPU1_APU:
			return "PPU1_APU";
		case PPU2_DMA:
			return "PPU2_DMA";
		case OLD_PAD:
			return "OLD_PAD";
		case CART_SPECIFIC:
			return "CART_SPECIFIC";
		default:
			return "OTHER";
	}
}
