#ifndef PPU_H
#define PPU_H


#include <stdint.h>


typedef struct {
	uint32_t dot_clock;
	uint8_t line;
	uint8_t lcdc;
	uint8_t stat;
	uint8_t bgp;
} PPU;

PPU ppu_create();

struct emulator;
void ppu_step(struct emulator *emu, uint8_t cycles);


#endif // PPU_H
