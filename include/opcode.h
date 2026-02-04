#ifndef OPCODE_H
#define OPCODE_H


#include "emulator.h"
#include <stdint.h>

void opcode_execute(Emulator* emulator, uint8_t opcode);


#endif // OPCODE_H
