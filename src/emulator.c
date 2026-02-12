#include "emulator.h"

#include "cpu.h"
#include "display.h"
#include "interrupts.h"
#include "joypad.h"
#include "logger.h"
#include "memory.h"
#include "ppu.h"
#include "timer.h"


Emulator emulator_create() {
	Emulator emu = {0};
	emu.memory = memory_create();
	emu.cpu = cpu_create();
	emu.interrupt = interrupt_create();
	emu.ppu = ppu_create();
	emu.joypad = joypad_create();
	return emu;
}


void emulator_destroy(Emulator* emulator) {
	memory_destroy(emulator->memory);
	ppu_destroy(&emulator->ppu);
	emulator->memory = NULL;
}


void emulator_run_frame(Emulator* emu) {
	static const uint32_t MAX_CYCLES = 70224;
	uint32_t cycles = 0;
	while (cycles < MAX_CYCLES) {
		uint8_t t_cycle = cpu_step(emu);
		timer_step(emu, t_cycle);
		ppu_step(emu, t_cycle);
		
		cycles += t_cycle;
	}
	interrupt_trigger(emu, INTERRUPT_VBLANK);
}

