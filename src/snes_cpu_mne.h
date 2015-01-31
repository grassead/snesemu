#ifndef SNES_CPU_MNE_H
#define SNES_CPU_MNE_H

#include "snes_cpu_defs.h"
#include "snes_cpu_addressing_mode.h"
#include "snes_cpu_stack.h"

void snes_cpu_mne_execute(snes_cpu_mnemonic_t mne, struct snes_effective_address eff_addr, snes_cpu_t *cpu);

#endif //SNES_CPU_MNE_H
