#include <stdint.h>
#include <stdio.h>
#include <raylib.h>

#include "cartridge.h"
#include "display.h"
#include "emulator.h"
#include "memory_map.h"

char* gb_file = "./assets/tetris.gb";
// char* gb_file = "./assets/super_mario_land.gb";
// char* gb_file = "./assets/links_awakening.gb";

// test_header("./assets/links_awakening.gb");
// test_header("./assets/pokemon_crystal.gbc");
// test_header("./assets/super_mario_land.gb");

void draw_display_to_image(Emulator *emu, Image *image) {
	// static Color raylib_palette[] = { WHITE, LIGHTGRAY, GRAY, BLACK };
	Color palette[4] = {
		{155, 188, 15, 255}, // #9BBC0F
		{139, 172, 15, 255}, // #8BAC0F
		{48, 98, 48, 255},   // #306230
		{15, 56, 15, 255}    // #0F380F
	    };
	for (int y = 0; y < DISPLAY_HEIGHT; y++){
	for (int x = 0; x < DISPLAY_WIDTH; x++) {
		GBColor idx = emu->display.screen[y][x];
		ImageDrawPixel(image, x, y, palette[idx]);
	}}
}

int main(void) {
	Cartridge cart = cartridge_load(gb_file);
	cartridge_dump_header(&cart);

	static const uint8_t SCALE = 4;

	InitWindow(DISPLAY_WIDTH * SCALE, DISPLAY_HEIGHT * SCALE, cart.title);
	SetTargetFPS(60);

	Image screen = GenImageColor(DISPLAY_WIDTH, DISPLAY_HEIGHT, BLANK);
	Texture2D screen_texture = LoadTextureFromImage(screen);

	Emulator emu = emulator_create();
	emu.cartridge = &cart;

	while(!WindowShouldClose()) {
		emulator_run_frame(&emu);

		draw_display_to_image(&emu, &screen);
		UpdateTexture(screen_texture, screen.data);
		BeginDrawing();
		ClearBackground(BLACK);
		DrawTextureEx(screen_texture, (Vector2){0, 0}, 0.0f, SCALE, WHITE);
		EndDrawing();
	}
	CloseWindow();
	UnloadTexture(screen_texture);
	UnloadImage(screen);

	emulator_destroy(&emu);
	cartridge_free(&cart);

	return 0;
}

