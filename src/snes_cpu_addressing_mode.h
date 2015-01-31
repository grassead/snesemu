#ifndef SNES_CPU_ADDRESSING_MODE_H
#define SNES_CPU_ADDRESSING_MODE_H

#include "snes_bus.h"
#include "snes_cpu_registers.h"
#include "snes_cpu.h"
#include "snes_cpu_mne.h"
#include "snes_cpu_defs.h"


struct snes_effective_address snes_cpu_addressing_mode_decode(snes_bus_t *bus,  snes_cpu_registers_t *registers, snes_cpu_mnemonic_t mne, snes_cpu_addressing_mode_t mode, uint32_t relative_address);
#endif //SNES_CPU_ADDRESSING_MODE_H
