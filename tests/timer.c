#include "timer.h"
#include "./unit.h"
#include "emulator.h"
#include <stdbool.h>
#include <stdint.h>

#define DIV_TICK (0x40 * 4)

#define TAC_DISABLE (0b000)
#define TAC_256M (0b100)
#define TAC_4M (0b101)
#define TAC_16M (0b110)
#define TAC_64M (0b111)

int test_div() {
	Emulator emu = emulator_create();


	for (uint16_t tick = 0; tick < DIV_TICK; tick++) {
		timer_step(&emu, 1);
		uint16_t expected = (tick + 1);
		assert_eq(emu.timer.internal_timer, expected, "%04X");
	}
	assert_eq(timer_div_read(&emu), 1, "%d");

	emu.timer.internal_timer = 0;
	timer_step(&emu, DIV_TICK);
	assert_eq(timer_div_read(&emu), 1, "%d");
	assert_eq(emu.timer.internal_timer, DIV_TICK, "%04X");
	timer_step(&emu, DIV_TICK / 2);
	assert_eq(timer_div_read(&emu), 1, "%d");
	timer_step(&emu, DIV_TICK / 2);
	assert_eq(timer_div_read(&emu), 2, "%d");
	timer_step(&emu, DIV_TICK * 2);
	assert_eq(timer_div_read(&emu), 4, "%d");

	timer_div_reset(&emu);

	assert_eq(timer_div_read(&emu), 0, "%d");
	assert_eq(emu.timer.internal_timer, 0, "%d");

	return SUCCESS;
}


int test_tima() {
	Emulator emu = emulator_create();

	// Disabled
	timer_tac_write(&emu, 0b0000);
	timer_step(&emu, 1024 * 4);
	assert_eq(emu.timer.tima, 0, "%d");

	timer_div_reset(&emu);
	uint16_t tima;

	timer_tac_write(&emu, TAC_4M);
	tima = 24;
	timer_step(&emu, tima * 16);
	assert_eq(emu.timer.tima, tima, "%d");

	timer_div_reset(&emu);
	timer_tima_write(&emu, 0);

	timer_tac_write(&emu, TAC_16M);
	tima = 12;
	timer_step(&emu, tima * 64);
	assert_eq(emu.timer.tima, tima, "%d");

	timer_div_reset(&emu);
	timer_tima_write(&emu, 0);

	timer_tac_write(&emu, TAC_64M);
	tima = 10;
	timer_step(&emu, tima * 64 * 4);
	assert_eq(emu.timer.tima, tima, "%d");

	timer_div_reset(&emu);
	timer_tima_write(&emu, 0);

	timer_tac_write(&emu, TAC_256M);
	tima = 13;
	timer_step(&emu, tima * 256 * 4);
	assert_eq(emu.timer.tima, tima, "%d");

	return SUCCESS;
}


int test_tima_overflow() {
	Emulator emu = emulator_create();
	
	uint8_t tma_value = 0x80;
	timer_tma_write(&emu, tma_value);
	timer_tac_write(&emu, TAC_4M);
	
	timer_tima_write(&emu, 0xFF);
	timer_div_reset(&emu);
	
	timer_step(&emu, 16);
	assert_eq(emu.timer.tima, 0xFF, "%02X");
	timer_step(&emu, 4);
	
	assert_eq(emu.timer.tima, tma_value, "%02X");
	
	bool interrupt_requested = (emu.interrupt.flag & (1 << 2));
	assert_eq(interrupt_requested, true, "%d");

	return SUCCESS;
}

int test_tima_falling_edge_glitch() {
	Emulator emu = {0};

	timer_tac_write(&emu, TAC_256M);
	timer_tima_write(&emu, 0x00);
	timer_div_reset(&emu);

	timer_step(&emu, 128 * 4);
	assert_eq(emu.timer.tima, 0, "%d");

	timer_tac_write(&emu, 0b000 | 0b00);

	timer_step(&emu, 4);

	assert_eq(emu.timer.tima, 1, "%d");

	return SUCCESS;
}

int test_cgb_glitch_behavior() {
	Emulator emu = emulator_create();
	emu.timer.is_cgb = true;

	timer_tac_write(&emu, TAC_256M);
	timer_div_reset(&emu);

	timer_step(&emu, 128 * 4);
	assert_eq(emu.timer.tima, 0, "%d");

	timer_tac_write(&emu, TAC_DISABLE); 
	timer_step(&emu, 4);
	assert_eq(emu.timer.tima, 0, "%d");

	timer_tac_write(&emu, TAC_256M);
	timer_div_reset(&emu);
	timer_step(&emu, 4);
	assert_eq(emu.timer.tima, 1, "%d");

	return SUCCESS;
}


int main() {
	TEST_SETUP();

	TEST_RUN(test_div);
	TEST_RUN(test_tima);
	TEST_RUN(test_tima_overflow);
	TEST_RUN(test_tima_falling_edge_glitch);
	TEST_RUN(test_cgb_glitch_behavior);

	TEST_FINISH();
}

