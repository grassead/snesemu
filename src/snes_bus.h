#ifndef SNES_BUS_H
#define SNES_BUS_H

#include <stdint.h>
#include "snes_cart.h"
#include "snes_ram.h"
#include "snes_apu.h"

typedef struct _snes_bus snes_bus_t;

snes_bus_t *snes_bus_power_up(snes_cart_t *cart, snes_ram_t *wram, snes_apu_t *apu);
void snes_bus_power_down(snes_bus_t *bus);

uint8_t snes_bus_read(snes_bus_t *bus, uint32_t address);
void snes_bus_write(snes_bus_t *bus, uint32_t address, uint8_t data);

#endif //SNES_BUS_H
