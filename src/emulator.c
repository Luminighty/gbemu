#include "emulator.h"

#include "cpu.h"
#include "display.h"
#include "interrupts.h"
#include "logger.h"
#include "memory.h"
#include "timer.h"


Emulator emulator_create() {
	Emulator emu = {0};
	emu.memory = memory_create();
	emu.cpu = cpu_create();
	emu.interrupt = interrupt_create();
	return emu;
}


void emulator_destroy(Emulator* emulator) {
	memory_destroy(emulator->memory);
	emulator->memory = NULL;
}

void emulator_run_frame(Emulator* emu) {
	DEBUG("FRAME\n");
	static const uint32_t MAX_CYCLES = 70224;
	uint32_t cycles = 0;
	while (cycles < MAX_CYCLES) {
		uint8_t t_cycle = cpu_step(emu);
		timer_step(emu, t_cycle);
		ppu_step(emu, t_cycle);
		
		cycles += t_cycle;
	}
	interrupt_trigger(emu, INTERRUPT_VBLANK);
	display_dump(emu);
}
