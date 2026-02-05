#include "./unit.h"

#include <stdint.h>

#include "opcode.h"
#include "emulator.h"


int test_ld() {
	Emulator emu = emulator_create();

	char *registers_labels[] = { 
		"b", "c", "d", "e",
		"h", "l", NULL, "a"
	};
	uint8_t *registers[] = { 
		&emu.cpu.b, &emu.cpu.c, &emu.cpu.d, &emu.cpu.e,
		&emu.cpu.h,&emu.cpu.l, NULL, &emu.cpu.a
	};
	for (int i = 0; i < 8; i++) {
		// Skip (HL) cases
		if (i == 6) continue;

		uint8_t value = i * 7;
		emu.cpu.b = 0;
		*registers[i] = value;
		
		int opcode = 0x40 + i;
		opcode_execute(&emu, opcode);
		
		assert_eq(emu.cpu.b, value, "%d", "[OPCODE %x] LD b, %s", opcode, registers_labels[i]);
	}

	return SUCCESS;
}


int main() {
	TEST_SETUP();

	TEST_RUN(test_ld);

	TEST_FINISH();
}

