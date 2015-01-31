#ifndef SNES_RAM_H
#define SNES_RAM_H

#include <stdint.h>

typedef struct _snes_ram snes_ram_t;

snes_ram_t *snes_ram_init(uint32_t size);
void snes_ram_destroy(snes_ram_t *ram);

uint8_t snes_ram_read(snes_ram_t *ram, uint32_t addr);
void snes_ram_write(snes_ram_t *ram, uint32_t addr, int8_t data);


#endif //SNES_RAM_H
