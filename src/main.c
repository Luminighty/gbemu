#include <stdio.h>

#include "cartridge.h"

char* gb_file = "./assets/tetris.gb";


static inline void test_header(char* file) {
	Cartridge cart = cartridge_load(file);
	cartridge_dump_header(&cart);
	cartridge_free(&cart);
	printf("\n");
}

int main(void) {
	test_header("./assets/links_awakening.gb");
	test_header("./assets/pokemon_crystal.gbc");
	test_header("./assets/super_mario_land.gb");
	Cartridge cart = cartridge_load(gb_file);
	cartridge_dump_header(&cart);
	return 0;
}


