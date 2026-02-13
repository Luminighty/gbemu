#include "ppu.h"
#include "display.h"
#include "emulator.h"
#include "interrupts.h"
#include "memory_map.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define OAM_EMPTY 0xFF

PPU ppu_create() {
	PPU ppu = {0};
	ppu.vram = malloc(sizeof(uint8_t) * VRAM_SIZE);
	ppu.oam = malloc(sizeof(uint8_t) * OAM_SIZE);
	for (uint8_t i = 0; i < 10; i++)
		ppu.objects_to_render[i] = OAM_EMPTY;
	assert(ppu.vram);
	assert(ppu.oam);
	return ppu;
}


void ppu_destroy(PPU *ppu) {
	free(ppu->vram);
	free(ppu->oam);
	ppu->vram = NULL;
	ppu->oam = NULL;
}


static inline TileData* tile_data(PPU *ppu) { return (TileData*)ppu->vram; }
static inline uint8_t* tile_map(PPU *ppu) { return &ppu->vram[0x1800]; }


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


static inline SpriteObject* get_sprite_object(PPU *ppu, uint8_t index) {
	return &((SpriteObject*)ppu->oam)[index];
}


static inline GBColor object_pixel(Emulator *emu) {
	PPU *ppu = &emu->ppu;
	uint8_t x = ppu->x;
	uint8_t y = ppu->line;
	uint8_t selected_color_idx = 0;
	uint8_t current_x = 0xFF;

	for (uint8_t i = 0; i < 10; i++) {
		if (emu->ppu.objects_to_render[i] == OAM_EMPTY)
			break;
		assert(emu->ppu.objects_to_render[i] < 40);
		SpriteObject *object = get_sprite_object(&emu->ppu, emu->ppu.objects_to_render[i]);
		if (x + 8 < object->x || x + 8 >= object->x + 8)
			continue;

		uint8_t local_x = (x + 8) - object->x;
		uint8_t local_y = (y + 16) - object->y;
		uint8_t tile_id = object->tile;

		if (emu->ppu.lcdc.obj_size) {
			tile_id = local_y < 8 ? (tile_id & 0xFE) : (tile_id | 0x01);
			if (local_y >= 8)
				local_y -= 8;
		}
		assert(local_x < 8);
		assert(local_y < 8);
		TileData *tile = &tile_data(ppu)[tile_id];
		uint8_t color_idx = tile_color(tile, local_x, local_y);
		if (color_idx == 0)
			continue;

		// Already have an object, but found a higher prio one
		if (selected_color_idx != 0 && current_x <= object->x)
			continue;

		selected_color_idx = color_idx;
		current_x = object->x;
	}

	// TODO: Lookup palette
	return selected_color_idx;
}

static inline GBColor tile_pixel(Emulator *emu) {
	PPU *ppu = &emu->ppu;
	uint8_t x = ppu->x;
	uint8_t y = ppu->line;

	uint8_t tile_x = x / 8;
	uint8_t tile_y = y / 8;
	uint16_t tile_idx = tile_y * 32 + tile_x;
	uint8_t tile_id = tile_map(ppu)[tile_idx];
	TileData *tile = &tile_data(ppu)[tile_id];

	uint8_t local_x = x % 8;
	uint8_t local_y = y % 8;
	uint8_t color_idx = tile_color(tile, local_x, local_y);

	// TODO: Lookup palette
	return color_idx;
}


static inline void drawing_step(Emulator *emu) {
	// TODO: Penalties
	PPU *ppu = &emu->ppu;

	GBColor color = object_pixel(emu);
	// GBColor color = 0;
	if (color == 0)
		color = tile_pixel(emu);

	uint8_t x = ppu->x;
	uint8_t y = ppu->line;
	display_set(emu, x, y, color);

	ppu->x++;
	if (ppu->x >= 160) {
		ppu->mode = PPU_MODE_HBLANK;
		ppu->x = 0;
	}
}




static inline bool is_on_scanline(PPU *ppu, SpriteObject *object) {
	uint8_t ly = ppu->line + 16;
	uint8_t object_size = ppu->lcdc.obj_size ? 16 : 8;
	uint8_t offset_y = ly - object->y;
	return offset_y < object_size;
}


static inline void oam_scan_step(Emulator *emu) {
	static const uint16_t OAM_SCAN_LENGTH = 80;
	if (emu->ppu.dot_clock < OAM_SCAN_LENGTH)
		return;
	emu->ppu.mode = PPU_MODE_DRAWING;
	uint8_t size = 0;
	for (uint8_t i = 0; i < SPRITE_OBJECT_SIZE; i++) {
		SpriteObject *object = get_sprite_object(&emu->ppu, i);
		if (!is_on_scanline(&emu->ppu, object))
			continue;
		emu->ppu.objects_to_render[size] = i;
		size++;
		if (size == 10)
			return;
	}
	if (size < 10)
		emu->ppu.objects_to_render[size] = OAM_EMPTY;
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


uint8_t ppu_vram_read(PPU *ppu, uint16_t address) {
	if (ppu->mode == PPU_MODE_DRAWING) return 0xFF;
	if (address >= VRAM_SIZE) return 0xFF;
	return ppu->vram[address];
}


void ppu_vram_write(PPU *ppu, uint16_t address, uint8_t value) {
	// if (ppu->mode == PPU_MODE_DRAWING) return;
	if (address >= VRAM_SIZE) return;
	ppu->vram[address] = value;
}


uint8_t ppu_oam_read(PPU *ppu, uint16_t address) {
	if (address >= OAM_SIZE) return 0xFF;
	return ppu->oam[address];
}

void ppu_oam_write(PPU *ppu, uint16_t address, uint8_t value) {
	if (address >= OAM_SIZE) return;
	ppu->oam[address] = value;
}


void ppu_oam_dma_write(Emulator *emu, uint16_t value) {
	if (emu->ppu.mode == PPU_MODE_OAM_SCAN) return;
	if (emu->ppu.mode == PPU_MODE_DRAWING) return;
	// TODO: Cycles
	uint16_t source = value << 8;
	for (uint16_t i = 0; i < 160; i++) {
		emu->ppu.oam[i] = memory_read(emu, source + i);
	}
}

