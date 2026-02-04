#include "cartridge.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


static inline uint8_t* load_file(char *filename, size_t* size);


Cartridge cartridge_load(char* filename) {
	Cartridge cartridge = {0};
	cartridge.content = load_file(filename, &cartridge.size);
	if (cartridge.content == NULL) {
		return (Cartridge){.is_load_success = false };
	}
	// NOTE: Titles with 16 chars will break this due to the missing \0. For now it should be fine though
	cartridge.title = (char*)&cartridge.content[0x134];
	cartridge.is_color = cartridge.content[0x143];
	cartridge.licensee = cartridge.content[0x144] << 8 | cartridge.content[0x145];
	cartridge.is_super_gb = cartridge.content[0x0146] != 0;
	cartridge.type = cartridge.content[0x0147];
	cartridge.rom_size = cartridge.content[0x0148];
	cartridge.ram_size = cartridge.content[0x0149];
	cartridge.destination_code = cartridge.content[0x014A];
	cartridge.licensee_code = cartridge.content[0x014B];
	cartridge.mask_rom_version = cartridge.content[0x014C];
	cartridge.complement_check = cartridge.content[0x014D];
	cartridge.checksum = cartridge.content[0x014E] << 8 | cartridge.content[0x014F];
	return cartridge;
}


void cartridge_free(Cartridge *cartridge) {
	free(cartridge->content);
	cartridge->content = NULL;
	cartridge->title = NULL;
	cartridge->size = 0;
}


static inline void print_flag(char* label, bool value);
static inline void print_cartridge_type(CartridgeType type);
static inline void print_rom_size(ROMSize type);
static inline void print_ram_size(RAMSize type);
static inline void print_destination_code(DestinationCode code);
static inline void print_licensee_code(LicenseeCode code);

void cartridge_dump_header(Cartridge *cartridge) {
	printf("TITLE: %s\n", cartridge->title);
	printf("LICENSEE: %x\n", cartridge->licensee);
	print_flag("COLOR GB", cartridge->is_color);
	print_flag("SUPER GB", cartridge->is_super_gb);
	printf("CART TYPE: "); print_cartridge_type(cartridge->type); printf("\n");
	printf("ROM: "); print_rom_size(cartridge->rom_size); printf("\n");
	printf("RAM: "); print_ram_size(cartridge->ram_size); printf("\n");
	printf("DEST: "); print_destination_code(cartridge->destination_code); printf("\n");
	printf("LICENSE CODE: "); print_licensee_code(cartridge->licensee_code); printf("\n");
	printf("MASK ROM VERSION: %x\n", cartridge->mask_rom_version);
	printf("COMPLEMENT CHECK: %x\n", cartridge->complement_check);
	printf("CHECKSUM: %x\n", cartridge->checksum);
}



static inline void print_flag(char* label, bool value) {
	if (value) {
		printf("%s: YES\n", label);
	} else {
		printf("%s: NO\n", label);
	}
}

static inline void print_cartridge_type(CartridgeType type) {
	switch (type) {
	case CARTRIDGE_TYPE_0_ROM_ONLY:
		printf("ROM ONLY (0)"); return;
	default:
		printf("UNSUPPORTED (%x)", type); return;
	}

}
static inline void print_rom_size(ROMSize type) {
	switch (type) {
	case ROM_SIZE_256KBIT: printf("256KBIT"); return;
	case ROM_SIZE_512KBIT: printf("512KBIT"); return;
	case ROM_SIZE_1MBIT: printf("1MBIT"); return;
	case ROM_SIZE_2MBIT: printf("2MBIT"); return;
	case ROM_SIZE_4MBIT: printf("4MBIT"); return;
	case ROM_SIZE_8MBIT: printf("8MBIT"); return;
	case ROM_SIZE_16MBIT: printf("16MBIT"); return;
	case ROM_SIZE_9MBIT: printf("9MBIT"); return;
	case ROM_SIZE_10MBIT: printf("10MBIT"); return;
	case ROM_SIZE_12MBIT: printf("12MBIT"); return;
	default: printf("UNDEFINED (%x)", type); return;
	}
}

static inline void print_ram_size(RAMSize type) {
	switch (type) {
	case RAM_SIZE_NONE: printf("NONE"); return;
	case RAM_SIZE_16KBIT: printf("16KBIT"); return;
	case RAM_SIZE_64KBIT: printf("64KBIT"); return;
	case RAM_SIZE_256KBIT: printf("256KBIT"); return;
	case RAM_SIZE_1MBIT: printf("1MBIT"); return;
	default: printf("UNDEFINED (%x)", type); return;
	}
}

static inline void print_destination_code(DestinationCode code) {
	switch (code) {
	case DESTINATION_CODE_JAP: printf("JAP"); return;
	case DESTINATION_CODE_NON_JAP: printf("NON-JAP"); return;
	default: printf("UNDEFINED (%x)", code); return;
	}
}

static inline void print_licensee_code(LicenseeCode code) {
	switch (code) {
	case LICENSEE_CODE_CHECK: printf("LICENSEE"); return;
	case LICENSEE_CODE_ACCOLADE: printf("ACCOLADE"); return;
	case LICENSEE_CODE_KONAMI: printf("KONAMI"); return;
	default: printf("UNDEFINED (%x)", code); return;
	}
}

static inline uint8_t* load_file(char *filename, size_t* size) {
	FILE *file = fopen(filename, "r");
	if (file == NULL)  { return NULL; }

	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t *buffer = malloc(*size + 1);

	fread(buffer, 1, *size, file);
	buffer[*size] = '\0';
	fclose(file);

	return buffer;
}

