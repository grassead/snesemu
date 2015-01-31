#ifndef SNES_H
#define SNES_H

#include "snes_cart.h"

typedef struct _snes snes_t;

snes_t *snes_power_up(snes_cart_t *cart);
void snes_power_down(snes_t *snes);

void tick(snes_t *snes);
void nmi(snes_t *snes);


#endif //SNES_H
