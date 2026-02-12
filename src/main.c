#include <stdint.h>
#include <stdio.h>
#include <raylib.h>

#include "cartridge.h"
#include "display.h"
#include "emulator.h"
#include "joypad.h"
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

static inline void update_inputs(Emulator *emu) {
	struct {int key; JoypadButton gb;} keys[] = {
		{.key = KEY_DOWN, .gb = GB_BUTTON_DOWN},
		{.key = KEY_UP, .gb = GB_BUTTON_UP},
		{.key = KEY_LEFT, .gb = GB_BUTTON_LEFT},
		{.key = KEY_RIGHT, .gb = GB_BUTTON_RIGHT},
		{.key = KEY_X, .gb = GB_BUTTON_A},
		{.key = KEY_C, .gb = GB_BUTTON_B},
		{.key = KEY_SPACE, .gb = GB_BUTTON_SELECT},
		{.key = KEY_ENTER, .gb = GB_BUTTON_START},
	};
	for(int i = 0; i < 8; i++) {
		if (IsKeyReleased(keys[i].key))
			joypad_release(emu, keys[i].gb);
		if (IsKeyPressed(keys[i].key))
			joypad_press(emu, keys[i].gb);
	}
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
		update_inputs(&emu);

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

