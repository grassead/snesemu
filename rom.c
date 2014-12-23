#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

enum rom_type {
	ROM_TYPE_HIROM,
	ROM_TYPE_LOROM,
};

struct interrupt_vectors{
	uint16_t cop;
	uint16_t brk;
	uint16_t abort;
	uint16_t nmi;
	uint16_t reset;
	uint16_t irq;
};

struct nintendo_header{
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
	struct interrupt_vectors native_vectors;
	uint32_t unknown2;
	struct interrupt_vectors emulation_vectors;
};

struct snes_rom{
	struct nintendo_header header;
	enum rom_type type;
	int fd;
	const uint8_t* entirerom;
	const uint8_t* usefullrom;
	off_t size;
	off_t usefull_size;
};

uint16_t calc_checksum(const uint8_t* usefullrom, size_t size)
{
	uint16_t checksum = 0;
	int i;
	for(i = 0; i < size;i++) {
		checksum += usefullrom[i];
	}
	return checksum;
}

static struct snes_rom rom;

static int rom_init_header()
{
	int loROM_offset = 0x7fc0;
	int hiROM_offset = 0xffc0;
	uint16_t checksum = calc_checksum(rom.usefullrom, rom.usefull_size);
	
	//Try hiRom First
	memcpy(&(rom.header),&(rom.usefullrom[hiROM_offset]),sizeof(struct nintendo_header));
	if (checksum != rom.header.checksum) {
		//Rom is not hiROM, trying loROM
		memcpy(&(rom.header),&(rom.usefullrom[loROM_offset]),sizeof(struct nintendo_header));
		if (checksum != rom.header.checksum) {
			printf("Unable to find ROM type !\n");
			return -1;
		} else {
			//ROM is loROM
			rom.type = ROM_TYPE_LOROM;
		}
	} else {
		//ROM is hiROM
		rom.type = ROM_TYPE_HIROM;
	}
}

static int rom_is_headered()
{
	if (rom.size % 1024)
		return 1;
	else
		return 0;
}

int rom_init(const char* path)
{
	struct stat sb;
	int err;

	rom.fd = open(path,O_RDWR);
	if (rom.fd < 0) {
		printf("Fail to open rom file !\n");
		return -1;
	}
	
	fstat(rom.fd, &sb);
	rom.size = sb.st_size;
	
	rom.entirerom = mmap(NULL, rom.size, PROT_READ, MAP_SHARED, rom.fd, 0);
	if (rom.entirerom == MAP_FAILED) {
		printf("Map error !\n");
		goto maperr;
	}
	if (rom_is_headered()) {
			rom.usefull_size = rom.size - 512;
			rom.usefullrom = &(rom.entirerom[512]);
	} else {
			rom.usefull_size = rom.size;
			rom.usefullrom = &(rom.entirerom[0]);
	}
	
	err = rom_init_header();
	if (err < 0) {
		goto fail_detect;
	}

	return 0;

fail_detect:
	munmap((void *)rom.entirerom,rom.size);
maperr:
	close(rom.fd);
	return -1;
}

int main(int argc, char *argv[])
{
	int err;
	err = rom_init(argv[1]);
	if (err < 0){
		printf("ROM init failed !\n");
	} else {
		printf("Game name : %s\n",rom.header.name);
	}
}
