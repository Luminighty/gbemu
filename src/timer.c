#include "timer.h"
#include "emulator.h"
#include "interrupts.h"
#include <stdint.h>

static inline void tima_step(Emulator* emu);

void timer_step(Emulator* emu, uint16_t m_cycles) {
	for (uint16_t t_cycle = 0; t_cycle < m_cycles * 4; t_cycle++) {
		emu->timer.internal_timer++;
		tima_step(emu);
	}
}


static const uint16_t TAC_BITS[] = {
	[0b00] = 0b1000000000,
	[0b01] = 0b0000001000,
	[0b10] = 0b0000100000,
	[0b11] = 0b0010000000,
};

#define clock_select(tac) ((tac) & 0b11)
static inline void tima_step(Emulator* emu) {
	Timer *timer = &emu->timer;

	bool is_tima_enabled = timer->tac_enabled;
	uint16_t selected_bit = TAC_BITS[timer->tac_clock_select];

	bool was_bit_set = timer->tac_selected_bit_state;
	bool is_bit_set = timer->internal_timer & selected_bit;

	// NOTE: On DMG, the enabled flag is before the falling edge detector
	//  Therefore we need to apply it before detecting the falling edge
	if (!timer->is_cgb)
		is_bit_set = is_bit_set && is_tima_enabled;
	timer->tac_selected_bit_state = is_bit_set;

	bool is_falling_edge = was_bit_set && !is_bit_set;
	if (!is_falling_edge)
		return;

	// NOTE: On CGB, the enabled flag is checked after the falling edge detector
	if (timer->is_cgb && !is_tima_enabled)
		return;

	switch (timer->tima_state) {
	case TIMA_STATE_JUST_OVERFLOWED:
		timer->tima = timer->tma;
		timer->tima_state = TIMA_STATE_COUNTING;
		interrupt_trigger(emu, INTERRUPT_TIMER);
		break;
	case TIMA_STATE_WILL_OVERFLOW:
		timer->tima = 0;
		timer->tima_state = TIMA_STATE_JUST_OVERFLOWED;
		break;
	case TIMA_STATE_COUNTING:
	default:
		if (timer->tima == 0xFF) {
			timer->tima_state = TIMA_STATE_WILL_OVERFLOW;
		} else {
			timer->tima += 1;
		}
		break;
	}
}


void timer_tac_write(Emulator *emu, uint8_t value) {
	emu->timer.tac_clock_select = value & 0b11;
	emu->timer.tac_enabled = value & 0b100;
}

void timer_div_reset(Emulator *emu) {
	emu->timer.internal_timer = 0;
}

void timer_tima_write(Emulator *emu, uint8_t value) {
	emu->timer.tima = value;
}

uint8_t timer_div_read(Emulator *emu) {
	return emu->timer.internal_timer >> 8;
}

uint8_t timer_tac_read(Emulator *emu) {
	return (emu->timer.tac_clock_select | emu->timer.tac_enabled | 0b11111000);
}

void timer_tma_write(Emulator *emu, uint8_t value) {
	emu->timer.tma = value;
	// NOTE: To ensure that the overflow does not happen during "Cycle A", we set the state to COUNTING whenever the value gets set
	// See: https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html
	emu->timer.tima_state = TIMA_STATE_COUNTING;
}

