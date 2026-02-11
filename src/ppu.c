#include "ppu.h"
#include "emulator.h"
#include "interrupts.h"

PPU ppu_create() {
	PPU ppu = {0};
	return ppu;
}

// NOTE: This is just trash ai code to get things working and dump the vram while syncing with interrupts
void ppu_step(Emulator *emu, uint8_t cycles) {
	emu->ppu.dot_clock += cycles;

	// Each scanline takes 456 T-cycles
	if (emu->ppu.dot_clock >= 456) {
		emu->ppu.dot_clock -= 456;
		emu->ppu.line++;

		// VBlank happens when LY hits 144
		if (emu->ppu.line == 144) {
			// Trigger VBlank Interrupt (Bit 0 of IF)
			interrupt_trigger(emu, INTERRUPT_VBLANK);
		}

		// Total of 154 lines (0-153) per frame
		if (emu->ppu.line > 153) {
			emu->ppu.line = 0;
		}
	}
}
