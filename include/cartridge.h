#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef enum {
	CARTRIDGE_TYPE_0_ROM_ONLY = 0x00,
} CartridgeType;

typedef enum {
	ROM_SIZE_256KBIT = 0,
	ROM_SIZE_512KBIT = 1,
	ROM_SIZE_1MBIT= 2,
	ROM_SIZE_2MBIT = 3,
	ROM_SIZE_4MBIT = 4,
	ROM_SIZE_8MBIT = 5,
	ROM_SIZE_16MBIT = 6,
	ROM_SIZE_9MBIT = 52,
	ROM_SIZE_10MBIT = 53,
	ROM_SIZE_12MBIT = 54,
} ROMSize;


typedef enum {
	RAM_SIZE_NONE = 0,
	RAM_SIZE_16KBIT = 1,
	RAM_SIZE_64KBIT= 2,
	RAM_SIZE_256KBIT = 3,
	RAM_SIZE_1MBIT = 4,
} RAMSize;

typedef enum {
	DESTINATION_CODE_JAP = 0,
	DESTINATION_CODE_NON_JAP = 1,
} DestinationCode;

typedef enum {
	LICENSEE_CODE_CHECK = 0x33,
	LICENSEE_CODE_ACCOLADE = 0x79,
	LICENSEE_CODE_KONAMI = 0xA4,
} LicenseeCode;


typedef struct {
	bool is_load_success;
	uint8_t *content;
	size_t size;
	char title[17];
	uint16_t licensee;
	bool is_color;
	bool is_super_gb;
	CartridgeType type;

	ROMSize rom_size;
	RAMSize ram_size;
	DestinationCode destination_code;
	LicenseeCode licensee_code;
	uint8_t mask_rom_version;
	uint8_t complement_check;
	uint16_t checksum;
} Cartridge;


Cartridge cartridge_load(char* filename);
void cartridge_free(Cartridge *cartridge);
void cartridge_dump_header(Cartridge *cartridge);

#endif // CARTRIDGE_H
