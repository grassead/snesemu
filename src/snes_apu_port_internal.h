#ifndef SNES_APU_PORT_INTERNAL_H
#define SNES_APU_PORT_INTERNAL_H

#include "snes_apu_port.h"

uint8_t snes_apu_port_internal_read(snes_apu_port_t *port, uint32_t address);
void snes_apu_port_internal_write(snes_apu_port_t *port, uint32_t address, uint8_t data);


#endif //SNES_APU_PORT_INTERNAL_H
