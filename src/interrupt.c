#include "emulator.h"
#include "interrupts.h"
#include "logger.h"
#include "memory_map.h"

#include <stdint.h>


#define UNUSED_BITS (0b11100000)


Interrupt interrupt_create() {
	return (Interrupt) {
		.ime = false,
		.enable = UNUSED_BITS,
		.flag = UNUSED_BITS,
	};
}

uint8_t interrupt_enable_read(Interrupt *interrupt) {
	return interrupt->enable | UNUSED_BITS;
}
uint8_t interrupt_flag_read(Interrupt *interrupt) {
	return interrupt->flag | UNUSED_BITS;
}
void interrupt_enable_write(Interrupt *interrupt, uint8_t value) {
	interrupt->enable = value & ~UNUSED_BITS;
}
void interrupt_flag_write(Interrupt *interrupt, uint8_t value) {
	interrupt->flag = value & ~UNUSED_BITS;
}


static inline void interrupt_idle(Interrupt *interrupt);
static inline void interrupt_pc_set(Emulator *emu);


void interrupt_trigger(struct emulator *emu, InterruptFlag flag) {
	emu->interrupt.flag |= flag;
}

static inline void interrupt_pc_set(Emulator *emu) {
	static const uint16_t ADDR_VBLANK = 0x40;
	static const uint16_t ADDR_LCD = 0x48;
	static const uint16_t ADDR_TIMER = 0x50;
	static const uint16_t ADDR_SERIAL = 0x58;
	static const uint16_t ADDR_JOYPAD = 0x60;

	uint8_t to_interrupt = emu->interrupt.flag & emu->interrupt.enable;

#define try_trigger_interrupt(FLAG, ADDRESS) \
	if (to_interrupt & (FLAG)) { \
		emu->cpu.pc = (ADDRESS); \
		emu->interrupt.flag &= ~(FLAG); \
		emu->interrupt.flag |= UNUSED_BITS; \
		return; \
	}

	try_trigger_interrupt(INTERRUPT_VBLANK, ADDR_VBLANK);
	try_trigger_interrupt(INTERRUPT_LCD, ADDR_LCD);
	try_trigger_interrupt(INTERRUPT_TIMER, ADDR_TIMER);
	try_trigger_interrupt(INTERRUPT_SERIAL, ADDR_SERIAL);
	try_trigger_interrupt(INTERRUPT_JOYPAD, ADDR_JOYPAD);
	
	emu->cpu.pc = 0x00;
}


static inline void interrupt_idle(Interrupt *interrupt) {
	if (!interrupt->ime) return;

	uint8_t to_interrupt = interrupt->flag & interrupt->enable; 
	if (!to_interrupt) return;
	
	interrupt->state = INTERRUPT_STATE_NOP0;
	interrupt->ime = false;
}


uint8_t interrupt_step(Emulator *emu) {
	switch (emu->interrupt.state) {
	case INTERRUPT_STATE_IDLE:
		interrupt_idle(&emu->interrupt);
		break;
	case INTERRUPT_STATE_NOP0:
		emu->interrupt.state = INTERRUPT_STATE_NOP1;
		break;
	case INTERRUPT_STATE_NOP1:
		emu->interrupt.state = INTERRUPT_STATE_PC_STORE0;
		break;
	case INTERRUPT_STATE_PC_STORE0:
		memory_write(emu, --emu->cpu.sp, emu->cpu.pc >> 8);
		emu->interrupt.state = INTERRUPT_STATE_PC_STORE1;
		break;
	case INTERRUPT_STATE_PC_STORE1:
		memory_write(emu, --emu->cpu.sp, emu->cpu.pc & 0xFF);
		emu->interrupt.state = INTERRUPT_STATE_PC_SET;
		break;
	case INTERRUPT_STATE_PC_SET:
		interrupt_pc_set(emu);
		emu->interrupt.state = INTERRUPT_STATE_IDLE;
		break;
	}
	return 4;
}

uint8_t interrupt_pending(struct emulator *emu) {
	return emu->interrupt.flag & emu->interrupt.enable;
}

bool is_interrupt_handler_running(struct emulator *emu) {
	return emu->interrupt.state != INTERRUPT_STATE_IDLE;
}
