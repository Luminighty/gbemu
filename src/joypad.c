#include "joypad.h"
#include "emulator.h"
#include <stdint.h>
#include <stdio.h>

Joypad joypad_create() {
	Joypad joypad = {0};
	for (int i = 0; i < 4; i++) {
		joypad.dpad[i] = true;
		joypad.buttons[i] = true;
	}
	return joypad;
}

uint8_t joypad_read(Joypad *joypad) {
	if (!joypad->select_buttons && !joypad->select_dpad)
		return 0xFF;

	static const uint8_t BASE = 0b11000000;
	if (!joypad->select_buttons) {
		return BASE |
			(joypad->select_buttons << 5) |
			(joypad->buttons[3] << 3) |
			(joypad->buttons[2] << 2) |
			(joypad->buttons[1] << 1) |
			(joypad->buttons[0] << 0);
	} else if (!joypad->select_dpad) {
		return BASE |
			(joypad->select_dpad << 4) |
			(joypad->dpad[3] << 3) |
			(joypad->dpad[2] << 2) |
			(joypad->dpad[1] << 1) |
			(joypad->dpad[0] << 0);
	} else {
		return BASE;
	}
}

void joypad_write(Joypad *joypad, uint8_t value) {
	joypad->select_buttons = (value >> 5) & 0b1;
	joypad->select_dpad = (value >> 4) & 0b1;
}

void joypad_press(Emulator *emu, JoypadButton button) {
	if (button >= GB_BUTTON_A) {
		emu->joypad.buttons[button - GB_BUTTON_A] = false;
	} else {
		emu->joypad.dpad[button] = false;
	}
}

void joypad_release(Emulator *emu, JoypadButton button) {
	if (button >= GB_BUTTON_A) {
		emu->joypad.buttons[button - GB_BUTTON_A] = true;
	} else {
		emu->joypad.dpad[button] = true;
	}
}

