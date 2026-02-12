#include "ppu.h"
#include "display.h"
#include "emulator.h"
#include "interrupts.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


PPU ppu_create() {
	PPU ppu = {0};
	ppu.vram = malloc(sizeof(uint8_t) * VRAM_SIZE);
	assert(ppu.vram);
	return ppu;
}


void ppu_destroy(PPU *ppu) {
	free(ppu->vram);
	ppu->vram = NULL;
}


uint8_t ppu_vram_read(PPU *ppu, uint16_t address) {
	if (address >= VRAM_SIZE) return 0xFF;
	return ppu->vram[address];
}


void ppu_vram_write(PPU *ppu, uint16_t address, uint8_t value) {
	if (address >= VRAM_SIZE) return;
	ppu->vram[address] = value;
}

static inline TileData* tile_data(PPU *ppu) {
	return (TileData*)ppu->vram;
}
static inline uint8_t* tile_map(PPU *ppu) {
	return &ppu->vram[0x1800];
}

static inline uint8_t tile_color(TileData* tile, uint8_t lx, uint8_t ly) {
	assert(lx < 8);
	assert(ly < 8);
	uint8_t lower = tile->lines[ly].lower;
	uint8_t higher = tile->lines[ly].higher;
	uint8_t offset = 7 - lx;
	uint8_t lower_bit = (lower >> offset) & 0b1;
	uint8_t higher_bit = (higher >> offset) & 0b1;
	return lower_bit | (higher_bit << 1);
}

static inline void drawing_step(Emulator *emu) {
	// TODO: Penalties
	PPU *ppu = &emu->ppu;

	uint8_t tile_x = ppu->x / 8;
	uint8_t tile_y = ppu->line / 8;
	uint16_t tile_idx = tile_y * 32 + tile_x;
	uint8_t tile_id = tile_map(ppu)[tile_idx];
	TileData *tile = &tile_data(ppu)[tile_id];

	uint8_t local_x = ppu->x % 8;
	uint8_t local_y = ppu->line % 8;
	uint8_t color_idx = tile_color(tile, local_x, local_y);
	// TODO: Lookup palette

	display_set(emu, emu->ppu.x, emu->ppu.line, color_idx);

	ppu->x++;
	if (ppu->x >= 160) {
		ppu->mode = PPU_MODE_HBLANK;
		ppu->x = 0;
	}
}


static inline void oam_scan_step(Emulator *emu) {
	static const uint16_t OAM_SCAN_LENGTH = 80;
	if (emu->ppu.dot_clock == OAM_SCAN_LENGTH)
		emu->ppu.mode = PPU_MODE_DRAWING;
}


static const uint16_t LINE_DOTS_LENGTH = 456;
static inline void hblank_step(Emulator *emu) {
	if (emu->ppu.dot_clock < LINE_DOTS_LENGTH)
		return;

	emu->ppu.line++;
	emu->ppu.dot_clock -= 456;

	static const uint8_t VBLANK_START = 144;
	if (emu->ppu.line == VBLANK_START) {
		interrupt_trigger(emu, INTERRUPT_VBLANK);
		emu->ppu.mode = PPU_MODE_VBLANK;
	} else {
		emu->ppu.mode = PPU_MODE_OAM_SCAN;
	}
}


static inline void vblank_step(Emulator *emu) {
	if (emu->ppu.dot_clock < LINE_DOTS_LENGTH)
		return;
	emu->ppu.line++;
	emu->ppu.dot_clock -= 456;
	static const uint8_t VBLANK_END = 154;
	if (emu->ppu.line == VBLANK_END) {
		emu->ppu.line = 0;
		emu->ppu.mode = PPU_MODE_OAM_SCAN;
	}
}


void ppu_step(Emulator *emu, uint8_t cycles) {
	PPU *ppu = &emu->ppu;
	for (uint8_t cycle = 0; cycle < cycles; cycle++) {
		emu->ppu.dot_clock++;

		switch (ppu->mode) {
		case PPU_MODE_HBLANK: hblank_step(emu); break;
		case PPU_MODE_VBLANK: vblank_step(emu); break;
		case PPU_MODE_DRAWING: drawing_step(emu); break;
		case PPU_MODE_OAM_SCAN: oam_scan_step(emu); break;
		}
	}
}


uint8_t ppu_lcdc_read(PPU *ppu) {
	return  (ppu->lcdc.ppu_enable << 7) |
		(ppu->lcdc.window_tilemap_area << 6) |
		(ppu->lcdc.window_enable << 5) |
		(ppu->lcdc.bg_win_tilemap_area << 4) |
		(ppu->lcdc.bg_tilemap_area << 3) |
		(ppu->lcdc.obj_size << 2) |
		(ppu->lcdc.obj_enable << 1) |
		(ppu->lcdc.bg_window_enable_prio << 0);
}


void ppu_lcdc_write(PPU *ppu, uint8_t value) {
	ppu->lcdc.ppu_enable = (value >> 7) & 0b1;
	ppu->lcdc.window_tilemap_area = (value >> 6) & 0b1;
	ppu->lcdc.window_enable = (value >> 5) & 0b1;
	ppu->lcdc.bg_win_tilemap_area = (value >> 4) & 0b1;
	ppu->lcdc.bg_tilemap_area = (value >> 3) & 0b1;
	ppu->lcdc.obj_size = (value >> 2) & 0b1;
	ppu->lcdc.obj_enable = (value >> 1) & 0b1;
	ppu->lcdc.bg_window_enable_prio = (value >> 0) & 0b1;
}
