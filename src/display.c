#include "display.h"
#include "emulator.h"
#include "memory_map.h"
#include <stdint.h>
#include <stdio.h>


Display display_create() {
	Display d = {0};
	return d;
}

void display_set(Emulator *emu, uint8_t x, uint8_t y, GBColor color) {
	if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
	emu->display.screen[y][x] = color;
}


static inline void dump_tile(Emulator *emu, uint8_t tx, uint8_t ty, uint16_t start_address) {
	for (uint8_t line = 0; line < 8; line++) {
		uint8_t lower = memory_read(emu, start_address + (line * 2));
		uint8_t higher = memory_read(emu, start_address + (line * 2) + 1);
		for (uint8_t x = 0; x < 8; x++) {
			uint8_t offset = 7 - x;
			uint8_t lower_bit = (lower >> offset) & 0b1;
			uint8_t higher_bit = (higher >> offset) & 0b1;
			GBColor color = lower_bit | (higher_bit << 1);
			display_set(emu, tx * 8 + x, ty * 8 + line, color);
		}
	}
}

void display_dump(Emulator *emu) {
	static const uint8_t TILE_WIDTH = DISPLAY_WIDTH / 8;
	static const uint8_t TILE_HEIGHT = DISPLAY_HEIGHT / 8;
	for (uint8_t tx = 0; tx < TILE_WIDTH; tx++) {
	for (uint8_t ty = 0; ty < TILE_HEIGHT; ty++) {
		uint16_t start_address = 0x8000 + (tx + ty * TILE_WIDTH) * 16;
		dump_tile(emu, tx, ty, start_address);
	}}
}

