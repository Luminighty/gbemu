#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stdint.h>

#include "emulator.h"

uint8_t memory_read(Emulator *emu, uint16_t address);
void memory_write(Emulator *emu, uint16_t address, uint8_t value);

void memory_write_16(Emulator *emu, uint16_t address, uint16_t value);
uint16_t memory_read_16(Emulator *emu, uint16_t address);


#endif // MEMORY_MAP_H
