#ifndef SNES_APU_H
#define SNES_APU_H

#include <stdint.h>
#include "snes_apu_port.h"

typedef struct _snes_apu snes_apu_t;

snes_apu_t *snes_apu_power_up();
void snes_apu_power_down(snes_apu_t *apu);

snes_apu_port_t *snes_apu_get_port(snes_apu_t *apu);

#endif //SNES_APU_PORT_H
