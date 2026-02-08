#ifndef EMULATOR_H
#define EMULATOR_H

#include "cpu.h"
#include "interrupts.h"
#include "memory.h"
#include "cartridge.h"
#include "timer.h"


typedef struct emulator {
	CPU cpu;
	Memory *memory;
	Cartridge *cartridge;
	Timer timer;
	Interrupt interrupt;
} Emulator;


Emulator emulator_create();
void emulator_destroy(Emulator* emulator);


#endif // EMULATOR_H
