#ifndef PPU_H
#define PPU_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define VRAM_SIZE 0x2000
#define TILEMAP_SIZE 0x400
#define OAM_SIZE 0xA0
#define SPRITE_OBJECT_SIZE 40

typedef enum {
	PPU_MODE_HBLANK,
	PPU_MODE_VBLANK,
	PPU_MODE_OAM_SCAN,
	PPU_MODE_DRAWING,
} PPUMode;


typedef struct {
	PPUMode mode;

	uint8_t *vram;
	uint8_t *oam;
	uint8_t objects_to_render[10];

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


uint8_t ppu_oam_read(PPU *ppu, uint16_t address);
void ppu_oam_write(PPU *ppu, uint16_t address, uint8_t value);

uint8_t ppu_lcdc_read(PPU *ppu);
void ppu_lcdc_write(PPU *ppu, uint8_t value);

struct emulator;
void ppu_step(struct emulator *emu, uint8_t cycles);
void ppu_oam_dma_write(struct emulator *emu, uint16_t value);


typedef union {
	struct {
		uint8_t lower;
		uint8_t higher;
	} lines[8];
	uint8_t data[16];
} TileData;

typedef struct {
	uint8_t y;
	uint8_t x;
	uint8_t tile;
	uint8_t attr;
} SpriteObject;


static_assert(sizeof(TileData) == 16, "Tiledata should be 16 bytes long");
static_assert(sizeof(SpriteObject) == 4, "SpriteObject should be 4 bytes long");
static_assert(sizeof(SpriteObject) * SPRITE_OBJECT_SIZE == OAM_SIZE, "OAM should contain 40 objects");


#define OBJ_ATTR_PRIO		(1 << 7)
#define OBJ_ATTR_FLIP_Y		(1 << 6)
#define OBJ_ATTR_FLIP_X		(1 << 5)
#define OBJ_ATTR_DMG_PALETTE	(1 << 4)
#define OBJ_ATTR_BANK		(1 << 3)
#define OBJ_ATTR_CGB_PALETTE	(0b111)


#endif // PPU_H
