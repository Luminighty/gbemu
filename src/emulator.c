#include "emulator.h"

#include "cpu.h"
#include "memory.h"


Emulator emulator_create() {
	Emulator emu = {0};
	emu.memory = memory_create();
	emu.cpu = cpu_create();
	return emu;
}


void emulator_destroy(Emulator* emulator) {
	memory_destroy(emulator->memory);
	emulator->memory = NULL;
}

