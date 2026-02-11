#ifndef MEMORY_H
#define MEMORY_H


#include <stdint.h>


typedef struct {
	uint8_t wram[8191];
	uint8_t highram[127];
	// NOTE: This is here temporarily
	//  This should be moved somewhere else
	uint8_t interrupt_enabled;

	uint8_t temp_vram[0x2000];
} Memory;

Memory* memory_create();
void memory_destroy(Memory *memory);

#endif // MEMORY_H
