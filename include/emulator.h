#ifndef EMULATOR_H
#define EMULATOR_H

#include "cpu.h"
#include "memory.h"
#include "cartridge.h"


typedef struct emulator {
	CPU cpu;
	Memory *memory;
	Cartridge *cartridge;
} Emulator;


Emulator emulator_create();
void emulator_destroy(Emulator* emulator);


#endif // EMULATOR_H
