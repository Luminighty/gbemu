#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdbool.h>
#include <stdint.h>


typedef enum {
	GB_BUTTON_RIGHT,
	GB_BUTTON_LEFT,
	GB_BUTTON_UP,
	GB_BUTTON_DOWN,

	GB_BUTTON_A,
	GB_BUTTON_B,
	GB_BUTTON_SELECT,
	GB_BUTTON_START,
} JoypadButton;

typedef struct {
	bool dpad[4];
	bool buttons[4];
	bool select_buttons;
	bool select_dpad;
} Joypad;


Joypad joypad_create();
uint8_t joypad_read(Joypad *joypad);
void joypad_write(Joypad *joypad, uint8_t value);

struct emulator;
void joypad_press(struct emulator *emu, JoypadButton button);
void joypad_release(struct emulator *emu, JoypadButton button);

#endif // JOYPAD_H
