#ifndef SNES_ROM_H
#define SNES_ROM_H

#include <stdint.h>

struct _snes_interrupt_vectors{
	uint16_t cop;
	uint16_t brk;
	uint16_t abort;
	uint16_t nmi;
	uint16_t reset;
	uint16_t irq;
};

typedef struct _snes_interrupt_vectors snes_interrupt_vectors_t;
typedef struct _snes_rom snes_rom_t;

enum snes_rom_type {
	SNES_ROM_TYPE_HIROM,
	SNES_ROM_TYPE_LOROM,
	SNES_ROM_TYPE_UNKNOWN,
};

const char *snes_rom_type_to_string(enum snes_rom_type type);

snes_rom_t *snes_rom_init(const char *path);
void snes_rom_destroy(snes_rom_t *rom);

enum snes_rom_type snes_rom_get_type(snes_rom_t *rom);
uint32_t snes_rom_get_rom_size(snes_rom_t *rom);
uint32_t snes_rom_get_sram_size(snes_rom_t *rom);
snes_interrupt_vectors_t snes_rom_get_emu_interrupt_vectors(snes_rom_t *rom);
snes_interrupt_vectors_t snes_rom_get_nat_interrupt_vectors(snes_rom_t *rom);

void snes_rom_print_header(snes_rom_t *rom);

uint8_t snes_rom_read(snes_rom_t *rom, uint32_t address);

#endif //SNES_ROM_H
