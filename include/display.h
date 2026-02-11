#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 144

typedef uint16_t GBColor;

typedef struct {
	GBColor screen[DISPLAY_HEIGHT][DISPLAY_WIDTH];
} Display;


Display display_create();

struct emulator;
void display_set(struct emulator *emu, uint8_t x, uint8_t y, GBColor color);
void display_dump(struct emulator *emu);

#endif // DISPLAY_H
