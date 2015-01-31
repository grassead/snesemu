#ifndef SNES_CPU_REGISTERS_H
#define SNES_CPU_REGISTERS_H

#include <stdint.h>

typedef struct _snes_cpu_registers snes_cpu_registers_t;

enum snes_cpu_register_length{
	CPU_REGISTER_8_BIT,
	CPU_REGISTER_16_BIT
};

struct snes_cpu_register_value{
	enum snes_cpu_register_length len;
	union {
		uint16_t value16;
		struct {
			uint8_t value8_low;
			uint8_t value8_high;
		};
	};
};

#define STATUS_FLAG_C 1 << 0
#define STATUS_FLAG_Z 1 << 1
#define STATUS_FLAG_I 1 << 2
#define STATUS_FLAG_D 1 << 3
#define STATUS_FLAG_X 1 << 4
#define STATUS_FLAG_B 1 << 4
#define STATUS_FLAG_M 1 << 5
#define STATUS_FLAG_V 1 << 6
#define STATUS_FLAG_N 1 << 7

snes_cpu_registers_t *snes_cpu_registers_init();
void snes_cpu_registers_destroy(snes_cpu_registers_t *registers);


void snes_cpu_registers_dump(snes_cpu_registers_t *registers);

void snes_cpu_registers_update16_z(snes_cpu_registers_t *registers, uint16_t value);
void snes_cpu_registers_update8_z(snes_cpu_registers_t *registers, uint8_t value);
void snes_cpu_registers_update16_c(snes_cpu_registers_t *registers, uint16_t value);
void snes_cpu_registers_update8_c(snes_cpu_registers_t *registers, uint8_t value);

void snes_cpu_registers_update8(snes_cpu_registers_t *registers, uint8_t value);
void snes_cpu_registers_update16(snes_cpu_registers_t *registers, uint16_t value);
void snes_cpu_registers_accumulator_set(snes_cpu_registers_t *registers, uint16_t value);
void snes_cpu_registers_accumulator_set16(snes_cpu_registers_t *registers, uint16_t value);
struct snes_cpu_register_value snes_cpu_registers_accumulator_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_x_set(snes_cpu_registers_t *registers, uint16_t value);
struct snes_cpu_register_value snes_cpu_registers_x_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_y_set(snes_cpu_registers_t *registers, uint16_t value);
struct snes_cpu_register_value snes_cpu_registers_y_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_direct_page_set(snes_cpu_registers_t *registers, uint16_t value);
uint16_t snes_cpu_registers_direct_page_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_stack_pointer_set(snes_cpu_registers_t *registers, uint16_t value);
struct snes_cpu_register_value snes_cpu_registers_stack_pointer_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_program_counter_set(snes_cpu_registers_t *registers, uint16_t pc);
uint16_t snes_cpu_registers_program_counter_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_program_counter_inc(snes_cpu_registers_t *registers);
void snes_cpu_registers_program_bank_set(snes_cpu_registers_t *registers, uint8_t value);
uint32_t snes_cpu_registers_program_bank_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_data_bank_set(snes_cpu_registers_t *registers, uint8_t value);
uint32_t snes_cpu_registers_data_bank_get(snes_cpu_registers_t *registers);
uint8_t snes_cpu_registers_status_flag_get(snes_cpu_registers_t *registers);
void snes_cpu_registers_status_flag_force(snes_cpu_registers_t *registers, uint8_t flags);
void snes_cpu_registers_status_flag_set(snes_cpu_registers_t *registers, uint8_t flags);
void snes_cpu_registers_status_flag_reset(snes_cpu_registers_t *registers, uint8_t flags);
uint8_t snes_cpu_registers_status_flag_isset(snes_cpu_registers_t *registers, uint8_t flag);
void snes_cpu_registers_emulation_set(snes_cpu_registers_t *registers);
void snes_cpu_registers_emulation_reset(snes_cpu_registers_t *registers);
uint8_t snes_cpu_registers_emulation_isset(snes_cpu_registers_t *registers);

#endif //SNES_CPU_REGISTERS_H
