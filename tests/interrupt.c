#include "interrupts.h"
#include "./unit.h"
#include "emulator.h"


int test_interrupt_trigger_logic() {
	Emulator emu = emulator_create();

	emu.interrupt.enable = INTERRUPT_TIMER;
	emu.interrupt.ime = true;
	emu.interrupt.state = INTERRUPT_STATE_IDLE;


	interrupt_trigger(&emu, INTERRUPT_TIMER);
	assert_eq(emu.interrupt.flag & INTERRUPT_TIMER, INTERRUPT_TIMER, "%d");

	interrupt_step(&emu);
	assert_eq(emu.interrupt.state, INTERRUPT_STATE_NOP0, "%d");
	interrupt_step(&emu);
	assert_eq(emu.interrupt.state, INTERRUPT_STATE_NOP1, "%d");

	interrupt_step(&emu);
	assert_eq(emu.interrupt.state, INTERRUPT_STATE_PC_STORE0, "%d");
	interrupt_step(&emu);
	assert_eq(emu.interrupt.state, INTERRUPT_STATE_PC_STORE1, "%d");

	interrupt_step(&emu);
	assert_eq(emu.interrupt.state, INTERRUPT_STATE_PC_SET, "%d");

	interrupt_step(&emu);
	assert_eq(emu.interrupt.state, INTERRUPT_STATE_IDLE, "%d");
	assert_eq(emu.interrupt.ime, false, "%d");
	assert_eq(emu.interrupt.flag & INTERRUPT_TIMER, 0, "%d");

	return SUCCESS;
}

int test_interrupt_priority() {
	Emulator emu = emulator_create();

	emu.interrupt.enable = INTERRUPT_VBLANK | INTERRUPT_TIMER;
	emu.interrupt.ime = true;

	interrupt_trigger(&emu, INTERRUPT_TIMER);
	interrupt_trigger(&emu, INTERRUPT_VBLANK);

	interrupt_step(&emu);
	interrupt_step(&emu);
	interrupt_step(&emu);

	interrupt_step(&emu);
	interrupt_step(&emu);
	interrupt_step(&emu);

	assert(!(emu.interrupt.flag & INTERRUPT_VBLANK), "VBlank should be handled");
	assert(emu.interrupt.flag & INTERRUPT_TIMER, "Timer should still be pending.");

	return SUCCESS;
}

int test_interrupt_ime_mask() {
	Emulator emu = emulator_create();

	emu.interrupt.enable = INTERRUPT_TIMER;
	emu.interrupt.ime = false;
	emu.interrupt.state = INTERRUPT_STATE_IDLE;

	interrupt_trigger(&emu, INTERRUPT_TIMER);

	assert(emu.interrupt.flag & INTERRUPT_TIMER, "Timer should be set.");

	for (int i = 0; i < 10; i++) {
		interrupt_step(&emu);
		assert_eq(emu.interrupt.state, INTERRUPT_STATE_IDLE, "%d");
	}

	return SUCCESS;
}

int test_interrupt_late_stat_trigger() {
	Emulator emu = emulator_create();
	emu.interrupt.enable = INTERRUPT_VBLANK | INTERRUPT_TIMER;
	emu.interrupt.ime = true;

	interrupt_trigger(&emu, INTERRUPT_TIMER);
	interrupt_step(&emu);

	interrupt_trigger(&emu, INTERRUPT_VBLANK);

	interrupt_step(&emu);
	interrupt_step(&emu);
	interrupt_step(&emu);
	interrupt_step(&emu);
	interrupt_step(&emu);

	assert(!(emu.interrupt.flag & INTERRUPT_VBLANK), "VBlank should have hijacked the ISR");
	assert(emu.interrupt.flag & INTERRUPT_TIMER, "Timer should still be pending");

	return SUCCESS;
}


int main() {
	TEST_SETUP();

	TEST_RUN(test_interrupt_trigger_logic);
	TEST_RUN(test_interrupt_priority);
	TEST_RUN(test_interrupt_ime_mask);
	TEST_RUN(test_interrupt_late_stat_trigger);

	TEST_FINISH();
}

