#ifndef EMULATOR_H
#define EMULATOR_H

#include "cpu.h"
#include "display.h"
#include "interrupts.h"
#include "memory.h"
#include "cartridge.h"
#include "ppu.h"
#include "timer.h"


typedef struct emulator {
	CPU cpu;
	Memory *memory;
	Cartridge *cartridge;
	Timer timer;
	Interrupt interrupt;
	Display display;
	PPU ppu;
} Emulator;


Emulator emulator_create();
void emulator_destroy(Emulator* emulator);

void emulator_run_frame(Emulator* emulator);


#endif // EMULATOR_H
