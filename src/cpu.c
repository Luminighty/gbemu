#include "cpu.h"
#include "interrupts.h"
#include "memory_map.h"
#include "opcode.h"
#include <stdint.h>

CPU cpu_create() {
	CPU cpu = {0};
	cpu.pc = 0x0000;
	return cpu;
}


uint8_t cpu_step(Emulator* emu) {
	bool has_interrupts = interrupt_pending(emu);
	if (emu->cpu.is_halted) {
		if (has_interrupts) {
			emu->cpu.is_halted = false;
		} else {
			return 4;
		}
	}

	if (is_interrupt_handler_running(emu) || (emu->interrupt.ime && has_interrupts))
		return interrupt_step(emu);

	bool ime_was_scheduled = emu->cpu.ime_scheduled;
	bool was_halt_bugged = emu->cpu.is_halt_bugged;

	emu->cpu.opcode_length = 0;
	emu->cpu.cycles = 0;
	uint8_t opcode = memory_read(emu, emu->cpu.pc);
	opcode_execute(emu, opcode);

	emu->cpu.pc += emu->cpu.opcode_length;
	if (was_halt_bugged) {
		emu->cpu.pc--;
		emu->cpu.is_halt_bugged = false;
	}

	if (ime_was_scheduled) {
		emu->interrupt.ime = true;
		emu->cpu.ime_scheduled = false;
	}
	return emu->cpu.cycles;
}

