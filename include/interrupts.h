#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdbool.h>
#include <stdint.h>


typedef enum {
	INTERRUPT_STATE_IDLE,
	INTERRUPT_STATE_NOP0,
	INTERRUPT_STATE_NOP1,
	INTERRUPT_STATE_PC_STORE0,
	INTERRUPT_STATE_PC_STORE1,
	INTERRUPT_STATE_PC_SET,
} InterruptState;


typedef enum {
	INTERRUPT_VBLANK = 0b00001,
	INTERRUPT_LCD    = 0b00010,
	INTERRUPT_TIMER  = 0b00100,
	INTERRUPT_SERIAL = 0b01000,
	INTERRUPT_JOYPAD = 0b10000,
} InterruptFlag;


typedef struct {
	bool ime;
	uint8_t enable;
	uint8_t flag;
	InterruptState state;
} Interrupt;



Interrupt interrupt_create();


void interrupt_enable_write(Interrupt *interrupt, uint8_t value);
void interrupt_flag_write(Interrupt *interrupt, uint8_t value);

uint8_t interrupt_enable_read(Interrupt *interrupt);
uint8_t interrupt_flag_read(Interrupt *interrupt);

struct emulator;

uint8_t interrupt_step(struct emulator *emu);
void interrupt_trigger(struct emulator *emu, InterruptFlag flag);
uint8_t interrupt_pending(struct emulator *emu);
bool is_interrupt_handler_running(struct emulator *emu);


#endif // INTERRUPT_H
