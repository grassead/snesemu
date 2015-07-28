#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "snes_rom.h"

struct snes_cart_header{
	const char name[21];
	uint8_t rom_layout;
	uint8_t cartridge_type;
	uint8_t rom_size_byte;
	uint8_t ram_size_byte;
	uint8_t country_code;
	uint8_t licensee_code;
	uint8_t version_number;
	uint16_t checksum_comp;
	uint16_t checksum;
	uint32_t unknown;
	snes_interrupt_vectors_t native_vectors;
	uint32_t unknown2;
	snes_interrupt_vectors_t emulation_vectors;
};

struct _snes_rom{
	struct snes_cart_header header;
	enum snes_rom_type type;
	int fd;
	const uint8_t* entirerom;
	const uint8_t* usefullrom;
	off_t size;
	off_t usefull_size;
};

static uint16_t snes_rom_calc_checksum(const uint8_t* usefullrom, size_t size)
{
	uint16_t checksum = 0;
	int i;
	for(i = 0; i < size;i++) {
		checksum += usefullrom[i];
	}
	return checksum;
}


static int snes_rom_init_header(snes_rom_t *rom)
{
	int loROM_offset = 0x7fc0;
	int hiROM_offset = 0xffc0;
	uint16_t checksum = snes_rom_calc_checksum(rom->usefullrom, rom->usefull_size);

	//Try hiRom First
	memcpy(&(rom->header),&(rom->usefullrom[hiROM_offset]),sizeof(struct snes_cart_header));
	if (checksum != rom->header.checksum) {
		//Rom is not hiROM, trying loROM
		memcpy(&(rom->header),&(rom->usefullrom[loROM_offset]),sizeof(struct snes_cart_header));
		if (checksum != rom->header.checksum) {
			printf("Unable to find ROM type !\n");
			return -1;
		} else {
			//ROM is loROM
			rom->type = SNES_ROM_TYPE_LOROM;
		}
	} else {
		//ROM is hiROM
		rom->type = SNES_ROM_TYPE_HIROM;
	}
	return 0;
}

static int snes_rom_is_headered(snes_rom_t *rom)
{
	if (rom->size % 1024)
		return 1;
	else
		return 0;
}

snes_rom_t *snes_rom_init(const char *path)
{
	struct stat sb;
	int err;
	snes_rom_t *rom = (snes_rom_t *)malloc(sizeof(snes_rom_t));
	if(rom == NULL) {
		goto error_alloc;
	}
	rom->fd = open(path,O_RDWR);
	if (rom->fd < 0) {
		printf("Fail to open rom file (%s)!\n",strerror(errno));
		goto error_open;
	}

	fstat(rom->fd, &sb);
	rom->size = sb.st_size;

	rom->entirerom = mmap(NULL, rom->size, PROT_READ, MAP_SHARED, rom->fd, 0);
	if (rom->entirerom == MAP_FAILED) {
		printf("Map error !\n");
		goto maperr;
	}
	if (snes_rom_is_headered(rom)) {
			rom->usefull_size = rom->size - 512;
			rom->usefullrom = &(rom->entirerom[512]);
	} else {
			rom->usefull_size = rom->size;
			rom->usefullrom = &(rom->entirerom[0]);
	}

	err = snes_rom_init_header(rom);
	if (err < 0) {
		goto fail_detect;
	}

	return rom;

fail_detect:
	munmap((void *)rom->entirerom,rom->size);
maperr:
	close(rom->fd);
error_open:
	free(rom);
error_alloc:
	return NULL;
}

void snes_rom_destroy(snes_rom_t *rom)
{
	close(rom->fd);
	free(rom);
}

enum snes_rom_type snes_rom_get_type(snes_rom_t *rom)
{
	switch(rom->header.rom_layout) {
		case 0x20:
		case 0x30:
			return SNES_ROM_TYPE_LOROM;
		case 0x21:
		case 0x31:
			return SNES_ROM_TYPE_HIROM;
		default:
			return SNES_ROM_TYPE_UNKNOWN;
	}
}

uint32_t snes_rom_get_rom_size(snes_rom_t *rom)
{
	return (0x400 << rom->header.rom_size_byte);
}

uint32_t snes_rom_get_sram_size(snes_rom_t *rom)
{
	return (0x400 << rom->header.ram_size_byte);
}

void snes_rom_print_header(snes_rom_t *rom)
{
	printf("Game title : %s\n",rom->header.name);
	printf("ROM Layout : %s\n",snes_rom_type_to_string(snes_rom_get_type(rom)));
	printf("Cartridge type : 0x%X\n",rom->header.cartridge_type);
	printf("ROM size (in octets) : 0x%X\n",snes_rom_get_rom_size(rom));
	printf("RAM size (in octets) : 0x%X\n",snes_rom_get_sram_size(rom));
	printf("Country code : 0x%X\n",rom->header.country_code);
	printf("Licensee code : 0x%X\n",rom->header.licensee_code);
	printf("Version number : 0x%X\n",rom->header.version_number);
}


const char* snes_rom_type_to_string(enum snes_rom_type type)
{
	switch (type)
	{
		case SNES_ROM_TYPE_HIROM:
			return "HIROM";
		case SNES_ROM_TYPE_LOROM:
			return "LOROM";
		default:
			return "Unkonwn";
	}
}

uint8_t snes_rom_read(snes_rom_t *rom, uint32_t address)
{
	return rom->usefullrom[address];
}

snes_interrupt_vectors_t snes_rom_get_emu_interrupt_vectors(snes_rom_t *rom)
{
	return rom->header.emulation_vectors;
}

snes_interrupt_vectors_t snes_rom_get_nat_interrupt_vectors(snes_rom_t *rom)
{
	return rom->header.native_vectors;
}
