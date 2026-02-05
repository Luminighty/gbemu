#include "cpu.h"
#include "memory_map.h"
#include "opcode.h"
#include <stdint.h>

CPU cpu_create() {
	CPU cpu = {0};
	cpu.pc = 0x0100;
	return cpu;
}


int cpu_step(Emulator* emu) {
	if (emu->cpu.is_halted) {
		// TODO: Check for wakeup
		return 4;
	}

	// TODO: Handle interrupts

	bool ime_was_scheduled = emu->cpu.ime_scheduled;
	emu->cpu.opcode_length = 0;
	emu->cpu.cycles = 0;
	uint8_t opcode = memory_read(emu, emu->cpu.pc);
	opcode_execute(emu, opcode);

	emu->cpu.pc += emu->cpu.opcode_length;

	if (ime_was_scheduled) {
		emu->cpu.ime = true;
		emu->cpu.ime_scheduled = false;
	}
	return emu->cpu.cycles;
}
