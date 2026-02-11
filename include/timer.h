#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	TIMA_STATE_COUNTING,
	TIMA_STATE_WILL_OVERFLOW,
	TIMA_STATE_JUST_OVERFLOWED,
} TIMAState;


typedef struct {
	uint16_t internal_timer;

	uint8_t tima;
	uint8_t tma;
	
	uint8_t tac_clock_select;
	uint8_t tac_enabled;
	bool tac_selected_bit_state;

	TIMAState tima_state;

	// NOTE: Used for timer quirks
	bool is_cgb;
	bool debug;
} Timer;


struct emulator;
void timer_step(struct emulator *emu, uint16_t m_cycles);

void timer_tma_write(struct emulator *emu, uint8_t value);
void timer_tima_write(struct emulator *emu, uint8_t value);

void timer_div_reset(struct emulator *emu);
uint8_t timer_div_read(struct emulator *emu);

void timer_tac_write(struct emulator *emu, uint8_t value);
uint8_t timer_tac_read(struct emulator *emu);

#endif // TIMER_H
