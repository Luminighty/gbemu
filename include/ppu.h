#ifndef PPU_H
#define PPU_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define VRAM_SIZE 0x2000
#define TILEMAP_SIZE 0x400

typedef enum {
	PPU_MODE_HBLANK,
	PPU_MODE_VBLANK,
	PPU_MODE_OAM_SCAN,
	PPU_MODE_DRAWING,
} PPUMode;


typedef struct {
	PPUMode mode;

	uint8_t *vram;

	uint32_t dot_clock;
	uint8_t line;
	uint8_t x;

	uint8_t stat;
	uint8_t bgp;

	uint8_t scx;
	uint8_t scy;

	struct {
		bool ppu_enable;
		bool window_tilemap_area;
		bool window_enable;
		bool bg_win_tilemap_area;
		bool bg_tilemap_area;
		bool obj_size;
		bool obj_enable;
		bool bg_window_enable_prio;
	} lcdc;
} PPU;


PPU ppu_create();
void ppu_destroy(PPU *ppu);

uint8_t ppu_vram_read(PPU *ppu, uint16_t address);
void ppu_vram_write(PPU *ppu, uint16_t address, uint8_t value);

uint8_t ppu_lcdc_read(PPU *ppu);
void ppu_lcdc_write(PPU *ppu, uint8_t value);

struct emulator;
void ppu_step(struct emulator *emu, uint8_t cycles);


typedef union {
	struct {
		uint8_t lower;
		uint8_t higher;
	} lines[8];
	uint8_t data[16];
} TileData;

static_assert(sizeof(TileData) == 16, "Tiledata should be 16 bytes long");

#endif // PPU_H
