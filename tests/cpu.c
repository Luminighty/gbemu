#include "cpu.h"
#include "./unit.h"
#include "emulator.h"
#include "interrupts.h"
#include "memory_map.h"
#include <stdint.h>


int test_halt_bug_inc() {
	Emulator emu = emulator_create();
	uint16_t start_pc = 0xC000; // WRAM for ease

	emu.interrupt.ime = false;
	emu.interrupt.enable = INTERRUPT_TIMER;
	emu.interrupt.flag = INTERRUPT_TIMER;
	emu.cpu.b = 0;
	emu.cpu.pc = start_pc;

	memory_write(&emu, start_pc, 0x76);     
	memory_write(&emu, start_pc + 1, 0x04); 

	cpu_step(&emu); // Execute HALT
	assert(emu.cpu.is_halt_bugged, "Halt bug should be armed");

	cpu_step(&emu);
	assert_eq(emu.cpu.b, 1, "%d");
	assertm_eq(emu.cpu.pc, start_pc + 1, "%d", "PC should not have moved!");

	cpu_step(&emu);
	assert_eq(emu.cpu.b, 2, "%d");
	assertm_eq(emu.cpu.pc, start_pc + 2, "%d", "PC should have moved!");

	return SUCCESS;
}


int test_halt_bug_add_d8() {
	Emulator emu = emulator_create();
	uint16_t start_pc = 0xC000; // WRAM for ease

	emu.interrupt.flag = INTERRUPT_TIMER;
	emu.interrupt.enable = INTERRUPT_TIMER;
	emu.cpu.a = 0;
	emu.cpu.b = 0;
	emu.cpu.pc = start_pc;

	static const uint8_t HALT = 0x76;
	static const uint8_t ADD_d8 = 0xC6;
	static const uint8_t INC_B = 0x04;

	memory_write(&emu, start_pc, HALT);
	memory_write(&emu, start_pc + 1, ADD_d8);
	memory_write(&emu, start_pc + 2, INC_B); // Also d8 as $0x04

	cpu_step(&emu); // HALT
	assert(emu.cpu.is_halt_bugged, "Halt bug should be armed");
	cpu_step(&emu); // ADD A, 0x04
	assert(!emu.cpu.is_halt_bugged, "Halt bug should be reset");

	assertm_eq(emu.cpu.pc, start_pc + 2, "%04X", "PC should point at the operand");
	assert_eq(emu.cpu.a, 4, "%d");

	cpu_step(&emu); // INC B
	assertm_eq(emu.cpu.pc, start_pc + 3, "%04X", "PC should move to 0x102");
	assert_eq(emu.cpu.b, 1, "%d");

	return SUCCESS;
}

int test_ei_delay() {
	Emulator emu = emulator_create();
	uint16_t start_pc = 0xC000;
	uint8_t cycles;

	emu.interrupt.enable = INTERRUPT_VBLANK;
	emu.interrupt.flag = INTERRUPT_VBLANK;
	emu.interrupt.ime = false;
	emu.cpu.pc = start_pc;

	static const uint8_t EI = 0xFB;
	static const uint8_t NOP = 0x00;
	static const uint8_t INC_A = 0x3C;
	memory_write(&emu, start_pc, EI);
	memory_write(&emu, start_pc + 1, NOP);
	memory_write(&emu, start_pc + 2, INC_A);

	cpu_step(&emu);
	assert(!emu.interrupt.ime, "IME should still be false immediately after EI");
	assert(emu.cpu.ime_scheduled, "IME should be scheduled for next instruction");

	cpu_step(&emu);
	assert(emu.interrupt.ime, "IME should be true now that the instruction after EI finished");
	assertm_eq(emu.cpu.pc, start_pc + 2, "%04X", "Should have reached the instruction after NOP");

	cpu_step(&emu);
	assertm_eq(emu.interrupt.state, INTERRUPT_STATE_NOP0, "%02X", "ISR should have started instead of executing INC A");

	return SUCCESS;
}

int main() {
	TEST_SETUP();

	TEST_RUN(test_halt_bug_inc);
	TEST_RUN(test_halt_bug_add_d8);
	TEST_RUN(test_ei_delay);

	TEST_FINISH();
}

