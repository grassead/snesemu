#ifndef SNES_APU_PORT_H
#define SNES_APU_PORT_H

#include <stdint.h>

typedef struct _snes_apu_port snes_apu_port_t;

snes_apu_port_t *snes_apu_port_init();
void snes_apu_port_destroy(snes_apu_port_t *port);

uint8_t snes_apu_port_read(snes_apu_port_t *port, uint32_t address);
void snes_apu_port_write(snes_apu_port_t *port, uint32_t address, uint8_t data);

#endif //SNES_APU_PORT_H
